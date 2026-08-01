# Simulator Stability, Logging, and Current Visual Re-audit

- Date: 2026-08-01
- Branch: `codex/arm64-apple`
- Exact baseline source: `43245107c279302baf083582f91743bcb47d6a51`
- Corrected diagnostic source: uncommitted working tree based on that commit
- Runtime: iOS 26.5 ARM64 Simulator
- Sole device: `CTRPad Import Negatives`
- Protected device kept shut down: `CTRPad Import Validation`
- Status: **not accepted; Simulator stability and broad graphical integrity
  remain the physical-device gate**

## Why this checkpoint exists

The user explicitly required a stable, visibly correct Simulator product with
robust logs before any native physical-device attempt. The earlier visibility-
cache correction and its one-run visual evidence were not treated as a waiver.
The current build had to be observed again, and a frame that merely launched or
compiled could not qualify.

This pass found three different facts that must not be collapsed into one
claim:

1. the current baseline rendered the specific title, menu, character, track,
   Crash Cove and Adventure screens inspected here without the former missing-
   asset signature;
2. Simulator performance on this host was only about 4-8 FPS for much of the
   run, and short keyboard/accessibility actions were not consistently usable;
3. the production log overwrote the only previous run and lacked timestamps,
   session identity and input correlation.

The logging and accessibility-action defects were corrected and live-tested.
The broader Simulator product gate remains open until an exact post-commit
build survives repeated scene/level churn with reliable controls and no
graphical corruption.

## Resource and Simulator discipline

Both builds were compiled with nice priority 15, one job and no Simulator
process or booted device. Only the disposable `CTRPad Import Negatives` device
was booted for runtime work. `CTRPad Import Validation` stayed shut down for the
entire checkpoint. Compilation never overlapped a booted Simulator.

The complete imported runtime image remained 605,698,800 bytes across both
update installs. CoreSimulator migrated the data-container UUID on each
install, so every path was re-resolved rather than reused. No retail file or
retail-derived screenshot was added to Git.

The baseline runtime reached about 83% CPU for CTRPad and 53% for WindowServer.
At the same reading, the host had about 23 GB resident use, about 10 GB of
compressed memory, 272 MB free and load averages `15.21 25.23 47.16`. Those
figures explain why this is a hostile Simulator environment, but they do not
turn dropped input or untested graphics into acceptance.

## Exact baseline build

The clean baseline was configured and built before booting a device:

```text
source identity       43245107c279
iOS build time        67.99 seconds real
compiler warnings     32 established warnings
architecture          Mach-O 64-bit executable arm64
executable SHA-256    d37cdf88fbec01c4f05c4c434b6fcbf76086b5e78bf68c816f0f2534f5f6c493
version               0.1.0-beta.7.1
SDL identity          SDL-3.4.10-beta-7.1-174-g43245107c
```

The linker's incomplete ad-hoc resource signature failed strict bundle
verification as expected. The exact build remained untouched. A temporary
copy was fully ad-hoc signed for installation and passed deep/strict
verification; its transformed executable SHA-256 was
`5f22d274bcd621934ea22f5225ca4d71e0b9b3a079e45366492bc76edacb176a`.
That transform is Simulator installation plumbing, not a release signature.

## Baseline visual route and findings

The device had retained a non-upright orientation from earlier work. Its Home
screen and CTR content were both rotated, proving this was device state rather
than game-only geometry. Two ordinary Simulator Rotate actions restored an
upright landscape presentation. This orientation normalization is not counted
as a product fix.

Computer Use then observed these retail surfaces:

- copyright and main menu;
- Time Trial highlight;
- Crash character selection;
- Crash Cove track list and preview;
- No Ghost selection;
- Crash Cove fly-in, starting grid and live lap state;
- pause menu.

The observed frames contained the expected menu text, Crash/vehicle models,
character portraits, track preview, course geometry, start banner, kart, HUD,
lights and minimap. No inspected frame reproduced the earlier widespread
missing-visibility geometry. This is specific visual evidence, not a complete
game-wide asset census.

