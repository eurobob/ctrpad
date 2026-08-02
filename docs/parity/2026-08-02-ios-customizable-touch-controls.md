# iOS/iPadOS customizable touch controls — implementation plan

Scope: extend CTRPad's existing UIKit overlay and PS1 input compositor only;
leave emulation, rendering, retail data, and `ref/` unchanged.

1. Replace fixed Auto Layout gameplay positions with safe-area-relative CTRPad
   defaults plus normalized, separately persisted phone/tablet + handedness
   overrides. Keep Change Disc and Controls as fixed utility actions.
2. Add an in-app layout editor entered from Controls: release input, select and
   drag the gameplay stick/buttons, resize the selected control from 70–150%,
   clamp to the live safe area, reset the current profile, save on Done, and
   re-clamp after rotation/rebuild.
3. Preserve the earlier edge-anchored control sizing and reach pattern. Keep
   the steering thumb dedicated to the stick by grouping both drift/boost
   controls with the right-hand action buttons (mirrored for steer-right), and
   place Select plus the verified PS1 `START` action (`Start / Pause`) beside
   View and Item. Keep Controls and Change Disc together at the top right.
4. Add a two-second Cross/Gas hold-to-lock gesture with haptic/visual feedback;
   tapping Gas again, editing, rebuilding, modal presentation, backgrounding,
   runtime shutdown, or overlay removal must release it.
5. Preserve CTRPad's native PS1 masks, analog-stick publisher, multi-touch
   buttons, keyboard/controller composition, SDL hot-plug path, and physical
   gamepad mappings. Add targeted host tests where the UIKit layer permits.
6. Integrate an original opaque racing icon through an AppIcon asset catalog,
   compile it into both Simulator/device bundles, and retain the 1024px master
   under `docs/design/`.
7. Build, install, launch, inspect, and capture representative iPhone and iPad
   Simulator evidence. Record Simulator-only proof honestly and leave physical
   thumb reach, Bluetooth hardware, haptics, and simultaneous-finger feel as
   device checks.

## HarkinianPad translation notes

Adopt: passthrough empty space, independent touch contacts, normalized device
profiles, safe-area clamping, reset-to-default behavior, a high-opacity editor,
transition input cancellation, and latch feedback.

Translate: direct CTRPad PS1 masks replace N64 SDL keyboard events; Cross is
Gas and gets the latch (not N64 Z); L1/R1 remain CTR drift/boost; Circle is
Item, Triangle is View, Square is Brake, and the retail PS1 Start bit is labeled
`Start / Pause`. Exclude Zelda C-buttons, Z duplication, native Zelda HUD art,
and Shipwright-specific menu/CVar integration.

## Validation record

- Built the universal arm64 Simulator bundle and installed/launched it on an
  iPhone 17 Pro and an iPad Pro 13-inch (M5), both on iOS 26.5.
- On iPhone: moved Gas, saved, terminated/relaunched, confirmed persistence,
  then reset it; locked Gas after two seconds and released it with a tap;
  held steering + Brake + R1 simultaneously; opening Controls released all
  held state; backgrounding also returned Gas to Released.
- On iPad: inspected the tablet defaults independently, selected Gas, resized
  it to 120%, saved and relaunched to confirm persistence, then reset it;
  entered/exited the editor and rotated the running app; all controls remained
  inside the live safe area and input accessibility state remained Released.
- Built the arm64 iOS device bundle. `actool` populated `Assets.car` plus phone
  and iPad icon renditions without catalog warnings or missing-slot notices.
- Built macOS arm64, ran `ctr_native --self-test-input` (touch + controller
  composition, virtual gamepad buttons/axes/rumble/hotplug), and passed all
  26 CTest cases.

These are Simulator and host-test results, not physical-device proof. Real
Bluetooth/MFi pairing, multi-finger feel, haptic strength, and subjective thumb
reach remain physical iPhone/iPad checks.
