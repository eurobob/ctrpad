# iOS Memory-Card Atomicity and Persistence — 2026-07-31

## Result

Commit `4b078065ff03b71e02ce7b8a5b03351a777e2830` changes the
production memory-card writer from direct truncation to a same-directory
temporary-file transaction. The temporary file is completely written,
flushed to the operating system, closed, and atomically renamed over the old
save. Any write, flush, close, or replacement failure removes the temporary
file and leaves the previous save untouched.

The implementation and its failure-preservation oracle pass the clean macOS
ARM64, combined ASan/UBSan ARM64, direct optimized i686-source, iOS Simulator
ARM64, and iOS device ARM64 build boundaries described below. The exact
committed Simulator app also launches the user's already imported retail image
and visibly renders coherent game textures.

At the time this report was opened, a clean frame-zero iOS replay-seeded
recording was still running toward the accepted input stream's real
game-created save event. That run, its background/relaunch follow-up, and the
final elapsed-time reading are recorded later in this report. Until those
results are present, atomic writer implementation is accepted but full M9
save persistence is not.

## Starting boundary

The prior iOS storage and Files-import checkpoints established these owners:

```text
application bundle                    immutable fallback assets
Documents/CTRPad/assets               user-imported retail media
Application Support/.../CTRPad        private logs, reports and memory cards
```

The already accepted macOS report had created a checksum-valid 6,016-byte
`BASCUS-94426-SLOTS` profile through gameplay and read all 5,760 payload bytes
in a second process. On iOS, however, the writer still opened the final path
with `"wb"`. A crash or suspension after truncation and before a complete
write could therefore destroy an existing save. Simulator import/relaunch did
not close that failure mode and was not promoted to save persistence evidence.

## Production transaction

`NativeMemcard_WriteSaveData` now validates all sizes and required buffers,
then derives a hidden temporary path next to the requested final path:

```text
<save directory>/.ctrpad-<save filename>.tmp
```

The same directory is intentional: the final rename remains on one filesystem.
The writer performs the following sequence:

1. open the temporary file without changing the final save;
2. write the icon and profile payload, including valid zero-length sections;
3. call `fflush`;
4. call Apple `F_FULLFSYNC`, falling back to POSIX `fsync` when unavailable or
   unsuccessful; Windows uses `_commit`;
5. close the file and treat close failure as transaction failure; and
6. replace the final path with POSIX `rename` or Windows
   `MoveFileExA(MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)`.

Every failed path removes the temporary file. The final path is never opened
for truncation before the replacement boundary. The implementation makes a
single writer transaction crash-safe with respect to the old-or-new file
choice; it does not claim filesystem behavior after physical-media failure or
a directory-metadata flush that the platform APIs do not provide here.

## Failure oracle and correction history

CTest `ctr_native_memcard_atomic_write` exercises the production writer with a
temporary isolated root. It requires:

- initial write and payload readback;
- replacement with different bytes and readback;
- absence of a leaked temporary file;
- an injected temporary-path open failure made by placing a directory at the
  temporary filename;
- byte-for-byte preservation of the earlier final save after that failure;
- successful retry after removing the fixture; and
- final root cleanup.

Its exact success marker is:

```text
[CTR Memcard] atomic-write self-test passed: write=flushed replace=atomic failure=preserves-existing temp=clean retry=checked
```

Two rejected compile routes are part of the history:

1. An initial draft attempted to use SDL temporary-file/process APIs that do
   not exist in the vendored SDL 3.4.10 surface. The implementation was
   replaced with the platform C/POSIX/Win32 primitives above before commit.
2. The first strict i686 translation-unit compile exposed an implicit
   `fileno` declaration because strict C17 libc hides that POSIX prototype.
   The source now carries the explicit non-Windows declaration. Recompiling
   the exact i686 flags with `-Werror=implicit-function-declaration` passed.

Neither rejected draft was committed or used as evidence.

## Exact clean validation matrix

The clean committed source produced these results:

```text
macOS ARM64 CTest       21/21 in 2.00 s
ARM64 ASan + UBSan      21/21 in 10.63 s; fail-fast; no finding
i686 source compile     clean with the exact optimized i686 flags and
                        -Werror=implicit-function-declaration
iOS Simulator ARM64     linked; 32 established warnings
iOS device ARM64        linked; 32 established warnings
```

The ordinary macOS build repeated 32 established warnings and the sanitizer
build repeated 59. No new warning was assigned to the memory-card change.
No Windows cross-compiler was installed locally, so the Win32 branch received
source/API audit rather than an executed binary claim.

The four-file source audit reported 207 insertions and 6 deletions in
`CMakeLists.txt`, `include/platform/native_memcard.h`, `main.c`, and
`platform/native_memcard.c`. The implementation was committed as
`4b078065ff03` with message `fix: write memory cards atomically` and pushed to
`origin/codex/arm64-apple`. Local HEAD and the remote-tracking branch matched
immediately afterward. The user's retail media, save files, reports, app
packages, screenshots, and crash reports remained ignored/local-only.

## Exact Simulator product and visible game

