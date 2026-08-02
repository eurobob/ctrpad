# iOS sustained accessible-control acceptance

**Date:** 2026-08-01 CDT

**Accepted implementation:** `a3523c7a858379a030934932fb335e6ed502457e`

**Branch:** `codex/simulator-performance-next`

**Status:** locally accepted, documented and published to `main` as
`be9c81605133d2329041f36ac7ca77b6f9347ead`; signed physical-iPad acceptance
remains open

## Question and result

The Simulator control surface could expose a normal accessibility activation,
but UIKit translated that activation into only a 100-ms tap. That was enough
for menu selection and deliberately buffered retail input edges. It was not
enough to hold Gas, sustain analog steering, or combine steering with a drift
button. Desktop Computer Use also does not synthesize genuine iOS multi-touch.
Consequently, a visibly running race could not by itself establish that the
current control state was independently holdable and releasable.

The accepted change adds explicit `Hold` and `Release` accessibility actions to
every retail input button, full and slight held steering actions to the analog
stick, visible `Held`/`Released`/`Centered` state, and one neutralization path
for settings, control rebuilding and disc reselection. On the exact clean
Simulator build, Gas, slight-right steering and L drift were simultaneously
reported held. Opening Controls and returning released both buttons and
centered the stick. The current log proves Gas and L drift reached the retail
poll. The fully textured game remained visible and the targeted log scan found
zero graphics, fatal or error signatures.

This accepts sustained independent control state on the sole Simulator. It is
not a claim that Computer Use completed a three-lap race, that desktop
automation reproduced physical multi-touch, or that the controls have passed
ergonomic testing on an iPad.

## Source design

### Buttons

`CTRPadInputButton` retains separate held and pending-tap state. Normal
accessibility activation still emits the existing short down/up pair, while
`Hold` publishes a down edge once and `Release` publishes the matching up edge
only when an accessibility action is active
(`platform/apple/native_ios_touch.m:131-178`). Every retail button receives
the two custom actions and begins with value `Released`
(`platform/apple/native_ios_touch.m:557-580`). Physical and accessibility
routes continue through the same `buttonDown:`/`buttonUp:` methods and
`Platform_InputTouchButton` call
(`platform/apple/native_ios_touch.m:599-617`).

The UIKit 26.5 SDK declares `UIAccessibilityCustomAction.target` weak. The
button or stick owns its actions, but the actions do not retain their target;
this design therefore does not create an action/target retain cycle.

### Steering

The stick exposes `Hold left`, `Hold right`, `Hold slight left`, `Hold slight
right`, `Hold up`, `Hold down` and `Center`
(`platform/apple/native_ios_touch.m:184-203`). All actions use the production
analog publisher rather than a parallel test state
(`platform/apple/native_ios_touch.m:230-276`,
`platform/apple/native_ios_touch.m:296-331`). Full directions reach the
existing 68% outer-ring D-pad threshold for menus. Slight left/right use 45%
analog deflection, deliberately remaining below that threshold so steering can
be held without also moving a menu selection.

A physical finger beginning on the stick first releases an accessibility hold
and then becomes the active source (`platform/apple/native_ios_touch.m:334-348`).
Release centers the knob, emits any required D-pad-up edge and disables the
analog stick (`platform/apple/native_ios_touch.m:360-385`).

### Neutralization

`resetControlState` clears each local button latch/value/visual alpha, releases
the stick, and then calls the existing engine-wide touch reset
(`platform/apple/native_ios_touch.m:619-637`). Controls settings, disc
reselection and control reconstruction call that method before changing UI
ownership (`platform/apple/native_ios_touch.m:639-690`). The settings
controller also calls it on Done and disappearance
(`platform/apple/native_ios_touch.m:533-543`). This prevents a held action
from surviving modal presentation, control replacement or disc transition.

## Timestamped implementation and validation chronology

All timestamps below are Central Daylight Time. Exact log and Git timestamps
are called exact. Events known only from the retained command sequence are
reported as intervals rather than assigned an invented minute.

