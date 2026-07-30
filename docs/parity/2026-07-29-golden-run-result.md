# 2026-07-29 NTSC-U Golden Run Result

**Source and binary:** `868a4e308a1c80222948938efe6b8f9ab86aee73`

**Local report:** `build-linux-i686-baseline/debug/reports/20260729/ctr-225420`

**Status:** gameplay coverage recorded; strict verification exposed a
host-input omission and is not yet accepted

## Recorded evidence

The user-supplied NTSC-U BIN is a single MODE2/2352 track with boot ID
`SCUS_944.26`. Its SHA-256 is:

```text
f780bf2331476aabfc00772fa758b12dd95ebfbc907968132cbd3cdd4e2c07c0
```

The finalized version-2 report contains 24,232 frame records and 81 rolling
checkpoints at 300-frame intervals. `coverage.txt` marks all eight required
behaviors as observed:

- startup and title flow;
- menu and race loading;
- acceleration and both steering directions;
- a successful retail powerslide boost;
- item acquisition and use;
- lap advancement;
- a save-producing action; and
- observation of the persisted result after returning to a menu.

The recording log contains the retail successful-boost branch and finishes
normally:

```text
[CTR Gameplay] player powerslide boost: reserves=384 success=1
[CTR Replay] report finalized by hotkey: frames=24232
```

The first active race-driver frame is 1711. That remains the intended
known-mutation frame for the final verifier.

## Strict playback failure

The first unchanged verifier process matched timing, RNG, drivers, world,
pad checksum, and VBlank packets through frame 22,391. At frame 22,392 only
the allocation digest differed:

```text
expected allocation=b22103f8b76271f5
live     allocation=15328911282b5717
first canonical state difference: allocation
```

The live allocation digest at frame 22,392 equals the recorded digest from
frame 22,391. The missing allocation transition is the profile/save object
batch created after confirming the native name-entry screen.

The root cause is an incomplete input boundary, not game-state drift.
Keyboard events call `SubmitName_UseKeyboard` from the SDL event path
(`platform/native_platform.c:170-179`). The name-entry screen uses that
host-only scancode to synthesize retail Circle/save input
(`game/SubmitName.c:216-285`). Version-2 replay frames recorded the mapped
PS1 pad snapshot but not the originating SDL scancode, so playback reproduced
raw Start while the native Enter shortcut remained absent.

The repaired boundary stores the native name-entry scancode in the three
previously reserved bytes of each fixed-size pad snapshot
(`platform/native_input.c:994-1069`). New reports therefore preserve the
shortcut without changing the replay frame size. Legacy reports can recover
Enter from raw Start because this recording was keyboard-driven; other
legacy name-entry scancodes are explicitly not recoverable.

## Fresh replay-seeded recording and checkpoint limitation

An attempted state-preserving migration matched the source through frame 366
and then failed closed at frame 367. Two VBlanks emitted during asynchronous
loading occurred between tracked frames, so they were absent from the source
frame's VBlank packet list. Timing and driver digests consequently differed.
This is a second precise limitation of the version-2 harness: its VBlank packet
stream covers `BeginFrame` through `EndFrame`, not loading work between those
boundaries.

`--record-from-replay` is therefore deliberately narrower. It starts from a
fresh game initialization, clones the source report's memcard seed, and uses
each validated old pad snapshot as an input script after adding the missing
shortcut metadata. It records the new run's own timing and fresh checkpoint;
it does not claim canonical equivalence to the source report. The resulting
coverage must be observed again, and only normal playback of that new report
can become M1 evidence.

Rebuilding a binary and bypassing the replay identity gate is not a valid
alternative. Checkpoint image pointers are currently relocated by image-base
offset (`platform/native_checkpoint.c:434-455`), while callback fields such as
`Thread.funcThTick` are identified only as image pointers
(`platform/native_checkpoint.c:777-795`). Unity compilation can change
function offsets after a small source edit. In one diagnostic build, captured
offset `0xf880` named `Particle_FuncPtr_SpitTire` in the recording binary but
landed inside `MATH_Matrix_TrigSinCos` in the rebuilt binary. This proves that
`--replay-bypass-header` is diagnostic-only across different code layouts.

Portable symbolic callback identities remain required before an i686
checkpoint can bootstrap an ARM64 parity run. This is tracked under M5; it is
not being hidden by widening the current image-offset rule.

## Acceptance remaining

M1 remains in progress until the fresh replay-seeded report:

1. completes all 24,232 input frames and re-observes the required coverage;
2. replays unchanged twice from separate address layouts;
3. fails at a selected active-driver frame after the deliberate position
   mutation; and
4. has its final hashes and environment manifest recorded here.

## 2026-07-30 prefix follow-up

Replay version 4 now captures the missing native-input and complete inter-frame
VSync boundaries. A fresh 2,200-frame startup-to-race prefix matches timing,
RNG, drivers, world, allocation, and root across ARM64 and i686, and the same
prefix completes under combined ASan/UBSan.

That result and the complete defect/fix evidence are recorded in
`2026-07-30-arm64-prefix-parity.md`. It validates the corrected harness and
measured prefix, but it does not satisfy the four full-run acceptance items
above; the original 24,232-frame version-2 report remains historical evidence,
not an accepted parity baseline.
