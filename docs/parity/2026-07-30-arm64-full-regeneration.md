# 2026-07-30 ARM64 Full-Regeneration Result

**Source state:** `2f341999be63250d8cc58d48f585c1fbc3413bff` plus the
documented language-table correction

**Status:** accepted as a 24,232-frame ARM64 allocator/coverage regeneration;
not yet accepted as the full cross-width parity oracle

## Why this run exists

The historical i686 coverage report
`build-linux-i686-baseline/debug/reports/20260729/ctr-225420` contains the
complete 24,232-frame pad script, but replay version 2 does not capture every
VSync call boundary. It is therefore valid input automation, not a
cross-architecture timing oracle.

ARM64 used those pads to create a fresh version-4 report with complete local
timing. The next parity step is for the optimized i686 build to consume this
version-4 report and compare all six canonical components plus pads and VSync.

## Rejected first attempt and allocator correction

The first report, `build-macos-arm64/debug/reports/20260730/ctr-112652`,
stopped making progress at replay frame 22,156 and retained `finalized=0`.
Sampling and LLDB placed the process in the intentional allocation-error loop
at `game/MEMPACK.c:88-94`. A late level load requested 96,000 bytes with only
86,264 bytes free.

Same-frame checkpoint analysis showed a constant 32,268-byte ARM64
displacement after language initialization. The old LP64 implementation
appended a maximum-width host pointer table to the retail language allocation.
That table is derived host state, not retail game data, and incorrectly
reduced the retail-pressure arena.

The accepted implementation:

- allocates only the retail `langBufferSize` in MEMPACK
  (`game/LOAD/LOAD_Assets.c:306-309`);
- stores the LP64 `char *` table in fixed native storage and validates every
  serialized offset (`game/LOAD/LOAD_Assets.c:242-284,319-351`); and
- relocates only the language file during checkpoint restore, then rebuilds
  the derived table (`platform/native_checkpoint.c:1700-1717,2758-2767`).

The memory-model rationale is recorded in `docs/MEMORY_MODEL.md`, and the
decision is recorded in `docs/DECISIONS.md`.

## Accepted report

```text
report:
build-macos-arm64/debug/reports/20260730/ctr-115352

producer copy:
build-macos-arm64/ctr_native-115352-producer
```

The report finalized normally:

```text
replay_version=4
frame_record_size=440
frame_count=24232
checkpoint_count=81
finalized=1
recording_status=finalized
```

It crossed the rejected run's failure point, wrote checkpoints at frames
22,200 through 24,000, reproduced the successful powerslide boost near frame
9,300, wrote a 6,016-byte recording memory card, and closed the replay log
normally.

Final identities:

```text
producer SHA-256:
588786e7eaaee5c845dc2c1e6c101178cc93be3a62e94857aea338338cba769e

input.ctrreplay:
size 10,662,228
SHA-256 bf022938a8580e91fa06045f0cabb6b58a67fb7cc16bbd4909601b7e1ce93b86

state.ctrstates:
size 359,201,012
SHA-256 8fdf6999fd8796228cd9e801f49be1525cd750cf17dda09c395a1fcfeaee0842

metadata.txt:
SHA-256 29b992e1a12a73a825307f7f531daea0fc3f2c71ba42dded5048c0393734305d

ctr-native.log:
SHA-256 24de8fd1f9d395a7aa7599a0158ca6fa04c4714d70e83280c0d713754c16aa22

memcard.recording/slot0/BASCUS-94426-SLOTS:
size 6,016
SHA-256 6a01b0f5562ed7a279d8f8e51e3b1874ac39a6120f55db4fe3873288950619a3
```

The post-run ARM64 build passes all 13 media-free CTests. The comparator
passes `node --check`, `git diff --check` is clean, and the protected
historical i686 executable remains byte-identical at SHA-256
`afe7b3d264bd2674192e71485037842ab329d1eca9c0c34ca50da5d4487c76a7`.

## Visual investigation

Raw PS1 VRAM was first misread as a presented image. That conclusion was
withdrawn because indexed texture pages, CLUT data, display buffers, and
direct-color regions coexist in the atlas. Default-framebuffer capture is the
visual oracle.

A live ARM64 menu capture showed readable UI and a coherent textured track
preview. A later race capture showed recognizable HUD, kart, effects, and
minimap but severe high-frequency track-surface corruption.

The exact optimized i686 producer was then restored at checkpoint 6 under
Xvfb and llvmpipe. Its internal F12 capture and an ARM64 debugger capture were
both taken at replay frame 1,802. Camera position, kart placement, and polygon
boundaries line up across the two images. The ARM64 image has striped texture
corruption where i686 presents a flat surface. The i686 image itself has
incorrect cyan/blue coloration under llvmpipe, so it is a structural oracle,
not a retail-correct color oracle.

This evidence narrows the open visual defect toward texture sampling, texture
state, or texture-coordinate presentation. It does not support the earlier
geometry-corruption hypothesis. The internal screenshot path is implemented
at `platform/native_platform.c:154-167,215-218`; the renderer's textureless
isolation switch substitutes a white texture at
`platform/native_renderer.c:1406-1409`.

The retail-derived captures remain ignored and are not committed:

```text
/tmp/ctrpad-window-115352.png
/tmp/ctrpad-window-race-115352.png
/tmp/ctrpad-i686-race-frame1802-upright.png
/tmp/ctrpad-arm64-race-frame1802.png
```

## Remaining acceptance work

1. Run optimized i686 regeneration from this exact version-4 ARM64 input.
2. Require timing, RNG, drivers, world, allocation, root, pads, and VSync to
   match for all 24,232 frames.
3. Replay the accepted reports unchanged in separate processes and run the
   deliberate mutation gate.
4. Isolate the surface defect with textureless/wireframe captures and inspect
   the shared renderer's texture-format, CLUT, and UV paths.
5. Re-observe and record all eight golden-run coverage checks rather than
   inferring them solely from the inherited pad script.
