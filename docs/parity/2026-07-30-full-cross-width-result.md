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

## First post-correction regeneration and second defect

Committed producer `7af15bea2157` regenerated the full ARM64 report:

```text
producer:
  build-macos-arm64/ctr_native-potion-fix-producer-7af15bea2157
producer SHA-256:
  d971fce784b8ac0470c5d1603d372a4f2a7f96f7625f861c2868f6328631823a
report:
  build-macos-arm64/debug/reports/20260730/ctr-211646
frames/checkpoints/finalized:
  24232 / 81 / 1
```

Comparing it with the immutable earlier i686 report `ctr-223323` is not a
clean-pair acceptance test, because the i686 report predates the correction.
It is nevertheless a controlled diagnostic comparison:

```text
timing:      equal=24232 mismatched=0   ranges=none
rng:         equal=24232 mismatched=0   ranges=none
drivers:     equal=24232 mismatched=0   ranges=none
world:       equal=23733 mismatched=499 ranges=22158-22656
allocation:  equal=23759 mismatched=473 ranges=22158-22630
root:        equal=23733 mismatched=499 ranges=22158-22656
pads:        equal=24232 mismatched=0   ranges=none
vsync:       equal=24232 mismatched=0   ranges=none
```

The complete disappearance of the old frame-16,561 RNG/world/allocation
ranges and frame-17,213 driver range is end-to-end evidence that the potion
correction fixed the first defect. It does not pass M6 because the independent
later range remains.

Decoding checkpoint 22,200 on both reports localized the remaining state:

```text
                              i686   ARM64
numParticles                    10       0
particle-pool free / maximum 22/32   32/32

thread, instance, small, medium, large, oscillator, and rain pool counts:
identical
```

The ten i686 ordinary-particle records have lifetimes 14 down through 5,
color flags `0x00a3`, and the exact axis/scale values emitted by
`R233.particleEmitterData[46]`. This is overlay-233 cutscene particle config
6, which emits one particle per frame with a lifespan of 15.

Seven overlay-233 particle groups used an axis-union initializer to encode
their function header:

```text
.InitTypes.AxisInit = {{0, colorFlags, lifespan}, {0, 0, 0}}
```

On ILP32, the union begins at offset 4 and the four-byte null value followed
by color/lifespan happens to reproduce the retail `FuncInit` bytes. On LP64,
the union begins at offset 8 and `particle_funcPtr` is eight bytes; the
initializer therefore places color/lifespan inside that pointer and leaves
the real LP64 fields zero. ARM64 calls the emitter, but the zero lifespan
prevents the particles from persisting.

The seven group headers at indices `0,10,20,28,38,46,54` now use semantic
`FuncInit` initializers. Their color/lifespan pairs are:

```text
00a3/12 00a3/12 00a3/15 00a3/15 00c3/15 00a3/15 50a2/8
```

The new `--self-test-cutscene-particle-emitter-layout` entry point validates
all seven headers, their terminators, and all eight particle configs on both
pointer widths. On i686 it additionally compares each complete 0x24-byte
header against the original retail words.

Source-level verification:

```text
macOS ARM64 Release:
  16/16 CTests
  SHA-256 021821a4cddafecd05f9c1ba5fe79db87296618ea70843de9b728027712aeb19

macOS ARM64 ASan/UBSan:
  16/16 CTests
  SHA-256 0ac33ae7f62b11b842390f4bf43a33a5e70ac655246430eee1952055d1417c57

Linux optimized i686:
  16/16 CTests, including exact retail-record comparisons
  Build ID 05a883c8d6efcef3db117611b5ad43ee5172bda5
  SHA-256 9a5af521e2dcf1c97413dbbb1b8fd805a18699b50bbd6acdc2220e02fde13820

git diff --check:
  passed
```

The ARM64 builds repeated only established warnings. The i686 build repeated
the established two format-security and two maybe-uninitialized warnings.

An unchanged committed i686 producer is separately regenerating the earlier
report under ordinary Xvfb/llvmpipe execution. At this documentation
checkpoint it was alive at frame 3,000 with 11 checkpoints. It is diagnostic
continuity evidence and cannot accept the new source correction.

## Finalized ARM64 proof after both emitter corrections

The overlay correction and documentation were committed and pushed as:

```text
eee2a8df5b9605d27c7b20e943bba76174a4f6fc
fix: preserve cutscene emitter layout across widths
```

GitHub, `origin/codex/arm64-apple`, and local `HEAD` all resolved to that
commit. Draft PR #1 remained open against `main`; `origin/main` remained at
`95417c723518407d6bfe3c81a37606294963efe2`.

Clean same-source producers:

```text
ARM64:
  build-macos-arm64/ctr_native-cutscene-fix-producer-eee2a8df5b96
  SHA-256 fa9a7d46292ab09b143e0e2b514317daa251af48f6341dc437b1f5d81ded3961
  16/16 CTests

i686:
  /private/tmp/ctrpad-i686-vehlap-8IzCKm/ctr_native-cutscene-fix-producer-eee2a8df5b96
  SHA-256 d2e6f06023ccaedae689f11b36b33e005cb30d7bbc70d2a5e3e036f57b276c8e
  Build ID 14387ee999252f9fb16177bb0fdfb76e2c099f4b
  16/16 CTests, including exact retail-record comparisons
```

