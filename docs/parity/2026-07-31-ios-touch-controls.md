# iOS/iPadOS Native Touch Controls — 2026-07-31

## Result

Commits `c496c27f04c878fdd27af78749ed82ea3618341e` and
`c783eda740c4cbfec538c276fa39220674810f64` add a native, safe-area-aware,
multi-touch UIKit control overlay and compose it into player one's existing
PS1-shaped input snapshot. The controls visibly navigate the exact Simulator
game through Adventure -> Load, display a persisted profile, and enter a Time
Trial race using touch alone. Repeated Gas contacts visibly move Crash off the
grid and beneath the start banner; Pause, device-rotation reflow and Resume
also work. Buttons, continuous analog steering, outer-ring D-pad menu
directions, quick taps and controller-plus-touch composition have deterministic
coverage.

Checkpoint `db45004f909d` resolves the later portrait-launch question as an
iPadOS 26 compatibility requirement rather than a failed rotation hint. The
iPad target now declares every iPad orientation and ignores the deprecated
full-screen compatibility key starting with version 26, while retaining the
landscape preference for iPhone and older full-screen iPadOS. A current-head
re-audit later found that SDL's runtime hint still reduced the iPad controller
to landscape-only, clipping it inside a portrait shell. Commit
`2c78c040bf2a` corrects that separate hint intersection; exact
portrait/landscape/portrait scenes now render coherently and retain every
control. The correction evidence is in
`2026-08-01-ios-orientation-hint.md`.

This accepts the implementation, adaptive iPadOS 26 Simulator layout and
Simulator interaction boundary. It does not accept physical-iPad performance
or ergonomics, real-device rotation feel, Apple
development/distribution signing, a complete touch-only race, or practical
drift/boost execution. Desktop automation also cannot accept a human-held
two-contact Gas-plus-steering gesture.

## User-facing layout

The overlay attaches only after SDL establishes the game view controller and
uses UIKit safe-area anchors. It stays transparent so the center game viewport
remains visible. Accessibility labels and identifiers are assigned to every
interactive control.

```text
left side        analog stick; outer ring also emits menu D-pad
right side       Gas (Cross), Brake (Square), Item (Circle), View (Triangle)
upper controls   L Drift/Boost (L1), R Drift/Boost (R1)
system controls  Pause (Start), Select
```

L2/R2 and stick-click buttons remain available to physical controllers and
desktop keyboards but are not displayed because the current retail gameplay
consumer audit found no required touch action for them. The overlay does not
replace a connected controller. Touch buttons are combined active-low with
the existing keyboard/controller state, and an active touch stick replaces
only player one's left axes while retaining controller buttons and right axes.

## Implementation boundary

`include/platform/native_ios_touch.h` and
`platform/apple/native_ios_touch.m` own the UIKit-only view lifecycle. `main.c`
starts and stops that overlay around the iOS display loop. The public touch
surface in `include/platform/native_input.h` exposes enable, button, left-stick
and reset operations without leaking UIKit into the portable input module.

`platform/native_input.c` stores active-high touch intent, converts it at the
same active-low pad boundary used by keyboards and gamepads, and resets
contacts during suspend, resume, shutdown, replay installation and state
restore. Replay-installed frames remain authoritative and bypass live touch.
This preserves one retail input path rather than creating touch-specific game
or physics behavior.

## First implementation and rejected menu assumption

Commit `c496c27f04c8` implemented the overlay, continuous left-stick steering,
multi-button holds and a one-host-snapshot tap latch. Its exact products passed
the initial validation matrix and the signed Simulator build visibly rendered
coherent Sony, Naughty Dog crate, CTR title and seven-row menu content beneath
the controls. A Gas tap selected Adventure.

Short touch-stick drags did not move the stable main-menu selection. That
observation was rejected as successful navigation. Source inspection showed
the menu consumes D-pad bits, not the analog axes being produced by the
prototype. The stick therefore gained a 0.68-radius outer ring that emits
direction press/release edges while preserving the continuous analog value for
racing.

## Live packet trace and two-snapshot correction

