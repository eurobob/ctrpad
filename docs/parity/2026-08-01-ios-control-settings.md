# iOS Control Settings and Keyboard Discoverability

- Date: 2026-08-01
- Accepted implementation: `828d095809fc0f4e1984a11f4e478f56fd6950c0`
- Target: iOS 26.5 ARM64 iPad Simulator
- Validation device: disposable `CTRPad Import Negatives`
- Protected device: `CTRPad Import Validation`, not modified

## Result

CTRPad now exposes an in-game **CONTROLS** sheet. A tester can mirror the
steering/action clusters, choose among three control sizes and three opacity
levels, restore defaults, and read the practical hardware-keyboard layout
without leaving the game. Changes apply immediately and persist locally.

This accepts the Simulator software boundary for handedness, size, opacity,
short-height scrolling, minimum setting targets, Reset/Done reachability,
touch-state neutralization and keyboard-map discoverability. It does not
accept physical-iPad ergonomics, physical-keyboard delivery, human simultaneous
Gas/steer/drift input, repeated three-boost drift chains, device cadence or
energy use.

## Keyboard request and existing input path

The user explicitly requested basic keyboard controls during this checkpoint.
The audit found that the requested controls were already implemented and
published:

- `platform/native_input.c:321-352` maps arrows/WASD, ZXCV/IJKL, shoulder
  aliases, Return/P and Space/Tab;
- `platform/native_input.c:718-799` samples held keys and the two-host-snapshot
  quick-tap latch;
- `platform/native_input.c:813-831` composes the result into the same active-
  low PS1-shaped pad packet as controllers and touch; and
- `README.md:205-249` already documents the complete map.

Adding a second UIKit keyboard bridge would have created conflicting ownership
and a second physics/input path. The implementation therefore retains the
shared SDL mapper and adds a concise, accessible legend to **CONTROLS**:

```text
Steer / menus          W A S D
View Brake Gas Item   I J K L
Drift / boost          Q / E
Pause / Select         P / Tab
```

The previously accepted exact Simulator report in
`2026-07-31-ios-hardware-keyboard.md` remains the live delivery proof: keyboard-
only P/S/K input reached the Crash Cove Time Trial grid and decoded as player-
one Start, Down and Cross presses with neutral releases. CTest 6 reconfirmed
all aliases, quick taps, iOS primary sharing and controller/touch composition
in ordinary and ASan/UBSan matrices during this checkpoint.

## Implementation

`platform/apple/native_ios_touch.m` owns this platform-only UI:

- three clamped `NSUserDefaults` values store handedness, size and opacity;
- steering-right mirrors the steering stick and face-button clusters while
  retaining both independent drift buttons and centered utility controls;
- scales are 0.88, 1.0 and 1.12; opacity choices are 0.36, 0.58 and 0.80;
- the modal form sheet uses a safe-area-constrained `UIScrollView`, so its
  complete content remains reachable on short landscape displays;
- segmented controls are at least 44 points high, Reset is at least 44 points,
  and Done is at least 50 points;
- Reset removes all three preference keys instead of persisting redundant
  default values; and
- presenting, rebuilding, dismissing or disappearing resets touch contacts so
  changing layout cannot preserve a held button or analog vector.

The existing Change Disc path remains a separate utility action. All overlay
controls retain accessibility identifiers, and the keyboard legend has a
spoken description independent of its visual monospace formatting.

## Live validation chronology

The dirty-source Simulator and iPhoneOS builds first compiled after the
settings implementation. The disposable validation clone was the only device
modified. Before installation its accepted retail data identities were:

```text
BIN   inode 111450682, 605698800 bytes,
      f780bf2331476aabfc00772fa758b12dd95ebfbc907968132cbd3cdd4e2c07c0
save  inode 111309627, 6016 bytes,
      6a01b0f5562ed7a279d8f8e51e3b1874ac39a6120f55db4fe3873288950619a3
```

Computer Use opened **CONTROLS**, selected Steer right, Large and High, and
observed the action cluster mirror to the left while controls grew and became
more opaque. The settings survived an update install. The sheet then scrolled
far enough to expose Reset and Done; both appeared in the accessibility tree.
Reset immediately restored Steer left, Standard and Standard. Its preferences
plist contained no remaining custom keys, and Done returned to coherent
animated gameplay.

