# Shared GLES 3 renderer dialect bring-up — 2026-07-31

## Result

Commits `4695d9cb340d` and `78ef952dbecc` establish the first bounded M7
renderer slice without claiming M7 acceptance. The production renderer can
now be compiled as either the established desktop OpenGL 3.3/core + GLSL 140
dialect or an OpenGL ES 3.0 + GLSL ES 300 dialect. The choice is a CMake
configuration boundary, not a second renderer implementation.

The exact clean source tip passed 18/18 tests on ordinary ARM64 macOS,
GLES-configured ARM64 macOS, combined ASan/UBSan ARM64 macOS, and optimized
Linux i686. A thin ARM64 iOS Simulator executable compiled and linked against
UIKit and OpenGLES. The established desktop renderer still initialized every
PSX shader and VRAM pipeline on Apple M2 and opened CoreAudio.

This does **not** accept the GLES runtime yet. This host has no ANGLE/EGL
runtime for SDL's Cocoa GLES backend, so the macOS GLES configuration reaches
the correct window/context boundary and reports the missing library. The iOS
artifact is compile/link evidence only: its generated metadata is incomplete,
its linker signature is not an installable bundle signature, and it has not
run in Simulator or on an iPad.

## Reference audit

The populated Android reference was inspected at:

```text
ref/ctr-native-android
branch: feature/add-android-support
tip:    34648097...
key renderer change: a9c805a...
```

The load-bearing renderer changes in that branch were:

- request an ES context and use GLSL ES 300 precision declarations;
- resolve GL entry points with `SDL_GL_GetProcAddress`;
- avoid desktop-only polygon-mode wireframe calls; and
- use an ES-compatible VRAM texture format.

The current tree already used packed `GL_RG8` VRAM and contained no
`glGetTexImage`, so those parts were not copied again. The existing checked-in
glad loader was generated with shared GL/GLES declarations and recognizes an
`OpenGL ES` version prefix; the new path therefore reuses it and verifies the
specific ES 3 entry points consumed by the renderer. No wholesale reference
branch merge was performed.

The local SDL source established two different Apple backends:

- Cocoa GLES creates an EGL-backed layer and requires an external ANGLE/EGL
  runtime such as `libEGL.dylib`.
- UIKit uses Apple's OpenGLES framework directly.

No ANGLE/EGL dynamic libraries were present on this Mac. That absence is kept
as an explicit M7 runtime dependency rather than disguised as a shader or game
failure.

## Implementation

`CTR_NATIVE_RENDERER_GLES` is a CMake option. It defaults on for Android and
iOS and off elsewhere. Its production contract is:

```text
desktop: context 3.3, core profile, GLSL 140, native glad loader
GLES:    context 3.0, ES profile, GLSL ES 300, SDL proc loader
```

The profile attributes are set before `SDL_CreateWindow`. This ordering is
required because SDL's Cocoa video driver decides between its CGL and EGL
window setup during window creation.

The GLES path:

- requires `GLAD_GL_VERSION_3_0`;
- checks vertex-array, framebuffer, pixel-read and pixel-store entry points;
- emits `#version 300 es` plus explicit integer/float precision;
- disables desktop-only `glPolygonMode` wireframe;
- disables KHR debug-label calls; and
- disables the desktop GPU timer-query path.

Desktop GL retains the existing 3.x context fallback, GLSL 140 shader text,
debug labels, wireframe and timer instrumentation.

`--self-test-renderer-dialect` is CTest 14 of 18. It requires the exact
compile-time contract for the selected dialect and calls renderer shutdown
twice before any context exists. The latter covers idempotent cleanup after a
failed SDL/context boundary.

## Failure-path defect found and corrected

The first exact macOS GLES launch correctly failed to create an SDL window,
then exited 139. SDL had initialized, but renderer shutdown still called
unresolved GL deletion functions, and `main` continued after the failed
`Platform_Init` return.

Commit `78ef952dbecc` corrects both sides:

