# Targeted desktop-GL / iOS-GLES renderer pixel semantics

Date: 2026-07-31

Implementation commit: `818bc0e161d3`

Documentation parent: `818bc0e161d3`

Result: **accepted for M7's representative pixel-semantics criterion; M7 and
the overall goal remain in progress**

## Scope and verdict

The production renderer now has a deterministic, retail-media-free 32-by-16
pixel oracle. It sends ordinary PS1 draw packets through
`ParsePrimitivesLinkedList` and `DrawAllSplits`, captures the real render
target, asks the production framebuffer packer to write RGB5551 back into
VRAM, and checks both representations.

Exact commit `818bc0e161d3` produced the same complete-frame FNV-1a hash on
Apple M2 desktop OpenGL and UIKit OpenGL ES:

```text
851169f2644a1675
```

The oracle directly accepts all locally testable cases that were still open
after the full 24,232-frame renderer-state comparison:

- 4-bit and 8-bit indexed textures with their real CLUT lookup paths;
- 16-bit direct textures;
- transparent-zero discard and preservation of the existing framebuffer;
- non-STP texels in the opaque pass and STP texels in the average-blend pass;
- exact alpha/STP transport;
- the CTR-facing E6 output-mask bit;
- a 16-bit texture page overlapping the active framebuffer, forcing the real
  framebuffer-feedback barrier; and
- RGBA-to-RGB5551 packing followed by GPU-to-CPU VRAM readback.

This closes M7 acceptance criterion 2 for the implemented game-facing
semantics. It does not make M7 complete. The host still lacks the ANGLE/EGL
runtime needed to execute SDL's Cocoa GLES backend, and physical-iPad cadence
and energy remain unmeasured.

## Source audit and deliberately bounded mask claim

The existing CTR-facing `setDrawStp` macro emits E6 bit 0 only
(`include/psx/libgpu.h:234`). `SetPSXMaskState` likewise stores only bit 0
(`platform/native_gpu.c:1095-1098`), and the game-facing call sites do not
request destination-mask rejection. A separate SDK header can describe E6 bit
1, but it is not the production CTR packet path.

The oracle therefore validates the implemented output-mask bit and never
claims PS1 destination-mask rejection. Adding an unconsumed bit-1 test would
have overstated the current game-facing contract.

The CLI gate is declared in `include/platform/native_renderer.h:6-10`, parsed
once in `main.c:510-584`, and runs after sandbox storage initialization but
before retail-media selection at `main.c:643-660`. This order lets the exact
same media-free command run under UIKit without opening or modifying the
imported disc image or memory card. Desktop Apple GL registers the test with
CTest at `CMakeLists.txt:313-318`; iOS uses an explicit `simctl launch` because
UIKit owns the app process and display lifecycle.

## The production test scene

The scene is constructed at `platform/native_renderer.c:2586-2786`. Blue is
the initial framebuffer background. The selected checks are:

| X range | Production path | Required observation |
| --- | --- | --- |
| 0-3 | 4-bit texture + 16-entry CLUT | zero preserves blue; red, STP green and blue decode correctly |
| 4-7 | semi-transparent 4-bit texture | zero is discarded; non-STP red is opaque; STP green averages with blue |
| 8-11 | 8-bit texture + 256-entry CLUT | red, STP green, zero and blue decode correctly |
| 12-15 | 16-bit direct texture | red, STP green, zero and STP white preserve exact semantics |
| 16-19 | E6 output-mask set + red tile | rendered alpha is set and packed VRAM word is `0x801f` |
| 24-27 | framebuffer-overlapping 16-bit page | the prior blue/red/STP-green/blue pixels are sampled through the feedback barrier |

RGBA comparisons at `platform/native_renderer.c:2794-2813` use small color
tolerances for host rasterization but require alpha exactly. VRAM comparisons
at `platform/native_renderer.c:2815-2823` require exact RGB5551 words. The
entire 2,048-byte RGBA result is also hashed, so a change outside the selected
pixels cannot silently pass (`platform/native_renderer.c:2678-2687`).

## Rejected fixture and corrected expectation

The first desktop fixture colored the mask tile with input red 248 and
expected output 248 / RGB5551 `0x801f`. It failed with red 241 and word
`0x801e`. That result was the production 16-bit shader's normal modulation and
quantization, not a mask failure. The input was corrected to 255 so the test
isolates E6 output-mask behavior instead of coupling it to an accidental
double-quantization expectation.

After that fixture correction, desktop GL passed every selected RGBA and VRAM
word with hash `851169f2644a1675`.

## The live GLES failure that exposed a real bug

The first UIKit/GLES run was intentionally treated as an independent gate,
not assumed correct because the screen looked coherent. Every RGBA semantic
check passed and the complete-frame hash already matched desktop GL exactly,
but every packed VRAM expectation read as zero.

The failing path used `glReadPixels(..., GL_RG, GL_UNSIGNED_BYTE, ...)` on the
RG8 VRAM framebuffer. Desktop GL accepts that pair. OpenGL ES 3 guarantees an
RGBA/UNSIGNED_BYTE readback pair, but not this RG readback pair; Apple's GLES
implementation returned `GL_INVALID_OPERATION`. The prior code did not check
the error and then marked the GPU-newer tiles synchronized, leaving a stale
zero CPU mirror.

The correction at `platform/native_renderer.c:1976-2012` now:

1. allocates a bounded temporary four-byte-per-pixel buffer;
2. reads the RG8 attachment through guaranteed RGBA/UNSIGNED_BYTE;
3. checks the GL error;
4. repacks R and G into the low and high bytes of each CPU RGB5551 word; and
5. clears dirty-tile ownership only after a successful readback.

