# iOS bounded accessible race-control checkpoint

**Dates:** 2026-08-01 to 2026-08-02 CDT

**Accepted implementation:** `c56162f68a74ebc3f381a6d77d5a00b064a6c8b2`

**Branch:** `codex/simulator-performance-next`

**Status:** locally accepted; publication and every signed physical-iPad gate
remain open at this checkpoint

## Question and result

The prior sustained-control checkpoint proved that Gas, analog steering and a
drift button could remain independently held. Its first extended Crash Cove
attempt exposed a different automation hazard: a Computer Use screen read can
take 15 to 25 seconds while a held action continues in the game. A test that
intended to observe the next corner therefore drove into a wall or the ocean
before the observation returned. That behavior was a harness/control-duration
problem, not missing rendering; every inspected retail frame was coherent.

The accepted change adds one- and three-second self-releasing actions to every
retail button, 450-ms full and 45%-deflection steering nudges, and generation
guards that keep an older delayed release from cancelling a newer action. The
existing short activation and explicit Hold/Release/Center actions remain.
All paths still emit through the production UIKit targets and retail touch
publisher (`platform/apple/native_ios_touch.m:141-204`,
`platform/apple/native_ios_touch.m:222-341`, and
`platform/apple/native_ios_touch.m:624-648`).

The new actions were exercised in the sole Simulator. Log down/up pairs prove
one-second and three-second Gas durations and prove every down edge reached the
retail poll. A clean retry visibly traversed the opening tunnel, shoreline,
central rock, inland beach, shipwreck, bridge, and final climb of Crash Cove.
The kart then reached a climb wall. The attempt is rejected as a complete lap:
the HUD remained `LAP 1/3`, no finish-line transition was observed, and no
three-lap or drift-chain claim is made.

This checkpoint accepts bounded, self-neutralizing accessible race input and a
broad visible track traversal. It does not accept complete-race ergonomics or
replace physical multi-touch testing.

## Source design

### Bounded buttons

`accessibilityPressForDuration:value:` first releases any prior accessibility
state, starts a new generation, sends the normal `UIControlEventTouchDown`, and
schedules the matching up event on the main queue. The delayed block releases
only if its generation is still current and no explicit hold has superseded it
(`platform/apple/native_ios_touch.m:141-157`). Normal activation remains 100
ms; the new custom actions request exactly one or three seconds
(`platform/apple/native_ios_touch.m:160-177`). Hold, Release and control-state
reset advance the generation so an obsolete timer cannot release newer state
(`platform/apple/native_ios_touch.m:179-204` and
`platform/apple/native_ios_touch.m:688-706`).

### Steering nudges

Full and 45% left/right nudges share `accessibilityNudgeX:y:value:`. It uses the
same analog/D-pad publication path as held and physical steering, then centers
after 450 ms only if no newer accessibility action or physical touch has taken
ownership (`platform/apple/native_ios_touch.m:261-325`). `releaseTouch`
advances the steering generation before neutralizing analog and any outer-ring
D-pad direction (`platform/apple/native_ios_touch.m:426-439`).

The generation rule matters because assistive actions and physical fingers can
overlap. A stale nudge must not center a later hold or a later finger contact.

## Timestamped chronology

Wall times are CDT. Runtime logs store UTC; the times below are their exact
CDT conversion. Intervals are used where only command order, not a retained
minute, is authoritative.

