# iOS sandbox-storage checkpoint — 2026-07-31

## Result

Commit `02a6623f80a0f0999165f97f56c69a7cde64b4aa` separates installed
resources, user-supplied retail media, and private writable state on iOS and
iPadOS:

```text
application bundle                 immutable fallback assets
Documents/CTRPad/assets            user-visible retail import; preferred
Library/Application Support/
  chrissotraidis/CTRPad            logs, memcards and private diagnostics
```

Desktop behavior remains portable: the selected asset base remains the
writable root. A new media-free storage test passes on desktop GL,
GLES-selected macOS, combined ASan/UBSan and optimized i686. Exact ARM64
Simulator and device products contain the Files-sharing metadata, target the
correct Apple platform, and contain no retail data.

A clean 3.4 MB ARM64 Simulator package containing only its executable,
metadata and local signature launched the retail presentation from a
605,698,800-byte raw NTSC-U BIN placed only at
`Documents/CTRPad/assets/ctr-u.bin`. Its console selected the Documents base,
created the log beneath Application Support, initialized GLES 3, all four PSX
shader modes, the VRAM pipelines and CoreAudio, and reached textured rendered
pixels. The retail file, package, logs and screenshot remain local-only.

This is the M9 path and runtime foundation, not M9 acceptance. A fresh install
without media still exits after console validation instead of presenting a
document picker. Invalid-format/region/truncation UI, security-scoped import,
game-driven iOS save creation/reload, background save atomicity, settings and
crash-report policy, and physical-device Files behavior remain open.

## Starting boundary and path audit

The prior exact iOS lifecycle checkpoint returned control to UIKit and
survived Home/resume, but startup still changed the current directory to the
selected asset base. Every ordinary relative writer inherited that directory.
On iOS the fallback asset base is the installed application bundle, which is
not a writable persistence surface.

The audit traced these owners before editing:

- asset discovery and all retail reads through `platform/native_assets.c`;
- the default log through `platform/native_log.c`;
- host-backed PS1 memory cards through `platform/native_memcard.c`;
- performance CSVs through `platform/native_perf.c`;
- savestates through `platform/native_savestate.c`;
- replay reports through `platform/native_replay_scheduler.c`; and
- screenshots/VRAM dumps through `platform/native_renderer.c`.

SDL 3.4.10 already supplies `SDL_GetPrefPath`,
`SDL_GetUserFolder(SDL_FOLDER_DOCUMENTS)` and recursive
`SDL_CreateDirectory`, so no Apple-private path or entitlement was invented.

## Storage contract

`NativeStorage_Init` resolves and normalizes the iOS Application Support and
Documents roots, then creates the private root and public
`CTRPad/assets` directory (`platform/native_storage.c:67-104`). Asset discovery
checks that preferred Documents base before the bundle and its desktop parent
fallbacks (`platform/native_assets.c:535-587`). This ordering is significant:
a user's imported image replaces a stale bundle fallback without modifying the
installed application.

Before retail validation, startup changes into the writable root and gives the
log and memory-card backends explicit absolute paths
(`main.c:421-478`). Remaining relative diagnostics therefore inherit private
Application Support on iOS. The generated Info.plist enables
`UIFileSharingEnabled` and `LSSupportsOpeningDocumentsInPlace`
(`platform/apple/Info-iOS.plist.in:28-39`). Those keys expose the Documents
boundary; they are not described as a completed document-picker workflow.

Desktop finalization deliberately resets the writable root to the selected
asset base (`platform/native_storage.c:116-129`). A normal ignored-retail
desktop launch confirmed that the base, assets and writable data still resolve
to `build-macos-arm64-app`, CoreAudio and desktop GL initialize, and the
existing `memcards/slot0/BASCUS-94426-SLOTS` remains discoverable.

## Deterministic test

CTest 16, `ctr_native_storage`, validates normalized sandbox roots, the
Documents import paths, the private `memcards` path and Windows-style portable
desktop normalization without reading retail media
(`platform/native_storage.c:171-210`, `CMakeLists.txt:292-295`). Its exact
marker is:

```text
[CTR Storage] self-test passed: bundle=read-only preferences=private documents=user-visible imports=preferred desktop=portable
```

This is a deterministic path-policy test. Actual sandbox directory creation,
writes and import selection are established by the live Simulator run below,
not inferred from the marker.

## Diagnostic Simulator process

The first live process used a dirty build identified as
`5d6a3baac1c0-dirty`. A disposable package with the ignored BIN in its bundle
proved the private side first: launch created
`Documents/CTRPad/assets` and wrote `Crash Team Racing.log` beneath
`Library/Application Support/chrissotraidis/CTRPad`. The private log recorded
the software GLES renderer, four PSX shader variants, ready VRAM pipelines,
the UIKit display loop and an 8.39-FPS window.

