# CTRPad Decision Log

This log records decisions that constrain later implementation. Detailed
milestone state and acceptance criteria live in `docs/ROADMAP.md`.

## 2026-07-29 — Preserve both project histories

**Decision:** implement on `codex/arm64-apple` in this downstream repository.
Keep `ref/ctr-native` and `ref/ctr-native-android` read-only. Join the existing
CTRPad documentation ancestry with upstream ctr-native's ancestry using a
two-parent merge.

**Why:** the checkout began as a documentation-only repository, while the goal
requires a maintained downstream port and says reference clones must remain
read-only. Commit `268ff6977` preserves provenance for both histories without
copying an unversioned source snapshot.

**Verification:** the merge's second parent is upstream commit `2df55dc5a`.
There is no post-merge source difference from that commit in `CMakeLists.txt`,
`CMakePresets.json`, `main.c`, `game/`, `include/`, `platform/`, or
`externals/`.

## 2026-07-29 — Freeze beta-7.1 as the first evidence baseline

**Decision:** reproduce and measure ctr-native at `2df55dc5a` before changing
its build or memory model.

**Why:** `upstream/master` still matches the viability report exactly. A fixed
baseline makes compiler failures, runtime behavior, and future parity artifacts
attributable.

**Revisit when:** upstream moves or a separable upstream portability fix is
needed. Any update will be an explicit merge or rebase with the baseline
evidence retained.

## 2026-07-29 — Treat retail media as untrusted input

**Decision:** do not rename, link, or depend on the local CloneCD image until
the disc reader validates it. Ignore all common disc and extracted-retail
formats repository-wide.

**Why:** `ref/CTR/CTR.ccd:78-79` and exact sector-size arithmetic establish a
raw MODE2/2352 container, but not NTSC-U identity, revision, completeness, or
loader compatibility.

**Verification:** `.gitignore:8-35` covers the local media directory and known
retail extensions; upstream also ignores `assets/` at `.gitignore:40`.

## 2026-07-29 — Mac ARM64 parity gates iOS

**Decision:** do not debug unresolved pointer-width corruption through an iOS
app shell. A correct, parity-checked Apple Silicon macOS build is required
before the iOS milestone.

**Why:** macOS exercises Apple Clang, ARM64, LP64, and the same core code with
better diagnostics and without signing, lifecycle, and sandbox variables.

**Revisit when:** never as a sequencing shortcut. Renderer research may overlap
the memory work, but iOS acceptance remains gated.

## 2026-07-29 — Serialize native menu shortcuts with pad input

**Decision:** treat the native name-entry scancode as replay input state. Store
it in the three reserved bytes already present in each fixed-size pad snapshot,
without changing the version-2 frame size.

**Why:** the first complete golden playback matched through frame 22,391 and
then omitted the save transition because SDL Enter was read outside the
recorded pad snapshot. A deterministic replay must capture every host semantic
that can change retail game state, even when that semantic eventually
synthesizes an ordinary retail button.

**Compatibility:** marker-present snapshots distinguish a keyboard shortcut
from physical gamepad Start. Legacy keyboard-driven reports can recover Enter
from their raw Start bit; other legacy name-entry scancodes cannot be inferred
and are not guessed.

## 2026-07-29 — Do not treat image-offset relocation as cross-build portability

**Decision:** replay identity bypass remains diagnostic-only when code layout
changes. Cross-architecture checkpoints require stable symbolic callback
identities rather than an image-base delta.

**Why:** unity compilation shifted function offsets after a small source edit.
A captured callback offset naming `Particle_FuncPtr_SpitTire` in the clean
recording binary landed inside `MATH_Matrix_TrigSinCos` in a rebuilt binary.
ASLR relocation and cross-build relocation are different problems.

**Revisit when:** M5 versions the checkpoint format and every serialized code
pointer has a stable ID with a checked live-function resolver.

## 2026-07-30 — Bind native replays to the exact executable

**Decision:** replay format version 3 stores a 64-bit FNV-1a fingerprint of
the running executable and includes it in header identity. Recording and
playback are disabled if the executable cannot be identified. A mismatch is
reported before any checkpoint is restored.

**Why:** two materially different local builds both advertised
`a40a7584c576-dirty`, so the old build string and equal checkpoint sizes
accepted incompatible unity-build callback layouts. The correct copied binary
restored and replayed the same report, proving that ASLR was not the problem.

**Compatibility:** version-2 reports remain tied to their producing binaries.
Version 3 intentionally rejects them and any different executable, even when
the source commit label and state sizes match. `--replay-bypass-header` remains
diagnostic-only and cannot produce acceptance evidence.