The first outer-ring prototype still missed a bounded Simulator menu press.
LLDB then traced one Down contact through the production path. The UIKit
callback called `Platform_InputTouchButton` with mask `0x0040`. At the inlined
touch consume, both the consumed and retained register values were `0x0040`.
Immediately after `NativeInput_ApplyTouch`, player one's exact eight bytes
were:

```text
00 73 bf ff 80 80 80 80
```

That is the expected analog-pad identifier with active-low Down. The following
`GAMEPAD_ProcessHold`, however, observed neutral `controllerInput1=0xff` and
`controllerInput2=0xff`. A later neutral host/display update had overwritten
the correct one-snapshot packet before the next retail poll. The game was not
stuck, UIKit had delivered the contact, and the pad mapper had assembled it
correctly; the retained edge was simply shorter than the observed host-to-
retail cadence boundary.

Commit `c783eda740c4` retains each keyboard or touch press edge for two host
snapshots. The first consume transfers the current latch into a next latch;
the second consume exposes it once more, then neutralizes it unless the input
is still held. The deterministic oracle requires the button in both snapshots
and neutrality in the third. This supersedes the historical one-snapshot
contract documented at `24aff7d88` and `2c10b00b34df` without changing held
input or replay semantics.

The corrected signed prototype moved Adventure -> Time Trial with one outer-
ring edge, moved back with the reverse edge, and selected Adventure with Gas.
The exact committed product later repeated menu navigation. An input delivered
immediately as a retail screen was transitioning could still be ignored; after
the screen became stable, Down moved New -> Load on the first tap and Gas
opened Load on the first tap. This is recorded as retail state timing, not a
transport loss.

## Deterministic and cross-target validation

The final input self-test covers:

- true left-stick analog values;
- outer-ring D-pad direction;
- simultaneous Cross + R1 + Down;
- quick Circle tap present for two host snapshots, then neutral;
- controller buttons/right axes preserved while touch owns left axes; and
- touch disable/reset neutralization.

CTest requires this exact marker:

```text
tap-latch=c+right two-host-snapshots aliases=12 held=k+d+e alias-tap=k+d touch=analog+dpad+chord+tap+gamepad-peer
```

Exact `c783eda740c4` results are:

| Build | Result | Executable SHA-256 |
|---|---|---|
| macOS ARM64 Release | 21/21 CTests in 0.88 s; strict/deep sign, plist and thin ARM64 checks passed | `c77d84b15558d1e9c284a56f1a73945291feb7c58c0974c8c251a5a2b11ee642` |
| macOS ARM64 ASan+UBSan | 21/21 in 5.98 s; fail-fast; no finding | `69115f770ce129fe6400b4c3793756d0c96b7a760d12b4348f0cd61906f40cf4` |
| iOS Simulator ARM64, unsigned | linked; thin ARM64; minimum iOS 15.0; SDK 26.5 | `27fc3a7d23cb42f718db57803aab2cb6a752ab7c424780fc7668716af17bd6c3` |
| iOS device ARM64, unsigned | linked; thin ARM64; minimum iOS 15.0; SDK 26.5 | `4211ceb7a5800a726ac4031f66d760b86b88d58c5d0b02095b160a7ba381a819` |

The ordinary build repeated 32 established warnings; the sanitizer build
repeated 59; both iOS builds repeated 32. No new compiler warning, sanitizer
finding or failed test was assigned to the change. A direct optimized i686 C17
compile of `platform/native_input.c`, using the vendored SDL pre-include and
`-Werror=implicit-function-declaration`, passed. This translation-unit check is
not a full i686 application matrix.

The exact direct compile command was:

```sh
docker run --rm --platform linux/amd64 -v "$PWD:/src:ro" -w /src \
  ctrpad-linux-i686:ubuntu-24.04 \
  gcc -m32 -std=c17 -O2 -fno-strict-aliasing -fwrapv \
  -DCTR_NATIVE -DCTR_INTERNAL -DBUILD=926 -Iinclude \
  -Iexternals/SDL/include -include SDL3/SDL.h -Wall -Wextra \
  -Werror=implicit-function-declaration -c platform/native_input.c \
  -o /tmp/ctrpad-native-input.o
```

