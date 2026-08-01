# iOS UIKit view-controller lifecycle correction

Date: 2026-08-01

Implementation commit: `6b268157888fbe66b8a4910ae6bf02db6b095d2b`

Documentation parent: `6b268157888fbe66b8a4910ae6bf02db6b095d2b`

Result: **accepted for the locally testable Simulator appearance-transition,
Home/resume and rotation boundary; M8 and the overall goal remain in progress**

Current-layout correction: this report's controller-lifecycle and coherent-
rendering result remains valid, but its broad touch-overlay reflow claim was
reopened by a current-head portrait frame. SDL still advertised landscape-only
despite the iPad plist's four orientations, clipping the landscape controller
inside a portrait scene. Commit `2c78c040bf2a` corrects that separate hint
intersection and now passes exact portrait/landscape/portrait layout with all
11 controls. See `2026-08-01-ios-orientation-hint.md`.

## Scope and verdict

CTRPad no longer resets SDL's installed UIKit root view controller merely to
attach a replacement OpenGL ES view. On the project's iOS 15-or-newer target,
the root controller is preserved and the replacement view is attached directly
when necessary. This keeps UIKit's appearance lifecycle balanced without
losing the drawable.

Exact clean commit `6b268157888f` produced all of the following on the
disposable ARM64 iPad Simulator:

- zero `Unbalanced calls to begin/end appearance transitions` warnings during
  ordinary production startup, a real Home/background transition, icon-driven
  foreground/resume, rotation, continued rendering, and bounded `simctl`
  termination;
- ordered lifecycle markers with audio suspended in the background and active
  again after foregrounding;
- coherent animated title/menu pixels before and after resume and rotation;
- a reflowed, accessible touch overlay after rotation;
- the unchanged renderer semantic hash `851169f2644a1675`; and
- unchanged canonical retail BIN and memory-card inode, size, and SHA-256.

One appearance warning remains after the deliberately immediate
`--self-test-renderer-pixels` route returns from its short-lived `SDL_main`.
That residual is recorded as self-test teardown behavior. It did not occur in
the production lifecycle path and is not represented as fixed. Natural
app-initiated termination and the same transitions on physical iPad hardware
also remain open.

## Baseline reproduction and source audit

The exact pre-correction implementation at `818bc0e161d3` emitted two
appearance-transition warnings during an ordinary production launch. The
short renderer self-test emitted three during its immediate teardown. The
count appeared before renderer work on production startup, so the renderer,
disc image, and touch overlay were not plausible causes.

The common path was in
`externals/SDL/src/video/uikit/SDL_uikitview.m`. Both view-removal and
view-addition paths performed this sequence after replacing the controller's
view:

```objc
data.uiwindow.rootViewController = nil;
data.uiwindow.rootViewController = data.viewcontroller;
```

The nearby comment explains that clearing and restoring the controller avoids
orientation problems on iOS 7 and below. CTRPad declares iOS 15 as its minimum.
The same historical sequence was still present when the current upstream SDL
source was checked, so this was treated as a narrow project modernization, not
as an assumed upstream correction:

<https://github.com/libsdl-org/SDL/blob/main/src/video/uikit/SDL_uikitview.m>

## Rejected first correction: warnings gone, drawable lost

The first bounded experiment removed both `rootViewController = nil` writes
but retained reassignment of the same controller. It reduced the short
self-test warning count from three to one and removed both ordinary-launch
warnings. GLES still initialized, all shaders and VRAM pipelines compiled, and
the render loop ran.

The visible result was nevertheless completely black. Screenshot
`/tmp/ctrpad-uikit-root-test.png` was 104,142 bytes with SHA-256
`97de7803f8e9c1186c3097079455d17a36720cdeb3d59668c36bbae7f3b30a5b`.
Reassigning the same root controller is a UIKit no-op, so the controller's new
view was not inserted into the window hierarchy. A warning-only check would
have accepted a broken game.

That experiment was fully reverted before the accepted implementation. It was
not committed.

## Accepted correction

Both SDL UIKit view-replacement paths now use the same bounded rule:

1. set the controller's replacement view;
2. if that controller is already the window's root, attach the replacement
   view with `addSubview:` only when it has no superview; or
3. install the controller as root only when it is not already the root.

This preserves the controller responsible for rotation and status-bar policy,
avoids a spurious nil-to-controller appearance cycle, and explicitly repairs
the view hierarchy that the rejected attempt lost. The source comments state
the iOS 15 minimum and the reason for departing from the historical iOS 7
workaround.

The dirty-source proof immediately restored visible title/demo rendering.
Screenshot `/tmp/ctrpad-uikit-root-test2-live.png` was 964,291 bytes with
SHA-256
`fddf49e3aba96237086ff07e86a057bd51c7d909b73053951b9421e64fea3d83`.
That run also completed Home/resume and rotation with no appearance warning,
but it was treated only as a precommit proof.

## Exact clean Simulator validation