The first attempt to replace that package with a clean no-resource app is
explicitly rejected as Documents-only evidence. `simctl install` migrated the
Documents file to a new data-container UUID but incrementally retained the old
bundle's `assets/ctr-u.bin`; the installed bundle was 581 MB and its resource
seal no longer matched. That attempt nevertheless showed that the Simulator
migrated the user document through an application update. It does not prove a
no-bundle launch.

The correction uninstalled the disposable app, locally re-signed a fresh
3.4 MB package, installed it into a new container, verified that the installed
bundle held no `assets` directory, and only then cloned the ignored BIN into
the new Documents import directory. The live console reported:

```text
Base:               .../Data/Application/.../Documents/CTRPad
Assets:             .../Data/Application/.../Documents/CTRPad/assets
Writable data:      .../Data/Application/.../Library/Application Support/chrissotraidis/CTRPad
User import assets: .../Data/Application/.../Documents/CTRPad/assets
```

It then reported UIKit framebuffer/renderbuffer 1, Apple Software Renderer,
OpenGL ES 3.0, GLSL ES 3.00, ready 4-bit/8-bit/16-bit/RGBA PSX shaders, ready
VRAM pipelines, the UIKit display loop, native memory arena, and 44.1-kHz
stereo CoreAudio. The first FPS windows were 9.40, 8.30 and 8.96. Simulator
software-renderer cadence remains rejected exactly as in the lifecycle
checkpoint.

A 2064-by-2752 local-only PNG visibly shows the orange Naughty Dog crate with
wood grain, logo lettering, its shaded side face and the starfield. Its
SHA-256 is
`5d47b5661ddfe14b665900d415d45b8ab7fb6b24fc08ef606ac3fff9f25165d5`.
The raw Simulator capture is portrait-oriented while the app presents a
landscape frame, so the content appears rotated 90 degrees. This confirms
textured pixels but does not close the existing rotation/view-controller gate.
No retail-derived screenshot is tracked.

Ctrl-C ended the console-bound `simctl` process and the app was no longer
running. That is a bounded local termination, not proof of a UIKit natural
termination notification.

## Exact clean live process

The committed Simulator product was rebuilt after the source commit, copied to
`/private/tmp/ctrpad-ios-storage-exact.Cb5A7b/CTRPad.app`, locally ad-hoc
signed, and passed strict deep verification. Its locally signed executable
SHA-256 is
`019e004b9793569f88def7dfe766d4b8925cd7da71b82dc2655ebf18d3d08411`.
The installed package remained 3.4 MB and contained no `assets` directory.

Installing it as an update migrated the existing data container from UUID
`39CA500F-9766-4130-B86C-709CE96B1EC7` to
`13DFD41B-6192-4DB1-946C-5964EC8CA796`. The Documents import retained the
same 605,698,800-byte size and inode across that migration. This proves
Simulator document persistence through this app update; it does not by itself
prove physical-device Files behavior or memory-card persistence.

The exact console then reported:

```text
build:              02a6623f80a0
base/assets:        Documents/CTRPad[/assets]
writable data:      Library/Application Support/chrissotraidis/CTRPad
bundle asset:       absent
surface:            1376x1032, framebuffer/renderbuffer 1
renderer:           Apple Software Renderer, OpenGL ES 3.0, GLSL ES 3.00
PSX shaders:        4-bit, 8-bit, 16-bit, RGBA ready
VRAM pipelines:     ready
audio:              CoreAudio, 44100 Hz, stereo, 1024 sample frames
FPS windows:        9.27, 7.15, 8.61
```

The private 16-line log is 794 bytes with SHA-256
`6c2d5fc1b5e30ee471bc7abd2aee1a40aaf1d4d7fe1c7935f6c0379b8b3bb54a`.
The exact local-only 2064-by-2752 PNG clearly shows the wood grain, braces,
logo letters and Crash artwork on the Naughty Dog crate; its SHA-256 is
`7b194105f2f9db5dcf1a91ec6c7525b0e63271c42fe0d51082f19b83fd25b88f`.
It repeats the 90-degree raw-capture orientation. Two unbalanced UIKit
appearance-transition warnings and the duplicate Simulator WebCore/WebKit
accessibility-class message also repeated; neither is concealed or marked
fixed. Ctrl-C ended the bounded console process and PID 88996 no longer
existed afterward. No natural UIKit termination is inferred.

