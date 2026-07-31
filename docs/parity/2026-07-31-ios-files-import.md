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

This is a Simulator acceptance result, not M9 completion. At this checkpoint a
physical iPad, development/distribution signing, live wrong-region/incomplete/
inaccessible negative cases, import interruption/backgrounding, and game-
driven memory-card persistence remained open. Later reports accept the save
path; the follow-up below accepts wrong-region and incomplete-image behavior.

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
- next-launch cleanup of importer-owned stale stages with visible retry status;
- coherent visible geometry/textures; and
- exact ARM64 macOS/Simulator/device builds plus sanitizer coverage.

Still open:

- signed installation and Files behavior on physical iPad hardware;
- inaccessible-provider and actual copy-interruption paths exercised through
  Files; the follow-ups below accept wrong-region/incomplete validation and the
  next-launch recovery state;
- live background/resume or termination during the 605 MB copy;
- explicit re-import/settings UX beyond manual file replacement behavior;
- game-driven iOS memory-card creation, reload, app-update persistence and
  background save atomicity;
- device controller/audio/renderer cadence acceptance; and
- touch-first controls and signed sideloadable distribution.

M9 therefore advances materially but remains in progress.

## Follow-up — live wrong-region and incomplete-image paths

The two implemented validation outcomes that were still source-inspection-only
above were later exercised through the real iOS 26.5 Files picker. This was an
evidence-only continuation: no runtime change was made because both branches
behaved correctly.

### Isolation and exact runtime

The accepted `CTRPad Import Validation` Simulator was shut down but never
deleted. `simctl clone` created a disposable `CTRPad Import Negatives` device,
UDID `26F3DEE8-8840-446D-85FE-C882009C9C06`. The clone initially reported the
source device's absolute bundle/container URLs even though its own copied data
existed. That metadata was rejected as ambiguous. Reinstalling a disposable
copy of the exact signed app corrected the paths to the clone and migrated its
data to container `FBB4DE38-856C-43D6-BC82-0D2D1777BAAE`.

The installed app passed strict/deep signature verification and its executable
SHA-256 was
`ed53ba9f26eba0a8e501f1e02aaabead1016e3b729751ce97d34c0b28879c11c`.
It embeds runtime source `a37cdf2aa5af`; the only later implementation commit
changed the standalone package script's entitlement construction, so this is
the exact current runtime rather than a claim about the later script.

Before the negative selections, the disposable clone held:

```text
accepted BIN  inode 111309681   605698800 bytes   f780bf2331476aabfc00772fa758b12dd95ebfbc907968132cbd3cdd4e2c07c0
memory card   inode 111309627        6016 bytes   6a01b0f5562ed7a279d8f8e51e3b1874ac39a6120f55db4fe3873288950619a3
```

The accepted BIN was renamed inside only the clone so startup entered the
media-free coordinator. It remained present as
`ctr-u.bin.accepted-before-negative`; this did not mutate the original evidence
device. The ignored local picker inputs were:

```text
PAL CloneCD image       740179104 bytes   84aeb6f990954abb0eed4580fe2e4d2b17ce8a6cb266385b56b96440b523f41a
truncated NTSC-U BIN     94080000 bytes   1c1fe7713840cf3e2c92dce9ef40016ba6faa98b2bff22a85d66718ce8067508
```

The incomplete fixture was the first 40,000 whole 2,352-byte sectors of the
user's accepted image. It retained a readable raw filesystem and the expected
NTSC-U identity but ended before all production assets, making the incomplete
classification materially different from the earlier 118-byte invalid-format
fixture. No fixture or retail byte entered Git.

### Observed Files outcomes

Computer Use opened the app's **Choose CTR disc image** control, navigated the
real picker through **On My iPad → CTRPad**, and selected each input:

1. The PAL image completed the coordinated copy and displayed `Wrong disc
   region (SCES_021.05). CTRPad currently requires NTSC-U SCUS-94426.` The
   chooser was immediately enabled again.
2. The 40,000-sector NTSC-U image displayed `The disc image opened, but
   required CTR files were missing or unreadable. The existing import was not
   replaced.` The chooser was immediately enabled again.
3. After both rejections, the full 605,698,800-byte NTSC-U source was selected
   through the same picker. It installed at a new inode with the exact accepted
   hash and entered the rendered Sony presentation/touch overlay without
   relaunching.

PID `77827` was unchanged from media-free launch through both failures and the
successful game transition. A deliberate cold relaunch used PID `78515`,
bypassed onboarding, and rendered the copyright screen with the touch overlay.

After each rejection, the accepted backup and memory card retained their
original inodes, sizes, and hashes, no destination named `ctr-u.bin` appeared,
and no `.ctrpad-import-*` directory survived. After the successful selection,
the new destination inode was `111313696`, with the same 605,698,800-byte size
and `f780bf23...07c0` hash. The backup and memory card were still unchanged and
there was still no staging residue.

Local-only screenshots and SHA-256 values are:

```text
wrong region       9d3e631febddbeb24b09b2c7e233eef749c78bce776652edafcf09ec1e038e58
incomplete image   86007e9248d066ad1c1b2208569dd6a2ed79a9f9ab7b52d62e33f6976ab5c791
valid transition   9bd6c25460be52d6d578e9f2fc6c101e5535fe31e018608c971a2c88460ccb3c
cold relaunch      5a4414b826d722a64a7789acf70da3d7878e758ce2e8afa9d717c6cdbf1059b8
```

The disposable clone was terminated and shut down, not deleted. The original
`CTRPad Import Validation` Simulator was booted and relaunched. Its source BIN
remained inode `111131200` and its save inode `111222179`, with the same exact
accepted hashes. Thus the isolation procedure itself did not alter the durable
evidence container.

