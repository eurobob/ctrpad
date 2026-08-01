# NTSC-U Golden Run

**Status:** the historical full gameplay recording exposed input and timing
boundary omissions; replay version 4 closes both in current builds, but a fresh
full-coverage version-4 report is still pending

**Required boot ID:** `SCUS_944.26`

**Required container:** one MODE2/2352 data track whose byte size is a multiple
of 2352

This is the acceptance procedure for M1. It records a real 32-bit run, replays
it twice in separate ASLR processes, and deliberately changes a live vehicle
position field at an automatically selected active-driver frame. Retail data
stays local and ignored by Git.

The first complete report and its unresolved verification result are recorded
in `docs/parity/2026-07-29-golden-run-result.md`. Observed coverage is not M1
acceptance until both unchanged playbacks and the deliberate mutation run
pass.

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
[CTR Gameplay] player powerslide boost:
[CTR Replay] report finalized by hotkey
```

The first line is emitted only by the retail successful-boost branch; a
shoulder-button press while the meter is still green does not count.

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

`metadata.txt` must say `finalized=1`, use replay version 4, and report a
nonzero frame and checkpoint count. `ctr-native.log` must contain a record
host-address sample, race-driver activity transitions, and the successful
powerslide-boost line. The verifier requires all eight coverage keys to be
`pass`.

## 4. Verify unchanged and mutated playback

Run:

```sh
tools/verify-linux-i686-golden-replay.sh \
  build-linux-i686-baseline/debug/reports/YYYYMMDD/ctr-HHMMSS \
  auto
```

The defaults intentionally verify the protected baseline build against the
current clean commit. To verify an exact disposable i686 producer without
copying it over the baseline, select its build directory, binary, toolchain
manifest, and source commit:

```sh
CTRPAD_I686_BUILD_DIR=/absolute/path/to/disposable-build \
CTRPAD_I686_BINARY=/absolute/path/to/disposable-build/exact-producer \
CTRPAD_I686_ALT_LOADER=/absolute/path/to/disposable-build/ld-linux-i686.so.2 \
CTRPAD_TOOLCHAIN_PACKAGES=/absolute/path/to/toolchain-packages.txt \
CTRPAD_EXPECTED_SOURCE_COMMIT=0123456789abcdef0123456789abcdef01234567 \
CTRPAD_DISC_IMAGE=/absolute/path/to/user-owned-ntsc-u.bin \
tools/verify-linux-i686-golden-replay.sh \
  /absolute/path/to/disposable-build/debug/reports/YYYYMMDD/ctr-HHMMSS \
  auto
```

The selected binary, report, and optional alternate loader must be under the
selected build directory so the container receives exactly that run tree.
The toolchain manifest may be elsewhere. The alternate loader is needed only
when the environment maps separate direct i686 launches at identical virtual
addresses. Playback 1 remains a direct executable launch; playback 2 uses the
selected loader to force a second real mapping. The loader's path and hash
are written to `environment.txt`. Report metadata must be finalized replay
version 4, contain nonzero frame and checkpoint counts, and identify the
expected commit's 12-character build ID. The producer must embed the same
build ID.
`CTRPAD_EXPECTED_SOURCE_COMMIT` must name a full commit present in this
repository; it defaults to `HEAD`. The golden scenario defaults to exactly
24,232 frames and 81 checkpoints. `CTRPAD_EXPECTED_FRAME_COUNT` and
`CTRPAD_EXPECTED_CHECKPOINT_COUNT` exist for a separately documented
replacement scenario; do not lower them merely to make an incomplete report
pass.

The full-golden default is `CTRPAD_REQUIRE_COVERAGE=1`: `coverage.txt` and all
eight `pass` keys are mandatory. If structural coverage has been accepted in
a separate, explicitly cited report, the same script may run only the
two-process address-randomization and deliberate-mutation proof:

```sh
CTRPAD_REQUIRE_COVERAGE=0 \
CTRPAD_I686_BUILD_DIR=/absolute/path/to/immutable-run-tree \
CTRPAD_I686_BINARY=/absolute/path/to/immutable-run-tree/exact-producer \
CTRPAD_I686_ALT_LOADER=/absolute/path/to/immutable-run-tree/ld-linux-i686.so.2 \
CTRPAD_TOOLCHAIN_PACKAGES=/absolute/path/to/toolchain-packages.txt \
CTRPAD_EXPECTED_SOURCE_COMMIT=0123456789abcdef0123456789abcdef01234567 \
tools/verify-linux-i686-golden-replay.sh \
  /absolute/path/to/immutable-run-tree/debug/reports/YYYYMMDD/ctr-HHMMSS \
  auto
```

This mode still requires the full finalized frame/checkpoint counts, exact
build identity, clean tracked worktree, powerslide event, both unchanged
playbacks, address randomization, and the mutation rejection. It omits only
the manual coverage-form requirement and labels its result as replay
process-determinism/mutation verification. It must never be reported as a
full golden-coverage pass.

### Recover host-side finalization after an interrupted terminal

The verifier writes `container-exit-status.txt` only after its container
completes both unchanged playbacks and the mutation checks with exit 0. If the
host terminal is interrupted after Docker finishes but before evidence hashing,
rerun the exact command with:

```sh
CTRPAD_FINALIZE_ONLY=1 \
CTRPAD_REQUIRE_COVERAGE=0 \
CTRPAD_I686_BUILD_DIR=/absolute/path/to/immutable-run-tree \
CTRPAD_I686_BINARY=/absolute/path/to/immutable-run-tree/exact-producer \
CTRPAD_I686_ALT_LOADER=/absolute/path/to/immutable-run-tree/ld-linux-i686.so.2 \
CTRPAD_TOOLCHAIN_PACKAGES=/absolute/path/to/toolchain-packages.txt \
CTRPAD_EXPECTED_SOURCE_COMMIT=0123456789abcdef0123456789abcdef01234567 \
tools/verify-linux-i686-golden-replay.sh \
  /absolute/path/to/immutable-run-tree/debug/reports/YYYYMMDD/ctr-HHMMSS \
  auto
```

Finalize-only mode does not launch the game. It requires the captured status
to be exactly 0, then independently rechecks both normal-completion markers,
distinct restored raw checksums and host-address samples, the selected mutation
frame, the mutation divergence, and `drivers` as the first canonical
difference before writing the environment and evidence hashes. Never create or
change `container-exit-status.txt` by hand; without a captured successful
Docker exit, the run is incomplete and must not be promoted.

With full coverage enabled, the verifier:

- requires the finalized version-4 report, coverage note, clean source tree,
  exact source/build identity, matching binary, and local raw-sector retail
  image;
- plays every frame twice in separate processes and requires exit status 0;
- requires different logged host-address samples across those processes;
- requires different pointer-sensitive restored raw-checkpoint checksums
  across the processes and requires at least one to differ from the recorded
  payload;
- derives the first frame where `driver[0]` becomes active in an actual race;
- flips bit 0 of the real `driver[0].posCurr.x` field at that exact frame;
- requires exit status 2 and `drivers` as the first canonical difference;
- hashes the disc identity, replay, checkpoint, deterministic manifests for
  the seed and recording memory-card directories, metadata, environment, all
  three playback logs, and the required coverage note. Determinism-only mode
  produces the same manifest without `coverage.txt`.

Successful verification writes:

```text
playback-1.log
playback-2.log
playback-mutated.log
mutation-frame.txt
disc.sha256
memcard-seed.sha256
memcard-recording.sha256
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
