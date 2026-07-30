# Canonical Game-State Digest

**Schema:** 2

**Replay file version:** 4

**Status:** implemented, synthetic mutation proof passed, and the current
2,200-frame startup-to-race prefix matches across i686 and ARM64; the full
24,232-frame NTSC-U golden scenario remains pending

## Purpose

Checkpoint payload checksums contain process addresses, native pointers,
padding, and host-subsystem state. The original replay comparison covered
timers, high-level state, six RNG words, pads, and VBlank packets, but not
vehicle physics. The canonical digest adds a fixed-width, field-by-field parity
signal while retaining the replay scheduler as deterministic input and timing
transport.

`struct NativeStateDigest` is exactly 56 bytes on every target:

```text
u32 schemaVersion
u32 componentMask
u64 timing
u64 rng
u64 drivers
u64 world
u64 allocation
u64 root
```

Each integer is fed to FNV-1a 64 in explicit little-endian byte order. Native C
struct bytes are never hashed.

## Components

- `timing`: VBlank/frame counters, race/level timers, elapsed milliseconds,
  traffic-light timing, event timing, and frozen-time state. The host wall
  clock `clockFrameStart` is deliberately excluded.
- `rng`: mix, audio, adventure, and per-race RNG state.
- `drivers`: all eight driver-table slots, but a non-null entry is live only
  when it is an aligned object in the current large-stack pool and absent from
  that pool's free list. Live records include fixed-point current/previous
  position and velocity; rotations and surface normals; collision/action
  flags; acceleration, speed, steering, jump, wall, powerslide, turbo and
  reserve state; lap/rank/checkpoint progress; held-item and damage state;
  physics constants; active kart-state union scalars; and explicit bot
  navigation/physics state.
- `world`: game modes, level IDs, player/bot counts, laps, particles, time
  crates, adventure progress, cup points, battle state, loading stage, and
  menu state.
- `allocation`: free/taken counts and retail item capacity for all eight JIT
  pools, the active MEMPACK index, and each allocator's initialized-pointer
  topology, empty/full relationships, previous-allocation existence, and
  bookmark depth. Host slot widths, pool byte sizes, main-pack sizes, physical
  cursors, allocation byte counts, and bookmark addresses are excluded.
- `root`: schema, mask, and the five named component hashes.

Native pointers are either excluded or encoded as stable guest references:

- driver references become `(DRVR, slot)`;
- quadblock references become `(QUAD, byte offset from the level array)`;
- null and out-of-domain references use fixed sentinels.

Window/renderer handles, OpenGL names, file paths, host pointers, audio device
handles, padding, and wall-clock readings are excluded.

## Replay integration

`NativeReplaySchedulerFrameInfo` carries a digest at frame begin and end.
Replay format version 2 introduced it; current version 4 keeps the fixed frame
layout and also captures the complete inter-frame VSync timing boundary.
Playback compares every named component and the root at frame end. A mismatch
logs the first named component and both expected/live component values
alongside the existing pad, timer, RNG, and VBlank evidence. Playback stops on
that exact frame and the process exits with status 2, so CI cannot mistake a
logged divergence for a passing gate. Other replay runtime and finalization
failures exit with status 1; an unchanged replay that reaches its recorded end
exits with status 0.

Version 1 replay files are intentionally rejected by the format check. They do
not contain a trustworthy physics-parity signal.

## Mutation and address-independence proof

CTest invokes:

```sh
ctr_native --self-test-state-digest
ctr_native --self-test-replay-gate
```

The fixture creates equivalent game/driver state at different native
addresses, changes excluded renderer pointers, changes excluded padding, and
changes the excluded host wall-clock field. Their roots remain identical. It
then increments `Driver.posCurr.x` by one and requires exactly the `drivers`
component and root to change:

```text
[CTR StateDigest] self-test passed: address-independent root=947c0430f66c667d mutation component=drivers free-driver=ignored allocation-host-geometry=ignored mempack-coordinate=ignored
[CTR Replay] self-test passed: runtime-error=1 divergence=2 mutation-frame=17 component=drivers binary-identity=checked checkpoint-start=checked complete-vsync=checked
```

The replay-gate fixture also verifies frame parsing, identical-frame matching,
an isolated `drivers` component mismatch, and the distinct process statuses
for harness failure and canonical divergence. This proves the gate's basic
invariants without retail data.

`tools/compare-replay-state-components.mjs` separately compares the transport
that drives those states. In addition to the six digest values it can require
`pads` (all 48 snapshot bytes) and `vsync` (VBlank total, packet count,
pre-frame packet count, and every used run-length-encoded packet). Each input's
pad checksum, complete-record checksum, and decoded VSync total are validated
before equality is reported.

The schema-2 fixture additionally places a driver slot on the pool free list,
changes its reused bytes independently in two trackers, and requires the roots
to remain equal. Separate allocator fixtures require different host pool
strides and MEMPACK coordinates to remain equal while retaining the logical
population and lifecycle signals above. This addresses the exact false
mismatches found during the ARM64/i686 prefix investigation.

It does not replace the pending M1 proof: record and replay a full NTSC-U run
in separate processes, then use
`--replay-test-perturb-driver-x <frame>` while driver slot 0 is active and
observe status 2 at that exact frame. Recording and playback also log
host-address samples excluded from the digest. After restoring the bootstrap
checkpoint, playback recaptures its raw pointer-bearing payload and logs the
recorded/restored-process checksums as diagnostic evidence; those raw hashes
are expected to differ under ASLR and are never used as the parity decision.

`docs/parity/NTSC-U-GOLDEN-RUN.md` and
`tools/verify-linux-i686-golden-replay.sh` turn those signals into the final M1
acceptance run.

## Known scope

Schema 2 hashes held-item state, AI state, and allocation lifecycle, but it does
not yet serialize every field of every live dynamic weapon or particle
instance. Golden-run coverage must exercise item use; if a mutation can alter
a gameplay-relevant dynamic object without changing any current component,
schema 3 must add a canonical live-object component before M1 closes.