The exact disposable Simulator package is local-only at
`/private/tmp/ctrpad-ios-touch-exact.N9hvQT/CTRPad.app`. Ad-hoc signing changed
the executable SHA-256 to
`60a1373d13dcb66040b8ec504e6cd2970f7067f57c115ee116a1ecf7fab329f3`.
It passed strict/deep verification with identifier
`io.github.chrissotraidis.ctrpad`, no TeamIdentifier, and is not a physically
sideloadable Apple development-signed artifact.

## Exact Simulator visual and save-reader evidence

The exact app displayed coherent game geometry, fonts, colors and textures
under the unobtrusive overlay. Touch-only input entered Adventure, highlighted
Load and opened `CHOOSE A GAME TO LOAD`. The screen displayed slot `A` with
Crash's icon and populated counters; other slots were `EMPTY`. The save bytes
were the already accepted 6,016-byte game-created file documented in
`2026-07-31-ios-memory-card-atomicity.md`, deliberately copied into the default
private root to isolate the production cold-reader check. App installation
migrated the data container without changing either the save or imported BIN
hash.

Local-only evidence, excluded from Git, is:

```text
main-menu screenshot     150697 bytes
SHA-256                  aa31f5d6a73523ff30516d4874a8907ebd9c22db9a68897a8dcd78eab1cd214a
load-profile screenshot  121963 bytes
SHA-256                  a1c5135dee001ead4b68d39d619157a45632c610df0b7ae20874efd4c01f700b
runtime log SHA-256      99f5a1710c5adbcfc67c44f223f1b3496749adccac6d5d4c4c44e051c71a344a
```

The log records GLES initialization, shader setup, CoreAudio and
`[CTR Touch] touch-first overlay active`, followed by lifecycle events. Apple
Software Renderer cadence remained roughly 6–9 FPS. That is a Simulator
diagnostic and must not be extrapolated to a physical iPad.

## Touch-only Time Trial and rotation follow-up

The already exact signed `c783eda740c4` product was launched again with its
retained retail image and save hashes unchanged. Every game selection in this
route used the overlay: View advanced the presentation; a stick outer-ring
Down selected Time Trial; Gas selected Time Trial, Crash, Crash Cove and No
Ghost; and View skipped the race fly-in. The route reached Crash Cove's normal
starting grid with lap 1/3, HUD and minimap intact.

Desktop Computer Use exposes clicks and drags but no independent pointer-down
and pointer-up pair for a human-length two-finger hold. Twelve Gas taps advanced
the race timer without decisive displacement. Ten short in-button drags moved
the kart slightly. A later 60-contact bounded sequence visibly advanced Crash
from the grid to beneath the CTR banner: the timer changed from `0:44:53` to
`1:02:13`, the world camera advanced and the minimap marker moved. Alternating
Gas and right-stick contacts continued movement to `1:32:26`, but did not
produce a visually decisive heading change. This accepts live acceleration and
forward movement, not sustained steering, simultaneous input or lap
completion.

Pause opened the retail Pause menu on the first stable touch. The app had
launched while the simulated device remained portrait, producing a 743-by-1018
capture with a landscape game surface letterboxed inside it. Rotating the
device while paused produced a full 932-by-768 landscape capture: game content
filled the display and every safe-area control reflowed to the corresponding
corner or top edge. Gas selected Resume and the live race continued.

Local-only evidence is:

```text
portrait Pause       743x1018, 102560 bytes
SHA-256              23a7f8d6944277c634d11879426fd6428abe69ba0d2a8c38d6c8de397cc6f2db
landscape Pause      932x768, 148909 bytes
SHA-256              41949c131a1ffb0c64f92bc6f7b290d3cd84b6b3730ceeb0764b6767dd5fed80
forward movement     932x768, 185907 bytes, timer 1:02:13
SHA-256              d63fc9d41d70c0ed1a489d8c0ceff79554d7e544b7f591a94c65324ba62b3646
later moved frame    932x768, 177504 bytes, timer 1:32:26
SHA-256              1d188f44b4bc8e4ecb40cc6df83b77094b29e66d464a37c6b09277231dc55a5f
```

