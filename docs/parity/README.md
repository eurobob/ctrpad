# Retail-Parity Evidence

This directory will contain the deterministic baseline definition, scripted
input fixtures that contain no retail data, comparison-tool documentation, and
per-milestone results.

A formal same-commit full cross-width pair now passes: clean ARM64 report
`ctr-215303` and optimized i686 report `ctr-025812` match all eight
state/transport components for all 24,232 frames. The separate two-process
i686 verifier has also completed both unchanged layouts and rejected the
deliberate active-driver mutation exactly as required. Fresh current-build
version-4 report `ctr-223221` structurally advances a lap.

Current evidence and procedures:

- `STATE-DIGEST.md` defines canonical schema 2 and its mutation/address tests.
- `NTSC-U-GOLDEN-RUN.md` defines the full version-4 acceptance procedure.
- `2026-07-29-golden-run-result.md` records the historical version-2 run and
  the input/timing omissions it exposed.
- `2026-07-30-arm64-prefix-parity.md` records the corrected cross-width prefix,
  sanitizer, and audit evidence.
- `2026-07-30-arm64-full-regeneration.md` records the complete ARM64 capture
  and the rejected pre-correction i686 candidate.
- `2026-07-30-full-cross-width-result.md` records the finalized corrected
  i686 rejection, both first-divergence diagnoses and corrections, rejected
  tracing/extension routes, and accepted current-format lap coverage.
- `2026-07-30-macos-arm64-cadence.md` records the complete transport audit and
  direct checkpoint-local wall measurement against the exact NTSC VBlank
  model.
- `2026-07-30-macos-arm64-save-relaunch.md` records the game-driven save,
  retail CRC validation, and exact-producer second-process reload.
- `2026-07-30-macos-arm64-audio-output.md` records the exact-producer
  CoreAudio open, non-silent PCM capture, zero short-run transport faults,
  and direct initial XA-sector decode.
- `2026-07-31-macos-arm64-audio-mixer-oracle.md` records the media-free
  production SPU decode/panning/Room-reverb oracle, exact ARM64/i686 PCM
  digests, sanitizer result, and wet-tail boundary.
- `2026-07-31-shared-gles3-dialect-bringup.md` records the audited Android
  reference delta, shared desktop/GLES renderer dialect, context-failure
  cleanup correction, exact ARM64/sanitizer/i686 matrix, iOS Simulator
  compile/link probe, rejected attempts, and deliberately open runtime gate.
- `2026-07-31-ios-simulator-gles-bringup.md` records the reproducible iOS
  Simulator/device bundles, SDL-owned UIKit entry, nonzero presentation-FBO
  correction, exact clean live GLES/audio/title-menu result, bounded keyboard
  evidence, rejected black-frame/orientation attempts, and deliberately open
  lifecycle, cadence, device-signing, import and touch gates.
- `2026-07-31-ios-gles-desktop-gl-equivalence.md` records the fresh current-
  build 2,000-frame all-component macOS desktop-GL/iOS GLES match, exact
  representative vertex/draw-split trace equality, coherent live framebuffer,
  rejected cross-platform checkpoint bypass, media/save preservation and the
  deliberately open full-suite/macOS-GLES/physical-device boundaries.
- `2026-07-31-ios-gles-full-golden.md` records exact-current native macOS
  desktop-GL and iOS UIKit/GLES regeneration of all 24,232 frames and 81
  checkpoints, zero mismatches in all eight required components, exact
  checkpoint-80 playback, an identical late frame-24,001 render trace, the
  explicit non-lap coverage limit, keyboard revalidation, preservation hashes,
  rejected stale bundle ID and remaining macOS-GLES/device/visual boundaries.
- `2026-07-31-renderer-pixel-semantics.md` records the production 4/8/16-bit
  texture, CLUT, zero/STP transparency, blend, output-mask, framebuffer-
  feedback and RGB5551 oracle; the rejected fixture; the live Apple GLES RG
  readback defect and guaranteed-RGBA correction; exact cross-target hashes;
  sanitizer results; preservation checks; and deliberately open macOS-GLES
  runtime/physical-device boundaries.
- `2026-07-31-ios-lifecycle-display-loop.md` records the synchronous UIKit
  lifecycle reducer, cooperative display loop, explicit audio/input/VBlank
  suspension boundaries, fully yielding iOS wait, repeated Home/resume runs,
  Simulator software-renderer diagnosis, and deliberately open device gates.
