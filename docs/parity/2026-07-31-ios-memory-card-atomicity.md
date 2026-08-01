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

A clean frame-zero iOS replay-seeded recording subsequently reached the
accepted input stream's real game-created save event, preserved the exact file
through background/foreground, and finalized all 24,232 frames normally. A
later exact touch build read the same bytes from the production default root
after installation and displayed saved profile `A`. The run and the explicit
copy used to isolate the cold-reader test are recorded below; no report-root
to default-root transfer is inferred.

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

> **Follow-up:** a later delayed-start packet recording proved that Computer
> Use key events did reach SDL but were assigned to player two after Simulator
> controller enumeration. Commit `e6ba535a9c73` corrected iOS ownership, and an
> exact keyboard-only run reached a Time Trial grid. See
> `2026-07-31-ios-hardware-keyboard.md`. The bounded negative route below is
> retained as the historical evidence available at this checkpoint.

The requested basic keyboard controls were already published in commit
`2c10b00b34df`: `WASD`, `IJKL`, `Q/E`, `P`, and Tab are additive aliases for
D-pad, face buttons, shoulders, Start, and Select. They use the same active-low
PS1 pad transport as the original arrow/`Z/X/C/V` map and gamepads. The input
self-test at this checkpoint covered all aliases, one-snapshot taps, and the
simultaneous `K+D+E` chord; a signed macOS run had already proven menu
navigation, acceleration, and steering. Follow-up touch work in
`c783eda740c4` extended keyboard and touch edge retention to two host snapshots
after live iOS tracing showed that one snapshot could be overwritten before a
retail poll. The later exact matrix and supersession are recorded in
`2026-07-31-ios-touch-controls.md`.

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

At 13:54 CDT the live screen had reached the textured Roo's Tubes selection.
The local-only 743-by-1018 JPEG is 144,651 bytes with SHA-256
`4beff345e9861db4c1c4f3a600b5767f5d3c18a842be06e4e9afa4938b1d41e3`.
It visibly shows coherent menu text, track art and textures; it is not a
committed retail artifact.

The game created
`memcard.recording/slot0/BASCUS-94426-SLOTS` at 14:22:16 CDT, after checkpoint
74/frame 22,200 and before checkpoint 75/frame 22,500. The file is 6,016 bytes,
contains a 256-byte icon followed by a 5,760-byte profile, reports profile
version `-18`, profile size `0x1600`, one block, and has CRC remainder zero.
Its SHA-256 is
`6a01b0f5562ed7a279d8f8e51e3b1874ac39a6120f55db4fe3873288950619a3`.
No `.ctrpad-*.tmp` file remained beside it.

After that save, sending the app Home left PID `36490` alive at approximately
3.3 percent CPU. The save's inode, size, modification time and hash remained
unchanged. The log recorded `will-enter-background` and
`did-enter-background` with audio suspended. Foregrounding the app recorded
the foreground transition, then active audio, and returned to a coherent
kart/terrain/particle/HUD/minimap frame. That 743-by-1018 local-only JPEG is
124,957 bytes with SHA-256
`ce9c3a0a8d5f0421434ee000c60c8940ff41daa2041b57c021b94dd48419a901`.

The recording finalized naturally at 14:27:21 CDT with `frame_count=24232`,
`checkpoint_count=81`, `finalized=1`, `recording_status=finalized`, and build
identity `4b078065ff03`. The final log states
`replay-seeded recording finished after 24232 frames` before its closed-log
marker. Artifact SHA-256 values are:

```text
memory card     6a01b0f5562ed7a279d8f8e51e3b1874ac39a6120f55db4fe3873288950619a3
input replay    a471aa692c39a6d62813cd9d41acec9a3fb6e4893ac29ecb50ce6164112fb411
states          e52598538ef0e8b490560dd38024279289c46ec13e7c7021882e32af45e92965
metadata        90421662a92a44668c268a17fe75930b2a7caf63b12a703852e58c5db4211e91
runtime log     02b5f482d548a1c870e653cdb66b0211c260ae3cff4c2f18184d95e75cf923ed
```

UIKit intentionally kept the now-idle process alive after the report closed;
it was terminated through `simctl` only after the finalized files were
inspected. The accepted save was then copied byte-for-byte from the isolated
recording root into the default private memory-card root. This deliberate copy
kept the 6,016-byte size and SHA-256 but changed the inode. It separates the
reader test from report isolation: it proves that production default-path
lookup reads and retains these game-created bytes, not that a report sandbox
automatically migrates a save.

Later exact touch commit `c783eda740c4` was installed as an app update. The
Simulator migrated the data container while preserving the imported BIN and
default save hashes. A touch-only Adventure -> Load route displayed
`CHOOSE A GAME TO LOAD`, saved slot `A`, Crash's icon and populated counters;
the remaining slots were `EMPTY`. This is direct production cold-reader and
app-update retention evidence on Simulator. The local-only load-screen JPEG
is 121,963 bytes with SHA-256
`a1c5135dee001ead4b68d39d619157a45632c610df0b7ae20874efd4c01f700b`.

## Evidence boundary

This checkpoint accepts the production atomic replacement contract, its
failure-preservation oracle, clean cross-target build coverage, an exact
Simulator game-created save, background/foreground preservation, normal
recording finalization, app-update retention, and a later production cold read
of the accepted bytes. It does not accept a physical device,
development/distribution signing, physical-flash failure semantics, a physical
keyboard, a natural non-report save followed by cold relaunch, or automatic
transfer from an isolated report memory-card root to the default root.
