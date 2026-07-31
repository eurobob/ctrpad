# Full Cross-Width Parity Result — 2026-07-30

## Result

The corrected 24,232-frame ARM64 and optimized i686 reports do **not** pass
the full cross-width gate. Timing, pad snapshots, and VSync transport match
for every frame. RNG, drivers, world, allocation, and the aggregate root do
not.

This is a finalized parity rejection, not an incomplete run and not an
accepted platform difference. Fixed-point gameplay is expected to match
across these architectures, so M6 remains open and iOS work remains gated.

The defect that produced this rejection has since been isolated and corrected
in source. The rejected reports below remain the immutable evidence that
bounded it; they are not retroactively relabeled as passes. A new full pair
must still prove the correction across all 24,232 frames.

## Accepted producer and report identities

ARM64 reference:

```text
producer:
  build-macos-arm64/ctr_native-corrected-full-producer-55d3b71c6da5
producer SHA-256:
  49449fd9313a8a3414617058873dcd3a3f07401af770476b4468b8caa107f844
report:
  build-macos-arm64/debug/reports/20260730/ctr-170507
frames/checkpoints/finalized:
  24232 / 81 / 1
```

i686 candidate:

```text
producer:
  /private/tmp/ctrpad-i686-vehlap-8IzCKm/ctr_native-corrected-full-producer-55d3b71c6da5
producer SHA-256:
  e07be52d72e6a2f323587d9302467be366401485cecd29d86ae54846f973528c
report:
  /private/tmp/ctrpad-i686-vehlap-8IzCKm/debug/reports/20260730/ctr-223323
frames/checkpoints/finalized:
  24232 / 81 / 1
```

The i686 report files are:

```text
input.ctrreplay
  4bada362045669896cdbccf78239ff20cba3ca922531e0dc83eaf4167c9043cb
state.ctrstates
  c4a12812b3369ceab61891266e6a7e210a9292108f4261fdc9ec4a30cea4c17e
metadata.txt
  615b6ec9148e55a56ae9f0a1bb0b701ff2f92122d5913ac03592dadb9a527859
ctr-native.log
  81a5002f7d6f321c1289456f9d19beed5ad71981f59f8277ba3c140799a33ae4
```

The producer completed normally. The metadata records `finalized=1`,
`frame_count=24232`, and `checkpoint_count=81`; a crash or truncated output
cannot explain the rejection.

## Definitive all-frame comparison

Command:

```sh
node tools/compare-replay-state-components.mjs \
  --require timing,rng,drivers,world,allocation,root,pads,vsync \
  build-macos-arm64/debug/reports/20260730/ctr-170507/input.ctrreplay \
  /private/tmp/ctrpad-i686-vehlap-8IzCKm/debug/reports/20260730/ctr-223323/input.ctrreplay
```

Result:

```text
timing:      equal=24232 mismatched=0    ranges=none
rng:         equal=19387 mismatched=4845 ranges=16561-21405
drivers:     equal=24030 mismatched=202  ranges=17213-17414
world:       equal=23713 mismatched=519  ranges=16561-16580,22158-22656
allocation:  equal=23679 mismatched=553  ranges=16561-16580,17213-17227,17231-17275,22158-22630
root:        equal=18888 mismatched=5344 ranges=16561-21405,22158-22656
pads:        equal=24232 mismatched=0    ranges=none
vsync:       equal=24232 mismatched=0    ranges=none
```

Exit status was `2`, with required components
`rng,drivers,world,allocation,root` named as mismatched.

## First divergence at frame 16,561

At the end of frame 16,560, both reports contain the same particle RNG
state:

```text
deadcoed0=0x1b9ef97b
deadcoed1=0x533fcdb6
```

At the end of frame 16,561:

```text
ARM64: deadcoed0=0xa8c39902 deadcoed1=0x7d5e2621
i686:  deadcoed0=0xb5fd73cb deadcoed1=0xdeb84781
```

