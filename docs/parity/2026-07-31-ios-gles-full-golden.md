# Full current-build iOS GLES golden comparison

Date: 2026-07-31

Implementation build: `e6ba535a9c73`

Documentation branch at start: `95d840603534`

Result: **accepted for the full same-build renderer state/transport boundary;
M7 and the overall goal remain in progress**

## Scope and verdict

The exact current macOS desktop-GL and iOS UIKit/GLES executables each
regenerated the complete 24,232-frame version-4 golden scenario from the same
pad and VSync seed. Both reports finalized 81 rolling checkpoints. A strict
typed comparison required timing, RNG, drivers, world, allocation, root, pads
and VSync; every component matched on every frame with zero mismatches.

Exact native checkpoint-80 playbacks also reached the normal 24,232-frame
finish. At frame 24,001, both renderers emitted identical packed vertices and
draw-split state:

```text
render-trace frame=24001 flush=0 hash=2381a1fe20c00a91
vertices=3186 splits=4 formats=4:4,8:0,16:0,rgba:0
render-trace end frame=24001 hash=d1765e952537c48b
flushes=1 vertices=3186 splits=4 formats=4:4,8:0,16:0,rgba:0
```

This accepts the complete golden scenario for the current renderer-choice
state/cadence boundary. It does not make M7 complete. Cocoa cannot run the
shared GLES path on this host without an ANGLE/EGL runtime, explicit remaining
pixel edge cases are not all captured by this input, and physical-iPad wall
cadence/energy is still unmeasured.

The report is also not a multi-lap substitute. Direct checkpoint inspection
found `activeRecords=80`, `maxLap=0`, `maxCheckpoint=77` and
`lapAdvanced=no`. The separately accepted current-format lap report remains
the lap-structure oracle; a completed human race remains open.

## Existing keyboard requirement revalidated

The user explicitly requested basic keyboard controls while the iOS producer
was already running. Source inspection showed that the published branch
already satisfies the request in `platform/native_input.c:321-352`, with the
event path in `platform/native_platform.c:720-787` and the public table in
`README.md:218-248`:

```text
arrows or W/A/S/D       steering and menus
C or K                  Cross / accelerate / accept
X or J                  Square / brake
Z or I                  Triangle
V or L                  Circle / item / back where retail permits it
Left Shift or Q          L1 / hop and drift
Right Shift or E         R1 / hop and drift
Enter or P               Start / pause
Space or Tab             Select
```

The aliases enter the ordinary active-low PS1 pad snapshot; there is no
keyboard-only physics path. Quick key-down/key-up pairs are retained across
two host snapshots so they cannot disappear between approximately 29.9-Hz
retail polls. Current-tip `ctr_native_input` passed independently in 0.17
seconds, including all 12 aliases, quick `C+Right` and `K+D` taps, the held
`K+D+E` chord, primary input sharing, touch composition and virtual-gamepad
composition. The final complete macOS ARM64 matrix passed 21/21 in 1.01
seconds. No duplicate source change was made.

## Input seed and exact executables

The authoritative source input was:

```text
build-macos-arm64/debug/reports/20260730/ctr-215303/input.ctrreplay
version=4 frames=24232 checkpoints=81
size=10662228
SHA-256 dfd06c677f29d9c2155cb01cf00fd958dddfc06127d9b6c67937651039029e09
```

The strict comparer first rechecked this report against itself and found all
eight components equal on all 24,232 records. Both fresh producers used
`--record-from-replay`, which consumes the seed's complete pad/VSync stream but
captures fresh current-process canonical state and native checkpoints. No old
checkpoint was restored into either producer.

Exact executables:

```text
macOS ARM64  build-macos-arm64/ctr_native
build        e6ba535a9c73
SHA-256      cfd3d9b420e8e9592a622112bd368b20857ba427d68c92fab58bd8578d741dca

iOS ARM64    build-ios-simulator-arm64/CTRPad.app/CTRPad
build        e6ba535a9c73
SHA-256      1cef6404aa3c2bf094c3357e71eb1069e81ed3e6307b972d043bfe222c2c62cb
```

The two products truthfully have different platform identity checksums and
executable fingerprints. Their raw report hashes are therefore expected to
differ even when every canonical frame component is equal.

## Full iOS UIKit/GLES producer

Disposable Simulator `26F3DEE8-8840-446D-85FE-C882009C9C06` was booted with
the exact installed app and existing isolated imported retail image/save. The
full seed and its memory-card seed were copied into private Application
Support. PID `20290` launched at 21:11:22 CDT with `--record-from-replay` and
created:

```text
debug/reports/20260731/ctr-211128
```

