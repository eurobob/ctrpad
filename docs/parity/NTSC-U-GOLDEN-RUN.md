# NTSC-U Golden Run

**Status:** procedure and verifier implemented; supplied NTSC-U image accepted
and visually booted; full gameplay recording pending

**Required boot ID:** `SCUS_944.26`

**Required container:** one MODE2/2352 data track whose byte size is a multiple
of 2352

This is the acceptance procedure for M1. It records a real 32-bit run, replays
it twice in separate ASLR processes, and deliberately changes a live vehicle
position field at an automatically selected active-driver frame. Retail data
stays local and ignored by Git.

## 1. Supply and build

Place the user-owned image at:

```text
assets/ctr-u.bin
```

An alternate local path may be selected without copying it:

```sh
export CTRPAD_DISC_IMAGE=/absolute/path/to/ctr-u.bin
```

Then build the exact clean commit:

```sh
tools/build-linux-i686-baseline.sh
```

The recording launcher refuses a dirty tracked worktree, a binary whose
embedded 12-character build ID differs from `HEAD`, a missing image, or an
image whose size is not divisible by 2352. The application performs the
stronger check: it reads `SYSTEM.CNF` and accepts only `SCUS_944.26`.

## 2. Record interactively

Start the loopback-only browser session:

```sh
tools/run-linux-i686-golden-session.sh
```

Open the URL printed by the script:

```text
http://127.0.0.1:6080/vnc.html?autoconnect=true&resize=scale
```

The noVNC endpoint has no password because Docker publishes it only on
`127.0.0.1`. Xvfb, x11vnc, and websockify run inside the pinned amd64
container; the game process itself is the supported i686 binary. Recording
starts before the first game frame and uses detailed rolling checkpoints.
Mesa's shader cache is persisted under the ignored build directory. The first
llvmpipe launch on an ARM64 Docker host can spend several minutes compiling
the software-renderer shaders; subsequent launches reuse the cache. The log
reports each shader phase so a warm-up is distinguishable from a disc or game
failure.

Keyboard controls:

| Retail control | Key |
|---|---|
| Cross / accelerate / accept | `C` |
| Square / brake / reverse | `X` |
| Circle / use item | `V` |
| Triangle / rear view | `Z` |
| L1 / hop and powerslide | Left Shift |
| R1 / hop and powerslide | Right Shift |
| D-pad | Arrow keys |
| Start | Enter |
| Select | Space |
| Finalize report | `F10` |

The accepted trace must visibly cover all of the following:

1. startup and title/menu transitions;
2. selection and loading of a race;
3. sustained acceleration and both steering directions;
4. a hop, powerslide, and at least one successful boost;
5. acquisition and use of an item;
6. crossing a lap line so lap/checkpoint state advances;
7. a save-producing action;
8. returning to a menu and loading/observing the persisted result.

Press `F10` only after all eight items have occurred. Wait for:

```text
[CTR Replay] report finalized by hotkey
```

Then close the game window. The report appears under:

```text
build-linux-i686-baseline/debug/reports/YYYYMMDD/ctr-HHMMSS/
```

## 3. Record coverage evidence

Create `coverage.txt` in the new report directory. Do not claim an item that
was not observed:

```text
startup_and_title=pass
menu_and_race_load=pass
steering_and_acceleration=pass
powerslide_and_boost=pass
item_acquired_and_used=pass
lap_advanced=pass
save_action=pass
persisted_result_loaded=pass
track=
character=
item=
save_or_ghost_observed=
notes=
```

`metadata.txt` must say `finalized=1`, use replay version 2, and report a
nonzero frame and checkpoint count. `ctr-native.log` must contain a record
host-address sample and driver activity transitions.

## 4. Verify unchanged and mutated playback

Run:

```sh
tools/verify-linux-i686-golden-replay.sh \
  build-linux-i686-baseline/debug/reports/YYYYMMDD/ctr-HHMMSS \
  auto
```

The verifier:

- requires the report, coverage note, clean source tree, matching binary, and
  local retail image;
- plays every frame twice in separate processes and requires exit status 0;
- requires different logged host-address samples across those processes;
- requires the pointer-sensitive restored raw-checkpoint checksum to differ
  from the recorded payload in each process;
- derives the first frame where `driver[0]` becomes active;
- flips bit 0 of the real `driver[0].posCurr.x` field at that exact frame;
- requires exit status 2 and `drivers` as the first canonical difference;
- hashes the disc identity, replay, checkpoint, memcard seed, metadata,
  coverage note, environment, and all three playback logs.

Successful verification writes:

```text
playback-1.log
playback-2.log
playback-mutated.log
mutation-frame.txt
disc.sha256
environment.txt
evidence.sha256
```

The two unchanged runs prove the canonical digest survives real process
address randomization while raw checkpoint bytes do not. The mutated run proves
the gate fails at a known gameplay-state change rather than merely comparing
host layouts.

## 5. Acceptance review

Before M1 is marked complete, copy the report-relative hashes, build ID,
compiler versions, frame counts, automatically selected mutation frame, and
the relevant pass/failure lines into a dated result document under
`docs/parity/`. Never copy the disc, extracted assets, replay memcard contents,
or other retail-derived payloads into Git.