The clean ARM64 producer finalized:

```text
report:
  build-macos-arm64/debug/reports/20260730/ctr-215303
frames/checkpoints/finalized/exit:
  24232 / 81 / 1 / 0

input.ctrreplay:
  dfd06c677f29d9c2155cb01cf00fd958dddfc06127d9b6c67937651039029e09
state.ctrstates:
  a0ea4a59e7e26716e99249430aed02bd45794a096c4b929dcfd323e8ecd03632
metadata.txt:
  9ed1ba92c55da00135e5329bce682b2d82a8530a4daab3607bdd6b10eb0d9e4d
ctr-native.log:
  bea2c87b0c6694f139561ce5f2ba94eba7d46a6daa54b8bce17b39aba3d0de1a
```

The all-frame diagnostic comparison against the earlier immutable i686 report
`ctr-223323` now passes:

```text
timing:      equal=24232 mismatched=0 ranges=none
rng:         equal=24232 mismatched=0 ranges=none
drivers:     equal=24232 mismatched=0 ranges=none
world:       equal=24232 mismatched=0 ranges=none
allocation:  equal=24232 mismatched=0 ranges=none
root:        equal=24232 mismatched=0 ranges=none
pads:        equal=24232 mismatched=0 ranges=none
vsync:       equal=24232 mismatched=0 ranges=none
```

That earlier i686 binary predates both typed corrections, but both i686
self-tests prove the new typed tables preserve its complete original bytes.
The full comparison therefore proves that both ARM64 mismatch ranges are
removed without changing the retail i686 trajectory. It is strong end-to-end
correction evidence, but the formal same-commit clean-pair gate remains open.

The superseded first-fix-only i686 continuity run was stopped at frame 4,566
to return its CPU budget to the report that can satisfy the gate. Docker
reported exit 137. Its recorder shutdown path wrote `finalized=1`, but the
frame count is only 4,566 of the 24,232-frame seed and no normal exit status
was recorded; it is explicitly rejected as a partial report. Its files remain
at `/private/tmp/ctrpad-i686-potion-run-KMjNWQ`.

The clean `eee2a8df5b96` i686 producer is now recording:

```text
container:
  ctrpad-i686-cutscene-full
run directory:
  /private/tmp/ctrpad-i686-cutscene-run-4hjAQW
report:
  debug/reports/20260731/ctr-025812
documentation checkpoint:
  finalized=0 frame_count=1800 checkpoint_count=7
```

Its first 1,879 complete frame records match the clean ARM64 report on all
eight required components. The full report must still finalize and match all
24,232 frames before M6 can be accepted.

## Current-build version-4 lap coverage

### Rejected extension B

Commit `0e524c0c0` added `tools/extend-replay-input.mjs` so replay extensions
are validated and reproducible. It validates source, optional pad source, and
every output record; copies pad snapshots only when explicitly requested;
recomputes checksums; and refuses to overwrite its output. Copied state
digests are automation input, not parity evidence. Only a fresh recording
from the generated seed can be evaluated.

Extension B used a 36,000-frame version-4 seed, retained trial A's controller
transport, and retained clean version-4 VSync transport:

```text
seed:
  build-macos-arm64/debug/reports/20260730/ctr-215303/input-lap-extension-b.ctrreplay
frames/version:
  36000 / 4
SHA-256:
  2c98ea3c327770d3d90ba7eed09fd52be13f2b703b344c9cec5102091ac674b6
```

Clean producer `eee2a8df5b96` recorded report `ctr-221412`. The process was
stopped deliberately at frame 28,850 after the structural checkpoint had
remained at lap 0/checkpoint 56 from frame 21,300 onward: 7,550 frames with
no further course progress. The run had previously reached checkpoint 77 but
never a lap advance.

```text
report:
  build-macos-arm64/debug/reports/20260730/ctr-221412
frames/checkpoints/finalized/input-seed-frames:
  28850 / 97 / 1 / 36000

input.ctrreplay:
  ab5b4944d82f1fb3f9551805c278337fa7b6f3f4bf69756f49bf92657f579018
state.ctrstates:
  ee71a230587e9ffb2696854a86741296bc7751e6d31aea100bf025a9d12d428f
metadata.txt:
  7f55d2fb496156a01012651adf8ab6ef7ad9ed9849816ab46bc629d693834d35
ctr-native.log:
  5047232712cfa5e10c8d8777f453f23ae451086d8a09a491667dadac7f2f5cf8
```

`finalized=1` means the interrupt closed the report headers; it does not turn
28,850 recorded frames into the requested 36,000-frame normal completion.
Extension B is retained and explicitly rejected.

### Historical timing discovery

The finalized historical i686 report
`build-linux-i686-baseline/debug/reports/20260729/ctr-225420` is replay
version 2 and reaches lap 1 at frame 21,300. Comparing its records with the
inherited clean version-4 input established:

