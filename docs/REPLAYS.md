# Replays

Use this for bug reports in internal builds.

## Quick State

- `F5`: save `debug/states/quick.ctrstates`
- `F8`: load `debug/states/quick.ctrstates`

## Record

```sh
build/ctr_native --record
```

Windows: use `build\ctr_native.exe` instead.

Normal saves live in `memcards/slot0`.

When recording starts, the CTR save files from `memcards/slot0` and `slot1` are copied to `memcard.seed`. The game records with a writable copy named `memcard.recording`, so saves and ghosts made while recording stay in the report.

To choose when recording starts:

```sh
build/ctr_native --record --toggle
```

- Press `F9` to start.
- Press `F10` to stop.

For more detailed reports:

```sh
build/ctr_native --record --detailed
```

You can combine both:

```sh
build/ctr_native --record --toggle --detailed
```

## Seed a fresh recording from an input trace

An internal build can use a validated replay as a pad-input script:

```sh
build/ctr_native \
  --record-from-replay "debug/reports/20260605/ctr-123456/input.ctrreplay"
```

The source report must have a sibling `memcard.seed`. The command starts a
fresh game, clones that seed, validates every source frame and pad checksum,
and records a new replay plus rolling checkpoints. It stops automatically
after the source frame count.

Replay version 4 is a complete timing seed. It records every emitted VSync
packet, including packets emitted before the next tracked `BeginFrame` while
asynchronous loading is still running. The existing 440-byte frame record is
unchanged:

- the low 16 bits of `vblankPacketCount` hold the total encoded packet count;
- the high 16 bits hold the prefix count emitted before `BeginFrame`;
- each `u16` packet stores the emitted VBlank count in its low byte and
  `repeat count - 1` in its high byte; and
- adjacent equal calls are run-length encoded, up to 256 calls per entry.

The playback and `--record-from-replay` paths reproduce the pre-frame prefix
before beginning the tracked frame, then consume the remaining packets at the
original call boundaries. Zero packets, a prefix longer than the total, a
decoded total mismatch, more than 64 encoded entries, and an unconsumed packet
all fail validation.

Versions 2 and 3 remain accepted as compatibility input because their frame
and pad layouts are identical, but they do not contain the pre-frame boundary.
For those inputs, `--record-from-replay` remains pad automation and records the
live run's timing. Only version 4 can be used as strict cross-architecture
timing evidence.

This is still not state restoration: the new process boots and loads the
retail image itself. Coverage must be observed again, and the newly recorded
report must pass normal playback before it is acceptance evidence.

To compare canonical component traces while a seeded recording is running:

```sh
tools/compare-replay-state-components.mjs \
  --prefix \
  --require timing,rng,drivers,world,allocation,root,pads,vsync \
  old/input.ctrreplay \
  new/input.ctrreplay
```

The comparer validates both replay formats, frame sequencing, pad checksums,
complete-record checksums, actual 48-byte pad snapshots, and the semantic
VSync packet boundary before reporting mismatch ranges. `pads` requires the
snapshots to be byte-identical. `vsync` requires the emitted VBlank total,
encoded packet count, pre-frame packet count, and all used packet entries to
match. Omit `--prefix` after the new report is finalized. A matching component
and transport trace is supporting evidence; it does not replace coverage
review or unchanged playback of the new report.

## Play Back

Use the command written in that folder's `metadata.txt`.

It looks like:

```sh
build/ctr_native --replay "debug/reports/20260605/ctr-123456/input.ctrreplay"
```

Playback creates a fresh writable `memcard.playback` from `memcard.seed` every run and does not touch your real saves.

Versions 3 and 4 record a 64-bit fingerprint of the exact executable file.
The fingerprint appears in `metadata.txt` as `executable_fingerprint`. This is
stricter than the human-readable commit/build label: two different uncommitted
binaries can both display `COMMIT-dirty`, but they cannot restore each other's
process-local callback/checkpoint state. An exact copied executable remains
valid across ASLR layouts.

To begin playback from a specific rolling checkpoint instead of checkpoint
zero:

```sh
build/ctr_native \
  --replay "debug/reports/20260605/ctr-123456/input.ctrreplay" \
  --replay-start-checkpoint 6
```

The index is zero-based. Playback validates the complete checkpoint file,
restores the selected record, seeks the input stream to that record's replay
frame, and continues through the report's final frame. An out-of-range index,
invalid frame mapping, failed seek, checksum change, or restore failure exits
with status 1. This option does not relax executable identity: use the exact
version-3-or-newer producing binary when the result is checkpoint evidence.

If a developer asks you to bypass header identity checks:

```sh
build/ctr_native --replay "debug/reports/20260605/ctr-123456/input.ctrreplay" --replay-bypass-header
```

`--replay-bypass-header` is diagnostic-only. It does not make checkpoints
portable across rebuilt binaries: callback addresses currently use image
offsets, and unity-build function offsets can change after a small source edit.
Never use a bypassed run as checkpoint or parity evidence.

## Parity-gate proof

Internal builds expose one deliberately destructive playback-only test option:

```sh
build/ctr_native \
  --replay "debug/reports/20260605/ctr-123456/input.ctrreplay" \
  --replay-test-perturb-driver-x 1234
```

At replay frame `1234`, this flips bit 0 of the live
`driver[0].posCurr.x` gameplay field before the end-of-frame digest is
captured. Choose a frame where driver slot 0 is active. A working parity gate
must report that exact frame, name `drivers` as the first canonical component,
and exit with status 2. An invalid frame, missing driver, corrupt replay,
checkpoint failure, or replay I/O/finalization failure exits with status 1.
Unmodified playback exits with status 0 after all recorded frames match.

This option changes only the playback process and never rewrites the replay,
checkpoint, seed memcard, or retail image.

For the full two-process ASLR, raw-checkpoint, and automatic mutation proof,
follow `docs/parity/NTSC-U-GOLDEN-RUN.md`.