**Verification:** report `ctr-065640` records fingerprint
`b212b34099a6dbca`; its exact binary restored at a different process layout
and completed 2,200 frames. An earlier runnable version-3 binary with
fingerprint `dd9fef13fb4ae04a` rejected the report before restore.

## 2026-07-30 — Relocate each typed checkpoint slot at most once

**Decision:** the applied-relocation ledger is also the restore transaction's
idempotence set. Once a resident or image pointer slot has been relocated, a
second semantic walker must leave that slot unchanged.

**Why:** old and live virtual-address ranges can partially overlap. A pointer
relocated into the live range may still numerically fall inside the recorded
range. Reinterpreting that already-live value as an old-process pointer applies
the base delta twice. Thread, instance, rain, driver, and object walkers
intentionally overlap, so eliminating every duplicate traversal is fragile.

**Implementation:** a fixed-capacity open-addressed index keys the existing
65,536-entry applied-relocation ledger by slot address. Restore performs no
allocation, preserves the hard overflow rejection, and retains the old and
expected values used by stale-pointer validation.

**Verification:** the pointer-validation self-test uses overlapping synthetic
old/live ranges and invokes relocation twice on one slot. The second visit is
a no-op. In the sanitizer campaign, report `ctr-081155` recorded mempack base
`0x106ddb3a0`; an exact checkpoint-6 playback at `0x106c3b3a0` overlapped that
recorded arena and still completed frame 2,200 without an ASan/UBSan finding.

## 2026-07-30 — Record the complete VSync boundary in replay version 4

**Decision:** replay version 4 assigns every emitted VSync call to a tracked
frame, including calls made by asynchronous loading before that frame's
`BeginFrame`. Preserve the 440-byte record by splitting `vblankPacketCount`
into a 16-bit pre-frame prefix count and 16-bit total count, and encode adjacent
equal calls as low-byte value/high-byte repeat-count packets.

**Why:** versions 2 and 3 captured only calls made while a tracked frame was
open. An i686 source could therefore execute additional loader VSync calls that
an ARM64 `--record-from-replay` run never received. The pad stream still
worked, but RNG and later race state were compared at different points in the
boot timeline. Version 4 makes the seed a complete temporal boundary without
growing the file or checkpoint layouts.

**Compatibility:** readers and the component comparer accept versions 2
through 4. Versions 2 and 3 remain pad-only seeds because the omitted
pre-frame calls cannot be inferred. Strict replay-seeded timing requires
version 4 and logs `complete-vsync=yes`.

## 2026-07-30 — Keep decoded cutscene opcodes retail-shaped

**Decision:** `CsOpcodeArg` is a four-byte decoded guest value on every host.
Pointer-using opcode branches convert its `u32` member explicitly through
`uintptr_t` at the point of use. `CsOpcodeMeta` has unconditional offset and
size assertions for the retail 20-byte record.

**Why:** including `char *` in the argument union widened `CsOpcodeMeta` from
20 to 32 bytes on LP64. The decoder wrote `arg1` at native offset `0x10`, then
cleared retail rotation slots `0x10..0x13`, erasing the argument. Six intro
models consequently received animation endpoint zero and advanced the ARM64
RNG stream at different frames. These are decoded retail bytecode words, not
host pointer owners.

## 2026-07-30 — Canonical digests hash gameplay allocation semantics

**Decision:** digest schema 2 treats a driver as live only when its object slot
belongs to the current large-stack pool and is absent from that pool's free
list. Allocation hashing retains pool populations/capacities and mempack
lifecycle topology: initialized cursors, empty/full relationships,
previous-allocation existence, and bookmark depth. It excludes physical
coordinates and sizes, host-only pool strides, pool byte sizes, and the
LP64-expanded main-pack start/size.

**Why:** `gGT->drivers[]` can retain non-null pointers during menu and loader
transitions after those slots have been freed or replaced. Hashing the reused
bytes produced short ARM64/i686 mismatches despite identical timing, RNG, and
world state. Likewise, wider host pointers deliberately increase LP64 pool
strides. `MainInit` compensates the maximum widened-pool overhead so remaining
capacity pressure is retail-equivalent, but widened allocations elsewhere
still shift physical cursors and bookmarks. Allocation lifecycle, not those
host coordinates, is therefore canonical.

**Verification:** the media-free state-digest test proves that a live driver
position mutation changes only `drivers`, while different bytes in matched
free driver slots do not change any component. It also proves different host
pool strides and mempack coordinates do not change the canonical allocation
record.

## 2026-07-30 — Keep derived language pointers outside retail MEMPACK

**Decision:** LP64 keeps the language file's retail-sized `0x3f04` allocation
in MEMPACK, but materializes its host-width string-pointer table in fixed
native storage. Checkpoint restore rebuilds the table from the captured
language file's serialized `u32` offsets instead of persisting native table
bytes.