The app initialized UIKit framebuffer/renderbuffer 1, Apple Software Renderer,
OpenGL ES 3.0 / GLSL ES 300, all four PSX shaders, both VRAM pipelines, the
touch overlay and the UIKit lifecycle display loop. The report advanced
continuously to checkpoint 80 at frame 24,000, recorded the expected late
race-driver inactive event at frame 21,331, and closed normally:

```text
[CTR Replay] replay-seeded recording finished after 24232 frames
---- LOG CLOSED ----
```

Monitoring observed 100% completion after 1 hour, 7 minutes, 38 seconds.
Apple's Simulator software renderer varied roughly from 4 to 11 FPS during
later gameplay. This is a Simulator throughput observation, not a physical
device performance claim. Two malformed monitor-poll requests were corrected
immediately during the long run; neither request interacted with or restarted
PID `20290`, and report growth remained continuous.

Final metadata and files:

```text
platform=ios
build_id=e6ba535a9c73
identity_checksum=0x6ea1d76d
executable_fingerprint=e9d9b4240372487c
finalized=1
frame_count=24232
checkpoint_count=81

input.ctrreplay  10662228 bytes
SHA-256 056866ff50aa5044566fb5effe3f8a7ef2c79d1cd3a82f3f17531e575205c60d

state.ctrstates  359201012 bytes
SHA-256 94eca26bb9d543f362448dc779401fc3fdc29bc610c86578681fa72b344e105e

ctr-native.log   16700 bytes
SHA-256 cdc30ee79f5617798852d2c11012985aff62c2f1c0e207883b1b4e88767940a0

metadata.txt     1203 bytes
SHA-256 5f82393bf9bc61a84f251e30cc5f760418aad8752401739741622cdcc6496dc2
```

The first strict comparison used the previously accepted macOS oracle
`ctr-215303`. It found all 24,232 frames equal in all eight required
components. This proved that the new native iOS output retained the accepted
retail trajectory before the fresh same-build desktop producer was available.

## iOS checkpoint validation and rejected launch target

The finalized iOS input/state pair was copied to a local-only private playback
directory so the producer report would remain immutable. The first launch
attempt used stale bundle ID `com.chrissotraidis.CTRPad` and was rejected by
SpringBoard as `NotFound`; it did not start an app or touch the report.
`simctl listapps` resolved the installed product's actual bundle ID as
`io.github.chrissotraidis.ctrpad`, matching its `Info.plist`.

The corrected exact-binary launch used:

```text
--replay <copy>/input.ctrreplay --replay-start-checkpoint 80
```

It validated all 81 checkpoint records, restored frame 24,000 with recorded
checksum `0xe32367e8`, and reached:

```text
[CTR Replay] replay finished after 24232 frames
---- LOG CLOSED ----
```

The log also printed a different `restored-process` raw checksum. That field
is explicitly labeled diagnostic-only and contains current host addresses;
the same log lists those addresses as excluded from the canonical digest. No
canonical divergence, corruption, runtime failure or memory-card failure was
reported.

## Fresh exact-current macOS desktop-GL producer

Repository search found no existing 24,232-frame report for build
`e6ba535a9c73`. The source delta since the historical macOS oracle included
the shared renderer, UIKit shell and input work, so a fresh same-build producer
was warranted rather than relying only on the already successful cross-build
canonical comparison.

The exact macOS executable ran:

```sh
./ctr_native --record-from-replay \
  debug/reports/20260730/ctr-215303/input.ctrreplay
```

Apple M2 desktop GL initialized all PSX/VRAM pipelines. Report
`build-macos-arm64/debug/reports/20260731/ctr-222546` finalized all 24,232
frames and 81 checkpoints with process exit 0 after 13 minutes, 31 seconds.
Later throughput was 29.90-29.91 FPS. Its final artifacts are:

```text
platform=macos
build_id=e6ba535a9c73
identity_checksum=0xaa46653b
executable_fingerprint=8edd476c95a09be7
finalized=1
frame_count=24232
checkpoint_count=81

input.ctrreplay
SHA-256 78ef42512ad498a39c5496dfd245f8646526e1de576a4a4cb28fe264bb8cc32a

state.ctrstates
SHA-256 7595dafad17f86924552af1c77ab0599fd9c88917709889d8769827b7aa754c9

ctr-native.log
SHA-256 5bd05febe905eb0fc05dddcdf50ece808be83c905eedc8c5a8d6e1770ca0ca3e

metadata.txt
SHA-256 286b18134e2edd9bdc0f60ec949db8aa23ac5133318d2522183297ed1763ac35
```

No divergence, mismatch, corruption, runtime-failure or failed marker was
present in the producer log.

## Decisive same-build comparison

The exact command was:

