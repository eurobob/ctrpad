# iOS Files Import Result — 2026-07-31

## Result

Commit `7872f7e61ad658d1bdd941d0d8a8088740a61a12` provides a working
fresh-install import path in iPad Simulator. With no retail media in the app or
its Documents directory, CTRPad presents a native landscape onboarding screen
and Files picker instead of exiting. Cancellation returns to the chooser, a
small non-disc file receives a format-specific error, and the user's valid
NTSC-U raw MODE2/2352 BIN is copied, validated, installed and used to start the
game without relaunching the process. A later cold launch selects the installed
image directly.

This is a Simulator acceptance result, not M9 completion. A physical iPad,
development/distribution signing, live wrong-region/incomplete/inaccessible
negative cases, import interruption/backgrounding, and game-driven memory-card
persistence remain open.

## Starting boundary

The preceding storage checkpoint `02a6623f80a0` already separated these owners:

```text
application bundle                    immutable asset fallback
Documents/CTRPad/assets               user-visible imported retail media
Application Support/.../CTRPad        private writable state
```

It could boot from a manually placed Documents image and exposed the directory
through Files sharing, but a media-free launch still returned after a terminal
validation message. The implementation therefore needed to preserve UIKit's
event loop while a person selected a file, validate a large untrusted input,
avoid destroying an existing good import, and resume the existing runtime
without a second application entry.

## Implementation

`main.c` now separates asset selection from runtime startup. Missing/invalid
media on iOS creates a durable launch-options copy, starts the import bridge and
returns from `SDL_main`. UIKit continues running. A successful completion
callback reselects the installed Documents image, enters the ordinary writable
root/log/memory-card initialization, and starts the existing display loop
(`main.c:302-486`, `main.c:612-656`). Desktop behavior remains synchronous.

The ARC Objective-C bridge in `platform/apple/native_ios_import.m` owns:

- a landscape-only native onboarding window with accessibility identifiers
  `ctrpad.import.choose` and `ctrpad.import.status`;
- a single-selection `UIDocumentPickerViewController` using `UTTypeData` and
  `asCopy:YES`;
- balanced security-scoped access plus an `NSFileCoordinator` read;
- a unique `Documents/CTRPad/.ctrpad-import-<UUID>/assets/ctr-u.bin` staging
  path on the destination volume;
- main-thread progress/error state and background copy work;
- move-on-first-import or replace-on-reimport only after full validation; and
- unconditional staging cleanup on validation/copy/install failure.

The C validation callback uses the production asset and disc-image path. It
distinguishes unreadable raw media, non-`SCUS_944.26` identity, and missing
required content. `NativeDiscImage_Shutdown` explicitly releases the staged
file before Objective-C moves it (`platform/native_disc_image.c:407-418`).

The integration enables Objective-C only for iOS, compiles the bridge with ARC,
and links UIKit and Uniform Type Identifiers (`CMakeLists.txt:5-7`,
`CMakeLists.txt:121-125`, `CMakeLists.txt:226-229`).

## Diagnostic UI sequence

A new iPad Pro 13-inch (M5) Simulator named `CTRPad Import Validation`, UDID
`1D19A61F-20B7-46B0-AB52-B3A3406952E2`, was created on iOS 26.5. Its first
boot spent 3 minutes 54 seconds in Apple's first-use data migration. The older
evidence Simulator was shut down rather than deleted, preserving its state.

The initial development package contained no retail media. Computer Use
confirmed a navy onboarding screen with white/yellow instructions and a blue
**Choose CTR disc image** button, then opened the native Files picker. The raw
Simulator capture was portrait-sized until the Simulator UI was rotated; the
landscape app view then filled the simulated device. This is not physical
rotation acceptance.

The exercised paths were:

1. **Cancel.** Dismissing Files produced `No file selected. Choose the NTSC-U
   raw BIN when you are ready.` and left the button enabled.
2. **Invalid format.** A local-only 118-byte `not-a-disc.bin` fixture selected
   through Files produced the raw-MODE2/2352-specific error. There was no
   installed `ctr-u.bin` and no `.ctrpad-import-*` residue.
3. **Media-free relaunch.** A cold relaunch returned to onboarding.
4. **Valid import.** The user's ignored retail source at
   `ref/CTR/CTR - Crash Team Racing (USA).bin` was cloned into the Simulator's
   Files-visible root and then selected through the real picker. The source was
   605,698,800 bytes with SHA-256
   `f780bf2331476aabfc00772fa758b12dd95ebfbc907968132cbd3cdd4e2c07c0`.
   The destination matched both size and hash and left no staging directory.
5. **Same-process transition.** PID `93200` remained the launch PID from the
   media-free screen through the first rendered game view.
6. **Cold relaunch.** A subsequent launch bypassed onboarding and rendered from
   the installed Documents image.

This development sequence established the interaction before source commit.
It is diagnostic evidence only; the exact committed repeat below is the
acceptance producer.

## Exact committed repeat

After the implementation commit was pushed, all acceptance products were
reconfigured from source identity `7872f7e61ad6`.