1. renderer cleanup calls GL entry points only after the loader contract has
   become ready, and clears that readiness before destruction; and
2. `main` checks `Platform_IsInitialized()` immediately after platform setup
   and returns failure rather than entering retail code.

The exact clean rerun now reports:

```text
[CTR Renderer] ... Could not initialize OpenGL / GLES library
[CTR Native] Failed to initialise window
process exit: 1
```

It no longer signals 139. Running the same binary under LLDB also exited 1,
with no stopped faulting process.

## Exact clean matrix

All values below are from clean code commit
`78ef952dbecca8ec92647f4c80dfcf5bc9e0e73e`.

### Signed macOS ARM64 desktop-GL app

```text
build ID:      78ef952dbecc
CTest:         18/18 in 0.70 seconds
architecture:  Mach-O 64-bit arm64
signature:     strict deep verification passed
SHA-256:       7b1f6190e1345ce45d74a12855eddda970b5c0af326f1d438eb10be8c01c16e8
warnings:      32 established warnings
```

A separate non-bundle exact build launched against the user's ignored local
retail asset link. It reported Apple M2 / OpenGL 4.1 Metal 90.5, compiled all
4-bit, 8-bit, 16-bit and RGBA PSX shaders, initialized both VRAM pipelines,
and opened a 44.1 kHz stereo CoreAudio stream. The bounded probe was stopped
with terminal Ctrl-C after initialization and closed its log; it is not
reported as a normal UI-close test.

Before the final clean rebuild, a visual inspection of the same production
path showed a coherent SCEA frame and, after keyboard input, a textured orange
crate with the green binary-stream presentation. The local-only JPEG was
92,914 bytes with SHA-256
`3e6e3b92e8f235b7868b0ee3f98e14ecc86524eb7b23c1e644ac24d439d80485`.
It was not copied into or tracked by Git because it contains retail-derived
pixels. This visual evidence guards desktop regression; it is not a GLES
frame comparison.

### macOS ARM64 GLES configuration

```text
build ID:      78ef952dbecc
CTest:         18/18 in 0.70 seconds
architecture:  Mach-O 64-bit arm64
signature:     strict deep verification passed
SHA-256:       9b37ee8f35e9973cf6d1b9eb37ec73a20384f9ef3bf69e669bf21c05562a818a
warnings:      32 established warnings
runtime:       clean diagnostic exit 1; ANGLE/EGL library unavailable
```

This verifies compilation, dialect selection, self-test behavior and graceful
host dependency failure. It does not verify a running GLES context or pixels.

### Combined ASan/UBSan ARM64

```text
build ID:      78ef952dbecc
CTest:         18/18 in 5.00 seconds
finding:       none
SHA-256:       caa8bae0bd55b391d482863429b1272388154fea21729807e5f99a78f326ba48
warnings:      59 established warnings
```

The first invocation set `ASAN_OPTIONS=detect_leaks=1`. Apple's ASan runtime
reported that leak detection is unsupported and all 18 tests aborted before
testing project behavior. That invocation is rejected. The accepted run used
`detect_leaks=0`, `halt_on_error=1`, `abort_on_error=1`, and UBSan halt plus
stack traces.

### Optimized Linux i686

```text
build ID:      78ef952dbecc
CTest:         18/18 in 2.98 seconds
architecture:  ELF 32-bit LSB PIE, Intel 80386
interpreter:   /lib/ld-linux.so.2
GNU Build ID:  477cb77efbc42a006dc8bc773761c67d2de22f66
SHA-256:       b2a9637d40bd253cb3fad5bdf1f9618541af49d8c50be8d2b272960cf5c0f377
warnings:      four established warnings
```

The cached disposable tree was
`/private/tmp/ctrpad-i686-controller-gXAeRV`. The repository was mounted
read-only at `/src`, output read/write at `/out`, and the pinned
`ctrpad-linux-i686:ubuntu-24.04` image compiled and linked with `-m32`.

