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