For the exact Simulator repeat, the pre-sign executable had SHA-256
`aac8ff1a3903cf1ee083c82e830685ce072c1d43459c41e3936349000bbfa6e1`.
The app was copied to
`/private/tmp/ctrpad-ios-import-exact.N9bdqQ/CTRPad.app`, ad-hoc signed and
verified with strict/deep codesign. Its signed executable hash was
`44e66cfc1383852a33fa5af4962e80a6da6af4979a2fae637b008c0869de209a`,
which matched the installed executable. Inspection showed one ARM64 Mach-O,
IOSSIMULATOR platform 7, iOS 15.0 minimum, SDK 26.5, valid plist, Files keys,
and embedded source identity `7872f7e61ad6`.

To exercise the exact media-free path without deleting retained evidence, the
prior destination was renamed to `ctr-u.bin.pre-exact` outside Git. Installing
the exact app migrated the data container to a UUID beginning `A62A57C4`. The
exact app showed onboarding, opened
Files, imported the retained user source, and installed a destination with the
same 605,698,800-byte size and SHA-256. PID `99595` persisted from the
media-free launch through the rendered game view, proving same-process
continuation. No staging directory remained.

The local-only screenshot
`/private/tmp/ctrpad-import-exact-success.png` is 2064 by 2752 pixels, has
SHA-256 `e83b12b1551a9f5e62915f6ed4e401433da0070fe11c1e556cfc609040be36d9`,
and visibly shows coherent checkered-flag geometry and texture. Its portrait
raw capture orientation is not promoted to a rotation claim. The screenshot
was not added to Git.

An exact cold relaunch under a bounded console session reported:

```text
CTR Native 0.1.0-beta.7.1 (7872f7e61ad6)
base/assets: Documents/CTRPad and Documents/CTRPad/assets
writable:    Application Support/chrissotraidis/CTRPad
surface:     1376x1032, 4:3
renderer:    Apple Software Renderer; GLES 3.0 / GLSL ES 300
pipelines:   four PSX shader modes plus ready VRAM pipelines
audio:       CoreAudio 44.1 kHz stereo, 1024 sample frames
cadence:     7.01 FPS for the first 120-frame window
```

Ctrl-C bounded that console run and PID `1868` was confirmed gone; natural
UIKit termination is not inferred. The Simulator was then shut down without
deleting its data.

## Exact source matrix

```text
macOS ARM64 app
  20/20 CTests in 0.72 s; strict/deep ad-hoc signature; thin ARM64
  SHA-256 f488dc74261d887974d7935b722f76b9b2bc1d3f401c1f271e6500d273a53b7e

ARM64 ASan + UBSan
  20/20 CTests in 3.88 s; fail-fast ASan/UBSan; no finding; thin ARM64
  SHA-256 c28e83e96807ee82e2e00765593102b00500bf61cb4670b5b7ace819c43136cb

iOS Simulator ARM64
  iOS 15.0 floor; SDK 26.5; signed executable
  SHA-256 44e66cfc1383852a33fa5af4962e80a6da6af4979a2fae637b008c0869de209a

iOS device ARM64
  iOS 15.0 floor; SDK 26.5; unsigned and unrun; thin ARM64
  SHA-256 6350c11ca34a199ebd1f1449281331c0fe302ebcffb92c7bc19b28887d5f8eff

optimized Linux i686
  20/20 CTests in 4.04 s; ELF32 Intel 80386; GNU Build ID 063a0ff1b696ce52a52333233d92b1cbdf4a6c7c
  SHA-256 361d313ea607bc971dacda8da2661eea161d759780913b4b7599c5b85fabab12
```

The Apple builds repeated the 32 established C warnings and the sanitizer
build repeated its established warning set; the Objective-C importer added no
new warning. The i686 compile repeated its four established warnings. The
generated iOS device product is architecture/package evidence only and does
not satisfy signing or hardware execution.

## Warnings and rejected inferences

Simulator output still includes the existing missing-scene-configuration and
minimal-bundle `Assets.car` notices, the future `UIRequiresFullScreen` warning,
two unbalanced UIKit appearance-transition warnings, and a duplicate
WebCore/WebKit accessibility-class warning. They are retained as open
Simulator/package findings rather than hidden.

The picker copy appeared under the app Inbox before the coordinator staged,
validated and installed it. This is expected `asCopy:YES` behavior and is not a
claim that CTRPad modifies the user's original file. Simulator performance of
roughly 5–9 FPS uses Apple's software renderer and is not a physical-iPad
cadence estimate.

No retail image, copied Inbox file, test fixture, app package, log, screenshot,
memory card or staging output was staged or committed.

## Accepted and open boundary

Accepted on iPad Simulator:

- coherent native media-free onboarding and Files presentation;
- cancellation and retry;
- invalid raw-image rejection without destination/staging residue;
- full NTSC-U production-loader validation before installation;
- same-volume install only after validation;
- same-process transition into the game;
- cold relaunch from Documents;
- coherent visible geometry/textures; and
- exact ARM64 macOS/Simulator/device builds plus sanitizer coverage.

Still open:

- signed installation and Files behavior on physical iPad hardware;
- wrong-region, incomplete, inaccessible and copy-interruption paths exercised
  through Files (their distinct branches are implemented);
- background/resume or termination during the 605 MB copy;
- explicit re-import/settings UX beyond manual file replacement behavior;
- game-driven iOS memory-card creation, reload, app-update persistence and
  background save atomicity;
- device controller/audio/renderer cadence acceptance; and
- touch-first controls and signed sideloadable distribution.

M9 therefore advances materially but remains in progress.