## Exact clean cross-target matrix

All Apple producers were explicitly reconfigured after the implementation
commit and embed `02a6623f80a0`:

```text
macOS ARM64 desktop-GL app
  CTest:       20/20 in 0.39 seconds
  signature:   strict deep ad-hoc verification passed
  architecture: Mach-O 64-bit arm64
  SHA-256:     f6a3bf256034ffb76888f4c9cbcd246f195a9e7d827a4413553528a6fdeaa282

macOS ARM64 GLES configuration
  CTest:       20/20 in 2.66 seconds
  runtime:     expected diagnostic exit 1; Cocoa GLES library unavailable
  architecture: Mach-O 64-bit arm64
  SHA-256:     b4676a7aceaaa960790e5b0b12d14c1dcdc887eddc6e8ddf8fd56b5ff997f5b0

combined ASan/UBSan ARM64
  CTest:       20/20 in 3.94 seconds
  ASan:        detect_leaks=0, halt_on_error=1, abort_on_error=1
  UBSan:       halt_on_error=1, print_stacktrace=1
  finding:     none
  architecture: Mach-O 64-bit arm64
  SHA-256:     37aeba397080d01bebe7cc1d6a80f57da4a7bb88951a88562413e15f1580d262

iOS Simulator ARM64 before local package signing
  platform:    IOSSIMULATOR, iOS 15.0 floor, SDK 26.5
  Files keys:  sharing=yes, open-in-place=yes
  SHA-256:     806c1ec74c7a9f1f1e5543432482d066b970241c5958dc4ea08b93abd866e10f

iOS device ARM64, unsigned and not run
  platform:    IOS, iOS 15.0 floor, SDK 26.5
  Files keys:  sharing=yes, open-in-place=yes
  SHA-256:     d4d083b8275f849ebd1426dad243262d723e0ab41c79c64aa382d4b89137eb18

optimized Linux i686
  CTest:       20/20 in 3.85 seconds
  architecture: ELF 32-bit LSB PIE, Intel 80386, GNU/Linux 3.2.0
  GNU Build ID: e3fab55f8a436052e856dd313a72e08ef78f84b5
  SHA-256:     96158b047af41542bbe1797e1c16d4de5ecc0c51b358ba02ca06a5780b5bdd33
```

The normal Apple/iOS compiles repeated the established 32 warnings and the
sanitizer compile repeated 59 established warnings. The i686 compile repeated
four established warnings. No new implementation warning was accepted. Its
pinned toolchain includes GCC 13.2.0, CMake 3.28.3, Ninja 1.11.1 and
`libc6-dev-i386` 2.39.

## Acceptance boundary

Accepted at this checkpoint:

- iOS bundle, user-visible import and private writable ownership are distinct;
- a valid Documents image takes priority over bundle fallback media;
- the ordinary log and memory-card root receive absolute Application Support
  paths, while other relative diagnostics inherit that writable root;
- desktop portable placement remains compatible;
- a no-retail app bundle launches the production retail path from Documents;
- the Files-sharing keys exist in exact Simulator and device metadata; and
- the storage path contract passes every exact host configuration in the
  matrix.

Still open:

- a fresh-install picker/in-app import screen instead of console exit;
- security-scoped copy/reference behavior and user-facing progress;
- distinct wrong-region, cooked ISO, truncated and inaccessible-image errors;
- game-driven iOS memory-card creation, relaunch reload, background atomicity
  and app-update persistence;
- explicit settings, replay, savestate, screenshot and crash-report product
  policy;
- Files behavior, save behavior and cadence on a physical iPad; and
- the recurring UIKit appearance-transition/rotation warning.

M9 remains in progress. Neither M8 nor M9 is marked complete by this slice.

## Elapsed time and concurrent verifier

The preceding documented checkpoint ended at 155,756 goal seconds. The final
exact-matrix/documentation reading was 158,249 seconds, or 1 day, 19 hours,
57 minutes, 29 seconds cumulative. This slice added 2,493 seconds
(41 minutes, 33 seconds). The goal timer is cumulative product-task elapsed
time, not a labor estimate or benchmark.

The protected historical alternate-loader verifier was not paused, restarted,
rebuilt, terminated or used as this checkpoint's i686 producer. Its completion,
alternate-layout and deliberate-mutation results remain separate acceptance
gates until its own machine-written status is final. At the elapsed-time
reading Docker still reported container `ec58fcd7069c` running, unpaused and
not OOM-killed. Its machine status file remained zero bytes; playback 2 had
crossed ten 2,000-frame FPS windows, most recently 0.48 FPS. No completion is
inferred from that progress marker.