Replaying the exact recurrence in `game/MixRNG/RngDeadCoed.c:4-15` from the
common frame-16,560 state reaches the ARM64 result after 33 calls and the
i686 result after 38 calls. The first defect is therefore exactly five
additional i686 calls to `gGT->deadcoed_struct` during frame 16,561.

Timing, input, VSync, and driver digests still match at that frame. World,
allocation, RNG, and root differ. World and allocation remain different for
exactly 20 frames, 16,561 through 16,580, before those two components
reconverge. RNG cannot reconverge merely because the short-lived allocations
expire, so its mismatch persists through frame 21,405.

## Exact ARM64 caller trace

LLDB ran the accepted ARM64 producer from rolling checkpoint 55, frame
16,500. Breakpoints were set on `Particle_Init`,
`VehEmitter_Sparks_Ground`, `RB_FlameJet_Particles`, and
`RngDeadCoed(&gGT->deadcoed_struct)` for the first divergent frame.

The accepted ARM64 path made 12 `Particle_Init` calls:

```text
1 wall-spark particle       3 particle-RNG calls
6 ordinary exhaust particles, 2 for each of 3 drivers
                            5 particle-RNG calls each
5 potion-shatter particles 0 particle-RNG calls
                            ------------------------
total                       33 calls
```

The stack traces resolve those paths to:

- `VehEmitter_Sparks_Wall` at `game/Vehicle/VehEmitter.c:395-500`;
- `VehEmitter_Exhaust` at `game/Vehicle/VehEmitter.c:165-211`; and
- `RB_Explosion_InitPotion` /
  `RB_GenericMine_ThDestroy` at `game/231/RB_Explosion.c:36-72` and
  `game/231/RB_GenericMine.c:460-490`.

`Particle_Init` applies emitter-configured RNG fields through
`MixRNG_Particles` at `game/Particle.c:1354-1466,1492`. The frame did not
enter `VehEmitter_Sparks_Ground` or `RB_FlameJet_Particles`. At the later
frame-16,562 stop, ARM64 again had 34 active particles and 94 free particle
slots.

The initial ground-spark hypothesis is rejected:
`VEH_EMITTER_GROUND_SPARK_COUNT` is 10, not 5
(`game/Vehicle/VehEmitter.c:22,102,264-294`), and the accepted ARM64 path did
not call it. A complete flame-jet emission is also inconsistent with the
five-call delta: static tracing through `game/231/RB_FlameJet.c:228-276`
accounts for 14 calls, and the ARM64 breakpoint did not fire.

The ARM64 trace alone bounded the defect but did not name the missing i686
objects. The accepted i686 producer was subsequently inspected without
modifying its executable or replay headers, as described next.

## Exact i686 process-memory observation

A disposable amd64 Linux container ran the exact accepted producer
`e07be52d...` under the ordinary `qemu-i386` path. The retail image was mounted
read-only. The container had `SYS_PTRACE` solely so a helper could read
`/proc/66/mem`; no diagnostic instructions were added to the binary and no
checkpoint identity check was bypassed.

The process was stopped at the start and end of frame 16,561 by polling the
accepted producer's `s_replayFrame`. Its loaded ELF base was `0xb586b000`;
subtracting the linked guest base `0x40000000` gave the runtime relocation
delta used to resolve the accepted symbol addresses. At the common start:

```text
RNG:       1b9ef97b,533fcdb6
particles: 34
free:      94
```

At the end:

```text
i686 RNG:       b5fd73cb,deb84781
i686 particles: 39
i686 free:      89
ARM64 RNG:      a8c39902,7d5e2621
ARM64 particles:34
ARM64 free:     94
```

The ARM64 comparison was taken from the exact accepted ARM64 producer under
LLDB at `NativeReplayScheduler_EndFrame`, with `s_replayFrame == 16561`.
Its 19,456-byte particle pool was dumped from host address `0x1005ffe50`;
the ordinary-list head was `0x1006017d8` and the LP64 item size was 152.