Desktop GL retains its direct RG readback, now with the same error-aware dirty
tile behavior (`platform/native_renderer.c:2013-2024`). The maximum temporary
allocation is two MiB for the 1,024-by-512 VRAM surface.

The corrected UIKit run passed both RGBA and exact VRAM words with the same
hash as desktop. This was a production renderer fix found by the new oracle,
not merely test scaffolding.

## Exact-commit validation

The implementation was committed and pushed before final validation so every
listed binary embeds clean build ID `818bc0e161d3`.

| Target | Result | Executable SHA-256 |
| --- | --- | --- |
| macOS 26.5 ARM64 desktop GL | Apple M2; direct oracle passed; full CTest 22/22 | `0e46d0f6872f710f7fcdbce8f38b4d3b688e4ef3cf5d3b2a4d9abbc7c4d65980` |
| iPad Simulator ARM64 UIKit/GLES | Apple Software Renderer; live oracle passed | unsigned build product `f9a97d5e8d700f55142da9a9bb26ada4caccc2fa7c89a758ccdfd6e6d6cbf274` |
| iPhoneOS ARM64 | compile/link passed; Mach-O reports arm64 | `75df7a6002e7d2a7ad4397d3b544335317ce1fc661bdcc42439a27962b31b2ec` |
| macOS ARM64 GLES configuration | compile/link passed; build ID exact; runtime unavailable without EGL/ANGLE | `56f35dfc261135f2daed632f49b990bcdb183f21fb7777ff495ae08ef245901e` |
| macOS ARM64 ASan/UBSan | full CTest 22/22 | `5837b1e393d2e9c8ab4c4806b84192e5e3c44eee0e1c85c1cc3d47124ec5f9ac` |

The exact desktop marker was:

```text
[CTR Renderer] pixel self-test passed: api=gl size=32x16
formats=4,8,16 clut=4,8 transparency=zero,stp blend=average
mask=output-bit framebuffer=feedback vram=rgb5551
hash=851169f2644a1675
```

The live Simulator used disposable `CTRPad Import Negatives`, UDID
`26F3DEE8-8840-446D-85FE-C882009C9C06`. It reported a 1,032-by-1,376 UIKit
surface, presentation framebuffer/renderbuffer 1, Apple Software Renderer,
OpenGL ES 3.0 APPLE-23.1.1 and GLSL ES 3.00. All four PSX shaders and both
VRAM pipelines compiled before it emitted the corresponding `api=gles` marker
with the identical hash.

The temporary ad-hoc-signed app used only for installation had executable
SHA-256
`e5d515cf82720c9b70d7f66729465b06a1eaa823f6709c995cce5f99708b0f98`.
Signing explains why it differs from the unsigned build-product hash; no
retail data entered either app bundle.

## Commands and non-product failures

The clean desktop sequence was:

```text
cmake -S . -B build-macos-arm64
cmake --build build-macos-arm64 --parallel
build-macos-arm64/ctr_native --version
build-macos-arm64/ctr_native --self-test-renderer-pixels
ctest --test-dir build-macos-arm64 --output-on-failure
```

An initial exact-commit chain mistakenly named the executable
`build-macos-arm64/ctr`. Configuration and compilation succeeded, then zsh
reported that nonexistent path and the `&&` chain stopped before tests. The
same built product was immediately verified through the correct
`ctr_native` path; this is a command typo, not a build failure.

Before the exact commit, one sanitizer invocation set
`ASAN_OPTIONS=detect_leaks=1`. Apple's arm64 AddressSanitizer reports
LeakSanitizer unsupported, so every test process aborted before exercising
code. The option was removed and the supported default ASan/UBSan run passed
22/22. The final exact-commit sanitizer run also passed 22/22 without that
unsupported option.

After shutdown, a final `simctl get_app_container` query returned
CoreSimulator error 405 because a shutdown device cannot resolve its current
container through that command. The already printed exact container path was
used for the read-only save check. A broad hash loop also began traversing
several intentionally retained 600-740 MB negative-import fixtures; it was
stopped after the canonical media hash was confirmed because hashing every
archival duplicate added no evidence. Neither event launched the game or
changed repository/app data.

The UIKit app retains SDL's pre-existing repeated unbalanced appearance-
transition warnings after its self-test main returns. The semantic gate had
already completed successfully. The app was explicitly terminated and the
disposable Simulator shut down.

## Data preservation and boundary

The exact self-test returns before retail asset selection. After final install
and execution, the clone's canonical files retained their established inode,
size and digest:

```text
retail BIN
inode 111313696, 605698800 bytes
SHA-256 f780bf2331476aabfc00772fa758b12dd95ebfbc907968132cbd3cdd4e2c07c0

save
inode 111309627, 6016 bytes
SHA-256 6a01b0f5562ed7a279d8f8e51e3b1874ac39a6120f55db4fe3873288950619a3
```

The install assigned a new disposable-clone app/data container, which is
normal Simulator behavior, while preserving those file identities. The clone
was shut down without deletion. Protected `CTRPad Import Validation` remained
booted and was never installed to, launched, or inspected by the oracle.

No game asset, retail media, save, screenshot or generated build product was
added to Git. Only source and documentation are published.

## Remaining gates

M7 now has both accepted renderer-choice state/cadence evidence and accepted
targeted pixel semantics. It remains in progress for live execution of the
shared GLES path on macOS (or an equivalent reproducible ES host) and
physical-iPad cadence/energy confirmation.

The broader goal also retains user-owned signing/physical-device install,
natural physical multi-touch/controller/keyboard delivery, completed-race
playtesting and device Files/save lifecycle gates. This checkpoint makes none
of those claims, and the overall goal remains active.