The exact Simulator product embedded source identity `4b078065ff03`. Its
pre-sign executable SHA-256 was
`e7e3faac7e0e719a0e4e2dbf823ca63212a2b41497fe048c84b5ac2c8655c83f`.
A disposable copy at
`/private/tmp/ctrpad-ios-atomic.CR1jWC/CTRPad.app` was ad-hoc signed and passed
strict/deep verification. The signed executable SHA-256 was
`848c18d5692634413b47b553f5fb2e9887dba12ce8bf8e1474b7ca87684568ac`.
It was a thin ARM64 Simulator Mach-O and the installed executable matched.

The retained iPad Pro 13-inch (M5) iOS 26.5 Simulator kept the previously
imported 605,698,800-byte NTSC-U file across exact-app installation and data-
container migration. Visible inspection showed the Naughty Dog crate,
checkered flag, trophy/Crash title and menu, followed by changing attract-mode
frames. Geometry, text, colors and textures were coherent. The Apple Software
Renderer remained approximately 5–9 FPS in those windows; this is not physical
iPad performance evidence.

## Hardware-keyboard delivery audit

The requested basic keyboard controls were already published in commit
`2c10b00b34df`: `WASD`, `IJKL`, `Q/E`, `P`, and Tab are additive aliases for
D-pad, face buttons, shoulders, Start, and Select. They use the same active-low
PS1 pad transport as the original arrow/`Z/X/C/V` map and gamepads. The input
self-test covers all aliases, one-snapshot taps, and the simultaneous `K+D+E`
chord; a signed macOS run had already proven menu navigation, acceleration,
and steering.

For iOS Simulator, Computer Use enabled **Capture Keyboard** and
**Connect Hardware Keyboard**. UIKit/SDL logs detected a `Generic Keyboard`,
and `UIApplicationSupportsIndirectInputEvents` was already true. Repeated
`S`, `K`, `P`, `C`, arrow, and Return attempts sent to both the Simulator name
and its exact active application path produced no game response. A temporary
iOS-only diagnostic at `Platform_PollHostEvents` then observed zero SDL key
events for the same attempts. That line was removed immediately, the exact
clean app was restored, and `git status` returned clean.

The bounded conclusion is that this Computer Use host-key injection route does
not reach SDL in the Simulator environment. It neither rejects the mapping nor
accepts a physical iPad keyboard. macOS keyboard controls remain automated and
live-covered; physical iPad keyboard acceptance remains open.

## Rejected checkpoint shortcut

To avoid waiting for approximately 22,392 software-rendered frames, a
diagnostic-only attempt copied the accepted macOS replay and checkpoint files
into private Simulator storage and launched with checkpoint 74 (frame 22,200)
plus `--replay-bypass-header`. The log explicitly reported an identity
mismatch between producer `eee2a8df5b96` and live source `4b078065ff03`, then
reported a different restored-process raw checksum. This already placed the
route outside acceptance: `docs/REPLAYS.md` states that bypassing identity does
not make checkpoints portable across rebuilt binaries.

The app subsequently received `SIGSEGV` in
`VehBirth_SetStartlinePosition +172` before any memory-card write. The faulting
instruction read `level->DriverSpawn[spawnIndex]` through low address
`0x23be8dfc`, consistent with an address-bearing field from the incompatible
checkpoint. The local crash report is:

```text
/Users/chrissotraidis/Library/Logs/DiagnosticReports/
  CTRPad-2026-07-31-131048.ips
incident 68888543-7A98-49CE-9588-D153506B11B6
SHA-256 929abb4dfaac6bee74989bff92d37c5bbcb7d24429f808bb4ca7a9db8594fd7c
```

No playback memory-card file existed after the crash. It is therefore recorded
as a rejected incompatible-checkpoint shortcut, not as an atomic-write defect,
save attempt, parity result, or ordinary iOS crash. The crash report stays
outside Git.

## Clean game-driven persistence run

The replacement route starts the exact app from frame zero with
`--record-from-replay`, which consumes only the accepted 24,232-frame pad
stream and creates fresh checkpoints and a fresh isolated recording memory
card in the current iOS process. It restores no old address-bearing state.

The run started at 2026-07-31 13:14:59 CDT as PID `36490` and created private
report `debug/reports/20260731/ctr-131503`. Startup selected the retained
Documents retail image, initialized the 1376-by-1032 UIKit GLES surface and
44.1-kHz stereo audio, created empty `memcard.seed` and
`memcard.recording` directories, and began new rolling checkpoints at 300-frame
intervals. Early checkpoints 0 through 3 (frames 0 through 900) were written
normally while the app remained alive.

Final frame, save, temporary-file, suspend/resume, cold-read, artifact-hash,
and process-duration results will be appended here before this report is used
to change M9 acceptance.

## Evidence boundary

This checkpoint accepts the production atomic replacement contract, its
failure-preservation oracle, clean cross-target build coverage, and exact
Simulator startup/visual evidence. It does not yet accept a physical device,
development/distribution signing, physical-flash failure semantics, a physical
keyboard, or iOS game-save persistence until the clean game-driven run and
lifecycle/relaunch checks above complete.