Walking both ordinary-particle lists established:

- all 34 ARM64 records match i686 records 5 through 38 in their scalar fields
  and axes;
- i686 has exactly five extra records at the list head;
- each extra record has `framesLeftInLife=19`,
  `flagsSetColor=0x00a1`, `flagsAxisWord=0x000003a7`,
  `funcPtr=Particle_FuncPtr_PotionShatter`, and model/owner union `0x45`; and
- i686 has consumed five free particle slots while ARM64 has not.

These five objects explain both observed signatures. Their configured
20-frame lifetime produces the exact frame-16,561-through-16,580 world and
allocation mismatch. Their Y-axis emitter flags include randomized velocity
with a seed of 400, consuming exactly one particle-RNG call per object and
therefore the exact five-call delta.

## Root cause and correction

`game/231/RB_Explosion.c` stored the potion emitter as 81 raw `u32` words:
nine retail `ParticleEmitter` records with a 0x24-byte ILP32 stride. It then
cast that byte table to `struct ParticleEmitter *`.

That representation is valid only on 32-bit:

```text
                 ILP32    LP64
InitTypes offset 0x04     0x08
data offset      0x14     0x20
record size      0x24     0x30
```

Consequently ARM64 read the wrong union offsets and advanced through the table
with the wrong stride. The five `Particle_Init` calls seen in the ARM64 trace
were made, but their malformed records did not create the five persistent
retail particles.

The runtime table is now a semantic
`static const struct ParticleEmitter[]`. Native compilers therefore choose
the correct host offsets and stride, while the 32-bit layout remains byte-for-
byte retail-compatible. `Particle_Init` now accepts a const emitter pointer,
and the unsafe cast was removed.

A new media-free entry point,
`--self-test-potion-emitter-layout`, validates the function record, seven axis
records, terminator, 20-frame lifespan, and randomized Y velocity. On i686 it
also compares all nine typed records against the original 81 retail words.
Verification after the correction:

```text
macOS ARM64 Release:   15/15 CTests
macOS ARM64 ASan/UBSan:15/15 CTests
Linux optimized i686:  15/15 CTests
git diff --check:      passed
```

This is structural and sanitizer acceptance of the fix, not the full parity
acceptance. New clean producers and reports are still required.

## Rejected i686 tracing routes

Every tracing attempt below was kept separate from acceptance evidence:

1. Native GDB inside the amd64 container could not control the emulated i386
   process and failed with `Couldn't get CS register: Input/output error`.
2. A diagnostic binary using return-address introspection exited 139 before
   game initialization. It is rejected.
3. QEMU remote debugging exposed one emulation-only address-base artifact
   while restoring checkpoint 55. A debugger-only correction made the stale
   pointer check internally consistent, but disc validation and bootstrap
   were too slow to reach the target frame. No game file or accepted report
   was modified, and no trace from this route is evidence.
4. A second diagnostic binary used only frame-gated logging around
   `Particle_Init` and candidate emitters. Its SHA-256 was
   `1e5c3b25a1a36c5d16f94f3486698930db7b18aa94b88c100e6e9b8286fd9cfd`.
   Under ordinary Docker emulation it printed the startup/base/asset lines,
   then received target SIGSEGV and exited 139 before initialization. The
   untouched producer survived the same launch. This diagnostic is also
   rejected.
5. The same instrumentation was rebuilt through the ordinary CMake unity path
   rather than manual compilation. Its SHA-256 was
   `987c35ef9a5a5b6ef0bba7d3a243a8f82347b6e47f43de83cb2a7002005103cb`.
   A fresh boot remained alive, but checkpoint playback exited 139 during
   validation before the target frame. It is rejected as trace evidence.

All tracing-only source changes were reverted. The successful `/proc` route
observed the untouched accepted producer and did not require instrumentation.