```sh
node tools/compare-replay-state-components.mjs \
  --require timing,rng,drivers,world,allocation,root,pads,vsync \
  build-macos-arm64/debug/reports/20260731/ctr-222546/input.ctrreplay \
  <iOS Application Support>/debug/reports/20260731/ctr-211128/input.ctrreplay
```

Result:

```text
timing:     equal=24232 mismatched=0 ranges=none
rng:        equal=24232 mismatched=0 ranges=none
drivers:    equal=24232 mismatched=0 ranges=none
world:      equal=24232 mismatched=0 ranges=none
allocation: equal=24232 mismatched=0 ranges=none
root:       equal=24232 mismatched=0 ranges=none
pads:       equal=24232 mismatched=0 ranges=none
vsync:      equal=24232 mismatched=0 ranges=none
```

This is both same implementation build and complete scenario. `pads` proves
the complete PS1-shaped snapshots match. `vsync` proves emitted VBlank total,
packet counts, pre-frame counts and used packet entries match. The other six
components prove the canonical retail game state remained identical.

## Late frame-24,001 renderer trace

Each finalized report was copied to a disposable playback root. Exact native
producers restored their own checkpoint 80 and traced frame 24,001. macOS
checkpoint checksum `0xfb3ec794` and iOS checksum `0xe32367e8` differ because
native checkpoint files contain platform/process-local address-bearing state;
both files validated under their producing executable.

The complete three render-trace lines were byte-identical after extraction:

```text
[CTR GPU] render-trace begin frame=24001 vertex-size=20
[CTR GPU] render-trace frame=24001 flush=0 hash=2381a1fe20c00a91 vertices=3186 splits=4 formats=4:4,8:0,16:0,rgba:0
[CTR GPU] render-trace end frame=24001 hash=d1765e952537c48b flushes=1 vertices=3186 splits=4 formats=4:4,8:0,16:0,rgba:0
```

Both playbacks then reached frame 24,232 normally. Local-only log hashes:

```text
macOS dc5d9c30eeaec6f807e5b96ffe306cd6e6be9016a7ef3edb3c8531369ada7034
iOS   a789d24283a9c45ccb2a2e37a51a517140aaa1fba18214e66239779d18685985
```

This late trace supplements the already accepted identical traces at frames
1,802 and 1,813. It does not manufacture coverage for absent 8-bit, 16-bit or
RGBA batches at frame 24,001; the exact format count above remains the truthful
observation.

## Coverage, preservation and cleanup

`tools/inspect-replay-lap-coverage.mjs --changes-only` validated all 81 iOS
checkpoint records but reported:

```text
checkpointVersion=3 pointerSize=8 activeRecords=80
maxLap=0 maxCheckpoint=77 lapAdvanced=no
```

The run therefore strengthens full renderer equivalence, not multi-lap game
coverage. The accepted separate `ctr-223221` report remains the current-format
lap-advance evidence.

The iOS recorder, two checkpoint playbacks and trace left the clone's protected
default inputs byte-for-byte unchanged:

```text
retail BIN
inode=111313696 size=605698800
SHA-256 f780bf2331476aabfc00772fa758b12dd95ebfbc907968132cbd3cdd4e2c07c0

default save
inode=111309627 size=6016
SHA-256 6a01b0f5562ed7a279d8f8e51e3b1874ac39a6120f55db4fe3873288950619a3
```

All reports, state files, logs, copied retail data, saves and trace products
remain local-only outside Git. The disposable Simulator was shut down without
deletion. Protected source-validation PID `93637` remained running. The final
macOS ARM64 suite passed 21/21.

## Remaining boundary

Accepted here:

- all 24,232 frames and 81 checkpoints produced natively by exact current
  desktop GL and UIKit/GLES binaries;
- exact equality for timing, RNG, drivers, world, allocation, root, pads and
  VSync on every frame;
- exact checkpoint-80 restore/playback to normal finish on iOS;
- exact current native late draw-command equality at frame 24,001; and
- current keyboard controls and aliases through the shared PS1 pad path.

Still open:

- live Cocoa GLES/ANGLE, because this host has no suitable ANGLE/EGL runtime;
- explicit remaining renderer pixel/mask/feedback edge cases where the corpus
  and current traces do not exercise them;
- wall cadence, thermal behavior and energy on a physical iPad;
- signed installation with the user's identity/profile and actual device;
- physical keyboard/controller delivery; and
- natural human multi-touch steering + Gas + held drift + repeated boost,
  followed by a complete race/save cycle.

The preceding published timer was 196,487 seconds. The pre-documentation
reading was 202,463 seconds: 2 days, 8 hours, 14 minutes, 23 seconds cumulative,
adding 5,976 seconds (1 hour, 39 minutes, 36 seconds). This is cumulative goal
time including pauses, not a benchmark or person-hour estimate. The observed
iOS and macOS producer durations are reported separately above.