An initial command incorrectly requested Docker platform `linux/386` even
though the pinned builder image is `linux/amd64` and produces i686 through its
multilib toolchain. Docker attempted a nonexistent registry pull and exited
125. That invocation is rejected. Re-running the existing image as
`linux/amd64` produced the accepted 32-bit binary above.

### iOS Simulator ARM64 compile/link probe

```text
build ID:      78ef952dbecc
architecture:  Mach-O 64-bit arm64
linked:        UIKit, OpenGLES, Foundation, AVFoundation
shader text:   #version 300 es
loader text:   sdl-proc
SHA-256:       5b35b6160e04ebe64c90287987d617b3113d49b5606114e4822c5c6f369e1e22
warnings:      32 established warnings
```

Configuration used `CMAKE_SYSTEM_NAME=iOS`, the iPhone Simulator SDK, ARM64,
iOS 15.0 minimum, Release, GLES enabled, the macOS bundle disabled, and tests
disabled. SDL reported platform iOS, UIKit video, OpenGL off, OpenGLES on, and
its `ogl_es2` renderer backend.

The generated `Info.plist` lints as syntax but has empty identifier, name,
version and copyright fields. `codesign -d` reports an ad-hoc linker signature,
no team, unbound Info.plist and no sealed resources; strict verification
reports that resources required by the signature are absent. The artifact is
therefore not installable, signed-device, lifecycle, launch or M8 evidence.

## Rejected and local-only setup attempts

- The first app-bundle launch lacked a reachable retail asset and stopped in
  asset validation. It was rejected as renderer evidence.
- A temporary bundle-local `assets` symlink was then tried. Adding unsealed
  content invalidated the signed macOS bundle, so the link was immediately
  removed and strict deep verification was rerun successfully. No bundle
  mutation remains.
- Retail disc links existed only in ignored build directories. No disc bytes,
  hashes, frame capture or derived retail artifact entered Git.
- The macOS GLES dependency failure is retained as an open runtime boundary;
  it is not relabeled as a passing GLES render.

## Acceptance boundary and next work

Accepted by this slice:

- one shared production renderer now has explicit desktop and GLES 3 dialects;
- desktop-only features are isolated from the ES build;
- GL/GLES context profile selection occurs at the correct SDL lifecycle point;
- missing-context cleanup is safe and idempotent;
- desktop GL behavior still reaches shaders, VRAM and audio;
- the GLES path compiles on macOS and iOS ARM64; and
- ordinary, sanitizer and i686 deterministic gates agree at 18/18.

Still open for M7:

- supply a reproducible macOS ANGLE/EGL runtime or use another executable ES
  environment;
- launch the GLES renderer and capture representative frames;
- compare CLUT color, mask-bit, transparency and framebuffer-feedback cases;
- run the golden state/cadence suite under a live GLES renderer; and
- establish that renderer selection does not alter frame cadence.

Still open for M8 and later milestones:

- complete app metadata, SDL-owned iOS entry/lifecycle, display pacing,
  sandbox paths and signing configuration;
- install and run on Simulator and real ARM64 iPad;
- device audio, controller, background/resume and save acceptance;
- document-picker retail import; and
- playable simultaneous analog touch controls.

## Time and concurrent verifier

The bounded renderer checkpoint ran approximately 06:25–07:08 CDT. The goal
timer advanced from 144,159 to 146,335 active seconds during the recorded
work, a 2,176-second interval (36 minutes, 16 seconds), and stood at
1 day, 16 hours, 38 minutes, 55 seconds at the documentation checkpoint. This
is product-task elapsed time, not a labor estimate or renderer benchmark.

The protected historical i686 alternate-loader verifier was never restarted,
paused or rebuilt. At the checkpoint Docker reported running, unpaused and
not OOM-killed. Playback 2 had crossed fixed 2,000-frame windows through frame
10,000 at 2.26, 1.52, 1.06, 1.40 and 1.47 FPS. Its machine-owned status file
remained empty. Alternate playback completion, captured exit zero, raw-layout
separation and deliberate mutation therefore remain unaccepted.
