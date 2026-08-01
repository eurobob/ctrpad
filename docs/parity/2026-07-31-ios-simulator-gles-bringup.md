# iPad Simulator UIKit/GLES bring-up — 2026-07-31

## Result

Implementation commit `98ae2c6d86fe527fb4ae4977357cec51d8a5f46f`
turns the previous iOS compile/link probe into a CMake-generated application
bundle for ARM64 Simulator and ARM64 device targets. SDL owns the iOS entry
point, the bundle has non-personal metadata and landscape declarations, and
the production GLES renderer presents through the framebuffer and
renderbuffer that SDL's UIKit backend owns.

An exact clean Simulator build launched on an iPad Pro 13-inch (M5) Simulator,
reported source identity `98ae2c6d86fe`, created a 1376-by-1032-pixel GLES 3
surface, compiled all four PSX shaders and both VRAM pipelines, opened the
44.1-kHz stereo CoreAudio stream, and visibly rendered coherent full-frame CTR
presentation art and the textured title menu. This is the first accepted live
GLES pixel evidence in the project. It is a bounded M7/M8 bring-up result, not
acceptance of either milestone.

The local test package contained the user's ignored NTSC-U image solely so the
existing direct asset path could reach retail startup. The package and every
retail-derived screenshot stayed under `/private/tmp`; neither is tracked.
The generated app does not yet import a disc, persist it in the sandbox, or
represent a distributable build.

Basic keyboard controls were already implemented in commit
`2c10b00b34df4f0eb61aa8b72cbe99588a930ed6` and are documented in the root
README. The clean iPad run captured the host keyboard and sent `C` during the
intro, after which the title menu appeared; the natural intro could also have
ended in that interval, so the observation is not accepted on its own. An
earlier pre-commit run of the same UIKit/input path used `C` to advance the
title menu into Adventure immediately. Repeated clean-run menu taps were not
consistent, so the earlier result remains diagnostic and full iPad keyboard
delivery remains open. The media-free quick-tap gate and live macOS navigation,
acceleration and steering results remain the authoritative keyboard acceptance
evidence.

## Source changes

The implementation commit changes six files:

- `CMakePresets.json` adds `ios-simulator-arm64` and `ios-device-arm64`
  configure/build presets. Both target ARM64, iOS 15.0, GLES, Ninja and
  `RelWithDebInfo`, with tests disabled for the app bundle.
- `CMakeLists.txt` makes the iOS executable a bundle, generates its plist,
  identifies iPhone and iPad device families, and exposes
  `CTR_NATIVE_IOS_BUNDLE_IDENTIFIER` with the credential-free default
  `io.github.chrissotraidis.ctrpad`.
- `platform/apple/Info-iOS.plist.in` declares the package, iOS 15.0 floor,
  iPhone/iPad families, landscape-left/right support, indirect input events,
  full-screen/status-bar behavior and GPL/user-supplied-retail-data notice.
  It contains no developer team or signing identity.
- `main.c` keeps `SDL_MAIN_HANDLED` on non-iOS hosts but allows SDL/UIKit to
  own the iOS application entry point.
- `platform/native_platform.c` selects landscape through SDL's iOS hint,
  reports SDL initialization errors, uses physical-pixel dimensions and
  handles pixel-size changes.
- `platform/native_renderer.c` queries SDL's UIKit OpenGL framebuffer and
  renderbuffer window properties. The renderer binds that presentation FBO
  instead of assuming object zero and rebinds the UIKit RBO before swap.
  Desktop GL continues to use object zero.

The application still uses the single shared renderer introduced by
`4695d9cb3`; this is not an iOS-specific rendering fork.

## Bring-up chronology and rejected attempts

The complete observed sequence matters because a successful final frame alone
would hide three independent boundaries.

1. The first temporary Simulator package used the compile-probe's generic,
   mostly empty plist. It was manually patched and ad-hoc signed only to
   discover the next failure. That package is rejected as a reproducible
   application result.
2. The process initially stopped during SDL initialization. Added diagnostic
   output reported that SDL's main entry had not been initialized. The cause
   was the unconditional `SDL_MAIN_HANDLED`; allowing SDL to own iOS `main`
   corrected startup.
3. Retail code, `RenderSubmit`, `DrawOTag` and `glDrawArrays` then ran, but the
   Simulator stayed black. Switching from window points to physical pixels
   corrected sizing but did not correct presentation, so that hypothesis was
   rejected as the black-frame root cause.
4. SDL source and live window-property inspection showed that UIKit created
   framebuffer 1 and renderbuffer 1. The renderer had rebound framebuffer 0
   before every draw. On desktop that is the presentation target; under
   SDL/UIKit it is not. Querying and preserving SDL's presentation objects
   produced visible pixels.
5. The first visible full app was installed while the Simulator hardware was
   portrait. UIKit supplied a large landscape drawable inside the portrait
   host window, so the frame appeared clipped/letterboxed. The plist and SDL
   orientation hint were correct; rotating Simulator hardware to Landscape
   Right produced the full landscape presentation. This is a host Simulator
   state distinction, not evidence that physical-device rotation is complete.