## Lap-coverage audit and rejected extension

`tools/inspect-replay-lap-coverage.mjs` makes the former one-off typed
checkpoint audit reproducible. It verifies the checkpoint container and
payload checksums, resolves the player `Driver` through captured address
ranges, and reads `lapIndex` plus `checkpoint.currentIndex` from checkpoint
versions 2 and 3 on ILP32 and LP64.

The first extension trial preserved the original pad snapshots through frame
21,248, held Cross+Right for frames 21,249 through 21,320, then held Cross
through the end. VSync packets were regenerated by the playback producer.

Seed:

```text
build-macos-arm64/debug/reports/20260730/ctr-170507/input-lap-extension-a.ctrreplay
SHA-256:
b5db8c1903e06026052efde33ccbb747a8bb9f6e35677017e7dd338a805544ec
```

The bundled clean producer finalized:

```text
build-macos-arm64-app/debug/reports/20260730/ctr-192711
frames/checkpoints/finalized:
24232 / 81 / 1
```

Its files hash to:

```text
input.ctrreplay
  faaf50b56a7c410e031f7dce07722c7b89a5f4dc2918aafc669b6eb872e96090
state.ctrstates
  f75e1a1afa8dbcf52b6e60c4d5efc9b36343ea1f67c4e6b119730730177d3729
metadata.txt
  6df42d2efe5886b1cc7983acd0ae0ef9456377fb855a8373123baa3a3fa0fb35
ctr-native.log
  6851a3082cfe01bc169661c8dc11bd3024910ca1947efd851e7a59d13ab383a8
```

All 24,232 output pad snapshots match the extension seed. The typed audit:

```sh
node tools/inspect-replay-lap-coverage.mjs --changes-only \
  build-macos-arm64-app/debug/reports/20260730/ctr-192711/state.ctrstates
```

ends with:

```text
checkpointVersion=3 pointerSize=8 activeRecords=80
maxLap=0 maxCheckpoint=77 lapAdvanced=no
```

Representative late checkpoints are:

```text
frame 19800 lap 0 checkpoint 54
frame 20100 lap 0 checkpoint 50
frame 20400 lap 0 checkpoint 52
frame 20700 lap 0 checkpoint 50
frame 21000 lap 0 checkpoint 44
frame 21300 lap 0 checkpoint 39
frame 21600 lap 0 checkpoint 44
frame 21900 lap 0 checkpoint 45
frame 22200 lap 0 checkpoint 46
frame 22500 lap 0 checkpoint 47
frame 22800 lap 0 checkpoint 49
frame 23100 lap 0 checkpoint 56
frame 23400 lap 0 checkpoint 57
```

The kart continues around the track, but the extension does not cross the
lap boundary and is rejected for coverage.

As a historical control, the same inspector reads the finalized version-2
i686 report:

```text
build-linux-i686-baseline/debug/reports/20260729/ctr-225420
frame 21000 lap 0 checkpoint 72
frame 21300 lap 1 checkpoint 1
frame 21600 lap 0 checkpoint 0
maxLap=1 lapAdvanced=yes
```

At frame 21,300 the historical and current inherited inputs have the same
player-0 button mask and analog axes, but not the same full transport record
or preceding trajectory. This proves that the historical report really did
contain a lap advance; it does not prove that the current version-4 scenario
does. The fresh trajectory is sensitive enough that current coverage must be
recorded and accepted on its own.

## Required next work

1. Commit the typed-emitter correction and produce clean ARM64 and optimized
   i686 binaries with recorded identities.
2. Regenerate both full reports from those clean producers.
3. Require all eight components to match all 24,232 frames before running
   the two-process i686 and deliberate-mutation gates.
4. Record or derive a version-4 coverage input that structurally reaches
   `lapIndex >= 1`.
5. Keep M6 and all dependent iOS milestones open until these gates pass.