**Why:** appending the maximum `char *` table to the MPAK allocation consumed
32,268 more bytes than i686, including alignment. Every allocator bookmark
after language initialization carried that exact displacement. The first
24,232-frame ARM64 regeneration consequently reached a later level load with
86,264 bytes free, failed a 96,000-byte clip-buffer request by 9,736 bytes,
and entered the retail allocation-error loop at replay frame 22,156. The table
is a derived LP64 representation, not retail gameplay data, so charging it to
the retail-pressure arena was incorrect.

**Checkpoint rule:** `lngFile` remains serialized and relocated normally.
`lngStrings` is not relocated as captured LP64 storage; restore validates the
count, table extent, and every string offset before rebuilding the native
table.

## 2026-07-31 — Split iOS media, documents, and private state

**Decision:** on iOS/iPadOS, treat the installed application bundle as an
immutable fallback asset source, prefer user-supplied retail media beneath
`Documents/CTRPad/assets`, and place private writable state beneath the
Application Support directory returned by
`SDL_GetPrefPath("chrissotraidis", "CTRPad")`. Keep the established portable
desktop behavior, where assets and writable files remain beside the selected
asset base.

**Why:** the application bundle is read-only after installation, while retail
disc media must remain user-owned and must never ship in CTRPad. Files-visible
Documents is the appropriate staging boundary for that media; logs, memory
cards, replay reports, savestates, and other implementation state should not
clutter the import surface. The split is resolved before asset validation and
before any relative diagnostic writer runs (`platform/native_storage.c:67-168`,
`platform/native_assets.c:535-587`, `main.c:421-476`).

**Compatibility:** `NativeStorage_FinalizeForAssetBase` deliberately maps the
desktop writable root back to the selected asset base
(`platform/native_storage.c:116-129`). iOS advertises Files access through
`UIFileSharingEnabled` and `LSSupportsOpeningDocumentsInPlace`
(`platform/apple/Info-iOS.plist.in:28-39`). Checkpoint `02a6623f80a0` created
and used the import location; checkpoint `7872f7e61ad6` subsequently added the
fresh-install document picker and distinct format/region/content error paths.

**Verification:** media-free CTest 16 checks normalized sandbox, writable,
import, and portable path contracts (`platform/native_storage.c:171-210`,
`CMakeLists.txt:292-295`). Live Simulator evidence is recorded separately in
`docs/parity/2026-07-31-ios-sandbox-storage.md`; no retail byte or screenshot
is committed.

## 2026-07-31 — Validate a staged Files copy before replacing retail media

**Decision:** when iOS starts without valid media, return from `SDL_main` and
let a native UIKit coordinator own a nonblocking onboarding window and
`UIDocumentPickerViewController`. Request a Files copy, coordinate the
security-scoped read, copy it into a unique hidden directory beneath
`Documents/CTRPad`, validate it through the production disc/asset loaders, and
only then move or replace `assets/ctr-u.bin` on the same volume. After success,
reselect the installed image and enter the ordinary runtime startup path in the
same process (`main.c:312-486`, `main.c:635-656`,
`platform/apple/native_ios_import.m:170-347`).

**Why:** the 605 MB retail image cannot be trusted merely because Files returns
a URL or filename. Validation before replacement preserves a previously good
import, same-volume staging gives the final installation an atomic filesystem
boundary, and returning control to UIKit avoids blocking the application event
loop while the user browses. Using the production loader prevents the UI from
inventing a weaker interpretation of MODE2/2352, NTSC-U identity, or required
game contents.

**Failure behavior:** cancellation returns to the chooser. Format, wrong-region
and incomplete-content failures have distinct user-facing text; Files access,
copy, staging and installation errors retain their underlying localized error.
Every failure removes the unique staging directory. The explicit
`NativeDiscImage_Shutdown` closes the staged image before the coordinator moves
it (`platform/native_disc_image.c:407-418`).

**Verification boundary:** a fresh iPad Simulator exercised cancellation,
invalid-format rejection, successful import, same-PID game startup and cold
relaunch. Physical-device access, interruption during a large copy, and the
wrong-region/incomplete/inaccessible branches are not yet accepted. Exact
evidence is in `docs/parity/2026-07-31-ios-files-import.md`; no retail byte,
package or screenshot is committed.

## 2026-07-31 — Replace memory-card files only after a durable temporary write

**Decision:** write each memory-card update to a hidden file in the final
save's directory, flush it through the strongest available platform primitive,
close it, and atomically replace the final path. Remove the temporary file on
every failure and never open the existing final save for truncation before the
replacement boundary (`platform/native_memcard.c`).