This follow-up accepts live Simulator wrong-region identification,
incomplete-content rejection, retry, staging cleanup, valid recovery,
same-process transition, and cold relaunch. It does not accept inaccessible
Files providers, coordinated-copy interruption/background termination,
explicit in-app re-import of an already active destination, or any physical-
iPad behavior. Those remain M9 gates.

## Follow-up — next-launch recovery after an interrupted import

Checkpoint `c745390a55ebbbef08bba1b81f982914eabecc31` addresses the
durable staging residue that can survive when the process is killed before the
normal copy/validation/install error handlers run. This is a source and live
Simulator acceptance continuation; no retail byte, generated app, fixture or
screenshot entered Git.

### Cleanup contract

The importer now owns one file-scope `.ctrpad-import-` prefix for both staging
creation and recovery. Before media-free onboarding appears, it lists only the
direct children of `Documents/CTRPad`. It removes an entry only if all three
conditions hold:

1. the name starts with `.ctrpad-import-`;
2. the name has a nonempty suffix; and
3. the entry is a directory.

Missing import roots are a normal no-op. Other listing/removal failures are
logged, and only successful removals contribute to the user-visible recovery
count. One recovery uses singular text; multiple recoveries show the exact
count. In both cases the existing chooser is enabled. Staging generation uses
the same constant, eliminating a recovery/creation naming drift
(`platform/apple/native_ios_import.m:31-32,142-184,199-230,305-310`).

### Exact clean build matrix

After the implementation was committed, all targets were explicitly
reconfigured and rebuilt from source identity
`SDL-3.4.10-beta-7.1-135-gc745390a5` with no dirty suffix:

```text
target                    SHA-256                                                          result
iOS Simulator ARM64       019cf0a495696d30cca0bc9be2564c4e117c28ddd4bc9e52875d8e404c825fce  linked
iOS device ARM64          1cc45ac04a6b23136f10500ed3db194c314619b4c39071df3866b321f83f1919  linked
macOS ARM64               ea2c719c9565d3b6181086f2aab337e7748a4294ea4d09cd8e9f5f59d66ef934  21/21 CTests
```

All three are thin `arm64`. The Simulator and device Mach-O load commands are
respectively `IOSSIMULATOR` and `IOS`, each with iOS 15.0 minimum and SDK 26.5.
The established 32 C warnings repeated; the Objective-C recovery code added no
warning. CTest completed in 0.94 seconds. A unique disposable copy of the
Simulator app was ad-hoc signed: the unsigned executable matched the matrix
hash above, the signed executable was
`a879cae7c58e200097da431f369c24f7bb9a37a33f4a0e976c635795404c94a3`,
and strict/deep verification passed. This is Simulator authorization only, not
an Apple development signature.

### Isolated recovery fixture and controls

The original `CTRPad Import Validation` device was shut down and never edited.
Testing reused the disposable `CTRPad Import Negatives` clone at UDID
`26F3DEE8-8840-446D-85FE-C882009C9C06`. Its valid destination was renamed
inside only the clone to `ctr-u.bin.accepted-before-recovery`, preserving inode
`111313696`, 605,698,800 bytes and SHA-256
`f780bf2331476aabfc00772fa758b12dd95ebfbc907968132cbd3cdd4e2c07c0`.
The clone's memory card remained inode `111309627`, 6,016 bytes and SHA-256
`6a01b0f5562ed7a279d8f8e51e3b1874ac39a6120f55db4fe3873288950619a3`.

Two importer-owned stale directories were then seeded: one contained an
`assets/ctr-u.bin` marker and one was empty. Three negative controls were also
created:

```text
.ctrpad-keep-control          nonmatching directory
.ctrpad-import-               exact prefix, no suffix, directory
.ctrpad-import-control-file   matching prefix and suffix, ordinary 118-byte file
```

Installing the exact app migrated the Simulator's data-container UUID from
`FBB4DE38-856C-43D6-BC82-0D2D1777BAAE` to
`E6915D41-574B-4380-9FCB-2EA255109A3D`; paths were re-resolved before launch.
The asset and save retained their original clone inodes and hashes across that
migration.

Launch PID `81703` removed exactly the two importer-owned stage directories.
All three controls remained with their original types, the final
`assets/ctr-u.bin` destination remained absent, and the retained image/save
inodes, sizes and hashes were unchanged. The enabled onboarding screen visibly
reported:

```text
Recovered 2 interrupted imports. No partial image was installed; choose your
NTSC-U raw BIN to retry.
```

The local-only 2064-by-2752 screenshot
`/private/tmp/ctrpad-import-recovery-exact.png` has SHA-256
`405fd31047bf323ad71a1f325b8c3d154c1c9958ab21adfac983f0d47e464500`.

The accepted image was then moved back to the normal destination without
changing its inode. A deliberate cold launch at PID `81856` bypassed onboarding
and visibly rendered the retail copyright presentation with the complete touch
overlay. The three controls, accepted image and save still survived unchanged.
The local-only screenshot
`/private/tmp/ctrpad-import-recovery-normal-exact.png` has SHA-256
`dff60f6d43e36aff4c252c6848030df2065bd29a20ce407da39484c27be3da8d`.

Finally, the clone was terminated and shut down. The original validation
device was booted and relaunched at PID `82204`; its retail image remained inode
`111131200` and its save inode `111222179`, with the same accepted byte sizes
and SHA-256 values. The original evidence device is therefore still the active
Simulator and the disposable clone remains recoverable but shut down.

This accepts narrow next-launch cleanup, retry messaging, preservation of
similarly named non-owned entries, and normal startup after recovery. The
staging state was seeded to reproduce the post-kill durable condition; a live
Files-provider failure or actual background/termination event during the 605 MB
copy was not performed and remains open, as do physical-iPad import and Apple
signing.