The baseline app log grew to 88 lines / 3,704 bytes and had SHA-256
`12e8a9bfe12ea051217815942900684d9f5080e71e531c275a5abfe3b469fc07`.
It contained zero `[CTR AssetRef]`, `visibility cache exhausted`, `ERROR`,
`FATAL` or unbalanced-render lines. It reported roughly 4-8 FPS for the later
run, with an earlier short peak near 15 FPS. An earlier 1,000-byte log was
manually copied before launch because the production logger otherwise opened
the active path with `wb` and destroyed the preceding diagnosis.

Keyboard `S` moved the main-menu highlight, but two separate `K` actions did
not select. Touch Cross selected. In the pause menu, only some keyboard Down
actions moved the highlight. Short coordinate/accessibility Cross actions also
became unreliable. The run therefore did not satisfy the basic-controls part
of the user's Simulator gate.

The launch console additionally contained two Simulator/Foundation
diagnostics: a null `NSMapTable` argument and duplicate
`UIAccessibilityLoaderWebShared` classes in the iOS 26.5 runtime. Neither
appeared in the app-owned file log and neither terminated the process. They are
recorded rather than hidden or reclassified as app success.

## Logging correction

The former logger held one path and opened it with `wb`. The correction now:

- keeps four complete previous sessions as `.1` through `.4`
  (`platform/native_log.c:20,60-107,191-217`);
- prefixes the persistent copy of every app log call with UTC wall time,
  elapsed session time and `INFO`/`WARN`/`ERROR` severity while leaving console
  text compatible (`platform/native_log.c:22-58,109-146,239-264`);
- flushes every entry as before (`platform/native_log.c:119-131`);
- exposes the active/previous path and open state
  (`include/platform/native_log.h:6-11`);
- records exact version, build, compiler, target, asset root, writable root and
  log/archive paths after platform initialization (`main.c:116-126,416-433`);
- records mapped touch down/up and keyboard down edges only while a real log is
  open, so media-free self-test output remains stable
  (`platform/native_input.c:444-468,504-523`).

The log contains paths and input masks, not retail media bytes. Simulator OS
diagnostics remain in console/unified-log capture; the app does not claim to
own or persist messages emitted outside its logging API.

## Accessibility input root cause and correction

The game buttons published input-down only from
`UIControlEventTouchDown | UIControlEventTouchDragEnter` and released it from
touch-up/cancel events (`platform/apple/native_ios_touch.m:444-461`). A UIKit
accessibility activation can invoke the button's primary action without
synthesizing `UIControlEventTouchDown`. That made a control visibly activate
while the PS1-shaped pad received no down edge.

`CTRPadInputButton` now overrides `accessibilityActivate`, publishes the same
touch-down action as a finger, and sends touch-up 100 ms later on the main
queue (`platform/apple/native_ios_touch.m:37-38,116-131`). Only game-input
buttons use the subclass (`platform/apple/native_ios_touch.m:444-446`);
settings and disc utility buttons retain their ordinary UIKit actions.

This is an accessibility correctness fix, not a test-only Simulator hook.
Voice Control, Switch Control and other accessibility activation paths now
produce the input their visible button state promises.

## Build and media-free verification after the correction

No Simulator was running for either build:

```text
macOS ARM64 one-job build        107.14 seconds real; 32 warnings
macOS CTest                       22/22; 2.94 seconds test / 2.98 outer
iOS Simulator one-job build      36.47 seconds real; 32 warnings
iOS diagnostic executable SHA    6f2614393552b75e3151a59380f8ce56d85923abe67426a90d0e0d7f28fb384a
strict-signed temp-copy SHA       b4851964b15aa5c6a6d109fe58c9698581f6c38b568bc030fedb0a9579c12932
```

The logger was exercised twice in a fresh isolated directory through the
renderer pixel self-test. Both renderer hashes passed. The second run preserved
the first as `.1`:

```text
current log     1,340 bytes  ae03ee0ad2c3be66d951a53f3dbbb3f03f2d6977412c641796eea0c6d523058d
previous .1     1,310 bytes  bb937fcf14974d081c301ad0b1802284cf5331ea8963ae3025958c60394305ac
first close     +0.871 seconds
second close    +0.884 seconds
```

Each first line had an ISO-8601 UTC timestamp, `+0.000s`, `INFO`, the active
path and whether a prior session existed. Each last line was a timestamped
`WARN` close marker.