An exact clean branch-tip rebuild embedded `da151bfefb18` after the run. Its
unsigned Simulator executable SHA-256 was
`476ccfca0b052f5c0a2f0b6a510d5b813807ee5e83f2214a8c65efe7ec45f8a6`;
the disposable ad-hoc signed executable was
`896a13d7f16ff0219730d8e319a9ef3a9585fbeddfce69870bc70bec5798457b`.
The package passed strict/deep verification and the installed executable hash
matched. Repeated app updates preserved the imported BIN and both default and
report save inodes, sizes and SHA-256 values.

LLDB attached to that exact clean runtime and broke on
`Platform_InputTouchLeftStick`. A tap at the stick's right edge arrived on
ARM64 as:

```text
w0 x       0x00007ffe = 32766
w1 y       0x000000fa = 250
w2 active  0x00000001
release    x=0, y=0, active=0
```

This directly proves that UIKit turns a visible right-edge contact into a
nearly full positive signed analog value and neutralizes it on release. The
automation contact ended before a later host update could provide authoritative
live steering/packet evidence; the deterministic oracle remains the proof that
an active held stick writes those axes into the player-one snapshot.

The portrait cold-launch observation first prompted three bounded public-API
experiments: a scene geometry request immediately before overlay attachment,
the same request from `viewDidAppear`, and a `LandscapeRight`-only preference.
All compiled for Simulator/device ARM64 with the established 32 warnings, all
returned without an error callback, and none rotated the portrait cold launch.
Every experimental source line was removed with `apply_patch`; `git diff` and
`git status` returned clean before the exact rebuild. The rejected code and its
dirty packages are not acceptance evidence.