The source correction was committed and pushed before the final build matrix.
The clean Simulator executable reported version
`0.1.0-beta.7.1 (6b268157888f)` and ARM64 Mach-O identity.

| Artifact or observation | Exact result |
| --- | --- |
| unsigned Simulator executable | `5024f16b8d49638ad919fabe49e9a4569ea904c6538fa838a3c6bbe4fdc2f427` |
| temporary ad-hoc-signed executable | `7146c5bb907df1b6f943d211e16584b0298b6f096d0b3da45b93da06814633e4` |
| GLES renderer oracle | pass, `851169f2644a1675` |
| production UIKit surface | 1,376 by 1,032 points/pixels, FBO/RBO 1 |
| production startup | GLES 3.0, four PSX shaders and VRAM pipelines ready |
| production appearance warnings | zero |
| immediate renderer-test teardown warnings | one, explicitly still open |

The production asset-validation pause initially looked like a stalled black
launch. `ps` showed the app consuming CPU and `lsof` showed it reading the
605,698,800-byte canonical `ctr-u.bin` at inode `111313696`. It was slow asset
validation under the Simulator software renderer, not a renderer or lifecycle
regression. The same process then logged `UIKit display loop active` and
rendered normally.

The exact live screenshot `/tmp/ctrpad-uikit-root-exact-live.png` has SHA-256
`fb8fefc894477edec36b39a1fa8eb6d1baa2cfbbc2371c598269670c3f81765e`.
The live Simulator UI was also inspected directly: it showed the animated CTR
title/demo and the complete touch overlay rather than relying on log evidence.

The Simulator's actual Home toolbar control was selected. The app logged, in
order:

```text
[CTR Lifecycle] event=will-enter-background phase=will-background audio=suspended quit=0
[CTR Lifecycle] event=did-enter-background phase=background audio=suspended quit=0
```

The actual CTRPad SpringBoard icon was then selected. The retained process
resumed and logged:

```text
[CTR Lifecycle] event=will-enter-foreground phase=will-foreground audio=suspended quit=0
[CTR Lifecycle] event=did-enter-foreground phase=active audio=active quit=0
```

The Simulator Rotate toolbar control was selected after a fresh accessibility
state read. The app remained visible and animated, the title/menu was coherent,
and the native overlay reflowed to the new safe-area geometry. The exact
rotated screenshot has SHA-256
`eaaa7ee137bca48899a1e7ba2fa94585a075329b9ccb7a844f4705f953993081`.
The attached production console remained free of appearance-transition
warnings across startup, Home, resume, rotation and bounded termination.

## Exact cross-target regression matrix

Every build directory was reconfigured after the implementation commit so the
embedded build ID is exact.

| Target | Result | Executable SHA-256 |
| --- | --- | --- |
| macOS 26.5 ARM64 desktop GL | version exact; CTest 22/22 | `5583d39d3e313a04c19d711c3799f0a6759565bcbbca97a677d1488ef71780cc` |
| iPad Simulator ARM64 UIKit/GLES | oracle and production lifecycle pass | unsigned `5024f16b8d49638ad919fabe49e9a4569ea904c6538fa838a3c6bbe4fdc2f427` |
| iPhoneOS ARM64 UIKit/GLES | compile/link; ARM64 Mach-O | `3da318dbdc69645b8eeeddb474d519196a7e0890da689f177855fb59c0f404ab` |
| macOS ARM64 GLES configuration | compile/link; ARM64 Mach-O | `57af21a0d80956147e0ee82ebaacf9b1999d2e13e6c83b40d14b67bfce211f24` |
| macOS ARM64 ASan/UBSan | version exact; CTest 22/22 | `2f93ed66be90f08718b2ecfcaa2b5e3d7840deabbdf75aad1fad56312f5cb25d` |

CTest 6 revalidated the existing basic keyboard transport in both ordinary and
sanitized matrices. No duplicate keyboard implementation was added: the
published aliases remain arrows/WASD, C/K gas, X/J brake, V/L item, Q/E drift,
P/Enter Start, and Tab/Space Select.

## Preservation and test isolation

After the exact self-test, production startup, Home/resume and rotation, the
disposable clone retained the established canonical identities:

```text
retail BIN
inode 111313696, 605698800 bytes
SHA-256 f780bf2331476aabfc00772fa758b12dd95ebfbc907968132cbd3cdd4e2c07c0

save
inode 111309627, 6016 bytes
SHA-256 6a01b0f5562ed7a279d8f8e51e3b1874ac39a6120f55db4fe3873288950619a3
```

Disposable `CTRPad Import Negatives`
(`26F3DEE8-8840-446D-85FE-C882009C9C06`) was terminated and shut down without
deletion. Protected `CTRPad Import Validation`
(`1D19A61F-20B7-46B0-AB52-B3A3406952E2`) remained booted and was not installed
to, launched, rotated, backgrounded, or otherwise used by this checkpoint.
Retail data and screenshots remained outside Git.

