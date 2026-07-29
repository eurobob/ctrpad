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

## Play Back

Use the command written in that folder's `metadata.txt`.

It looks like:

```sh
build/ctr_native --replay "debug/reports/20260605/ctr-123456/input.ctrreplay"
```

Playback creates a fresh writable `memcard.playback` from `memcard.seed` every run and does not touch your real saves.

If a developer asks you to bypass header identity checks:

```sh
build/ctr_native --replay "debug/reports/20260605/ctr-123456/input.ctrreplay" --replay-bypass-header
```

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
