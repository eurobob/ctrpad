# Canonical Game-State Digest

**Schema:** 1

**Replay file version:** 2

**Status:** implemented and synthetic mutation proof passed; NTSC-U golden run
pending

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
- `drivers`: all eight canonical driver slots; fixed-point current/previous
  position and velocity; rotations and surface normals; collision/action
  flags; acceleration, speed, steering, jump, wall, powerslide, turbo and
  reserve state; lap/rank/checkpoint progress; held-item and damage state;
  physics constants; active kart-state union scalars; and explicit bot
  navigation/physics state.
- `world`: game modes, level IDs, player/bot counts, laps, particles, time
  crates, adventure progress, cup points, battle state, loading stage, and
  menu state.
- `allocation`: free/taken counts and dimensions for all eight JIT pools plus
  each MEMPACK allocator's sizes, bookmark count, and stable offsets.
- `root`: schema, mask, and the five named component hashes.

Native pointers are either excluded or encoded as stable guest references:

- driver references become `(DRVR, slot)`;
- quadblock references become `(QUAD, byte offset from the level array)`;
- MEMPACK pointers and bookmarks become `(MPAK, byte offset from arena base)`;
- null and out-of-domain references use fixed sentinels.

Window/renderer handles, OpenGL names, file paths, host pointers, audio device
handles, padding, and wall-clock readings are excluded.

## Replay integration

`NativeReplaySchedulerFrameInfo` carries a digest at frame begin and end.
Replay format version 2 records it in every frame. Playback compares every
named component and the root at frame end. A mismatch logs the first named
component and both expected/live component values alongside the existing pad,
timer, RNG, and VBlank evidence. Playback stops on that exact frame and the
process exits with status 2, so CI cannot mistake a logged divergence for a
passing gate. Other replay runtime and finalization failures exit with status
1; an unchanged replay that reaches its recorded end exits with status 0.

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
[CTR StateDigest] self-test passed: address-independent root=69d9c7bf8ca4aaf8 mutation component=drivers
[CTR Replay] self-test passed: runtime-error=1 divergence=2 mutation-frame=17 component=drivers
```

The replay-gate fixture also verifies frame parsing, identical-frame matching,
an isolated `drivers` component mismatch, and the distinct process statuses
for harness failure and canonical divergence. This proves the gate's basic
invariants without retail data.

It does not replace the pending M1 proof: record and replay a full NTSC-U run
in separate processes, then use
`--replay-test-perturb-driver-x <frame>` while driver slot 0 is active and
observe status 2 at that exact frame.

## Known scope

Schema 1 hashes held-item state, AI state, and allocation counts, but it does
not yet serialize every field of every live dynamic weapon or particle
instance. Golden-run coverage must exercise item use; if a mutation can alter
a gameplay-relevant dynamic object without changing any current component,
schema 2 must add a canonical live-object component before M1 closes.