One focus transition brought the protected `CTRPad Import Validation` window
to the front after Done. It was only read and was not clicked, installed into,
rotated or otherwise modified. The Simulator Window menu was used to refocus
the disposable clone. The protected screenshot is rejected as product
evidence because it came from the wrong device.

An early post-focus screenshot appeared to omit every trailing-anchored
control. Rotation repeated the apparent omission, so it was treated as a
possible layout regression rather than ignored. A normal LLDB attach stopped
the app without returning useful expressions; `SIGCONT` restored it. That
debugger-perturbed route is rejected.

A bounded diagnostic build logged scene, window, parent, overlay and safe-area
geometry plus every accessibility-identified frame. The first logging build
failed because it referenced nonexistent `platform/native_platform.h`; the
include was corrected to `platform/native_log.h`, after which both iOS targets
compiled. The live landscape result was:

```text
scene/window/parent/overlay  1376 x 1032
safe frame                    (0,0) 1376 x 1012; bottom inset 20
right stick                   (1154,790) 195 x 195
right drift                   (1195,11) 161 x 60
left action cluster           x=27..222, y=794..1005
```

Every frame was inside the overlay. A fresh, correctly focused Simulator state
then visibly showed all eleven controls and exposed all eleven identifiers.
The apparent clipping was stale/wrong-window visual state, not an Auto Layout
defect. The temporary geometry logging and its header include were removed
before commit.

The short F9/P/F10 keyboard attempt on the ordinary launch was also rejected:
that launch had not been armed with `--record --toggle`, so it could not create
a report. It neither replaces nor weakens the accepted exact keyboard report
cited above.

## Build and deterministic validation

Before publication, dirty-source builds passed for iOS Simulator ARM64,
iPhoneOS ARM64 and macOS ARM64. The ordinary and ASan/UBSan desktop matrices
each passed all 22 CTests; the sanitizer matrix reported no finding. Commit
`828d095809fc` was pushed before exact-build acceptance.

After the later level-visibility correction and machine-resource pause, the
user explicitly resumed the goal. A clean head containing the unchanged
control-settings implementation was reconfigured and completed at nice
priority 15, one target and one Ninja job at a time, with zero booted
Simulators during compilation:

```text
product                         build ID       SHA-256
iOS Simulator ARM64            4a4b148dd8d1   6c0189fafa44de71ce8aadf01c53a186bccd862ec8244f428aedbbde2468d78a
iPhoneOS ARM64                  4a4b148dd8d1   7dc667e7523e761806b488356824082515eef43604872c8d8f3badc1a93f63f5
macOS ARM64 desktop OpenGL      4a4b148dd8d1   7fe474c5e1299445e97ba0bd3d64a38346d0097f6b6509e51180a9a210786c56
macOS ARM64 ASan/UBSan          4a4b148dd8d1   da62529752f29f231f1566afc69035b0a32ae78bfec84441366238b81ecf1d5f
```

The ordinary and ASan/UBSan suites each passed 22/22 in 2.66 and 7.19 seconds,
respectively; input CTest 6 therefore revalidated the keyboard aliases in both
exact binaries. The exact Simulator app visibly rendered the complete touch
overlay during the accepted level-geometry runtime and installed without
changing the canonical BIN or save identities. The separate macOS ARM64 GLES
configuration was not rerun in this resource-bounded continuation and is not
listed as a pass; the shared keyboard/touch mapper is already covered by the
ordinary and sanitizer builds. Full resumed-runtime evidence is in
`2026-08-01-level-visibility-cache.md`.

## Resource correction during acceptance

While the original five-target exact command was running, the user reported
that the Mac was slow and that two Simulator devices were open. The command
was interrupted. The disposable clone was shut down, leaving exactly one
booted device, the protected validation simulator. No compiler process was
left running. Remaining builds were deliberately resumed with low scheduling
priority, one target at a time. This resource correction is part of the
historical record and is not hidden by the later successful results.

## Preservation and remaining boundary

After dirty update/install, settings changes, reset, modal dismissal and live
rendering, the canonical BIN and live save retained the same inode, size and
SHA-256 values listed above. Retail media, saves, screenshots and temporary
signed products remained outside Git.

Software customization and in-game keyboard discoverability are accepted on
Simulator. M10 and the overall goal remain active until physical-iPad control
feel, natural simultaneous multi-touch, repeatable drift boosts, a completed
race, physical hardware input and device performance are accepted.