| Time | Event | Result |
| --- | --- | --- |
| 20:43:08 | PR #19 merged the prior exact-source publication record to `main` as `88e453999`. | Local branch, remote branch and `main` began this follow-on aligned. |
| 20:43-21:04 | Re-entered the sole iPad Simulator with real stick drags and normal Cross activation; navigated main menu -> Time Trial -> Crash -> Crash Cove -> No Ghost. | The touch route worked, but a normal accessibility activation still lasted only 100 ms and could not sustain racing controls. |
| 20:43-21:04 | Added held button and steering actions. | The first build orchestration returned before two nested one-job Ninja processes had exited. A repeat briefly created overlapping builders. PIDs `39048/39051/39061` and `39257/39263/39269` were explicitly terminated; the process table was rechecked empty before continuing with one low-priority job. No output from the overlapped attempt was accepted. |
| 21:04:25 | First dirty implementation session opened as build `88e453999a35-dirty`. | The dirty label was retained and the session was used only for iteration, never release evidence. |
| 21:06:27-21:07:00 | Held and released Down (`0x0040`). | Main-menu highlight moved from Adventure to Time Trial and neutralized on release. |
| 21:07:37-21:08:07 | Held and released Gas/Cross (`0x4000`). | Accessibility value changed `Released -> Held -> Released`; the retail poll consumed the down edge. |
| 21:08:43-21:10:35 | Used short Cross actions to navigate character, track and ghost selection. | Reached Crash Cove's Time Trial grid on the current dirty implementation. |
| 21:10:35 | Held Gas in the race. | Timer later showed 0:17.36 and the kart had moved; Gas remained held. This is movement evidence, not a completed-race claim. |
| 21:11:18-21:11:53 | Held and released full Left (`0x0080`) while Gas stayed held. | Accessibility values and visible kart direction established simultaneous acceleration and steering. |
| 21:12:43-21:13:16 | Held and released L drift (`0x0400`) while Gas stayed held. | Independent simultaneous Gas/drift state and both input edges were visible; the down edge reached the retail poll. |
| 21:13-21:15 | Reviewed full-direction steering. | Full deflection was too coarse for desktop stepwise validation because it also emitted menu D-pad state; added 45% slight-left/right actions below the 68% D-pad threshold. |
| 21:15:33 | Relaunched the dirty slight-steering build. | `Hold slight right` displaced the knob and reported `Held slight right` without moving the Adventure menu selection; `Center` returned `Centered`. |
| 21:17-21:18 | Added and rebuilt the unified local/global reset. | Settings, disc confirmation and control rebuild now share one idempotent neutralization route. |
| 21:18:01 | Final dirty iteration launched. | Holding Gas, opening Controls and returning left Gas `Released` and the stick `Centered`; retail and save hashes remained unchanged. |
| 21:24:14 | Committed `a3523c7a8` (`Add sustained accessible touch controls`). | Established a clean exact source identity before acceptance builds. |
| 21:26:20 | Clean iPhoneSimulator ARM64 link completed. | `Info.plist` contained full source `a3523c7a858379a030934932fb335e6ed502457e` and build `a3523c7a8583`; source executable SHA-256 was `089100e5...c2c4`. Build emitted the established 32 warnings and no errors. |
| 21:27:07 | Guarded exact update launched PID 44201 on the sole Simulator. | Isolated signed staged and installed executable hashes matched at `443d08a9...dabc`; retail and slot-zero save inode/size/hash tuples were unchanged. |
| 21:27-21:32 | Computer Use visually observed copyright, animated Crash/trophy/title screens, full mode menu and the complete touch overlay. | Gas, slight-right steering and L drift simultaneously reported `Held`; Controls then reset every button to `Released` and the stick to `Centered`. |
| 21:29:31 | Clean-build Gas down edge reached the log and retail poll. | Log mask was `0x4000`; no automation-only state bypassed the retail consumer. |
| 21:30:23 | Clean-build L drift down edge reached the log and retail poll while Gas remained held. | Log mask was `0x0400`; AX state independently retained slight-right steering. |
| 21:33:30 | Clean iPhoneOS ARM64 link completed. | Physical-device app compiled with exact clean source identity, the established 32 warnings and no errors. It remains unsigned. |
| 21:34-21:35 | Clean macOS ARM64 build and CTest ran at nice 15 with one job. | 25/25 tests passed in 27.84 seconds; zero tests failed. |
| 21:35:52 | Packaged the exact unsigned IPA. | Seven members; ARM64; source `a3523c7...`; SHA-256 `781f8c03...ebf0`; retail media excluded. |
| 21:36:19 | Packaged exact corresponding source. | 3,264 members; SHA-256 `62562ce9...8ae0`; forbidden retail/runtime/package/profile/key scan returned zero. |
| 21:36-21:37 | Independently audited both artifacts. | Sidecars passed. The first member-list command incorrectly kept the shell in `dist/` and prefixed paths with `dist/`, so `unzip` and `tar` failed to find the unchanged files. The corrected root-directory repeat found seven IPA members, 3,264 source members and zero forbidden members. Extracted IPA reported ARM64, exact source/build metadata and expected unsigned `codesign` status 1. |
| 21:37:01 | Read the active Codex goal clock. | `timeUsedSeconds=281632`: 3 days, 6 hours, 13 minutes, 52 seconds. Goal remains active. |
| 21:43:31 | Committed the seven-document acceptance/history checkpoint as `76ef53931`. | The timestamped timeline, engineering journal, progress log, roadmap, decision, parity index and this report became durable. |
| 21:44:16 | Verified the documentation commit's corresponding source and reread the goal clock. | 3,265 members, required history/report files present, zero forbidden members, SHA-256 `8f4cb489...57a4`; `timeUsedSeconds=282054` (3 days, 6 hours, 20 minutes, 54 seconds). |
| 21:44-21:45 | Pushed three commits and opened draft PR #20. | Preferred GitHub connector returned private-repository HTTP 404 without creating a PR; authenticated CLI fallback created it. Audit found exact head `718fb73b0`, three commits, eight files, 611 additions/eight deletions, `MERGEABLE`/`CLEAN` and no configured checks. |
| 21:45:48 | Marked PR #20 ready and merged only protected head `718fb73b0`. | GitHub created final-main merge `be9c81605133d2329041f36ac7ca77b6f9347ead`; ancestry verification passed and the working branch fast-forwarded/pushed to it. |
| 21:46:42 | Verified final-main corresponding source and reread the goal clock. | 3,265 members, required files present, zero forbidden members, SHA-256 `e08098bb...a463`; `timeUsedSeconds=282209` (3 days, 6 hours, 23 minutes, 29 seconds). |