The corrected iOS build was intentionally diagnostic and uncommitted. Its
already-configured binary still embedded the preceding clean commit string.
It is not described as an exact source artifact. Publication evidence requires
a commit followed by a fresh clean configure/build.

## Corrected live diagnostic

The same sole disposable device was update-installed. Its complete imported
image remained 605,698,800 bytes after CoreSimulator migrated the data
container again.

Live accessibility activation then produced and consumed:

```text
+87.634s  touch Start down  mask=0x0008
+87.755s  touch Start up    held-before=0x0008
+145.033s touch Cross down  mask=0x4000
+145.178s touch Cross up    held-before=0x4000
```

One accessible Start skipped the presentation to the retail main menu. One
accessible Cross opened Adventure, later Cross actions entered character/name
screens, and a Cross action visibly entered `A` into the retail name field.
Those are game-consumption observations, not just button animations or log
lines.

The corrected visual route covered copyright/presentation, main menu,
Adventure New/Load submenu, character selection and name entry. These screens
retained their expected models, vehicles, text, background, stats and overlay.
The preceding exact baseline already covered Crash Cove through live lap
state. The corrected run did not complete an Adventure-hub transition or
repeat multiple full course loads, so the visibility-cache churn gate remains
open.

Hardware-keyboard capture logged Return as scancode 40 / Start mask `0x0008`,
but some short Return actions still needed repetition before the retail menu
responded. Attempting to type `A` through generic text automation was rejected:
the established game aliases correctly interpreted Shift as L1 and `A` as
D-pad Left rather than as text entry. The source did not add a second hidden
keyboard path to make automation appear successful.

The same PID completed one Home/background and foreground cycle. The
persistent timeline recorded:

```text
+959.758s will-enter-background  phase=will-background audio=suspended
+961.745s did-enter-background   phase=background       audio=suspended
+970.276s will-enter-foreground  phase=will-foreground audio=suspended
+970.647s did-enter-foreground   phase=active           audio=active
```

The name-entry frame and typed state were visible after resume. The final live
file contained 95 lines / 10,396 bytes at SHA-256
`17f50a77b9b446812584beba7139358d0ea508a73fe898c1dfdf23f2f7c63dc5`.
Its `.1` was the complete 88-line baseline file at the exact prior hash. The
current file contained zero AssetRef, visibility-cache-exhaustion, `ERROR`,
`FATAL` or unbalanced-render lines.

## Rejected shortcuts

- The rotated cold screen was not called an app orientation defect; the Home
  screen was rotated identically and ordinary device rotation normalized it.
- The linker's incomplete ad-hoc bundle signature was not called a release
  signature. A temporary copied bundle was strictly sealed for Simulator only.
- The diagnostic build's stale configured commit string was not called an
  exact working-tree identity.
- A coherent Crash Cove or Adventure frame was not generalized to all levels.
- Logged key-down was not generalized to reliable keyboard consumption.
- Simulator performance was not extrapolated to physical Metal hardware, and
  anticipated physical performance was not used to waive the Simulator gate.

## Remaining acceptance gate

Before physical-device work resumes, one exact clean post-commit Simulator
build must satisfy all of the following with one booted device only:

1. repeat presentation, main menu, Time Trial, Crash Cove and Adventure/Load;
2. churn between multiple levels/scenes enough to exercise memory-pack
   recycling, then inspect track, kart, HUD, effects, menu and overlay pixels;
3. show zero AssetRef/cache/error/fatal/unbalanced markers in the complete
   current and retained prior-session logs;
4. make basic keyboard and accessible touch navigation reliable rather than
   occasionally requiring duplicate actions;
5. complete rotation plus Home/resume without losing pixels, input, audio or
   state;
6. characterize or improve the 4-8 FPS software-renderer experience enough
   that the Simulator is a usable stability test rather than a slide show;
7. document the exact source, executable, install transform, log hashes,
   process times and any failure honestly.

The current work improves diagnosis and accessibility and shows coherent
specific scenes. It does not yet qualify the game for a physical iPad.

## Goal-time accounting

The documentation-open goal reading was 232,171 seconds: 2 days, 16 hours,
29 minutes, 31 seconds cumulative. Goal time includes pauses/resumes and is not
a build benchmark or person-hour estimate. The documentation-close reading is
recorded in the running progress log after final verification.