**Why:** the earlier `fopen(path, "wb")` sequence destroyed the previous save
before the new icon/profile bytes were complete. iOS can suspend or terminate
an application between those operations. A same-directory transaction gives
the filesystem an old-or-new final-name boundary and preserves the previous
save when open, write, flush, close, or rename fails. Apple uses
`F_FULLFSYNC` with `fsync` fallback; Windows uses `_commit` and write-through
replacement; other POSIX targets use `fsync` and `rename`.

**Verification boundary:** CTest 17 writes, replaces, reads back, injects a
temporary-path open failure, proves the preceding final payload is unchanged,
retries, and proves no temporary residue remains. Clean ARM64 Release and
ASan/UBSan pass 21/21; exact i686 flags compile the writer cleanly; iOS
Simulator and device ARM64 products link. Game-driven iOS save/relaunch and
physical-flash behavior are separate product evidence and are not inferred
from this unit oracle. Full evidence and rejected routes are in
`docs/parity/2026-07-31-ios-memory-card-atomicity.md`.

## 2026-07-31 — Compose touch as a player-one peer and retain edges for two host snapshots

**Decision:** implement iOS/iPadOS touch as a native safe-area-aware UIKit
overlay and compose it into player one's existing PS1-shaped pad snapshot.
Touch buttons combine active-low with controller and keyboard buttons. An
active touch stick replaces only the left analog axes, preserving the right
stick and all physical-controller buttons. The stick's continuous analog
position remains available for racing, while its outer ring emits D-pad edges
for retail menus (`platform/apple/native_ios_touch.m:89-117`,
`platform/native_input.c:432-479`, `platform/native_input.c:549-579`). Keyboard
and touch press edges remain
eligible for two consecutive host snapshots before returning to neutral
(`platform/native_input.c:487-500`, `platform/native_input.c:549-555`,
`platform/native_input.c:783-789`).

**Why:** a separate touch-only game path could diverge from retail input,
physics, replay, and controller semantics. Composing at the established pad
boundary keeps one gameplay transport and permits controller-plus-touch use.
The menu layer reads D-pad buttons rather than analog steering, so an outer
ring is needed without sacrificing real analog race input. Live Simulator
LLDB tracing proved that a one-host-snapshot edge could be assembled correctly
and still be replaced by a neutral host update before the next retail poll;
two snapshots bridge that observed host/display-to-retail cadence without
inventing a permanently held input.

**Verification boundary:** the retail-free input oracle proves analog axes,
D-pad direction, multi-button chords, quick taps across two snapshots followed
by neutralization, and controller-peer preservation. Exact `c783eda740c4`
builds pass 21/21 on macOS
ARM64 and combined ASan/UBSan, compile the input unit under exact optimized
i686 flags, and link thin ARM64 iOS Simulator/device products. A signed exact
Simulator build visibly navigates Adventure -> Load and displays saved profile
`A` using touch alone. Physical-iPad ergonomics, frame cadence, rotation,
development/distribution signing, full touch-only race completion and
drift/boost feel remain separate device acceptance. Full evidence is in
`docs/parity/2026-07-31-ios-touch-controls.md`.

## 2026-07-31 — Adapt to iPadOS 26 scenes instead of forcing landscape

**Decision:** retain `UIRequiresFullScreen` and the SDL landscape hint for
iPhone and older full-screen iPadOS behavior, add
`UIRequiresFullScreenIgnoredStartingWithVersion=26`, and declare all four iPad
orientations. On iPadOS 26 and later, let the existing renderer resize to the
scene and let the UIKit touch overlay reflow through safe-area Auto Layout
(`platform/apple/Info-iOS.plist.in`, `platform/native_platform.c`).