- `2026-08-01-ios-uikit-view-lifecycle.md` records the exact repeated-root-
  controller warning diagnosis, the rejected warning-free black-screen
  attempt, the iOS 15+ hierarchy-preserving correction, real Simulator
  Home/resume/rotation evidence, exact renderer/cross-target/sanitizer hashes,
  preservation checks, the residual immediate-self-test teardown warning, and
  deliberately open physical-device/natural-termination gates.
- `2026-08-01-ios-disc-reselection.md` records the confirmed in-game stop-and-
  reselect path, rejected hot-swap boundary, real Files cancel/invalid/valid
  outcomes, atomic disc replacement, exact save preservation, cold relaunch,
  cross-target hashes, and deliberately open physical-device/provider gates.
- `2026-08-01-ios-control-settings.md` records persisted handedness, size and
  opacity controls, the in-game keyboard legend, minimum setting targets,
  Reset/Done and short-height scrolling, the rejected stale-window clipping
  diagnosis and unarmed keyboard report, update preservation, exact builds,
  and deliberately open physical-iPad ergonomics/multi-touch gates.
- `2026-08-01-level-visibility-cache.md` records the user-visible missing-
  scene report, the exact 268-error cache-exhaustion signature, the eight-entry
  LP64 sidecar lifetime root cause, range-aware memory-pack correction,
  targeted/range/all recycling coverage, resource-throttled validation status,
  and deliberately pending post-fix live churn gate.
- `2026-07-31-ios-sandbox-storage.md` records the bundle/Documents/Application
  Support ownership split, Documents-priority retail startup, private log and
  memory-card roots, Files metadata, exact cross-target matrix, rejected
  incremental-install evidence, and deliberately open import/save UX gates.
- `2026-07-31-ios-files-import.md` records the native fresh-install Files
  chooser, security-scoped staged validation, cancel, invalid-format,
  detected-PAL-region and truncated-NTSC-U paths, exact valid import with
  same-process startup, cold relaunch, narrowly scoped every-launch recovery of
  interrupted stages for both missing and already-valid assets, preservation
  hashes, cross-target matrix, visible evidence, and deliberately open
  device/provider gates.
- `2026-07-31-ios-memory-card-atomicity.md` records same-directory durable
  temporary writes and atomic replacement, injected-failure preservation,
  exact cross-target validation, visible exact-app startup, the bounded iOS
  keyboard audit, the rejected incompatible-checkpoint shortcut, and the
  completed frame-zero game-driven iOS persistence run, lifecycle preservation
  and later production cold read.
- `2026-07-31-ios-touch-controls.md` records the safe-area-aware multi-touch
  overlay, player-one peer composition, analog plus outer-ring menu input,
  direct live packet trace, two-host-snapshot cadence correction, exact matrix,
  touch-only menu/save-reader evidence, the iPadOS 26 adaptive-orientation
  correction, and remaining physical-iPad gates.
- `2026-07-31-ios-hardware-keyboard.md` records the live player-two ownership
  diagnosis, iOS shared-primary correction, rejected debugger/immediate-record
  routes, exact cross-target matrix, keyboard-only presentation-to-race route,
  decoded player-one press/release packets, and remaining physical-iPad gate.
- `2026-07-31-ios-sideload-package.md` records the reproducible retail-free IPA
  workflow, embedded GPL/notices/Installation Information, profile and DER
  signing guards, rejected archive/signing bugs, exact artifact hashes, and the
  deliberately open user-owned Apple signature/device-install gate.
- `2026-07-31-macos-arm64-keyboard-tap.md` records the missing quick-tap root
  cause, historical one-snapshot press-edge transport, its later two-snapshot
  supersession, direct live PSX packet traces, and exact-commit
  ARM64/sanitizer/i686 validation plus live Time Trial acceleration and
  steering with the practical aliases.
- `2026-07-31-macos-arm64-controller-hotplug.md` records controller-slot
  ownership, duplicate-add/removal/reconnect coverage, standardized
  button/axis translation, and rumble through an SDL virtual gamepad.
- `2026-07-31-full-cross-width-acceptance.md` records the first complete clean
  same-commit ARM64/i686 all-eight-component match, the later independently
  completed alternate-layout two-process/mutation gate, and their exact
  report, process, transport, save, and hash evidence.

The first task is to evaluate `platform/native_replay_scheduler.c`,
`platform/native_checkpoint.c`, `platform/native_savestate.c`, and
`docs/REPLAYS.md` against a running 32-bit build. A usable gate must:

1. replay identical per-frame inputs;
2. compare game-visible state without embedding host pointer values;
3. cover startup, menus, racing, powerslides, items, transitions, and saves;
4. detect an intentional state mutation;
5. report the first divergent frame and field or region.

Screenshots and playtesting supplement this gate; they do not replace it.
