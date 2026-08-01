# macOS ARM64 Cadence Result — 2026-07-30

## Result

The clean current ARM64 producer follows the native NTSC timing model within
measurement error. A 3,232-frame playback segment containing 6,465 recorded
VBlanks was predicted to take 108.079040 seconds and reached the replay-finish
marker in 108.051815 seconds:

```text
deviation:
  -0.027225 seconds
  -0.025190 percent
observed segment cadence:
  29.911575 frames/second
model cadence for this segment:
  29.904040 frames/second
```

The process exited 0. No replay divergence or VSync mismatch was reported.
This accepts the macOS ARM64 desktop cadence measurement for M6. It does not
accept future iOS display-link or lifecycle pacing; those paths must be
measured again after implementation.

## Timing model

The native VBlank target is the exact rational used in
`platform/native_platform.c:531-567`:

```text
GPU clock:
  53,693,175 Hz
cycles per NTSC VBlank:
  897,619
target VBlank cadence:
  59.817333412 Hz
target two-VBlank frame cadence:
  29.908666706 Hz
```

`Native_AdvanceVBlankTarget` carries the division remainder forward, so
rounding does not accumulate drift. `VSync` emits the callbacks and advances
the root counter through the same clock at
`platform/native_platform.c:628-719`.

The NTSC-U game declares nominal `FPS=30` and `ELAPSED_MS=32` at
`include/macros.h:36-44`. `MainFrame_GameLogic` derives, clamps, and consumes
that elapsed value at `game/MAIN/MainFrame.c:190-213`. These are related but
not identical measures:

- host wall cadence is approximately 29.909 frames/s because one ordinary
  frame waits for two 59.817 Hz VBlanks; and
- the retail game advances an ordinary logic step by 32 ms, its nominal
  fixed-point timing quantum.

## Evidence identity

```text
producer:
  build-macos-arm64/ctr_native-cutscene-fix-producer-eee2a8df5b96
producer SHA-256:
  fa9a7d46292ab09b143e0e2b514317daa251af48f6341dc437b1f5d81ded3961
build ID:
  eee2a8df5b96

report:
  build-macos-arm64/debug/reports/20260730/ctr-215303
frames/checkpoints/finalized/exit:
  24232 / 81 / 1 / 0
input.ctrreplay SHA-256:
  dfd06c677f29d9c2155cb01cf00fd958dddfc06127d9b6c67937651039029e09
ctr-native.log SHA-256:
  bea2c87b0c6694f139561ce5f2ba94eba7d46a6daa54b8bce17b39aba3d0de1a
```

This is the same clean producer and report used by the accepted ARM64
all-component correction trace. It rendered through Apple M2 / OpenGL 4.1
Metal with all four PSX shaders and VRAM pipelines ready.

## Complete replay cadence audit

`tools/analyze-replay-cadence.mjs` validates the replay header, every frame
index, pad checksum, record checksum, packet boundary, and decoded VBlank
total before reporting cadence. Command:

```sh
node tools/analyze-replay-cadence.mjs \
  --log build-macos-arm64/debug/reports/20260730/ctr-215303/ctr-native.log \
  build-macos-arm64/debug/reports/20260730/ctr-215303/input.ctrreplay
```

Result:

```text
replay version/frames:
  4 / 24232
elapsedTimeMS:
  total=775726
  histogram=31:2,32:24216,48:9,64:5
pre-frame VBlank:
  total=857
  histogram=0:24231,857:1
in-frame VBlank:
  total=48498
  histogram=0:1,2:24215,3:9,4:2,5:1,6:2,8:2
post-bootstrap:
  frames=24231
  32-ms plus two-VBlank frames=24200
  percentage=99.872065
  cadence from recorded VBlanks=29.886465543 Hz
```

Frame zero owns all 857 pre-frame VBlanks emitted by initial asynchronous
loading and has no in-frame VBlank. It is correctly recorded timing but is
not an ordinary rendered frame, so the post-bootstrap measurement excludes
that prefix.

After frame zero, 24,215/24,231 frames use two VBlanks and 24,215/24,231 use
32 ms; 24,200 satisfy both at once. The remaining 31/48/64-ms or
three-through-eight-VBlank records are retained stalls/transitions, not
dropped data. They account for 36 VBlanks beyond a uniform two-per-frame
schedule and explain why the post-bootstrap trajectory's transport-derived
average is 29.886466 Hz rather than exactly 29.908667 Hz.

The report's 12 wall-clock samples are:

```text
31.31, 29.86, 29.83, 29.86, 29.91, 29.88,
29.90, 29.91, 29.91, 29.91, 29.91, 29.90
```

The first 2,000-frame window is a startup transient. The remaining 11 have
mean 29.889091, minimum 29.83, and maximum 29.91. Those log samples support
the transport audit, but the checkpoint-local wall measurement below is the
direct pacing test.

## Checkpoint-local wall measurement

Checkpoint 70 maps to replay frame 21,000. Frames 21,000 through 24,231
contain:

```text
frames:
  3232
in-frame VBlanks:
  6465
distribution:
  2:3231,3:1
model duration:
  6465 / 59.817333412 = 108.079040 seconds
```

The first `/usr/bin/time` attempt measured 126.31 seconds from process launch.
It was rejected as a cadence result because it included asset validation,
renderer/shader initialization, audio startup, checkpoint validation, and
checkpoint restoration before the timed segment.

A second attempt attached timestamps to redirected output but did not force C
stdio line buffering. The child emitted no trustworthy live markers, so that
run was stopped and rejected.

The accepted attempt ran the unchanged producer through `/usr/bin/stdbuf
-oL -eL` and timestamped only the emitted checkpoint-restore and
replay-finished lines:

```text
process-start to checkpoint restore:
  16.269339 seconds
checkpoint restore to replay finish:
  108.051815 seconds
process exit:
  0
```

The built-in FPS line during checkpoint playback was not used: its
2,000-frame counter began before checkpoint restoration and therefore did not
measure 2,000 frames within this segment.

The checked-in analyzer now automates the same line-buffered measurement:

```sh
node tools/analyze-replay-cadence.mjs \
  --measure-executable \
    build-macos-arm64/ctr_native-cutscene-fix-producer-eee2a8df5b96 \
  --start-checkpoint 70 \
  build-macos-arm64/debug/reports/20260730/ctr-215303/input.ctrreplay
```

Measurement mode was separately exercised with checkpoint 80, a short
232-frame/464-VBlank tail:

```text
expectedSeconds=7.756949
observedSeconds=7.722709
deviationSeconds=-0.034240
deviationPercent=-0.441409
exit=0
```

The roughly 30-ms fixed marker-delivery difference is proportionally visible
in that short run and consistent with the 27-ms difference in the accepted
108-second run. The long segment bounds the sustained cadence much more
tightly.

## Acceptance boundary

Accepted here:

- exact source timing constants;
- complete replay timing validation;
- normal 32-ms/two-VBlank dominance;
- explicit retention of loader/stall timing;
- direct long-segment macOS ARM64 wall pacing; and
- clean checkpoint playback exit.

Still open:

- the formal clean same-commit i686 full-state comparison;
- complete manual desktop play and persistent-save relaunch;
- audio-underrun acceptance;
- iOS lifecycle/display-link behavior; and
- device cadence under touch input.