6. Once the reproducible plist, presets, SDL-owned entry and UIKit presentation
   path were committed, every producer was rebuilt from clean
   `98ae2c6d86fe`. A fresh app copy was populated only in `/private/tmp`,
   ad-hoc signed, strictly verified, installed and launched again. The final
   log and visual evidence below come from that exact build.

No retail data, temporary package, signing material or captured frame was
copied into the repository during this sequence.

## Exact clean Simulator execution

Environment:

```text
source:       98ae2c6d86fe527fb4ae4977357cec51d8a5f46f
simulator:    iPad Pro 13-inch (M5), iOS 26.5, ARM64
bundle ID:    io.github.chrissotraidis.ctrpad
deployment:   iOS 15.0 minimum; iOS Simulator SDK 26.5
orientation:  Simulator hardware Landscape Right
package:      local-only ad-hoc signature, no team identifier
```

The production log established:

```text
[CTR Native] Version: 0.1.0-beta.7.1 (98ae2c6d86fe)
[CTR Renderer] *Window size: 1376x1032 points, 1376x1032 pixels
[CTR Renderer] *Presentation objects: framebuffer=1 renderbuffer=1
[CTR Renderer] *Video adapter: Apple Software Renderer by Apple Inc.
[CTR Renderer] *gles version: OpenGL ES 3.0 APPLE-23.1.1
[CTR Renderer] *GLSL version: OpenGL ES GLSL ES 3.00
[CTR Renderer] *PSX shaders ready
[CTR Renderer] *VRAM pipelines ready
[CTR Native] SDL audio stream opened: driver=coreaudio src=44100 Hz/2 ch
             dst=44100 Hz/2 ch device=44100 Hz/2 ch sampleFrames=1024
```

Visual inspection showed a correctly filled landscape viewport, checkered
cloth motion, Crash and the trophy, the blue CTR ring, logo, menu text,
transparency and coherent colors/textures. There was no longer a black frame,
portrait crop or missing presentation surface. The local-only 932-by-768 JPEG
is 199,375 bytes with SHA-256
`77916c2f69de6ef39432006a9aa3f8ca778aa20f261ff764576bd29be30a8a12`.
Its retail-derived pixels are deliberately not tracked.

The process emitted two identical warnings:

```text
Unbalanced calls to begin/end appearance transitions for
<SDL_uikitviewcontroller ...>
```

The Simulator runtime also emitted a duplicate accessibility-loader warning
from its own WebCore/WebKit bundles. Neither warning prevented the bounded
run, but the appearance-transition warning keeps suspend/resume, rotation and
view-controller lifecycle acceptance open. The app was terminated with
`simctl` after the bounded check; this is not a natural lifecycle-close test.

## Keyboard evidence and layout

The practical keyboard layout is:

```text
arrows or W/A/S/D       D-pad / steering
C or K                  Cross / accelerate / accept
X or J                  Square
Z or I                  Triangle
V or L                  Circle / brake / back where retail permits it
Left Shift/Ctrl/[        L1/L2/L3
Right Shift/Ctrl/]       R1/R2/R3
Q / E                    alternate L1 / R1
Enter or P               Start
Space or Tab             Select
```

The input layer latches mapped key-down edges until exactly one retail pad
poll, preserving a quick press whose down/up events both occur between CTR's
approximately 29.9-Hz polls. The existing media-free test requires exact
active-low PSX packet bits, aliases and a simultaneous `K+D+E` chord. Exact
ARM64, sanitizer and i686 coverage for that behavior is recorded in
`2026-07-31-macos-arm64-keyboard-tap.md`.

For this iPad slice, Simulator's Capture Keyboard state was visibly enabled.
On the exact clean app, `C` was sent during the CTR intro and the rendered
title menu appeared afterward. Because the intro could have ended naturally
during the observation interval, this is not sufficient delivery proof.
Repeated `C`, `K` and `S` taps at the resulting menu did not produce a reliable
visible transition. A pre-commit run of the same source path had advanced from
that menu into Adventure immediately with `C`; because the exact clean
repetition was inconsistent, that stronger observation is retained as
diagnostic evidence only. Keyboard capture was released before termination.

This does not weaken the already accepted macOS basic keyboard support. It
keeps the iPad Simulator delivery/menu-navigation repetition open, alongside
physical keyboard and controller tests on real iPad hardware.

## Exact clean build matrix

All rows use clean implementation commit `98ae2c6d86fe`.

### macOS ARM64 desktop GL

```text
CTest:         18/18 in 3.58 seconds
architecture:  Mach-O 64-bit arm64
signature:     strict deep ad-hoc verification passed; no team identifier
SHA-256:       ee690c9fc934c4a4735f1373b41a9a2dc5f479c706be136203daec6c4e8bfed1
warnings:      32 established warnings
```

### macOS ARM64 GLES configuration