The Simulator runtime then supplied the missing platform explanation:
`UIRequiresFullScreen` will be ignored and all orientations will be required.
Apple's current
[UIRequiresFullScreen reference](https://developer.apple.com/documentation/bundleresources/information-property-list/uirequiresfullscreen)
describes iPadOS 26 windowing and dynamic resizing and notes that an interface-
orientation update may not visually rotate a scene. Apple's
[TN3192 migration note](https://developer.apple.com/documentation/technotes/tn3192-migrating-your-app-from-the-deprecated-uirequiresfullscreen-key?changes=_3_3)
documents all-orientation support and the versioned compatibility key. The
earlier experiments were therefore rejected for assuming that landscape must
be forced, not because portrait was a broken renderer state.

Commit `db45004f909d` adds
`UIRequiresFullScreenIgnoredStartingWithVersion=26` and all four entries under
`UISupportedInterfaceOrientations~ipad`. It keeps the phone orientation list,
`UIRequiresFullScreen`, and SDL landscape hint so pre-iPadOS-26 full-screen
behavior retains its preference. No scene-geometry request was added.

The exact post-commit evidence is:

| Build | Result | Executable SHA-256 |
|---|---|---|
| iOS Simulator ARM64, unsigned | linked with the 32 established warnings; embeds `db45004f909d` without a dirty suffix | `4898a9af1b028d4bc75e8e9be11bd3a23131851bb255389aca5c1ef314b2beb0` |
| iOS Simulator ARM64, ad-hoc signed | strict/deep verification passed; installed executable matched | `408514002b4ae26c749c66be24d72c2925047dfd20364e995f4ce3cd8f9a0b96` |
| iOS device ARM64, unsigned | thin ARM64 linked with the 32 established warnings; generated plist contains the versioned key and all iPad orientations | `483ae8c6f21943a01153c4746c25371a1518147878dd7fe423e202b74d74fff0` |
| macOS ARM64 Release | thin ARM64; 21/21 CTests in 1.66 s | `404528725455bcb52ffa12d1337b23758fa5e19549194a3ffe8a92387633fca1` |

The exact app cold-launched in a 743-by-1018 portrait Simulator view with a
centered 4:3 game surface and all controls inside the safe area. Rotation
produced a full 932-by-768 landscape view with the shoulder, system, stick and
face controls reflowed; rotating back restored the portrait composition. A
post-launch query of `com.apple.runtime-issues` returned no configuration
fault. Local-only portrait evidence has SHA-256
`a09a91713d1e007ba116d3a314583a2289b5e8eef6e2d13a70517b265e27d4b3`;
the landscape capture has
`af3c031496b2d5db973a4f43a71a68f4462e3bc2fa36b4b21c250de0bf77ff2a`.
Neither screenshot entered Git.

The exact reinstall preserved the 605,698,800-byte retail BIN at SHA-256
`f780bf2331476aabfc00772fa758b12dd95ebfbc907968132cbd3cdd4e2c07c0`
and the 6,016-byte save at
`6a01b0f5562ed7a279d8f8e51e3b1874ac39a6120f55db4fe3873288950619a3`.
Adaptive Simulator orientation is accepted; physical-device windowing,
rotation feel and performance remain open.

## Current implementation live-race input trace

Branch documentation tip `db78d5a25` contained no production change after
implementation tip `560f6dd20`. The already audited Simulator sibling was
therefore reused: its unsigned executable SHA-256 was
`a84eb77d9f170372e732a44f3e752acaeb14eac65ae80ecc61d61677a891a4b3`,
and the installed ad-hoc signed executable was
`df2d6249f96497817af14382bfcc122b1795123a388c32bfaa8f95ead2cc41ee`.
This signature authorized only local Simulator execution.

The exact app was launched on the disposable `CTRPad Import Negatives` clone
with LLDB's wait-for-debugger route. Touch-only UI actions advanced the retail
presentations, selected Time Trial, Crash, Crash Cove and No Ghost, skipped the
fly-in with View, and reached a live lap-1/3 starting grid. The timer advanced,
textures/HUD/minimap remained coherent, and all nine accessibility-labelled
touch controls remained visible. The two full-resolution local-only captures
were:

```text
before input  2064x2752, 1,460,372 bytes
SHA-256       f6a357590df2c7353004e72b63694251dd53cebc7e390d5911d2e6db33fb5ac8
after input   2064x2752, 1,442,161 bytes
SHA-256       4e1ac6f19133a42bcc2a439713523b45d0677789a0aee5a98b0e5d6dd325b421
```

The after frame visibly placed Crash farther beneath the start banner. This
followed twelve Gas UI actions and accepts live Gas-to-gameplay delivery and
forward movement at the current implementation tip.

An auto-continuing breakpoint on `Platform_InputTouchLeftStick` observed the
gameplay steering drag. The first movement was `(341, 683, active=1)`; a later
movement reached `(32763, 516, active=1)`, nearly full positive X, and release
arrived as `(0, 0, active=0)`. Immediately before release the touch state held
the prior analog pair and outer-ring Right latch:

```text
held=0x0000 latched=0x0020 next=0x0020 x=32763 y=516 active=1
```

This accepts UIKit stick movement, analog range and neutral release during a
live race. It also supplied an authoritative negative result for the attempted
simultaneity test. Cross would have appeared in the first `heldButtons` field
as `0x4000` (little-endian bytes `0040`). It remained `0x0000` at every stick
callback. Gas taps later moved the kart, but the automation service serialized
them with the stick drag; neither tap/drag overlap nor two concurrent drag
requests represented two held UIKit contacts.

The Simulator Option-key route was also bounded. A modifier-only request was
rejected by the Computer Use service because it requires a non-modifier key;
the corrected Alt/Shift/U request completed without affecting the mapped game
keyboard path. The subsequent Option-assisted Gas-side drag caused no stick
callback and no visible movement. It is rejected rather than used as
multi-touch evidence.

After the trace, LLDB detached cleanly and the first stable Pause touch opened
the retail Pause menu. The clone BIN remained inode `111313696`, 605,698,800
bytes and SHA-256 `f780bf2331476aabfc00772fa758b12dd95ebfbc907968132cbd3cdd4e2c07c0`;
the save remained inode `111309627`, 6,016 bytes and SHA-256
`6a01b0f5562ed7a279d8f8e51e3b1874ac39a6120f55db4fe3873288950619a3`.
The clone was terminated and shut down without deletion. Source validation PID
`93637` remained running with source BIN/save inodes `111131200`/`111222179`
and the same hashes.

This checkpoint strengthens current-tip single-control live gameplay evidence.
It does **not** accept natural held Gas plus steering, Gas plus drift, repeated
three-boost chains, a complete touch-only race, or any physical-iPad behavior.
Those remain device acceptance gates.

## Correction and tooling history

All rejected or corrected routes are retained here:

1. The first Objective-C compile used `constraintEqualTo:` on
   `NSLayoutDimension`; the correct API is `constraintEqualToAnchor:`. Five
   calls were corrected before commit.
2. The initial one-host-snapshot touch latch passed its media-free unit test
   but failed live cadence, producing the exact LLDB packet sequence above.
   It was superseded by the two-snapshot contract.
3. The first CTest after that correction failed only because CMake still
   required the old `one-snapshot` output string. The self-test executable had
   passed. Updating the expected marker made the test and implementation
   contract agree.
4. A parallel macOS/iOS rebuild appeared idle at the compiler driver and was
   cancelled. Inspection showed its child `cc1` was CPU-active in the large
   unity translation unit. Sequential clean builds completed; the initial
   suspicion of a build lock was rejected. No source or evidence was lost.
5. A direct i686 input compile initially failed because the project's
   `internal` macro collided with an SDL struct field when SDL was parsed
   later. Pre-including `SDL3/SDL.h` matched the established direct-compile
   route and passed.
6. `clang-format --dry-run` was not used as a gate because the Objective-C
   mode was unsupported by that route and the repository's established C
   formatting also produced baseline differences. `git diff --check`, build,
   test, architecture and signature checks were used instead.
7. Three public `requestGeometryUpdateWithPreferences:` variants returned no
   error yet did not rotate a portrait cold launch. They were removed entirely;
   later Apple platform evidence showed that forcing landscape was the wrong
   iPadOS 26 contract.
8. One exact rebuild wrapper returned before Ninja started, and a retry began
   a duplicate identical build. Process inspection distinguished the two; only
   the newer duplicate PIDs were terminated, and the original completed.
9. A stale accessibility-tree index referred to the in-game Pause control
   after a state change instead of the Simulator Rotate control. The mistaken
   click was discarded; the accessibility tree was refreshed before exact
   portrait/landscape evidence was captured.
10. A normal live-process LLDB attach remained hung for about 90 seconds and
    was stopped. The accepted rerun used `simctl launch --wait-for-debugger`.
11. An optimized `NativeInput_ApplyTouch` breakpoint could not evaluate its
    local-variable condition and stopped every input poll, leaving a slow white
    launch. It was deleted. The accepted breakpoint used the non-optimized
    `Platform_InputTouchLeftStick` API boundary and auto-continued.
12. A modifier-only Alt/Shift Computer Use request was rejected before it
    reached Simulator. Adding an otherwise unmapped non-modifier satisfied the
    tool contract, but the resulting Option-assisted drag still produced no
    second contact and was rejected.
13. Concurrent Gas clicks and stick drag requests produced forward movement
    and analog callbacks separately. Live memory showed `heldButtons=0` at
    every stick callback, proving serialization rather than multi-touch.

## Publication and evidence boundary

The base implementation was committed and pushed as `c496c27f04c8`
(`feat: add native iOS touch controls`). The menu/cadence correction was
committed and pushed as `c783eda740c4` (`fix: make touch menu gestures
reliable`). Adaptive iPadOS 26 scene support was committed and pushed as
`db45004f909d` (`feat: adapt iPad layout for iPadOS 26`). Local HEAD and
`origin/codex/arm64-apple` matched immediately after each push. All remain in
draft pull request
[#1](https://github.com/chrissotraidis/ctrpad/pull/1); they are not merged to
`main`.

No retail image, extracted retail data, save, app package, screenshot, log,
certificate or credential entered Git. Remaining product acceptance requires
a physically signed iPad build, real-device rotation/cadence and thermal
observation, touch-target/contrast review, natural held-input play, a complete
touch-only race, and practical L/R drift-boost evaluation.

A final local availability audit ran `xcrun devicectl list devices` and
received `No devices found`; `security find-identity -v -p codesigning`
reported zero valid identities; the standard provisioning-profile directory
contained no profile file. No credential was created or requested. The next
physical gate therefore requires a connected iPad plus an Apple development
identity/profile; this environment result does not mark the active goal
complete or treat the implementation itself as failed.
