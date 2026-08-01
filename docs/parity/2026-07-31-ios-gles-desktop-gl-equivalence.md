# iOS GLES versus macOS desktop-GL equivalence

- Date: 2026-07-31
- Accepted implementation commit: `e6ba535a9c73`
- Documentation tip before this result: `23029b2c75e9`
- macOS renderer: Apple M2 desktop OpenGL 4.1 / GLSL 4.10
- iOS renderer: iOS 26.5 ARM64 iPad Simulator OpenGL ES 3.0 / GLSL ES 3.00

## Result

The shared production renderer has now crossed a representative deterministic
equivalence gate between macOS desktop GL and live SDL/UIKit GLES. Exact
current binaries independently regenerated a 2,000-frame boot-origin scenario
from the same pad and VSync seed. The reports match on every frame across all
six canonical state components plus pad and VSync transport.

Exact native playback then hashed the complete PS1 vertex and draw-split stream
at frames 1,802 and 1,813. macOS desktop GL and iOS GLES produced byte-for-byte
equal trace lines at both frames. A local-only iOS framebuffer captured during
the same scenario visibly showed coherent textured karts, track preview,
exhaust transparency, menu text and the complete touch overlay.

This accepts a 2,000-frame live-GLES state/cadence slice and representative
render-command equivalence. It does not accept the whole M7 milestone: this
Mac still lacks the ANGLE/EGL runtime needed for live Cocoa GLES, the full
24,232-frame suite has not run under GLES, and Simulator software-renderer
throughput is not physical-iPad performance evidence.

## Why fresh platform-native reports were required

The first diagnostic tried to play the mid-session iOS keyboard report
`ctr-201917` directly in the current macOS executable with
`--replay-bypass-header`. The report correctly identified a mismatch:

```text
replay: platform=ios   identity=0x6ea1d76d executable=e9d9b4240372487c
live:   platform=macos identity=0xaa46653b executable=8edd476c95a09be7
```

The bypass reached checkpoint restore, printed the expected unequal raw
host-address diagnostic, and then failed instead of producing a valid
cross-platform playback. macOS displayed a crash report for that diagnostic
process. This route is rejected. `docs/REPLAYS.md` already states that
`--replay-bypass-header` does not make checkpoints portable across rebuilt or
identity-incompatible executables and must not be used as parity evidence.

The accepted route used `--record-from-replay`. It consumes only validated pad
snapshots and complete version-4 VSync timing while each platform creates its
own native checkpoint and canonical state. This is the supported way to compare
different executable identities without importing host pointers from the other
process.

## Exact binaries

Both executables were previously built clean from implementation commit
`e6ba535a9c73` and remained unchanged for this result:

```text
macOS ARM64 desktop GL
SHA-256 cfd3d9b420e8e9592a622112bd368b20857ba427d68c92fab58bd8578d741dca
report fingerprint 8edd476c95a09be7

iOS Simulator ARM64 GLES
SHA-256 1cef6404aa3c2bf094c3357e71eb1069e81ed3e6307b972d043bfe222c2c62cb
report fingerprint e9d9b4240372487c
```

Both are thin ARM64 Mach-O executables. The Simulator executable was the same
installed image accepted by the keyboard milestone, not a modified diagnostic
build.

## Input and timing seed

The input producer was the accepted clean macOS version-4 report
`ctr-215303`. A local-only 2,000-frame prefix was made by retaining its exact
148-byte header and first 2,000 complete 440-byte records, then setting only
the declared frame count to 2,000. It had:

```text
size     880148 bytes
SHA-256  6a26357cc517ee57e10f9bd55a3a8f48db13edd93ae2a6fa1a225f5685aabf49
```

The existing strict comparator validated that all 2,000 records were an exact
prefix of `ctr-215303` across timing, RNG, drivers, world, allocation, root,
pads and VSync. That comparison validates the seed construction; the inherited
state digests were not reused as output evidence. Each current executable
generated fresh state through `--record-from-replay`.

The seed remained under `/private/tmp` and the Simulator's private Application
Support tree. It contains input/timing metadata and no retail asset bytes.

## Fresh current reports

### macOS desktop GL

```text
report              build-macos-arm64/debug/reports/20260731/ctr-204509
build ID            e6ba535a9c73
platform            macos
identity checksum   0xaa46653b
frames              2000
rolling checkpoints 7
input.ctrreplay      e02687209a6f4133b97afa78e4783d56777a2ecf186f6ff64d88b417ec3efd3d
state.ctrstates      1cc4229b323916d2d49b557607a347bf55f38fc46b3de49be057ed0152a44a9e
```

The report finalized naturally with `replay-seeded recording finished after
2000 frames` and `LOG CLOSED`. It observed the scenario's active race-driver
transition at frame 1,711.

### iOS UIKit/GLES

```text
report              debug/reports/20260731/ctr-204914 in the clone container
build ID            e6ba535a9c73
platform            ios
identity checksum   0x6ea1d76d
frames              2000
rolling checkpoints 7
input.ctrreplay      e6307c77e19b600c5658f0a1e73804e105ef22b0eb67787ba12438d13cb76d1f
state.ctrstates      0bb86d689edcb23638bc4c853d68c971449247d4ddd0c22be44dd4344442de8e
```

The exact app initialized framebuffer/renderbuffer 1, all four PSX shaders,
both VRAM pipelines, the touch overlay and the UIKit display loop. It finalized
naturally at 2,000 frames and seven checkpoints with `LOG CLOSED`, also
observing the race-driver transition at frame 1,711.

The two replay files do not have equal whole-file hashes because their headers
truthfully identify different platform executables and their records contain
platform-native report metadata. Equality was evaluated through the canonical
typed comparator rather than raw-file identity.