**Why:** the iPadOS 26 Simulator explicitly reported that
`UIRequiresFullScreen` will be ignored and all orientations will be required.
Apple documents the key as deprecated because current iPad scenes support
windowing and dynamic resizing; a scene may not visually rotate when its
interface-orientation preference changes. The correct compatibility contract
is therefore an adaptive scene, not a private or repeatedly ineffective
geometry-forcing workaround. See Apple's
[UIRequiresFullScreen reference](https://developer.apple.com/documentation/bundleresources/information-property-list/uirequiresfullscreen)
and
[TN3192 migration note](https://developer.apple.com/documentation/technotes/tn3192-migrating-your-app-from-the-deprecated-uirequiresfullscreen-key?changes=_3_3).

**Verification boundary:** exact commit `db45004f909d` cold-launched in a
portrait iPadOS 26 Simulator scene with a coherent centered 4:3 game surface
and safe-area controls, then reflowed to fill a 932-by-768 landscape scene and
back to portrait. The runtime-issues subsystem reported no configuration
fault. Thin ARM64 Simulator and device products linked, and the macOS ARM64
suite passed 21/21. This accepts the adaptive Simulator layout and generated
configuration; real-iPad window management, rotation feel and signing remain
physical-device gates.

## 2026-07-31 — Package a retail-free reproducible IPA and keep signing user-owned

**Decision:** build the thin device bundle with the existing CMake/Ninja preset,
embed the GPL, third-party notices and iOS Installation Information as bundle
resources, and package it through `package-ios.sh`. Normalize all staged mtimes
to `SOURCE_DATE_EPOCH` (the source commit time by default), reject retail-like
files and runtime containers, and refuse to overwrite an existing output. When
the user supplies both an identity and profile, validate the profile and sign
with a minimal exact entitlement set; otherwise produce an explicitly unsigned
IPA for a compatible re-signing tool.

**Why:** a generated `.app` is not yet a reproducible or GPL-complete release
artifact, and Apple credentials must never enter Git or a generic release
archive. Apple's provisioning model binds the signer, App ID, devices, expiry
and allowed entitlements. The user's profile and keychain identity therefore
remain late-bound inputs while the source-controlled workflow proves every
non-secret part of the artifact. See Apple's
[TN3125](https://developer.apple.com/documentation/technotes/tn3125-inside-code-signing-provisioning-profiles?changes=_5)
and
[current signature-format guidance](https://developer.apple.com/documentation/xcode/using-the-latest-code-signature-format).

**Verification boundary:** exact commit `207121134a05` produced a device ARM64
executable at SHA-256 `62e8148d...64be9` and two byte-identical unsigned IPAs
at `78b93b01...e0c63`. Archive extraction proved the seven-member
`Payload/CTRPad.app` tree, exact legal/install resources, iOS 15.0 minimum,
SDK 26.5, thin ARM64, and absence of a signature, profile, retail-like file or
runtime directory. An injected `ctr-u.bin` was rejected. A local-only ad-hoc
probe proved CodeDirectory v20400 and the four intended DER entitlements after
a dotted-key construction bug was corrected. This does not prove that an Apple
profile authorizes the app or that a real iPad installs/runs it; those require
the user's identity, profile and device.

## 2026-07-31 — Recover only reserved interrupted-import directories on launch

**Decision:** use one shared `.ctrpad-import-` staging prefix for creation and
recovery. Inspect only the direct children of `Documents/CTRPad` both before
presenting media-free onboarding and before starting the runtime with an
already-valid asset. Remove an entry only when it is a directory, starts with
that reserved prefix, and has a nonempty suffix. Leave ordinary files, the bare
prefix, nonmatching entries, the installed asset and all private Application
Support state untouched. The onboarding path reports the number removed in the
existing status label and leaves the chooser enabled; the valid-asset path logs
the count and continues into the game
(`platform/apple/native_ios_import.m:31-32,142-184,199-230,424-439`,
`main.c:675-689`).

**Why:** every handled copy/validation/install failure already deletes its
unique stage, but a process kill or OS termination can prevent those handlers
from running. The destination is not installed until validation succeeds, so a
surviving stage is disposable on the next launch. Cleanup cannot be limited to
media-free onboarding because termination after destination installation but
before final stage removal can leave both a valid asset and an empty stage.
Restricting cleanup to the importer-owned directory namespace avoids treating
Documents as a scratch area or deleting a similarly named user file.

**Verification boundary:** exact commit `c745390a55eb` first removed two
isolated seeded stages and visibly reported both recoveries while preserving
three controls: a nonmatching directory, the exact bare-prefix directory, and
a same-prefix ordinary file. A later real Files selection produced a complete
605,698,800-byte stage with the accepted hash; an event-driven local diagnostic
sent `SIGKILL` to that exact app PID before validation/install, and the next
launch removed the surviving stage while reporting one recovery. The retained
image and 6,016-byte save preserved their inodes and hashes, and restoring the
image produced a normal cold launch. This accepts Simulator process-death
recovery during an import. Follow-up commit `8c177e8327f3` exposed the same
narrow cleanup before runtime startup. An exact signed build launched with a
valid image plus a seeded reserved stage, removed only that stage, preserved all
three controls and the BIN/save identities, and visibly entered the game; a
cold relaunch remained clean. It does not claim an inaccessible-provider
callback, physical-iPad transfer interruption, or Apple signing.

## 2026-08-01 — Package corresponding source from one clean Git commit

**Decision:** create the release source tarball with `package-source.sh` from
an exact clean Git commit, not by traversing the working directory. Require the
build system, vendored SDL, licenses, Installation Information, modification
records and both packagers; fail on retail/runtime/package/profile/certificate/
key-like members. Use a commit-derived archive root and `gzip -n`, prepare and
hash the complete archive in private staging, then publish the tarball and its
basename-relative SHA-256 sidecar without overwriting an existing output.

**Why:** the future IPA needs a versioned source artifact that a recipient can
match to its embedded build identity. A working-tree copy could accidentally
include ignored media, saves, build output or credentials and could silently
omit uncommitted changes. Git's committed-object boundary selects the complete
published source and stable metadata without reading ignored files. Staging
both archive and sidecar prevents an interrupted hash from being mistaken for
a completed release pair.

**Verification boundary:** corrected exact commit `4091b602ab2a` produced two
byte-identical 17,482,944-byte, 3,233-member source archives at SHA-256
`d1c4b4fb...64df1`; both sidecars, gzip streams and tar listings passed. Fresh
extraction preserved executable packagers, contained no `.git` or prohibited
member, passed both shell syntax checks and enumerated all four Apple CMake
presets. Low host headroom prevented a responsible extracted-source compile,
so clean-machine build, legal review, same-identity IPA pairing, Apple signing
and physical-device acceptance remain open. Exact and rejected-route evidence
is in `docs/parity/2026-08-01-corresponding-source-package.md`.

Follow-up `21fb81296bd0` explicitly extends the ingress guard to Apple `.p8`
and `.pfx` keys, alternate provisioning-profile suffixes and common certificate
encodings. Fully documented exact head `71b68f118b18` then repeated two
byte-identical 17,488,503-byte, 3,234-member archives at SHA-256
`1681c458...53f3`, with both sidecars, expanded scans and extraction smoke
checks passing.

## 2026-08-01 — Select a signing keychain explicitly without global mutation

**Decision:** let `package-ios.sh` accept an optional `--keychain PATH` only
with identity/profile signing. Search that unlocked keychain explicitly for a
valid code-signing identity and pass the same path to `codesign`; never change
the user's default keychain or keychain search list from the packager.

**Why:** a release or CI identity may live outside the login keychain. Requiring
global search-list mutation makes isolation weaker and cleanup harder. Explicit
selection scopes both discovery and signing while preserving the existing
valid-identity/profile checks and leaving unlock/authentication policy with the
keychain owner.

**Verification boundary:** exact commit `37a5e16760ba` rejects a keychain
without identity/profile and rejects a present but untrusted synthetic identity
without producing an IPA. Two unchanged unsigned packages are byte-identical at
`582b8491...b929`. A separate synthetic CMS and ad-hoc DER diagnostic strictly
verified all four intended entitlements but explicitly had no TeamIdentifier;
it is not Apple signing evidence. The temporary keychain was deleted and user
search/trust state remained unchanged. A valid Apple identity/profile and
physical device remain required. Full evidence is in
`docs/parity/2026-08-01-ios-isolated-keychain-signing.md`.

## 2026-08-01 — Make Simulator stability the physical-device gate and retain logs

**Decision:** do not proceed to a physical iPad until an exact clean Simulator
build is visually coherent and operationally stable across repeated retail
scene/level transitions, mapped keyboard and touch input, rotation and
background/foreground. Retain the current native application log plus four
prior sessions with UTC, elapsed-session and severity context, flushing every
entry.

**Why:** compile success and one coherent frame cannot detect recycled asset
corruption, intermittent input or lifecycle failure. Overwriting the previous
log also destroys the before/after evidence needed for intermittent graphical
failures. Four retained sessions provide a bounded useful history without an
unlimited Application Support footprint.

**Verification boundary:** exact baseline `43245107c279` rendered the inspected
title/menu/Crash Cove frames coherently, but ran at roughly 4-8 FPS and exposed
intermittent short actions plus destructive single-session logging. Exact
logging checkpoint `7bcc51a790a1` retained real sessions but exposed that its
two-VSync quick-input latch could expire before the retail consumer. Exact
consumer checkpoint `ba80d153ae55` made 20/20 keyboard edges reach
`GAMEPAD_ProcessHold`, churned coherent Crash Cove and Roo's Tubes plus
rotation/Home-resume, and retained five logs without known asset/cache/app-
fault markers. This accepts that bounded input/visual/lifecycle route. It does
not accept average 7.37 FPS, all levels/effects, human multi-touch or a physical
device; those gates remain open. The initial cleanup used
`com.chrissotraidis.ctrpad` instead of plist identifier
`io.github.chrissotraidis.ctrpad`, so its `found nothing` response is rejected
as command-target error rather than app-exit evidence. Correct-ID termination
remains to be observed.

## 2026-08-01 — Resolve packed VRAM at logical size before host scaling

**Decision:** preserve the existing integer packed-PS1-VRAM decode, but execute
it once into a reusable RGBA framebuffer at the logical display size. Scale
that resolved image to the host presentation viewport with nearest framebuffer
blit. Keep the former direct path as a byte-comparison oracle and require the
GLES loader to provide `glBlitFramebuffer`.

**Why:** measured Crash Cove work assigned 47.201 ms per frame to the old final
presentation path because the decode shader ran over the complete 1032×1376
Simulator surface. Logical resolve removes redundant integer decoding without
changing PS1 texture, color, STP, blend, mask or feedback semantics. A native
blit is an explicit scale operation and fails at initialization if unsupported.

**Verification boundary:** the direct and staged 2× pixel captures compare
byte-for-byte with present hash `a7798c5a6ddee965`; the established logical
hash remains `851169f2644a1675`. All 22 native tests pass. One-Simulator live
inspection found coherent logo/menu, character, track, fly-in and Crash Cove
race pixels with clean rotating logs. Crash Cove present time fell to 7.737 ms
and total frame time to 181.024 ms. This accepts the staged algorithm, not the
full Simulator gate: split submission still costs 131.012 ms per frame, broader
asset churn and exact post-commit replay remain open, and physical-device work
remains prohibited. Correct-ID termination was observed in this run. Full
evidence is in `docs/parity/2026-08-01-simulator-renderer-profile.md`.

## 2026-08-01 — Reject unified fetch-state batching on live frame cost

**Decision:** retain the published same-state coherent-fetch batcher. Do not
merge texture-format or blend/STP/mask transitions through per-primitive shader
state on the Apple Software Renderer, even though both tested designs preserve
the exact pixel oracle and reduce host draw calls.

**Why:** the matched 119-split / 78-semitransparent published state averages
66 calls and 165.879 ms. A single dynamically branched shader uses 26 calls but
248.567 ms; three texture-format-specialized shaders use 52 calls but 236.441
ms. The best prototype is 42.54% slower total and 39.85% slower in triangle
submission. API-call count is diagnostic, not an acceptance criterion.

**Verification boundary:** both prototypes passed the established logical,
blend, fallback and actual-surface iOS hashes and rendered the bounded retail
route coherently. Correct-ID termination preserved full profiles/logs and
retail/save identities. All candidate source was restored; the restored macOS
ARM64 tree repeats the established 32 warnings, passes 22/22 tests and the
independent pixel oracle. Full chronology is in
`docs/parity/2026-08-01-ios-unified-fetch-state-rejection.md`. A different
optimization must pass the same matched-state performance gate before it is
retained.

## 2026-08-01 — Retain the RGB5551 lookup texture

**Decision:** keep the accepted 256x256 RG8-to-RGBA lookup texture in the
4/8/16-bit PS1 fragment paths. Do not replace it with the tested high-precision
integer RGB5551 decoder on the Apple Software Renderer.

**Why:** direct decode is logically attractive because nearest filtering avoids
one dependent lookup and bilinear filtering avoids four. It preserved the
historical `channel5 << 3` color and `STP << 7` alpha semantics, every desktop
and iOS pixel hash, draw/split structure, and the bounded retail appearance.
Nevertheless, the exact 66-call / 119-split / 78-fetch / 56-merge Crash Cove
state rose from 187.002 to 198.412 ms. Triangle time rose from 144.237 to
153.395 ms. Removing a texture access is not an optimization when the target
software rasterizer handles integer bit manipulation more slowly.

**Verification boundary:** the dirty macOS build passed 22/22 tests and the
desktop pixel oracle; the actual 1032x1376 iOS surface passed the coherent-
fetch-versus-fallback oracle and established logical/blend/presentation hashes;
Computer Use showed coherent menu/character/track/ghost/fly-in/grid content;
two rotating logs had zero targeted faults; update installation preserved the
retail BIN and save; and 248 matched candidate frames were compared with 200
accepted frames. The candidate source was restored before publication. Full
chronology is in
`docs/parity/2026-08-01-ios-direct-rgb5551-decode-rejection.md`.

## 2026-08-01 — Move cadence and touch-feel acceptance to physical iPad

**Decision:** retain the iPad Simulator as the ARM64 build, UIKit/GLES,
pixel-semantics, bounded visual, import/save, input/lifecycle and logging gate.
Do not require the Apple Software Renderer to meet retail 30-FPS cadence before
starting physical-device validation. Sustained cadence, frame pacing, thermals
and multi-touch drift/boost ergonomics are accepted only on the target iPad.

**Why:** the Simulator route is now coherent and diagnosable on the inspected
two-track churn, while matched profiles remain about five times over the retail
frame budget. The evidence proves a software-rasterizer bottleneck, not a
physical-iPad GPU result. Continuing to optimize that proxy delays the only
test capable of resolving the real release risk. This changes ownership of the
performance gate; it does not waive Simulator correctness or logging.

**Verification boundary:** before device work, rebuild the clean accepted
source with all Simulators shut down, pass the native and renderer-pixel tests,
update-install it on exactly one Simulator, replay the bounded route, terminate
the exact bundle identifier and inspect retained logs. Physical acceptance then
requires a user-owned signed install, retail Files import, complete touch race
including a three-boost drift, audio/lifecycle/update/save persistence, cadence
and thermal evidence. Exact scope and contingency are in
`docs/history/RELEASE-REBASELINE.md`.

Clean `d5772375fabc` now accepts the local Simulator side of this boundary:
22/22 tests, desktop and actual-surface pixel hashes, update preservation,
coherent live Crash Cove, keyboard/touch retail consumption,
Home/foreground/rotation, five-log fault scan and exact-ID shutdown all pass.
The thin iPhoneOS executable and unsigned IPA/source pair also build. The one
self-test-exit UIKit console warning and every real signing/physical-device
criterion remain open. Exact evidence is in
`docs/parity/2026-08-01-release-rebaseline-clean-smoke.md`.

## 2026-08-01 — Do not distort production lifecycle for self-test teardown

**Decision:** retain the single immediate renderer-self-test UIKit appearance
warning as a documented Simulator harness limitation. Do not manually drive
UIKit appearance methods, spin an arbitrary nested run loop, skip platform
shutdown or add an iOS-only asynchronous renderer-test state machine without
physical production evidence requiring it.

**Why:** the pixel test creates, exercises and destroys its window inside one
synchronous `SDL_main` call, before returning to UIKit's run loop. Production
returns with the display loop active and has repeatedly completed startup,
Home/foreground, rotation and bounded termination without the warning. The
candidate workarounds either manipulate private transition state, weaken
deterministic cleanup or add test-only platform complexity.

**Verification boundary:** clean `d5772375fabc` passes the complete
actual-surface GLES oracle before the warning, then separately passes the
normal retail lifecycle and five-log fault scan. Reopen this decision if the
signed physical application emits the warning on its normal route. Detailed
source ownership and rejected alternatives are in
`docs/parity/2026-08-01-ios-uikit-view-lifecycle.md`.

## 2026-08-01 — Require installed-hash identity for Simulator evidence

**Decision:** install Simulator products through
`tools/install-ios-simulator.sh` and accept live evidence only after the
installed executable SHA-256 equals the isolated signed staging executable.
Refuse installation unless exactly one Simulator is booted.

**Why:** an older installed build rendered plausible retail assets but lacked
the current input latch and diagnostic strings. Visual similarity and bundle
ID were insufficient to establish product identity. A build-tree bundle can
also carry a stale resource signature after resource generation, so the helper
must copy and sign rather than mutate or trust that source bundle.

**Verification boundary:** the helper rejected an explicitly requested
shutdown device without installing, then update-installed current executable
`c6d40aaf...187f` on the sole booted validation device. Installed and staged
hashes matched; the retail image and save retained exact inode, size and
SHA-256; the source executable remained `1699c36d...091`; the app relaunched
and rendered current controls with clean targeted logs. This is an ad-hoc
Simulator procedure only and supplies no physical-device signing evidence.
Full evidence is in `docs/parity/2026-08-01-exact-simulator-install.md`.

## 2026-08-01 — Separate physical preflight, device mutation and human acceptance

**Decision:** run the signed-iPad release campaign through distinct
`ios-device-campaign.sh preflight`, `prepare` and `collect` phases. Consume
only versioned `devicectl --json-output` files for automation, never scrape its
human output. Keep raw device evidence ignored and commit only a reviewed,
redacted copy of `docs/templates/IOS-DEVICE-ACCEPTANCE.md`.

**Why:** signature/profile correctness can be proven before touching a device;
install and launch have machine-readable CoreDevice outcomes; touch feel,
complete-race play, audio and thermal behavior require a human on hardware.
Combining those evidence classes would encourage an offline pass to masquerade
as a device pass. Raw JSON/save data also contains identifiers or user state
that should not enter Git.

**Verification boundary:** local syntax and privacy-path guards pass; the
unsigned IPA reaches and fails the missing-profile boundary after valid
sidecar/ZIP checks; a nonexistent device exits with CoreDevice error 1000 and
writes JSON version 3. No signed preflight, install, launch or collection has
passed because this Mac has no identity, profile or physical device. The
workflow never uninstalls and collection excludes the Documents retail tree.
Collection also rejects any evidence root without a successful prepare
manifest matching the exact requested UDID and bundle identifier.
Exact evidence is in
`docs/parity/2026-08-01-ios-physical-campaign-handoff.md`.