## Clean artifact and persistence evidence

### Simulator update

```text
SIMULATOR_UDID=1D19A61F-20B7-46B0-AB52-B3A3406952E2
BUNDLE_ID=io.github.chrissotraidis.ctrpad
SOURCE_EXECUTABLE_SHA256=089100e5090fc7ede2089cdb736d79c462bfc7da7ee2b97f0cac9fd46289c2c4
SIGNED_STAGED_EXECUTABLE_SHA256=443d08a93b144f832ce2337df134cea7d74d7ca6394ee75810f9de926130dabc
INSTALLED_EXECUTABLE_SHA256=443d08a93b144f832ce2337df134cea7d74d7ca6394ee75810f9de926130dabc
PERSISTENCE_VERIFIED=retail-and-slot-zero
LAUNCH_RESULT=io.github.chrissotraidis.ctrpad: 44201
```

Persistence before and after the update was identical:

```text
retail=111131200|605698800|f780bf2331476aabfc00772fa758b12dd95ebfbc907968132cbd3cdd4e2c07c0
save=111222179|6016|6a01b0f5562ed7a279d8f8e51e3b1874ac39a6120f55db4fe3873288950619a3
```

Exactly one Simulator was booted throughout. The application was terminated,
not the Simulator, while long builds ran so Apple Software Renderer did not
compete with the one-job compiler.

### Current log

The accepted session identifies build `a3523c7a8583`, initializes UIKit touch
and GLES 3, and records Gas and L drift at the retail poll. The targeted scan
for `[ERROR]`, `[FATAL]`, `CTR AssetRef` and visibility signatures returned
zero. Menu animation remained about 7-8 FPS on Apple Software Renderer. That
known Simulator performance is not used as a physical-device cadence result.

### Packages

```text
781f8c03079370818f0d038c320799d1d64ec2343443f797a575382da757ebf0  CTRPad-0.1.0-1-a3523c7a8583-unsigned.ipa
62562ce95d1bdf311320d707f86dc55caede8e75b23bc4bf626473c5a4878ae0  CTRPad-source-a3523c7a8583.tar.gz
```

The unsigned IPA contains only its payload directory, app directory, license,
third-party notices, installation information, executable and plist. The
source archive is generated from the clean commit and contains no retail disc,
runtime container, IPA, provisioning profile, certificate or key material.

## Accepted and still open

Accepted locally:

- clean exact-source Simulator, iPhoneOS and macOS ARM64 builds;
- all 25 macOS tests;
- visible copyright/title/menu retail graphics and complete touch overlay;
- independently sustained Gas, analog steering and drift state;
- retail-poll consumption of held button edges;
- neutralization across settings presentation/dismissal;
- exact update-install identity and retail/save preservation; and
- matching unsigned IPA and corresponding source.

Still open:

- Apple-issued signing identity and matching provisioning profile;
- signed IPA install and launch on a connected physical iPad;
- genuine finger multi-touch rather than accessibility-action control;
- a complete touch-only three-lap race and repeated three-boost drift chain;
- hardware cadence, frame pacing, audio latency, thermals, lifecycle and
  update-save persistence; and
- final signed physical-iPad acceptance campaign.

Therefore the overall goal remains active. The game is visibly and
interactively functional in the Simulator, but the signed physical-iPad
definition of done is not yet satisfied.