## Strict state and cadence comparison

The accepted command required every available semantic component:

```sh
node tools/compare-replay-state-components.mjs \
  --require timing,rng,drivers,world,allocation,root,pads,vsync \
  build-macos-arm64/debug/reports/20260731/ctr-204509/input.ctrreplay \
  /path/to/ctr-204914/input.ctrreplay
```

Result:

```text
timing:     equal=2000 mismatched=0 ranges=none
rng:        equal=2000 mismatched=0 ranges=none
drivers:    equal=2000 mismatched=0 ranges=none
world:      equal=2000 mismatched=0 ranges=none
allocation: equal=2000 mismatched=0 ranges=none
root:       equal=2000 mismatched=0 ranges=none
pads:       equal=2000 mismatched=0 ranges=none
vsync:      equal=2000 mismatched=0 ranges=none
```

Separate seed-to-output comparisons also found all 2,000 pad and VSync records
equal for both producers. This proves the renderer/platform choice did not
change game-visible state or the replayed retail timing boundary in this
slice. It is deterministic cadence evidence, not a wall-clock FPS claim.

## Representative render traces

The playback-only trace hashes packed 20-byte `GrVertex` data and canonical
draw-split state, including semantic texture kind, blend mode, texture format,
mask state, clip/draw/display environments and vertex ranges. Host GL object
names and pointers are excluded by design.

Each report was replayed only by its exact producer executable. Checkpoint 6
restored native frame 1,800 state; both playbacks reached their normal
2,000-frame finish without canonical divergence.

### Frame 1,802

Both renderers emitted exactly:

```text
flush 0 hash 85435f06301f654a
aggregate    63f8c781f85e4358
flushes      1
vertices     6000
splits       338
formats      4-bit=313, 8-bit=25, 16-bit=0, RGBA=0
```

An older rejected trace at this frame reported 25 16-bit feedback splits. That
was not desired coverage: it was the already-corrected LP64 mosaic texture-word
classification bug documented in the ARM64 regeneration history. The fixed
i686/ARM64 oracle required zero false 16-bit batches. Current desktop GL and
GLES agree on that corrected behavior.

### Frame 1,813

Both renderers emitted exactly:

```text
flush 0 hash 4971e1a64577397f
aggregate    15c8c5410da67002
flushes      1
vertices     5958
splits       346
formats      4-bit=319, 8-bit=27, 16-bit=0, RGBA=0
```

Direct `diff -u` of all three trace lines for each frame produced no output.
Both playbacks ended with `replay finished after 2000 frames` and
`LOG CLOSED`.

## Visual evidence

A local-only full Simulator framebuffer was captured as the recorder passed
frame 1,813:

```text
dimensions 2064 x 2752
SHA-256    c7ec9117fa5dd550aab4e9d6def3270e4f1b199870d4df40fa4a2b09a469a24e
```

It visibly showed the Arcade Crash Cove selection/preview with coherent kart
textures and colors, layered exhaust transparency, track/checker textures,
legible menu text and the entire portrait-safe-area touch overlay. There were
no missing primitives, striped LP64 texture corruption, black presentation
surface or obvious CLUT failure. The framebuffer remained outside Git because
it contains retail-derived pixels.

A macOS full-desktop capture attempt was obscured by the crash reporter left
by the rejected cross-platform checkpoint bypass. It is not used as visual
evidence. The exact macOS render trace and normal playback finish are the
accepted desktop comparison; prior current desktop-GL visual evidence remains
documented separately.

## Simulator performance boundary

The iOS recording initially reached high presentation rates, then reported
roughly 5 to 11 FPS through the textured scenario. The trace playback dropped
as low as roughly 3 FPS. Logs identify Apple's Simulator software renderer,
and prior profiling already attributes most time to software triangle
submission. These figures are retained as honest diagnostics. They are not
evidence about real iPad GPU cadence or energy use.

## Preservation and cleanup

Before and after both iOS recording/playback sequences, the clone retained:

```text
BIN   inode 111313696, 605698800 bytes,
      f780bf2331476aabfc00772fa758b12dd95ebfbc907968132cbd3cdd4e2c07c0
save  inode 111309627, 6016 bytes,
      6a01b0f5562ed7a279d8f8e51e3b1874ac39a6120f55db4fe3873288950619a3
```

Replay used isolated `memcard.seed`, `memcard.recording` and
`memcard.playback` roots. The disposable clone app was terminated and its
Simulator shut down without deletion. The protected source-validation app
remained running as PID `93637`. The crash reporter created by the rejected
macOS bypass process was dismissed after its evidence was recorded.

Retail media, saves, replay reports, logs, app containers, binaries and
screenshots remained ignored or outside the repository.

## Current acceptance boundary

Accepted:

- 2,000 fresh current-build frames under both desktop GL and live UIKit/GLES;
- zero mismatch in timing, RNG, drivers, world, allocation, root, pads or
  VSync;
- exact canonical render-command equality at two representative frames;
- coherent live indexed-texture/transparency output with touch overlay; and
- preservation of the imported disc and persistent default save.

Still open:

- live Cocoa GLES on macOS or a reproducible ANGLE/EGL runtime;
- the full 24,232-frame golden scenario under live GLES;
- additional targeted visual edge cases where pixel evidence is stronger than
  the CPU draw trace, including explicit mask-bit behavior;
- physical-iPad GPU cadence, energy and lifecycle repetition; and
- all physical signing, controller, Files, save and multi-touch ergonomics
  gates already listed under M8 through M11.

The overall goal therefore remains active.
