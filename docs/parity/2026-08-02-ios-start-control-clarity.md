# iOS Start-control clarity checkpoint — 2026-08-02

## Outcome

The touch overlay always emitted the retail `START` input, but the visible
button title was only `PAUSE`. That label was accurate during a race and
misleading at copyright/title/menu screens, where the same PlayStation control
starts or advances the game. The top-center control now visibly reads
`START / PAUSE` and exposes the accessibility label `Start or pause` while
retaining the existing `ctrpad.touch.start` identifier and
`PLATFORM_INPUT_TOUCH_START` mapping.

A one-Simulator Computer Use check found the two-line label visible and
unclipped. A real on-screen tap advanced the running retail game from its
copyright sequence to the textured CTR main menu. This is a UX-label fix, not
a new input implementation or physics/timing change.

## Root cause and change

`platform/apple/native_ios_touch.m` already constructed this button with mask
`PLATFORM_INPUT_TOUCH_START` and identifier `ctrpad.touch.start`; only its title
was wrong for pre-race use:

```text
before visible title: PAUSE
after visible title:  START\nPAUSE
accessibility label:  Start or pause
retail input mask:     PLATFORM_INPUT_TOUCH_START (unchanged)
```

The change is exact local implementation commit
`df1b803726b4375389476d6abf273b55b9580f98` (`Clarify touch Start control`).

## Timestamped validation and resource stop

All times are CDT on 2026-08-02.

| Time | Action and result |
| --- | --- |
| 02:40-02:43 | Investigated the user report. Source inspection proved the Start input existed but was labeled only `PAUSE`; this was accepted as a touch-first labeling defect rather than blamed on game state. |
| 02:43 | Changed the visible title to two lines, `START` / `PAUSE`, and added `Start or pause` as the explicit accessibility label. |
| 02:43-02:46 | Configured and incrementally built the dirty development Simulator app with one low-priority job. `native_ios_touch.m` compiled, the app linked, and the target retained the established 32 warnings and zero errors. |
| 02:47-02:50 | Booted only Simulator `1D19A61F-20B7-46B0-AB52-B3A3406952E2`. Guarded update-install verified the isolated ad-hoc staged/installed executable identity and preserved the exact retail image and slot-zero save tuples. |
| 02:50-02:52 | Computer Use visibly observed the unclipped top-center `START / PAUSE` button. The accessibility tree exposed `Description: Start or pause`, `ID: ctrpad.touch.start`, `Value: Released`. A screen-coordinate tap on that rendered button advanced the game from copyright presentation to the textured CTR main menu. |
| 02:52:24 | Captured a local-only 2,064×2,752 Simulator screenshot with the visible label; SHA-256 `e23782415875ae5c604c6e60b52a2670cbf1d0f87b503c7d26b4f3f505c07a66`. It contains retail presentation and is deliberately not committed. |
| 02:52:31 | Committed the two-line label/accessibility correction alone as `df1b803726b4`. The worktree was clean after commit. |
| 02:53 | The first exact iPhoneOS configure hit the intended stale-cache identity guard because the directory was pinned to `cb459a...`. Clearing only cached `CTR_NATIVE_SOURCE_COMMIT` allowed clean reconfiguration against `df1b803726b4`; no build directory or evidence was deleted. |
| 02:53-02:55:04 | The exact-commit iPhoneOS incremental build compiled eight affected steps and linked a thin ARM64 app with 32 established warnings and zero errors. `Info.plist` contains full source `df1b803726b4375389476d6abf273b55b9580f98`. |
| 02:55-03:03 | Began the corresponding exact Simulator rebuild at one low-priority job. With the software-rendered game and heavy host contention, load peaked at `188.43 181.14 128.81`; the unity compile received only a few percent CPU for part of the run. |
| approximately 03:03 | The user explicitly requested that all operations stop because the game was sluggish and bogging down the machine. No push, merge, package or documentation operation followed in that turn. The already-running build had reached its final link and wrote the exact-commit Simulator executable at 03:04:14 before cancellation/stop inspection completed. |
| 03:04 | Terminated the exact build pattern if present, terminated CTRPad if present and shut down the named Simulator. Final audit showed zero booted Simulators, no project build/runtime process, clean worktree, branch one local commit ahead of remote `main`, and no push. |

The final build completion is retained as compile evidence, but it was not
reinstalled after the clean commit because the user stop took precedence. The
earlier live UI proof ran code with the identical two source edits from a dirty
development build; it is not mislabeled as an exact-clean runtime result.

## Persistence evidence

The guarded update preserved both user-owned inputs exactly:

```text
retail image
inode   111131200
size    605698800
sha256  f780bf2331476aabfc00772fa758b12dd95ebfbc907968132cbd3cdd4e2c07c0

slot-zero save
inode   111222179
size    6016
sha256  6a01b0f5562ed7a279d8f8e51e3b1874ac39a6120f55db4fe3873288950619a3
```

The staged and installed ad-hoc Simulator executable hashes both resolved to
`12fdde832eba4b03218acc4bf55a3c8989e6d8a4b2dedac593874574f4254e66`.
That signature is Simulator-only and is not device-signing evidence.

## Claim boundary

The evidence proves the Start control is discoverable, accessible and sends
the existing production touch input far enough to advance the real game UI.
It does not prove physical touch feel or signed device execution. At the final
stop there was no booted Simulator and no active operation; publication of the
local commit and this record remained a later low-load task.