## Commands

The central exact-build and validation routes were:

```text
cmake -S . -B build-ios-simulator-arm64 \
  -DCMAKE_SYSTEM_NAME=iOS -DCMAKE_OSX_SYSROOT=iphonesimulator \
  -DCMAKE_OSX_ARCHITECTURES=arm64 -DCMAKE_OSX_DEPLOYMENT_TARGET=15.0 \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build-ios-simulator-arm64 --parallel

xcrun simctl install <disposable-udid> <ad-hoc-signed-app>
xcrun simctl launch --console <disposable-udid> \
  io.github.chrissotraidis.ctrpad --self-test-renderer-pixels
xcrun simctl launch --console <disposable-udid> \
  io.github.chrissotraidis.ctrpad

cmake -S . -B build-macos-arm64 -DCMAKE_OSX_ARCHITECTURES=arm64
cmake --build build-macos-arm64 --parallel
ctest --test-dir build-macos-arm64 --output-on-failure

cmake -S . -B build-ios-device-arm64 \
  -DCMAKE_SYSTEM_NAME=iOS -DCMAKE_OSX_SYSROOT=iphoneos \
  -DCMAKE_OSX_ARCHITECTURES=arm64 -DCMAKE_OSX_DEPLOYMENT_TARGET=15.0
cmake --build build-ios-device-arm64 --parallel

cmake -S . -B build-macos-arm64-gles \
  -DCMAKE_OSX_ARCHITECTURES=arm64 -DCTR_NATIVE_RENDERER_GLES=ON
cmake --build build-macos-arm64-gles --parallel

cmake -S . -B build-macos-arm64-sanitizers \
  -DCMAKE_OSX_ARCHITECTURES=arm64 \
  -DCMAKE_C_FLAGS='-fsanitize=address,undefined -fno-omit-frame-pointer' \
  -DCMAKE_EXE_LINKER_FLAGS='-fsanitize=address,undefined'
cmake --build build-macos-arm64-sanitizers --parallel
ctest --test-dir build-macos-arm64-sanitizers --output-on-failure
```

## Post-rebaseline self-test teardown assessment

The clean `d5772375fabc` actual-surface replay repeated the single warning
after the renderer pixel test had printed its complete passing result. A
source-level re-audit at documentation head `cd41e62c5d88` confirms the narrow
ownership:

- `main.c` selects `NativeRenderer_RunPixelSelfTest` synchronously from
  `SDL_main`;
- that one function calls `Platform_Init`, performs every draw/readback and
  calls `Platform_Shutdown` before returning;
- `Platform_Shutdown` synchronously destroys the SDL window and calls
  `SDL_Quit`; and
- UIKit window destruction detaches the root controller while the window was
  created in the same app-delegate/run-loop callback.

There is no UIKit run-loop return between installing and destroying the
controller. Ordinary production returns from `SDL_main` with the display loop
active, so UIKit completes its appearance lifecycle before later
Home/foreground, rotation or termination. The exact clean production route
again completed those events without this warning.

Four apparent fixes were rejected without editing source:

1. manually calling `beginAppearanceTransition:` or
   `endAppearanceTransition` has no supported public test for UIKit's private
   pending root transition and can introduce a second imbalance;
2. spinning or sleeping a nested run loop makes a deterministic pixel oracle
   depend on an arbitrary delay and reentrant application events;
3. skipping `Platform_Shutdown` leaks the renderer/window and changes the
   self-test's teardown and exit contract; and
4. splitting the monolithic renderer test into an iOS asynchronous state
   machine is substantial test-only platform code with no evidence of a
   production defect.

**Decision:** retain the warning as an honest Simulator self-test teardown
limitation. Do not patch the accepted production UIKit lifecycle or vendored
SDL teardown merely to suppress it. Reopen only if the normal signed physical
app emits the same warning, or if a future reusable asynchronous test harness
can preserve deterministic result and cleanup semantics.

Exact post-rebaseline build, pixel, retail, Home/foreground, rotation, log and
package evidence is in
`docs/parity/2026-08-01-release-rebaseline-clean-smoke.md`.

## Remaining boundary

This checkpoint closes the repeated ordinary Simulator appearance warning and
proves Simulator Home/resume/rotation with a visible exact binary. It does not
accept all of M8. The remaining lifecycle/device boundary includes:

- development signing, installation and execution on a physical ARM64 iPad;
- physical keyboard, controller and genuine simultaneous multi-touch play;
- completed-race, audio/XA and STR playtesting on device;
- natural app-initiated termination and low-memory delivery; and
- physical-device background save integrity, cadence and energy.

The preceding published timer was 205,801 goal seconds. The documentation-close
reading was 208,828 seconds: 2 days, 10 hours, 0 minutes, 28 seconds
cumulative, adding 3,027 seconds (50 minutes, 27 seconds). The timer includes
paused and resumed goal lifetime and is not a build benchmark or person-hour
estimate.