| Time | Event | Result |
| --- | --- | --- |
| 2026-08-01 21:47:46 | Commit `7b491d411` recorded the sustained-control publication. | Closed the prior publication record. |
| 21:48:31 | PR #21 merged that record to `main` as `3f7b16caa`. | Local branch, remote branch and `main` aligned before this follow-on. |
| 21:49:38 | The sole Simulator opened clean build `a3523c7a8583`. | Began extended touch-only race review without booting another Simulator. |
| 21:49-22:39 | Used held Gas/full/slight steering through menus and Crash Cove. | The kart moved substantially, but screen-read latency let held controls continue for tens of seconds; wall, wrong-way and ocean states were rejected. No complete lap. |
| 22:39:49 | First dirty bounded-control session opened as `3f7b16caa95e-dirty`. | Added steering nudges and generation ownership; dirty identity kept the session diagnostic-only. |
| 22:42:03-22:43:00 | Held/released Down through the retail poll. | Confirmed full-direction menu state still functioned. |
| 22:43-22:59 | Live-tested bounded/nudge behavior and rebuilt serially. | Nudge actions returned the stick to `Centered`; one-second button action returned Gas to `Released`. |
| 22:59:33 | Second dirty session opened. | Continued the serial bounded-action iteration. |
| 23:00:46-23:00:47 | One-second Gas down/reached retail poll/up. | Exact one-second self-release proof. |
| 23:14:17 | Final source edit timestamp. | Added three-second button actions and the final action-generation guard. |
| 23:15:05 | Final dirty validation session opened. | Build remained explicitly `3f7b16caa95e-dirty`. |
| 23:16:02-23:16:05 | Gas `0x4000` down/reached retail poll/up. | First exact three-second self-release proof. |
| 23:18-23:29 | Exercised short, one-second and three-second actions, pause/restart and menu navigation. | Cleanly restarted Crash Cove after rejecting the first wall/wrong-way attempt. |
| 23:33:34 | Read the active goal clock before continuing the race. | `timeUsedSeconds=288621`: 3 days, 8 hours, 10 minutes, 21 seconds. |
| 23:34:09-23:57:39 | Drove the bounded Crash Cove retry. | Visible coherent route covered tunnel, shoreline, rock, beach, shipwreck, bridge and climb. One-/three-second Gas pairs reached the retail poll and released. The final wall state remained lap 1/3, so the run was rejected as a lap completion. |
| 23:58:11 | Last bounded Pause edge in the dirty session. | The accessibility tree intermittently exposed only the Simulator menu bar while the game rendered; this was recorded as automation availability, not a graphics failure. |
| 2026-08-02 00:00:58 | Committed implementation `c56162f68`. | Established a clean source identity: 78 insertions and eight deletions in one file. |
| 00:05:34 | Exact one-job iPhoneSimulator ARM64 product linked. | Source executable SHA-256 `ef5b2de3...ece6d`; exact plist source/build `c56162f68a74...` / `c56162f68a74`; 32 established warnings, zero errors. |
| 00:07:02 | Guarded exact update launched the clean app on the sole Simulator. | Installed signed executable SHA-256 `28500b12...2ea2`; retail and slot-zero save tuples remained unchanged. |
| 00:07-00:20 | Clean app ran through copyright/title/menu/demo assets. | Computer Use visibly observed a coherent textured Crash Cove demo and complete overlay; current targeted log scan returned zero rows. |
| 00:12:04 | Exact one-job iPhoneOS ARM64 product linked. | Executable SHA-256 `d6c81c8b...3290`; same full clean source metadata; unsigned device-target compile accepted, signed execution still open. |
| 00:16:28 | Exact macOS ARM64 product linked. | Same implementation identity; 32 established warnings and no errors. |
| 00:17-00:18 | First CTest invocation continued after the command wrapper stopped returning output; a status probe then accidentally began a duplicate. | The duplicate reached 23/25 and was interrupted immediately. Its partial output was rejected. No build or Simulator was duplicated. |
| 00:18-00:19 | Final serialized CTest repeat. | 25/25 passed, zero failed, in 74.85 seconds. |
| 00:19:52 | Captured the exact clean Simulator visual checkpoint. | Local ignored JPEG (101,446 bytes) SHA-256 `17cf0ada...403a`; coherent demo textures and controls; retail-derived image is not committed. |
| 00:20:53 | Read the active goal clock after validation. | `timeUsedSeconds=291459`: 3 days, 8 hours, 57 minutes, 39 seconds. Goal remains active. |
| 00:28:34 | Completed the seven-document history audit before committing it. | `timeUsedSeconds=291910`: 3 days, 9 hours, 5 minutes, 10 seconds. Publication remained next. |

The raw one-second proof begins at `2026-08-02T04:00:46.605Z`; its local
conversion is 2026-08-01 23:00:46 CDT.

## Exact build, persistence and log evidence

```text
SOURCE_COMMIT=c56162f68a74ebc3f381a6d77d5a00b064a6c8b2
SIMULATOR_SOURCE_EXECUTABLE_SHA256=ef5b2de31278c37471a8f48cf41a2b8dc87e683d1a39b9786695e256c63ece6d
SIMULATOR_INSTALLED_EXECUTABLE_SHA256=28500b1245233a0e506e99a7ad7ce56f7b456eae7ed12d500ffbfcf3313f2ea2
DEVICE_EXECUTABLE_SHA256=d6c81c8b1144013344df5dfb100b0c00958f5d1f56f180c9a3f12b527a353290
SIMULATOR_UDID=1D19A61F-20B7-46B0-AB52-B3A3406952E2
SIMULATOR_NAME=CTRPad Import Validation
BOOTED_SIMULATOR_COUNT=1
```

Update persistence remained byte- and inode-identical:

```text
retail=111131200|605698800|f780bf2331476aabfc00772fa758b12dd95ebfbc907968132cbd3cdd4e2c07c0
save=111222179|6016|6a01b0f5562ed7a279d8f8e51e3b1874ac39a6120f55db4fe3873288950619a3
```

The clean log opened at 00:07:02 CDT, identified build `c56162f68a74`, enabled
GLES 3 framebuffer fetch and the UIKit touch/display loop, and continued for
more than 13 minutes. The targeted case-insensitive scan for `[ERROR]`,
`[FATAL]`, asset missing/invalid/failure, visibility failure, shader failure,
abort and assertion signatures returned zero.

## Accepted and open boundary

Accepted locally:

- bounded one- and three-second production touch-button state;
- auto-centering full/slight steering nudges with stale-timer protection;
- retail-poll consumption and exact self-release timing;
- broad visible Crash Cove traversal with coherent assets;
- clean exact iPhoneSimulator, iPhoneOS and macOS ARM64 builds;
- 25/25 final serialized tests;
- exact update preservation and one-Simulator discipline; and
- clean current-session targeted logging.

Still open:

- a completed three-lap touch-only race;
- repeatable three-boost drift chains and human ergonomic acceptance;
- Apple-issued signing identity/profile and exact signed IPA;
- connected physical-iPad install/launch;
- physical cadence, frame pacing, audio latency, thermals and energy; and
- physical lifecycle plus update/save persistence.

The goal therefore remains active.
