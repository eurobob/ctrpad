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
- `2026-07-31-ios-lifecycle-display-loop.md` records the synchronous UIKit
  lifecycle reducer, cooperative display loop, explicit audio/input/VBlank
  suspension boundaries, fully yielding iOS wait, repeated Home/resume runs,
  Simulator software-renderer diagnosis, and deliberately open device gates.
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