- the first nine bytes of each 12-byte pad snapshot—the status, ID, buttons,
  analog axes, and connected state actually transported to the PSX pad
  bus—match for all four pads on all 24,232 frames;
- the three reserved bytes differ because later replay versions use them for
  legacy submit-name migration state and are not pad-bus transport; and
- per-frame VBlank totals differ on 412 frames, beginning at frame zero.

The controller script was therefore not the cause of the changed trajectory.
The current inherited version-4 timing was. Version 2 already records each
in-frame VSync packet and end-of-frame elapsed time, but it has no way to mark
the pre-frame timing boundary introduced in version 4.

Commit `a269843a2` extended `tools/extend-replay-input.mjs` to accept validated
version-2 and version-3 sources. Promotion preserves the historical records
and in-frame timing after frame zero, changes the header to version 4, and
requires a complete version-4 bootstrap replay. Frame zero's entire VSync
block and end-of-frame elapsed time are taken from that bootstrap so the
otherwise-unrecorded initial boundary is complete. The tool rejects promotion
without that independent boundary.

The exact promotion was:

```sh
node tools/extend-replay-input.mjs \
  --frames 24232 \
  --bootstrap-from \
    build-macos-arm64/debug/reports/20260730/ctr-215303/input.ctrreplay \
  build-linux-i686-baseline/debug/reports/20260729/ctr-225420/input.ctrreplay \
  build-linux-i686-baseline/debug/reports/20260729/ctr-225420/input-promoted-v4.ctrreplay
```

The promoted seed validates as version 4 with 24,232 records:

```text
build-linux-i686-baseline/debug/reports/20260729/ctr-225420/input-promoted-v4.ctrreplay
SHA-256:
80522b7675089f4bddd6c3d04e09a86c41eb5524e6661b5e65f63d886405fcea
```

### Accepted fresh ARM64 coverage report

Clean current producer `eee2a8df5b96` recorded from the promoted input and
reached normal process exit:

```sh
cd build-macos-arm64
./ctr_native-cutscene-fix-producer-eee2a8df5b96 \
  --record-from-replay \
  ../build-linux-i686-baseline/debug/reports/20260729/ctr-225420/input-promoted-v4.ctrreplay
```

```text
report:
  build-macos-arm64/debug/reports/20260730/ctr-223221
frames/checkpoints/finalized/exit:
  24232 / 81 / 1 / 0
build ID:
  eee2a8df5b96

input.ctrreplay:
  d6a5c0340513ba14ed4c95fe3d074ba4de971ecc9ee537b8aacf3cffb6bab2c7
state.ctrstates:
  63f48fe6b60b3a145084cfcf908cc854447f155f7df8b75d0befda3867d3b23c
metadata.txt:
  4f997c9773fbb7d6bb25f13e0161f850a630015fd87b0465a8f511222dd5fcb1
ctr-native.log:
  04412d5d0305919e2025d6bb70c479e3403650d83084e22903b630fac7c9caa3
```

The typed checkpoint inspector reports:

```text
checkpointVersion=3 pointerSize=8 activeRecords=80
frame 21000 lap 0 checkpoint 72
frame 21300 lap 1 checkpoint 1
frame 21600 lap 0 checkpoint 0
maxLap=1 maxCheckpoint=77 lapAdvanced=yes
```

The later lap-zero state is the scenario's post-race reset; it does not erase
the observed lap transition. This fresh version-4 report supplies the
structural current-build coverage that extensions A and B did not.

### Reproducible transport audit

`tools/compare-replay-transport-semantics.mjs` validates both replay files and
compares the game-visible pad bytes, end-of-frame elapsed time, VBlank total,
and fully expanded pre-frame and in-frame VSync sequences. It treats raw
packet encoding differences as informational because version 4 may
run-length-encode repeated equal packets.

```sh
node tools/compare-replay-transport-semantics.mjs \
  build-linux-i686-baseline/debug/reports/20260729/ctr-225420/input-promoted-v4.ctrreplay \
  build-macos-arm64/debug/reports/20260730/ctr-223221/input.ctrreplay
```

```text
padTransport:             equal=24232 mismatched=0
elapsedTime:              equal=24232 mismatched=0
vblankTotal:              equal=24232 mismatched=0
rawVblankBlock:           equal=396   mismatched=23836
expandedPreFrameVblank:   equal=24232 mismatched=0
expandedInFrameVblank:    equal=24232 mismatched=0
```

The 23,836 raw-block differences are the expected RLE representation change;
the expanded sequences are identical. This audit proves that the fresh report
consumed the promoted timing and input transport exactly. It does not compare
copied seed state digests and does not replace the still-running formal
same-commit i686 parity report.

## Required next work

1. Allow clean i686 report `ctr-025812` to finalize.
2. Require all eight components to match the finalized clean ARM64 report for
   all 24,232 frames before running
   the two-process i686 and deliberate-mutation gates.
3. Keep M6 and all dependent iOS milestones open until these gates pass.
