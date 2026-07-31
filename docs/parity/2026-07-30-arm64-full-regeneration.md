# 2026-07-30 ARM64 Full-Regeneration Result

**Source state:** `2f341999be63250d8cc58d48f585c1fbc3413bff` plus the
documented language-table correction

**Status:** accepted as a historical 24,232-frame ARM64 allocator/coverage
regeneration. It is superseded as a cross-width parity candidate: the first
optimized-i686 comparison exposed a native out-of-bounds restart-node read at
frame 6,780. Both architectures must be regenerated after the documented
native bounds correction.

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

The preserved ARM64 ASan report and the optimized i686 Release report were
then compared directly. All eight canonical components match for all 2,200
frames, yet a second ARM64 capture at exact frame 1,802 retains the stripes.
The mismatch therefore exists with equal replay input and canonical game
state.

A textureless capture at exact frame 1,802 removes the stripes while retaining
coherent road, kart, camera, and track polygons. Textured sprites and fonts
become white blocks, as expected from the one-pixel white substitution. This
rejects geometry as the leading cause and isolates the defect to indexed
texture sampling/state, CLUT handling, UV presentation, or the still-unhashed
render primitive stream. The internal screenshot path is implemented at
`platform/native_platform.c:154-167,215-218`; the renderer's textureless
switch is at `platform/native_renderer.c:1406-1409`.

The next diagnostic added a canonical packed-vertex/draw-split trace. At frame
1,802, pre-fix ARM64 emitted 5,991 vertices through nine flushes and 524
splits, including 25 16-bit framebuffer-feedback splits. Exact i686 emitted
the same 5,991 vertices in one flush and 352 splits with no 16-bit splits.
This located the mismatch in the CPU primitive stream, before OpenGL.

A conditional trap found the first invalid ARM64 page in a DrawLevel
`POLY_GT4`: `tpage=0x67f0`. DrawLevel's native texture-word classifier cast a
fixed-width relocated word as a host pointer. That representation is a direct
32-bit pointer on i686 but an ADR-0001 guest reference on LP64. ARM64 therefore
misclassified a valid mosaic reference as an inline sentinel, selected the
wrong texture-layout bytes, and generated false 16-bit feedback pages.

`game/226/226_00_DrawLevelOvr1P.c` now resolves those words through
`CtrAssetRef_ResolveRequired`, validates their MEMPACK span, and resolves all
three deepest mosaic source reads before access. No renderer, shader, retail
asset, or geometry change was required.

The fixed report
`build-macos-arm64/debug/reports/20260730/ctr-133039` finalized all 2,200
frames. All eight gameplay components still match the pre-fix report. Its
frame-1,802 trace exactly equals i686:

```text
flush hash 4d1cbba5c0098b5c
aggregate  76303b9b4c2ee4c2
flushes=1 vertices=5991 splits=352
formats=4:327,8:25,16:0,rgba:0
```

The new default-framebuffer capture shows coherent Crash Cove textures and no
striped surface:

```text
/tmp/ctrpad-arm64-race-frame1802-fixed.png
SHA-256 03ce067255e843fbb010687bf2390930110a8e3201d99a085f519784dae08cc4
```

ARM64, ARM64 ASan, and i686 each pass all 13 CTests after the fix. Complete
accepted and rejected probe details are in
`docs/history/ENGINEERING-JOURNAL.md`.

The retail-derived captures remain ignored and are not committed:

```text
/tmp/ctrpad-window-115352.png
/tmp/ctrpad-window-race-115352.png
/tmp/ctrpad-i686-race-frame1802-upright.png
/tmp/ctrpad-arm64-race-frame1802.png
/tmp/ctrpad-arm64-asan-parity-frame1802.png
/tmp/ctrpad-arm64-race-frame1802-textureless-v3.png
/tmp/ctrpad-arm64-rg32f-frame1801-fast.png
/tmp/ctrpad-arm64-race-frame1802-fixed.png
```

## Rejected full i686 comparison and native restart-node boundary

The current-source i686 producer at source commit `53ab70e966b2` recorded
report:

```text
/tmp/ctrpad-i686-full-v4-current-cbRPWn/debug/reports/20260730/ctr-190458
producer SHA-256:
42df6c21f43539248212c9614e2c2fb3f5223d9cc2cfd4af876cbdd6d2f193a3
```

The first 6,780 frames established a much stronger prefix than the earlier
2,200-frame oracle: timing, RNG, drivers, world, allocation, root, pads, and
VSync all matched. At end-of-frame 6,780, only `drivers` and the aggregate
`root` changed:

```text
ARM64 drivers: 7d42e48e3ebba032
i686 drivers:  48f0799f58d94ac9
timing/RNG/world/allocation/pads/VSync: exact
```

Canonical field dumps for all eight drivers reduced the difference to two
values:

```text
driver 4 distanceToFinish_curr: ARM64 -4673, i686 31
driver 7 distanceToFinish_curr: ARM64 -4683, i686 12
```

Both drivers had `botData.ai_quadblock_checkpointIndex == 0xff`.
`VehLap_UpdateProgress` accepted that sentinel because the retail routine
checks only that the level's count fits in eight bits and that the signed
index is nonnegative. It then evaluated `nodes[0xff]` beyond the actual
restart array and followed another out-of-range `nextIndex_forward`.

This is not a floating-point or GTE difference. Instruction-level LLDB and
GDB captures proved that both ports produced the correct projection,
wrong-way dot product, track-length scale, and signed remainder for the bytes
they were given. The selected fake node bytes differed because adjacent level
asset slots had already been relocated:

- i686 stored direct, process-specific 32-bit host pointers;
- ARM64 stored checked fixed-width guest references; and
- a second i686 process could receive different direct-pointer values under
  ASLR.

Native code now requires `0 <= checkpointIndex < cnt_restart_points` before
resolving or indexing the restart array. Invalid state performs no progress
update, matching the function's existing no-valid-checkpoint behavior and
removing address representation from game state. The ASM-verified PS1 path
remains byte-for-byte unchanged.

CTest `ctr_native_vehicle_lap_checkpoint_bounds` exercises first, last,
one-past-end, `0xff`, empty, and null cases. Post-correction gates are:

```text
macOS ARM64 Release:       14/14
macOS ARM64 ASan/UBSan:    14/14
Linux i686 Release -m32:   14/14
i686 binary:               ELF 32-bit Intel 80386
protected baseline SHA-256:
afe7b3d264bd2674192e71485037842ab329d1eca9c0c34ca50da5d4487c76a7
git diff --check:          clean
```

The rejected report subsequently finalized all 24,232 frames and 81
checkpoints. Its complete map retains exact timing, pad snapshots, and VSync,
but has 4,387 driver mismatches and 9,529 aggregate-root mismatches. Exact
file hashes and ranges are in `docs/history/ENGINEERING-JOURNAL.md`. It cannot
pass the unchanged-process or mutation verifier, and no result from it is
acceptance evidence.

## Remaining acceptance work

Corrected committed ARM64 producer `55d3b71c6da5` subsequently finalized
report `build-macos-arm64/debug/reports/20260730/ctr-170507` with all 24,232
frames and 81 checkpoints. Its producer and report hashes are recorded in
`docs/history/ENGINEERING-JOURNAL.md`.

1. Corrected optimized-i686 report `ctr-223323` finalized all 24,232 frames,
   but is rejected. It first diverges at frame 16,561 when i686 advances the
   particle RNG 38 times and ARM64 advances it 33 times. Timing, pads, and
   VSync still match for every frame.
2. Isolate and correct the five-call i686 particle-RNG delta, then regenerate
   both reports and require timing, RNG, drivers, world, allocation, root,
   pads, and VSync to match for all 24,232 frames.
3. Replay the accepted replacement i686 report unchanged in separate
   processes and run its deliberate mutation gate. Corrected ARM64 already
   passes both unchanged processes and the mutation check.
4. Record a current version-4 lap-coverage input. Typed checkpoint inspection
   rejects both the inherited input and extension trial A because neither
   advances beyond `lapIndex=0`.

The finalized ranges, producer/report hashes, first-frame trace, rejected
diagnostic routes, and reusable typed checkpoint inspector are documented in
`docs/parity/2026-07-30-full-cross-width-result.md`.
