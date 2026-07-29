# Replay and Checkpoint Tooling Assessment

**Assessment date:** 2026-07-29

**Source baseline:** upstream `2df55dc5a`, downstream source still unchanged

**Verdict:** useful deterministic input/timing harness; not a sufficient
cross-architecture retail-parity oracle.

## What the replay system proves

Each replay frame stores:

- begin/end timing and high-level state;
- four complete platform pad snapshots;
- VBlank packet count and values;
- pad and record checksums
  (`platform/native_replay_scheduler.c:79-90,149-198`).

Playback installs the recorded pad snapshots before the game frame, supplies the
recorded elapsed time, and replays each VBlank packet
(`platform/native_replay_scheduler.c:1460-1609`). This is a strong way to feed
the same inputs and host timing to different builds.

At frame end, however, divergence detection compares only:

- frame/VSync timers, the high-level game state, loading stage, and level ID;
- six RNG words;
- pad checksum;
- VBlank packet totals and count
  (`platform/native_replay_scheduler.c:1300-1340,1623-1700`).

Those checks can detect control-flow, timing, loading, and RNG divergence. They
cannot detect a wrong vehicle position, velocity, rotation, collision result,
reserve value, lap state, item state, AI state, or most other game data if the
16 recorded frame-info values happen to remain equal.

## What checkpoints prove

Detailed reports capture a whole-machine checkpoint every 300 replay frames;
ordinary reports capture a bootstrap checkpoint only
(`platform/native_replay_scheduler.c:29-32,985-1067,1369-1380`).

The checkpoint payload is broad. It copies resident and overlay globals,
MEMPACK backing memory, scratchpad bytes, a relocated-asset pointer-slot map,
and input/audio/GPU native state (`platform/native_checkpoint.c:24-52,194-236,
2119-2153`; `platform/native_state.c:20-37,91-165`). Checkpoint files checksum
the raw payload, which is useful for corruption detection
(`platform/native_checkpoint_file.c:40-53,195-252`).

The raw checksum is not a parity hash:

- checkpoint headers store every live address-range start and a code anchor as
  `u32` (`platform/native_checkpoint.c:91-103,310-365`);
- the payload contains native and relocated pointers copied from process memory;
- checkpoint initialization fails when a pointer is above `0xffffffff`
  (`platform/native_checkpoint.c:116-126,325-328,2040-2047`);
- pointer relocation makes restore work across 32-bit image bases, but the
  checksum is calculated before canonicalization and therefore changes with
  host addresses.

Consequently, identical gameplay in two address-randomized 32-bit processes is
not required to produce identical checkpoint bytes, and the format cannot be
captured by the future 64-bit process in its current form.

## Decision

Keep the replay scheduler as the input, elapsed-time, VBlank, and bootstrap
transport. Do not use “replay completed without its current divergence log” as
evidence of retail parity.

Add a versioned, fixed-width **canonical game-state digest** to detailed replay
records before structural 64-bit work is accepted. Its schema must:

1. serialize gameplay fields explicitly rather than hashing native C struct
   bytes;
2. exclude padding, host handles, native pointers, file paths, wall clocks, and
   renderer object names;
3. encode guest references as a stable region kind plus offset/handle;
4. include fixed-point position, velocity, rotation, collision, powerslide,
   turbo reserve, lap/checkpoint, item, AI, timer, RNG, loading, and allocation
   state;
5. produce named component digests as well as a frame root digest so the first
   divergent subsystem is visible;
6. retain the current pad, elapsed-time, and VBlank checks;
7. use the same byte order and integer widths on i686 and ARM64.

The existing checkpoint region and pointer-slot inventories are inputs to this
schema, not the schema itself.

## Proof status

Completed:

- schema 1 explicitly serializes fixed-width gameplay fields and named
  components (`docs/parity/STATE-DIGEST.md`);
- an address-independent synthetic fixture passes with different host pointers,
  excluded padding, and wall-clock values;
- changing `Driver.posCurr.x` by one changes exactly the `drivers` component
  and the root;
- replay runtime errors and canonical divergence have distinct nonzero process
  statuses, and the media-free replay-gate CTest identifies a synthetic driver
  mutation as `drivers`;
- internal playback can perturb the real `driver[0].posCurr.x` field at an
  exact requested frame without altering the source replay;
- recording/playback log excluded host-address samples and playback recaptures
  the restored raw checkpoint for pointer-sensitive checksum comparison;
- a loopback-only noVNC launcher and automated two-process verifier implement
  the full pending procedure (`docs/parity/NTSC-U-GOLDEN-RUN.md`).

Still required before M1 can close:

- record a deterministic, detailed 32-bit run using the retail image;
- replay it in a second process and demonstrate no digest divergence;
- run the implemented gameplay mutation injection against the golden replay
  and demonstrate status 2 at the exact frame with `drivers` named;
- repeat the unchanged run under address randomization and demonstrate that the
  canonical digest remains stable even when raw checkpoint checksums differ.

The archived original image cannot supply the golden proof: its boot ID is PAL
Europe `SCES_021.05`, while this source baseline is NTSC-U
`SCUS_944.26`/`BUILD=926`. The replacement raw image is the required
`SCUS_944.26`, passes the runtime identity gate, and visibly boots with live
input (`docs/builds/2026-07-29-baselines.md`). The remaining work is now the
full run/replay/mutation evidence, not media acquisition.