```text
CTest:         18/18 in 1.60 seconds
architecture:  Mach-O 64-bit arm64
SHA-256:       1d9fb361edcf7442cc70338d7c2881aef6a7192ce0b2171d531dabf59f2bbfa4
runtime:       diagnostic exit 1; Cocoa ANGLE/EGL library unavailable
```

The exact log reports the missing GL/GLES library, closes the log and exits 1
without a signal. This remains expected host-dependency evidence, not a live
macOS GLES pass.

### Combined ASan/UBSan ARM64

```text
CTest:         18/18 in 10.82 seconds
ASAN_OPTIONS:  detect_leaks=0, halt_on_error=1, abort_on_error=1
finding:       none
SHA-256:       33ad829f4f1d0baa51a20ed3f1dad032ef7b04badbc453a32ceb2cf5be4dc700
warnings:      59 established warnings
```

### Optimized Linux i686

```text
CTest:         18/18 in 4.23 seconds
architecture:  ELF 32-bit LSB PIE, Intel 80386
interpreter:   /lib/ld-linux.so.2
GNU Build ID:  dfac03fc776068dfd25ee53f0284975b1d914217
SHA-256:       4bcc7844e9cd107ea0ddc67e734397a0df420d6b635843454975289742c21611
warnings:      four established warnings
```

The pinned `ctrpad-linux-i686:ubuntu-24.04-local` builder mounted the
repository read-only at `/src` and the disposable cached output tree at
`/out`. The binary embeds `98ae2c6d86fe`. It is separate from and did not
modify the protected long-running historical verifier.

### iOS Simulator ARM64

```text
architecture:  Mach-O 64-bit arm64; platform IOSSIMULATOR
deployment:    iOS 15.0 minimum; SDK 26.5
SHA-256:       9e09fb41b41ba63339e26ac733de31fc1b8c196f6a9089d97929d201b1709771
warnings:      32 established warnings
live result:   install, launch, GLES shaders/VRAM, audio and pixels reached
```

The raw CMake product has only its linker ad-hoc signature and fails strict
bundle verification because it has no sealed resources. The exact local test
copy was signed after its temporary asset was added and passed strict deep
verification. It has no team identifier and is Simulator-only.

### iOS device ARM64

```text
architecture:  Mach-O 64-bit arm64; platform IOS
deployment:    iOS 15.0 minimum; SDK 26.5
SHA-256:       09576e97b9bde31d89f41efcb52388777ed54f7330dcaf668cf9f5202f1d3f43
warnings:      32 established warnings
live result:   none
signature:     unsigned
```

This proves the physical-device source compiles and links. It does not prove
developer signing, installation, launch or execution on ARM64 iPad hardware.

## Acceptance boundary

Accepted by this slice:

- reproducible CMake application bundles for iOS Simulator and device ARM64;
- non-personal iPhone/iPad metadata and landscape declarations;
- SDL-owned iOS entry rather than a manually initialized foreign main;
- full-pixel-size UIKit GLES surface discovery;
- presentation through SDL/UIKit's nonzero framebuffer/renderbuffer;
- exact clean Simulator launch through production retail startup;
- live GLES 3 shader/VRAM output with coherent title-menu pixels;
- live CoreAudio stream initialization; and
- a bounded Simulator keyboard attempt with capture-state evidence and an
  explicitly unaccepted clean-run delivery result.

Still open for M7:

- representative CLUT, mask-bit, transparency and framebuffer-feedback frame
  comparisons rather than one visual title-menu check;
- renderer-independent state/cadence measurement under live GLES; and
- either a runnable macOS GLES dependency or a clearly iOS-scoped replacement
  for the macOS wording of the current acceptance gate.

Still open for M8 and later:

- SDL/UIKit lifecycle correction, background/suspend/resume and rotation;
- display-driven pacing that preserves the retail VBlank model;
- controller/MFi/Bluetooth input and complete keyboard repetition on iPad;
- physical iPad signing, install, launch, audio/video and full-race play;
- sandbox paths, document-picker retail import and persistent saves;
- genuinely simultaneous analog touch controls; and
- GPL-complete build/sign/install publication without retail bytes or secrets.

No M8 acceptance, signed-device claim or completion percentage is inferred
from the Simulator title menu.

## Time and concurrent verifier

This slice began after the prior renderer checkpoint at goal elapsed
146,335 seconds. The first documentation reading was 150,428 seconds. At the
pre-commit checkpoint at 08:23:51 CDT, the goal API reported 150,887 seconds,
or 1 day, 17 hours, 54 minutes, 47 seconds cumulative. The 4,552-second
interval since the prior checkpoint is 1 hour, 15 minutes, 52 seconds of
product-task elapsed time; it is not a labor estimate or performance
benchmark.

The protected historical i686 alternate-loader verifier was not restarted,
paused, rebuilt or terminated. Docker continued to report running, unpaused
and not OOM-killed, while its machine-owned status file remained empty. Its
alternate playback, captured process exit, host-layout separation and
deliberate mutation therefore remain unaccepted regardless of this app-shell
result.
