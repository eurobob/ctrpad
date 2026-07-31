# CTRPad Engineering Journal

This is the chronological, evidence-backed record of how the CTRPad port was
made. It complements the roadmap, architecture decisions, build records, and
parity reports by connecting them into one continuous account: what was
attempted, the exact result, why a decision was made, and what remained
unproven at that point.

## Journal protocol

This file is a required project artifact.

- Add an entry for every material implementation or validation session.
- Record the source commit/branch, commands or checked-in reproduction script,
  evidence path or hash, observed result, decision, and unresolved limitation.
- Record failed approaches. Do not erase them after a better approach is found.
- Correct an earlier entry with a dated correction rather than silently
  rewriting history.
- Distinguish `observed`, `validated`, and `accepted`. A compile is not parity
  evidence, and a temporary diagnostic bypass is not a supported workflow.
- Never commit retail-derived content. Retail filenames, geometry, boot ID, and
  cryptographic hashes are evidence; the files and screenshots remain ignored
  local material.
- Temporary paths document where evidence existed during the session. Durable
  acceptance evidence must also be summarized under `docs/`.

The task-level source of truth remains [ROADMAP.md](../ROADMAP.md). Detailed
technical decisions live in [DECISIONS.md](../DECISIONS.md) and
`docs/architecture/`; detailed parity evidence lives in `docs/parity/`.

## 2026-07-29 — Repository inventory and viability boundary

**Starting source:** `main` at `95417c723`

**Observed**

- The repository contained only `.gitignore`,
  `docs/ctr-native-viability.md`, and `ref/README.md`.
- The source trees described by `ref/README.md` were not present. There was no
  `CMakeLists.txt`, `main.c`, `game/`, `include/`, `platform/`, or vendored
  SDL source in the checkout.
- The host was Apple Silicon (`arm64`) with macOS 26.5, Xcode 26.6, Apple
  Clang 21.0.0, macOS/iPhoneOS/iPhoneSimulator 26.5 SDKs, CMake 3.27.1, and
  Ninja 1.13.2.
- The original ignored retail set was CloneCD-shaped: one MODE2 track with a
  740,179,104-byte `.img` and a 30,211,392-byte `.sub`.

**Method**

The viability report was read in full before implementation. The repository
and host were inventoried with Git file/status queries, filesystem listings,
tool version queries, `uname`, and Xcode SDK queries. The result was written
to [ROADMAP.md](../ROADMAP.md) before source work began.

**Decision**

The dependency order was fixed as:

```text
M0 repository/evidence foundation
  -> M1 reproducible i686 baseline and parity gate
  -> M2 64-bit memory model
  -> M3 runtime pointer/layout conversion
  -> M4 serialized asset conversion
  -> M5 scratchpad/checkpoint conversion
  -> M6 macOS ARM64
  -> M7 GLES
  -> M8 iOS/iPadOS lifecycle and controller input
  -> M9 import/storage
  -> M10 touch
  -> M11 signed packaging/publication
```

Mac ARM64 is intentionally the diagnostic gate before iOS. Asset import,
touch UI, signing, and packaging cannot make a pointer-truncating engine
correct.

## 2026-07-29 — Repository and evidence foundation

**Commits:** `c5496cbfe`, `268ff6977`, `6b3238104`

**Observed and performed**

- Cloned `CTR-tools/ctr-native` at `2df55dc5a`.
- Recorded the Android GLES reference branch at `34648097d`.
- Added `https://github.com/CTR-tools/ctr-native.git` as remote `upstream`.
- Joined the CTRPad documentation ancestry with the unchanged upstream source
  ancestry in merge commit `268ff6977`.
- Created implementation branch `codex/arm64-apple`.
- Expanded `.gitignore` protection for raw/cooked disc formats, extracted
  retail assets, the supplied `ref/CTR/` tree, and reference clones.

**Validation**

```sh
git diff upstream/master -- \
  CMakeLists.txt CMakePresets.json main.c game include platform externals
git ls-files
git check-ignore -v ref/CTR/example.bin
```

The source diff against the pinned upstream was empty, no retail media was
tracked, and representative `.bin`, `.img`, `.iso`, `.cue`, `.ccd`, `.sub`,
`.BIG`, `.HWL`, `.XA`, and `.STR` paths were ignored.

**Decision**

Reference clones remain read-only evidence. All implementation occurs in the
downstream worktree and preserves GPL-3.0 history.

## 2026-07-29 — Reproducible i686 baseline and Apple failure boundary

**Commits:** `1502955cb`, `f8974377b`, `a76ac25a4`

**Method**

The checked-in reproduction entry point is:

```sh
tools/build-linux-i686-baseline.sh
```

It builds `tools/docker/linux-i686.Dockerfile` from pinned Ubuntu image digest:

```text
sha256:4fbb8e6a8395de5a7550b33509421a2bafbc0aab6c06ba2cef9ebffbc7092d90
```

The Apple ARM64 starting boundary was captured with:

```sh
cmake -S . -B build-macos-arm64-baseline -G Ninja \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo
```

**Failure encountered**

The first i686 configure failed in SDL:

```text
Couldn't find dependency package for XCURSOR
```

Upstream's dependency set lacked the 32-bit Xcursor development package. The
container was corrected to include the necessary i386 X11/Xcursor/Xext/
Xfixes/Xi/Xrandr, ALSA, OpenGL, udev, and dbus packages.

**Validated result**

At `a76ac25a493d`, the container produced an ELF 32-bit Intel 80386 executable,
passed the version test, and reported:

```text
CTR Native 0.1.0-beta.7.1 (a76ac25a493d)
```

Its GNU build ID was
`1c1f862bb9bb4112d78a97894bceb0dfd93d37b3`. The unmodified Apple configure
failed at the intentional gate:

```text
CTR Native currently requires a 32-bit target.
```

No low-address allocation trick or source bypass was accepted. Full package
versions and configuration output are preserved in
[2026-07-29-baselines.md](../builds/2026-07-29-baselines.md).

## 2026-07-29 — Retail-region false start and input correction

**Commit:** `89d08cdee`

**Observed**

The original CloneCD image had valid raw-sector geometry but booted
`SCES_021.05`, the PAL Europe release. The `BUILD=926` native executable
expects NTSC-U `SCUS_944.26`. Before a region gate existed, the code used the
NTSC-U BIGFILE index `0x1fd`, interpreted unrelated PAL bytes as a
`VramHeader`, and aborted in `NativeRenderer_CopyVRAM`.

Original local media hashes:

```text
19f4ed5097951e72d2f302ea0a803bb2690e20569a7e50ca812213c15b6c339f  CTR.ccd
84aeb6f990954abb0eed4580fe2e4d2b17ce8a6cb266385b56b96440b523f41a  CTR.img
87d84e9ea413c3d0bcef657b1cf4524a09310a43b25fd75ed615feb124167f7a  CTR.sub
```

**Decision and implementation**

The runtime now extracts `SYSTEM.CNF`, requires `SCUS_944.26` before game
initialization, and bounds-checks VRAM copies. The PAL crash was classified as
an input/build-region mismatch, not an NTSC-U loader defect.

**Replacement retail evidence**

The user supplied a BIN/CUE set with:

```text
f780bf2331476aabfc00772fa758b12dd95ebfbc907968132cbd3cdd4e2c07c0  CTR - Crash Team Racing (USA).bin
5bac7f02de7b8fb081e1049f4a277be1062c83fcb7ac6c98308ba7985280e4c3  CTR - Crash Team Racing (USA).cue
```

The BIN is 605,698,800 bytes, exactly 257,525 MODE2/2352 sectors. Sector 16
contains ISO volume ID `SCUS-94426`; `SYSTEM.CNF` contains
`SCUS_944.26`. The runtime accepted it, reached the SCEA/Naughty Dog boot
sequence, and responded to live keyboard-to-pad input.

No retail file is tracked by Git.

## 2026-07-29 — Canonical parity harness

**Commits:** `a71db6914`, `d029ebc89`, `cc9c06f2a`, `df558504c`,
`868a4e308`

**Assessment**

Raw checkpoints were rejected as the parity oracle because they include
native pointers, allocator state, and process-image-dependent callback
addresses. Details are in
[TOOLING-ASSESSMENT.md](../parity/TOOLING-ASSESSMENT.md).

**Implementation**

- Added canonical gameplay state digest schema 1 to replay format version 2.
- Added a media-free test proving host-address independence and detecting a
  one-unit vehicle-position mutation as component `drivers`.
- Added replay-frame validation with distinct exit codes: `1` for harness
  failure and `2` for parity failure.
- Added a loopback-only noVNC recording environment and an automated
  two-process unchanged/mutation verifier.
- Required eight observed coverage behaviors before a report can be accepted.

At clean commit `cc9c06f2af53`, the reproducible i686 binary passed version,
state-digest, and replay-gate tests. Its GNU build ID was
`09133476a186e75f24bb1675f97af4bbe22e255a`.

**Acceptance rule**

A report must finish, contain all coverage evidence, replay unchanged from
separate address layouts, and fail at the expected canonical component after
a deliberate active-driver mutation. Recording alone is insufficient.

## 2026-07-29 — First complete NTSC-U recording

**Source/binary:** `868a4e308a1c80222948938efe6b8f9ab86aee73`

**Durable local report:**
`build-linux-i686-baseline/debug/reports/20260729/ctr-225420`

**Observed result**

- 24,232 replay frames.
- 81 rolling checkpoints at 300-frame intervals.
- All eight required startup, menu/load, driving, boost, item, lap, save, and
  persisted-result behaviors observed.
- First active race-driver frame: 1711.
- Retail successful powerslide-boost path observed.

**Strict playback failure**

Unchanged playback matched timing, RNG, drivers, world, pads, and VBlank
packets through frame 22,391. At frame 22,392, only `allocation` differed.
The live allocation digest was one frame behind the recording.

Root cause: native Enter reached `SubmitName_UseKeyboard` through SDL and
synthesized retail Circle/save input, but version-2 frames serialized only the
mapped PS1 pad state, not the originating host scancode.

**Correction**

The three reserved bytes in each fixed-size pad snapshot now carry marker
`0x53` and the native scancode low/high bytes. New recordings preserve the
semantic without changing frame size. This specific old keyboard trace can
recover Enter from raw Start; arbitrary legacy name-entry scancodes cannot be
reconstructed.

The complete analysis is in
[2026-07-29-golden-run-result.md](../parity/2026-07-29-golden-run-result.md).

## 2026-07-29 — Two replay migration approaches rejected

### State-preserving version-2 migration

The migrated run matched through frame 366 and failed closed at frame 367.
Two VBlanks emitted during asynchronous loading occurred between tracked
frames, outside the version-2 BeginFrame/EndFrame packet window. Timing and
driver state therefore diverged.

**Decision:** do not describe version-2 checkpoint migration as
state-preserving. A fresh run must record its own timing.

### Cross-build checkpoint restore

A diagnostic bypass showed that unity-build edits move callback addresses.
Captured image offset `0xf880` named `Particle_FuncPtr_SpitTire` in the
recording binary but landed inside `MATH_Matrix_TrigSinCos` in the rebuilt
binary.

**Decision:** `--replay-bypass-header` is diagnostic-only across code layouts.
Portable checkpoints require symbolic callback identities. An image-base
offset is not an ARM64 migration format.

One narrow checkpoint correction was retained: the relocation validator now
accepts the legal one-past pointer `Mempack.endOfMemory`. It does not make the
checkpoint format cross-build portable.

## 2026-07-29 — M2 memory-model census and decision

**Source base:** `868a4e308a1c80222948938efe6b8f9ab86aee73`

**Reproduction**

```sh
tools/audit-64bit-memory-model.sh
tools/verify-guest-reference-prototype.sh \
  build-linux-i686-baseline/debug/reports/20260729/ctr-225420
```

**Measured baseline**

```text
forced LP64 static-layout assertion failures: 673
unique pointer-to-integer coordinates:        162
unique integer-to-pointer coordinates:        121
unique narrowing source lines:                203
pointer-bearing pinned root definitions:      104
root/field pointer storage contexts:           939
```

Ownership classification:

```text
resident executable/overlay map: 575
runtime-only host:                264
scratchpad guest:                  50
serialized file:                   50
```

Accepted evidence hashes:

```text
i686 object:
7ebc25257acc868616a1616d8a1b4479068b6c9ccb27deb7fc2d703db72c357b

layout-census.tsv:
c1d77ab5ba9bb2f488754a29a5e3c2c06131d0d3343de79aca9ec8fa1d535550
```

**Prototype**

The real frame-1800 Roo's Tubes MPAK was copied to an ARM64 host allocation
above `UINT32_MAX`. Both candidate models traversed:

```text
GameTracker.level1
  -> Level.ptr_mesh_info
  -> mesh_info.ptrQuadBlockArray
  -> QuadBlock[0].ptr_texture_low
```

The asset contained 1,806 quad blocks, 8,507 vertices, and 1,011 BSP nodes.
Four corrupt-reference probes failed with bounded diagnostics.

**Decision**

[ADR-0001](../architecture/ADR-0001-guest-references.md) selects a checked
eight-bit-region-tag/24-bit-offset `GuestRef32` representation:

- serialized fields retain four-byte layouts and resolve at typed boundaries;
- scratchpad bytes retain the one-KiB guest layout and native pointers move to
  sidecars/locals;
- resident maps split guest schema from native runtime state;
- runtime-only structures use native pointers and explicit ILP32/LP64
  contracts;
- low host virtual-address allocation is forbidden as a correctness strategy.

## 2026-07-29 to 2026-07-30 — M3 runtime conversion and M4 boundary

**Worktree base:** `868a4e308`; implementation not yet committed

**Implementation approach**

- Added the `GuestRef32` region registry and corrupt-reference tests.
- Added an atomic asset-relocation boundary with size, alignment, duplicate,
  overflow, target-range, registration, and rollback checks.
- Integrated the boundary into two loader paths while preserving i686
  behavior.
- Replaced runtime pointer/integer round trips with typed native pointers or
  `uintptr_t`.
- Kept fixed-width SPU/device addresses as integers.
- Kept scratchpad images fixed-width and used a host-width sidecar where
  conversion was required.
- Added explicit ILP32 and LP64 contracts only for runtime-owned structures.
  Serialized and scratchpad assertions remain unconditional.
- Enabled compiler-checked `-fno-strict-aliasing` and `-fwrapv`; limited
  `-msse` to x86 targets.

**Measured progress as of 2026-07-30**

```text
forced LP64 static-layout assertion failures: 673 -> 471 (202 removed)
pointer-to-integer coordinates:                162 -> 0
integer-to-pointer coordinates:                121 -> 0
unique narrowing source lines:                 203 -> 0
```

The census now reports 288 pinned types, 104 pointer-bearing root
definitions, and 939 field contexts. The ownership totals remain:

```text
resident_map=575 runtime_host=264 scratchpad_guest=50 serialized_file=50
```

Runtime contracts converted so far include camera/audio metadata, allocator
and load queues, gamepad, list/item, ghost/audio/song, threads/buckets,
menus/particles/push buffers/instances, profile objects, render lists, bots,
visibility lists, and runtime objects in overlays 231–233.

**Validation**

The pinned amd64-container/i686 build passes:

```text
ctr_native_version
ctr_native_state_digest
ctr_native_replay_gate
ctr_native_guest_ref
ctr_native_asset_relocation
ctr_native_input
```

The active clean baseline executable and its accepted source report were not
rebuilt or overwritten. The detailed batch ledger is
[64-bit-conversion-log.md](../architecture/64-bit-conversion-log.md).

**Limitations still open**

- 471 LP64 layout failures remain. Most are serialized assets, scratchpad
  overlays, the large `Driver` runtime layout, and resident maps. They require
  ownership-aware conversion, not blanket assertion suppression.
- Serialized pointer consumers still need subsystem-by-subsystem `GuestRef32`
  conversion.
- Checkpoints still need symbolic callback identities.
- No macOS ARM64 executable is accepted yet; therefore iOS, import UI, touch,
  signing, and packaging have not begun.

## 2026-07-30 — Fresh replay-seeded recording in progress

**Purpose**

`--record-from-replay SOURCE` uses only the validated source pad snapshots and
memcard seed as an input script. It upgrades the recoverable legacy Enter,
starts a fresh game, and records its own timing/checkpoints. It does not claim
state equality with the old run.

**Isolated execution**

```text
container: ctrpad-replay-seeded-recording
run root:  /tmp/ctrpad-seeded-current-20260729
report:    /tmp/ctrpad-seeded-current-20260729/debug/reports/20260730/ctr-051139
```

The container runs an isolated copy of the tested binary, so ongoing source
builds cannot change the executable under the recording.

Snapshot at 2026-07-30, 38 minutes:

```text
frames written: 5,812 / 24,232
process state:  running
checkpoints:    through frame 5,700
```

Observed active/inactive race transitions match the input script at frames
1711, 3017/3070, 3920/3973, and 4636/4689. Local screenshots confirmed the
Roo's Tubes start grid, pause/quit menu, and active lap-one racing; screenshots
remain temporary retail-derived evidence and are not committed.

Monitor command:

```sh
report=/tmp/ctrpad-seeded-current-20260729/debug/reports/20260730/ctr-051139
bytes=$(wc -c < "$report/input.ctrreplay" | tr -d ' ')
printf 'report_frames=%s\n' "$(((bytes-148)/440))"
docker ps -a --filter name=ctrpad-replay-seeded-recording \
  --format '{{.Names}} {{.Status}}'
tail -30 "$report/ctr-native.log"
```

`tools/compare-replay-state-components.mjs` validates replay headers, frame
sequence, pad checksums, record checksums, canonical component hashes, and
mismatch ranges. It supports prefix comparison for a live report and rejects
truncated input. The old report passes a 24,232-frame self-comparison. A fresh
run is expected to differ in timing-dependent canonical state after loading;
only normal playback of the completed new report is the parity gate.

**Acceptance still required**

1. Finish all 24,232 scripted inputs and re-observe coverage.
2. Replay the new report unchanged twice from separate address layouts.
3. Perform the deliberate active-driver mutation and observe component
   `drivers` at the intended frame.
4. Record final report hashes, environment manifest, and results in durable
   documentation.

## Current handoff state — 2026-07-30

```text
branch: codex/arm64-apple
HEAD:   868a4e308a1c80222948938efe6b8f9ab86aee73
M1:     in progress; fresh replay recording running
M2:     design/census accepted
M3:     in progress; runtime pointer narrowing removed
M4:     boundary implemented; consumer migration incomplete
M5+:    not yet accepted
```

The working tree intentionally contains an uncommitted multi-file M2–M4 batch.
Before it is committed, the latest source must pass a fresh i686 rebuild, the
64-bit audit must be regenerated, the conversion ledger must be updated, and
`git diff --check` must be clean.

## 2026-07-30 — Runtime menus, metadata, and render-layout batch

**Source base:** `a40a7584c5761ce24b54bbb9280058a1d84b9dd5`

**Selection method**

The 471 forced-LP64 failures were grouped by asserted structure name, then
cross-checked against the M2 ownership census. Serialized navigation/model/
level data, scratchpad overlays, resident maps, and the large `Driver`
conversion were excluded from this batch. The selected types were
runtime-owned and had unambiguous native layouts in Apple Clang's record dump.

**Implementation**

Explicit ILP32/LP64 contracts were added for title/menu objects, warp-pad
state, terrain and collision-surface metadata, level/model metadata,
render-bucket entries, renderer dependencies already converted elsewhere, and
decal render entries. The source retains the retail ILP32 layout and asserts
the measured eight-byte-pointer layout on LP64.

No serialized or scratchpad assertion was weakened or hidden.

**Measured result**

```text
forced LP64 static-layout assertions: 471 -> 435
failures removed in batch:            36
failures removed overall:            238 of 673
pointer/integer narrowing sites:      0
```

The residual type list contains none of the structures converted in this
batch. It remains concentrated in the four expected ownership-sensitive
groups.

**Validation**

```sh
tools/audit-64bit-memory-model.sh

docker exec ctrpad-i686-debug sh -lc \
  'cmake --build /out --parallel 8 && \
   ctest --test-dir /out --output-on-failure'
```

The exact worktree rebuilt and linked under the pinned i686 environment. All
six tests passed in 1.75 seconds. The two existing format-security warnings
and two existing maybe-uninitialized warnings remained visible.

**Independent retail-run snapshot**

The isolated replay-seeded recording remained healthy while this batch was
compiled:

```text
report:      /tmp/ctrpad-seeded-current-20260729/debug/reports/20260730/ctr-051139
frames:      6,820 / 24,232
checkpoints: through frame 6,600
container:   running for 54 minutes
```

The report is generated by an isolated binary copy and was not affected by
this worktree rebuild.

## 2026-07-30 — Driver allocation audit and LP64 contract

**Source base:** `a40a7584c5761ce24b54bbb9280058a1d84b9dd5`

**Why this required more than assertion changes**

`Driver` caused 99 of the 435 remaining LP64 failures. Apple Clang measured
the widened structure at `0x6e8` bytes, while the unchanged large-stack pool
would expose only `0x668` bytes after its native list header. An assertion-only
change would therefore have hidden a 128-byte pool overrun.

All allocation, clearing, checkpoint, state-digest, and size-dependent uses
were searched before implementation. Runtime allocation was concentrated in
player, bot, ghost, and large-pool initialization paths.

**Implementation**

Three sizes are now explicit:

```text
                                i686    LP64
race prefix before ghost tail   0x62c   0x6d8
full ghost-capable Driver       0x638   0x6e8
large-stack pool item           0x670   0x728
```

The large item includes the native `Item` header, the full `Driver`, and the
retail `0x30` spare capacity. Player/bot creation and clearing use the race
prefix. Ghost clearing uses the full structure. The i686 ghost request remains
retail value `4` to preserve thread flag bits; LP64 requests the full `0x6e8`
so `PROC_BirthWithObject` validates the real requirement.

The original ILP32 offsets remain asserted. A complete LP64 block records the
widened function table, object pointers, physics regions, HUD pointers,
kart-state union, bot data, ghost tail, and final `0x6e8` size.

**Measured and validated result**

```text
forced LP64 assertions:         435 -> 336
Driver failures removed:        99
overall failures removed:       337 of 673
pointer/integer narrowing:      0
i686 tests:                     6 / 6 passed
i686 test time:                 2.02 seconds
```

The post-edit error list contains no Driver assertion, and the magic-size
search contains only the named constants and explanatory comments.

**Independent retail-run snapshot**

```text
report:      /tmp/ctrpad-seeded-current-20260729/debug/reports/20260730/ctr-051139
frames:      7,655 / 24,232
checkpoints: through frame 7,500
container:   running
```

## 2026-07-30 — Scratch pointer-word and host-sidecar batch

**Source base:** `a40a7584c5761ce24b54bbb9280058a1d84b9dd5`

**Problem and decision**

The next 45 LP64 failures came from native pointers embedded in retail-sized
vehicle and rendering scratch structures. Widening the fields would move the
following words and break the scratch ABI. Conditionalizing the assertions
would only conceal that break. The selected model keeps a fixed 32-bit word
in the scratch image and moves the live native pointer into a scoped
host-width parameter, local, or sidecar.

On i686 the fixed word receives the exact previous raw pointer value. On LP64
it is zero, making it impossible to accidentally consume a truncated host
address. The live pointer is used only through:

- an eight-entry local `Driver *` sidecar for vehicle ground shadows;
- a native `PushBuffer *` parameter for ground-skid emission;
- `DrawTiresHostRanges` for tire sprite and ordering-table ranges; and
- a local native ordering-table pointer for particle rendering.

The affected fixed words are explicitly named `driverPtr32`, `instPtr32`,
`sentinelDriverPtr32`, `pushBufferPtr32`, `wheelSpritesPtr32`, and `otPtr32`.
All prior scratch offsets and total sizes remain asserted on both data models.

**Commands and measured evidence**

```sh
tools/audit-64bit-memory-model.sh

docker exec ctrpad-i686-debug sh -lc \
  'cmake --build /out --parallel 8 && \
   ctest --test-dir /out --output-on-failure'

git diff --check
```

```text
forced LP64 assertions:         336 -> 291
ground-shadow reduction:         10
ground-skids reduction:           9
tire-render reduction:           24
particle-render reduction:        2
overall reduction:              382 of 673
pointer/integer narrowing:        0
i686 tests:                       6 / 6 passed
i686 test time:                   2.27 seconds
```

The exact unity build linked successfully. The two pre-existing
format-security warnings and two pre-existing maybe-uninitialized warnings
were unchanged.

**Independent retail-run snapshot**

The separately copied replay-seeded binary remained isolated from this
worktree rebuild and continued recording:

```text
report:      /tmp/ctrpad-seeded-current-20260729/debug/reports/20260730/ctr-051139
frames:      8,816 / 24,232
checkpoints: through frame 8,700
container:   running
```

Observed race-driver transitions through frame 8,160 still agree with the
source input script. This is ongoing independent evidence, not yet a completed
parity result.

## 2026-07-30 — Collision scratch host/guest split

**Source base:** `a40a7584c5761ce24b54bbb9280058a1d84b9dd5`

**Classification before editing**

The residual audit attributed 40 assertions directly to
`ScratchpadStruct`. Inspecting the complete type and all producer/consumer
sites showed that this was not an isolated 40-line assertion cleanup. The
structure is a fixed `0x20c` scratch image that contains:

- thread and function pointers in the thread-bucket union;
- two mesh pointers and a current BSP hitbox;
- candidate and accepted quadblock pointers;
- 15 visited BSP hitboxes;
- accepted level/search triangle pointers;
- nine search-vertex level pointers; and
- an extended 15-record scrub history.

It is used at the emulated scratch address, within the resident `sdata` map,
and inside scratch-work wrappers. Direct LP64 widening would have shifted
collision working state and the camera/push-buffer tail of
`ScratchpadStructExtended`.

The search covered `COLL.c`, `PROC.c`, `CAM.c`, `BOTS.c`, relevant overlay
231/232 callbacks, vehicle birth/pickup/stuck paths, declarations, resident
storage, and all fixed-offset assertions. Field-use counts were recorded
before implementation; for example, the mesh pointer appeared in 21 files,
while the callback/thread pair appeared in seven/six files.

**Selected design**

The retail image now contains named 32-bit pointer words only. A
`CollScratchHost` carries every live native pointer. A bounded 64-slot,
no-allocation LRU registry maps `ScratchpadStruct *` owners to their host
sidecars. This matches the current single-threaded engine and avoids changing
the callback ABI across the entire game.

All writes go through synchronized setters. i686 writes the same raw pointer
word as before and stores the live pointer in the sidecar. LP64 writes zero to
the scratch word and stores only the full host pointer in the sidecar. Reads
use the sidecar exclusively. Array setters reject indices outside their
retail capacities, preventing an out-of-bounds scratch write.

This also converted the helper records `BspSearchVertex`,
`BspSearchTriangle`, `BspSearchResult`, `CollLevelTriangle`, and
`CollBspSearchTriangle`; that is why the measured reduction is larger than
the 40 assertions initially attributed to `ScratchpadStruct`.

**First validation pass**

```sh
tools/audit-64bit-memory-model.sh

docker exec ctrpad-i686-debug sh -lc \
  'cmake --build /out --parallel 8 && \
   ctest --test-dir /out --output-on-failure'
```

```text
forced LP64 assertions:         291 -> 235
failures removed:                56
overall failures removed:       438 of 673
pointer/integer narrowing:        0
existing i686 tests:              6 / 6 passed
i686 test time:                   1.91 seconds
```

The forced-LP64 error stream contained no non-assert compiler error and no
remaining failure for the converted collision types. The i686 unity target
linked with only the four pre-existing warnings.

**Why a seventh test was added**

The six existing media-free tests proved the target compiled and that the
other conversion boundaries remained intact, but none directly exercised the
new collision sidecar. I therefore added:

```text
--self-test-collision-scratch
CTest name: ctr_native_collision_scratch
```

The self-test verifies both fixed sizes, synchronized raw words, host recovery
for all pointer categories, the highest valid array indices, function-pointer
storage, and isolation between two scratch owners.

**Final validation**

```sh
tools/audit-64bit-memory-model.sh

docker exec ctrpad-i686-debug sh -lc \
  'cmake --build /out --parallel 8 && \
   ctest --test-dir /out --output-on-failure'

docker exec ctrpad-i686-debug \
  /out/ctr_native --self-test-collision-scratch

git diff --check
```

```text
forced LP64 assertions:         235
pointer/integer narrowing:        0
i686 tests:                       7 / 7 passed
i686 test time:                   0.99 seconds
self-test output:
[CTR CollScratch] self-test passed: guest-size=0x20c extended=0x2f8 pointer-word=raw32 owners=isolated
```

The same four pre-existing compiler warnings remained visible.

**Independent retail-run snapshot**

The isolated replay-seeded recorder was not rebuilt from this worktree. It
continued as independent evidence:

```text
report:      /tmp/ctrpad-seeded-current-20260729/debug/reports/20260730/ctr-051139
frames:      10,300 / 24,232
checkpoints: through frame 10,200
container:   running
```

Observed driver transitions at 9,120/9,173 matched the source input sequence.
The report also logged one successful powerslide boost. This ongoing run does
not yet establish complete replay parity, and no retail-derived binary or
image was added to the repository.

## 2026-07-30 — Overlay 230 ownership decision and LP64 contract

**Source base:** `a40a7584c5761ce24b54bbb9280058a1d84b9dd5`

**Classification**

The post-collision residual list contained 68 overlay-230 failures: 67 field
anchors in `OverlayDATA_230` and the size of `OVR_230_VideoBSS`. Before
changing assertions, all `D230`/`V230` consumers and checkpoint handling were
searched.

Evidence established that these are resident native runtime objects:

- `D230.c` initializes `D230` as a C object and keeps a native initial-state
  copy for overlay reset;
- main-menu code accesses fields and subobjects directly;
- `V230` owns live decoder buffers and CD state;
- checkpoint region sizing uses the current `sizeof(D230)`/`sizeof(V230)`;
- checkpoint restore explicitly relocates all embedded menu, callback,
  character-select, title, buffer, and CD-location pointers; and
- no native path consumes a `D230`/`V230` member through its retail address or
  byte offset.

The selected ADR-0001 rule is therefore a host-width resident representation,
with separate explicit ILP32 and LP64 contracts. This decision does not apply
to serialized `Level`, `Model`, `QuadBlock`, or navigation records; their
retail layouts remain unconditional.

**Unsafe arithmetic found during the audit**

Two video-buffer calculations still converted `uint32_t *` through `int`.
They now add byte offsets to `u8 *` bases. This preserves the exact i686
result while avoiding LP64 truncation.

**Measured layouts**

Apple Clang record dumps measured:

```text
                           i686 retail   LP64 native
OverlayDATA_230            0x1580        0x1890
OVR_230_VideoBSS           0x0088        0x00a8
```

All 67 existing `D230` anchors remain asserted on i686 and are mirrored with
their measured LP64 offsets. The video block retains every scalar retail
anchor and adds LP64 anchors for the widened buffer arrays, slice, CD
locations, final pointer, and size.

**Commands and result**

```sh
tools/audit-64bit-memory-model.sh

docker exec ctrpad-i686-debug sh -lc \
  'cmake --build /out --parallel 8 && \
   ctest --test-dir /out --output-on-failure'

git diff --check
```

```text
forced LP64 assertions:         235 -> 167
D230 reduction:                  67
V230 reduction:                   1
overall failures removed:       506 of 673
pointer/integer narrowing:        0
i686 tests:                       7 / 7 passed
i686 test time:                   2.08 seconds
```

The exact unity build linked. The two existing format-security and two
existing maybe-uninitialized warnings were unchanged.

**Independent retail-run snapshot**

The isolated replay-seeded recording remained healthy but, because it uses a
copied pre-batch binary, is continuing baseline evidence rather than direct
validation of this overlay change:

```text
report:      /tmp/ctrpad-seeded-current-20260729/debug/reports/20260730/ctr-051139
frames:      11,107 / 24,232
checkpoints: through frame 11,100
container:   running
```

No retail-derived binary or screenshot was committed.

## 2026-07-30 — Overlay 232 ownership decision and validation correction

**Source base:** `a40a7584c5761ce24b54bbb9280058a1d84b9dd5`

**Classification**

The next residual group contained 13 absolute-address assertions in
`OverlayDATA_232`. Before changing them, the search covered the complete
`D232` field-use surface and `platform/native_checkpoint.c`.

The evidence classifies `D232` as resident native state:

- the executable links and initializes it as a C object;
- adventure-hub, mask-hint, and pause-menu code use typed field access;
- checkpoint capture uses the current `sizeof(D232)`;
- checkpoint restore relocates `menuTokenRelic`, `menuHintMenu`, all five
  `hubItemsXY_ptrArray` slots, `ptrPauseObject`, the embedded `pauseObject`,
  and a separately selected live pause object; and
- no native consumer was found indexing it by a retail address or loading it
  from a serialized asset image.

The selected representation is therefore the same resident host-width rule
used for overlay 230. This decision is deliberately local to `D232`; it does
not relax `Level`, `Model`, `QuadBlock`, navigation, or other on-disc
structures.

**Measured contracts**

```text
                           i686 retail   LP64 native
OverlayDATA_232            0x0898        0x0a50
PauseObject                0x00e4        0x0158
```

The existing 13 retail absolute-address checks remain under the i686
contract. Apple Clang record-layout evidence supplied matching LP64 offsets
for:

```text
saveObjCameraOffset        0x0e0
loadSavePrimOffset         0x0e8
hubArrowPrimOffset         0x0fc
hubArrowInnerOffset        0x200
hubArrowOuterOffset        0x20c
loadSavePos                0x21c
hubArrowPos                0x23c
fiveArrowPos               0x420
maskHintOffsets            0x468
maskWarppadDelayFrames     0x8cc
maskWarppadBoolInterrupt   0x8d0
ptrPauseObject             0x8d8
pauseObject                0x8e0
```

**Commands and accepted result**

```sh
tools/audit-64bit-memory-model.sh

docker exec ctrpad-i686-debug sh -lc \
  'cmake --build /out --parallel 8 && \
   ctest --test-dir /out --output-on-failure'

docker exec ctrpad-i686-debug sh -lc \
  'stat -c "%y %s %n" /out/ctr_native; \
   ctest --test-dir /out --output-on-failure'

git diff --check
```

```text
forced LP64 assertions:         167 -> 154
overall failures removed:       519 of 673
pointer/integer narrowing:        0
fresh i686 binary timestamp:     2026-07-30 07:25:05.790301064 +0000
fresh i686 binary size:          8,979,860 bytes
i686 tests:                       7 / 7 passed
i686 test time:                   2.32 seconds
```

**Validation correction retained for reproducibility**

The first build command continued inside the container after its outer tool
session stopped returning output. A separate diagnostic `ctest` invocation
then passed in 2.18 seconds, but `stat` showed that it had exercised the
previous binary (`07:14:00`), so that result was rejected. A later diagnostic
command accidentally started a second unity build concurrently. Its exact
process tree was identified and only the duplicate build/wait processes were
terminated; the original compiler was left running and no source was changed.

The original unity compile completed at `07:25:05`. The seven tests were then
run again explicitly against that new timestamp, and only the resulting 2.32
second pass is accepted above. This incident is recorded because a green test
suite is not evidence for a source change unless the tested artifact can be
tied to the completed rebuild.

**Independent retail-run snapshot**

The isolated replay-seeded recorder, using a copied pre-batch executable,
continued independently:

```text
report:      /tmp/ctrpad-seeded-current-20260729/debug/reports/20260730/ctr-051139
frames:      11,625 / 24,232
checkpoints: through frame 11,400
container:   running
```

This is longitudinal baseline evidence, not validation of the overlay-232
source edit. No retail-derived binary or screenshot was committed.

## 2026-07-30 — Overlay 233 split by actual ownership

**Source base:** `a40a7584c5761ce24b54bbb9280058a1d84b9dd5`

**Residual classification**

The 154-failure audit was traced assertion by assertion. Overlay 233 accounted
for 115 failures, but its objects have different owners:

- `R233` is a linked `const` initializer containing strings, byte-verified
  opcode streams, native pointer tables, emitters, matrix data, and boss
  cutscene metadata.
- `D233` is the mutable copy of the cutscene state and matrix data.
- `gGarage` is a resident `RectMenu` plus native garage state.
- `creditsBSS` contains resident live pointers but also embeds local copies of
  the serialized `Model` and `ModelHeader` structures.

The audit searched all `R233`/`D233`/garage/credits consumers, initializers,
raw address/offset uses, and checkpoint paths. Retail branch targets inside
opcode bytes are not interpreted by relying on native struct offsets:
`CS_OVR233_TranslateRetailOpcodePointer` maps each fixed retail range directly
to its named native byte-array field. Native code otherwise uses typed field
access.

Checkpoint evidence showed:

- `R233` participates only in the address-owner table so pointers into its
  scripts can be relocated; it is absent from the serialized region list.
- `D233` capture uses current `sizeof`, clears the derived matrix-table
  pointers, and reconstructs them on restore.
- garage capture uses current `sizeof` and restores/relocates its `RectMenu`.
- credits pointers are relocated, but their embedded serialized model copies
  make credits dependent on the upcoming model GuestRef conversion.

The resulting change therefore adds ILP32/LP64 contracts only for `R233`,
`D233`, and `gGarage`. Credits assertions remain strict and failing on LP64;
they were deliberately not conditionally suppressed.

**Measured contracts and audit**

```text
                           i686 retail   LP64 native
OverlayRDATA_233           0xbd90        0xc340
OverlayDATA_233            0x1818        0x1840
OVR233_Garage              0x00ac        0x00c0

forced LP64 assertions:       154 -> 74
overall failures removed:     599 of 673
pointer/integer narrowing:      0
```

The LP64 `R233` contract mirrors every existing anchor. Notable widened
boundaries include particle emitters/configs, five script pointer-table
families, four matrix-table entries, fourteen boss-cutscene records, and the
two final model pointers.

**Commands and accepted validation**

```sh
tools/audit-64bit-memory-model.sh

docker exec ctrpad-i686-debug sh -lc '
  log=/tmp/ctrpad-ovr233-resident-build.log
  (cmake --build /out --parallel 8 &&
   ctest --test-dir /out --output-on-failure) > \"$log\" 2>&1 &
  echo $! > /tmp/ctrpad-ovr233-resident-build.pid
'

docker exec ctrpad-i686-debug sh -lc '
  tail -80 /tmp/ctrpad-ovr233-resident-build.log
  stat -c \"%y %s %n\" /out/ctr_native
'

git diff --check
```

```text
fresh binary timestamp:   2026-07-30 07:37:48.860981673 +0000
fresh binary size:        8,979,860 bytes
i686 tests:               7 / 7 passed
i686 test time:           2.62 seconds
compiler warnings:        2 format-security + 2 maybe-uninitialized (unchanged)
```

Unlike the previous validation session, this rebuild wrote to a unique log
and PID file. The accepted timestamp and tests came from the same logged
build, preventing an old executable from being mistaken for the edited
source.

**Independent retail-run snapshot**

The isolated copied-binary replay recorder continued independently:

```text
report:      /tmp/ctrpad-seeded-current-20260729/debug/reports/20260730/ctr-051139
frames:      12,656 / 24,232
checkpoints: through frame 12,600
container:   running
```

This remains longitudinal baseline evidence, not direct validation of the
overlay-233 edit. No retail-derived binary or screenshot was committed.

## 2026-07-30 — Serialized model boundary and credits sidecar

**Source base:** `a40a7584c5761ce24b54bbb9280058a1d84b9dd5`

**Status:** implementation accepted by the forced-LP64 compiler gate and
seven media-free i686 tests; retail/ARM64 runtime parity remains unproven

### Problem and ownership decision

The post-overlay-233 audit had 74 layout failures. Its model failures came
from treating serialized four-byte relocation slots as C pointers in
`Model`, `ModelHeader`, and `ModelAnim`. Widening those fields would make the
loaded retail image incompatible. The fields were therefore converted to
`CtrAssetRef32`, while runtime consumers received checked host-pointer
accessors (`include/namespace_Instance.h:316-424`,
`platform/native_asset_ref.c:96-218`).

The unusual case was overlay-233 credits. Each ghost uses a resident
`Model` copy but a native, resident two-element header array. Storing that
native pointer in `Model.headers` would violate the serialized `0x18` model
contract. The implementation uses an eight-entry, allocation-free sidecar:
five entries cover the fixed ghost copies and three remain spare. Reset,
destroy, and checkpoint-restore paths explicitly clear or reconstruct the
derived mappings (`game/233/CS_Credits.c:76-134`,
`game/233/D233.c:93-101,203-206`,
`platform/native_checkpoint.c:1733`).

### Implementation sequence

1. Changed the serialized slots to `CtrAssetRef32` and added model/header/
   animation accessors with checked range, alignment, required/optional, and
   array-overflow handling.
2. Migrated render-bucket, texture-cycle, instance, vehicle, hub, item,
   banner, burst, blowup, and cutscene consumers.
3. Searched for direct dereferences with:

   ```sh
   rg -n -- \
     '->headers|->ptrFrameData|->ptrTexLayout|->ptrColors|->ptrAnimations|->animtex|->ptrDeltaArray' \
     game platform include
   ```

   After the migration, matches were limited to the central resolver,
   explicit `.bits` null tests, and already-native runtime draw-state fields.
4. Ran the forced-LP64 audit. The first complete consumer pass produced 61
   static assertions, zero pointer/integer narrowing coordinates, and no
   non-layout type error. This was an intermediate observation, not an
   accepted endpoint.
5. Used Apple Clang record-layout output to measure credits after the embedded
   serialized model layouts became fixed:

   ```sh
   xcrun clang \
     -DBUILD=926 -DCTR_INTERNAL -DCTR_NATIVE \
     -DCTR_NATIVE_BUILD_ID='"lp64-layout"' \
     -DCTR_NATIVE_VERSION='"lp64-layout"' \
     -Iinclude \
     -Ibuild-linux-i686-baseline/externals/SDL/include-revision \
     -Iexternals/SDL/include \
     -std=c17 -Wno-everything \
     -Xclang -fdump-record-layouts -fsyntax-only main.c
   ```

   Observed layouts were `CreditsObj = 0x380` and
   `Ovr233_Credits_BSS = 0x3d0`; the unchanged i686 contracts are `0x340`
   and `0x374`. Those exact field anchors are asserted in
   `include/ovr_233.h:1133-1206`.
6. Extended the asset-relocation test to traverse a synthetic serialized
   model/header/animation graph and exercise install/get/clear behavior for
   the runtime header sidecar
   (`platform/native_asset_relocation.c:195-333`,
   `CMakeLists.txt:177-180`).

### Forced-LP64 evidence

Commands:

```sh
git diff --check
tools/audit-64bit-memory-model.sh

rg -n 'error:' build-64bit-audit/forced-lp64-compile.stderr
```

Final result:

```text
forced LP64 assertions:          74 -> 28
overall failures removed:       645 of 673
pointer-to-integer coordinates:   0
integer-to-pointer coordinates:   0
unique narrowing source lines:    0
non-layout compiler errors:        0
```

All 28 compiler errors are intentional remaining static assertions: 17 for
serialized level/navigation structures and 11 for the resident executable
and data maps. The audit still exits nonzero by design until those contracts
are converted; it must not be described as a successful ARM64 build.

### Rejected i686 validation

The first uniquely logged build used:

```sh
docker exec ctrpad-i686-debug sh -lc '
  log=/tmp/ctrpad-model-guestrefs-build.log
  pidfile=/tmp/ctrpad-model-guestrefs-build.pid
  : > "$log"
  (cmake --build /out --parallel 8 &&
   ctest --test-dir /out --output-on-failure) > "$log" 2>&1 &
  echo $! > "$pidfile"
'
```

It created `/out/ctr_native` at
`2026-07-30 07:52:53.665629856 +0000`, size 9,001,216 bytes, and passed all
seven tests in 0.91 seconds. It also introduced two new warnings:

```text
unused variable 'reference'
unused parameter 'guestError'
```

Both came from the i686 branch of the new cross-width self-test. Although the
tests were green, this run was rejected because it worsened the compiler
diagnostic baseline. The correction put `reference` inside the LP64 block and
explicitly consumed `guestError` in the i686 block.

### Accepted i686 validation

The correction was rebuilt under a second unique log:

```sh
docker exec ctrpad-i686-debug sh -lc '
  log=/tmp/ctrpad-model-guestrefs-build-accepted.log
  pidfile=/tmp/ctrpad-model-guestrefs-build-accepted.pid
  : > "$log"
  (cmake --build /out --parallel 8 &&
   ctest --test-dir /out --output-on-failure) > "$log" 2>&1 &
  echo $! > "$pidfile"
'

docker exec ctrpad-i686-debug sh -lc '
  /out/ctr_native --self-test-asset-relocation
  sha256sum /out/ctr_native
  stat -c "%y %s %n" /out/ctr_native
'

git diff --check
```

Accepted result:

```text
fresh binary timestamp: 2026-07-30 07:55:43.543918623 +0000
fresh binary size:      9,001,344 bytes
fresh binary SHA-256:   5695056d5b033a0dd5464ebb263bd7ea3cf8d18dbeb65c855f41f7766036ca98
i686 tests:             7 / 7 passed
i686 test time:         0.88 seconds
compiler warnings:      2 format-security + 2 maybe-uninitialized (unchanged)
asset/model test:       model=checked override=checked
```

The accepted binary timestamp, warning count, self-test output, and CTest
result all came from the same second log/build. The immutable golden baseline
binary was not touched.

### Independent replay snapshot

The isolated copied-binary replay recorder continued during the work:

```text
report:      /tmp/ctrpad-seeded-current-20260729/debug/reports/20260730/ctr-051139
frames:      14,193 / 24,232
checkpoints: through frame 14,100
container:   running
```

At frame 13,765 the recorded race driver became inactive, and checkpoint
capture continued afterward. This replay uses a copied pre-batch executable,
so it remains longitudinal baseline evidence only. It does not validate the
model conversion, and no retail-derived content was committed.

### Next boundary

The remaining layout failures are now sharply divided:

```text
17 serialized Level/NavHeader/QuadBlock/BSP/SpawnType2/SCVert assertions
11 resident executable/data-map assertions
```

The next batch will convert the serialized level graph through the same
checked asset-reference boundary before the resident executable maps are
finalized. Retail parity and ARM64 runtime-load acceptance remain open.

## 2026-07-30 — Serialized Level graph: starting inventory and access boundary

**Status:** implementation in progress; no accepted build or parity claim.

The first source change in this batch replaced the direct native-pointer
members in the serialized `Level`, `mesh_info`, `QuadBlock`, `PVS`, `BSP`,
`SCVert`, `WaterVert`, `SpawnType2`, `Skybox`, `LevTexLookup`, `InstDef`, and
`NavHeader` layouts with four-byte `CtrAssetRef32` values
(`include/namespace_Level.h`, `include/namespace_Instance.h`,
`include/namespace_Bots.h`). This restored their retail byte layouts, but it
intentionally broke every consumer that still assumed those values were
native pointers.

The immediate forced-LP64 inventory was:

```text
forced LP64 static assertions:       28 -> 11
non-static compiler errors:             307
pointer-to-integer coordinates:           0
integer-to-pointer coordinates:           0
unique narrowing source lines:            0
```

This is not a 17-blocker completion. The assertion count only measures struct
shape; the 307 errors expose the actual consumer graph. The audit command was:

```sh
tools/audit-64bit-memory-model.sh
rg -n 'error:' build-64bit-audit/forced-lp64-compile.stderr
```

### Decisions made before consumer migration

- Added overflow-checked optional/required array resolution to the central
  `CtrAssetRef32` API (`include/ctr_asset_ref.h`,
  `platform/native_asset_ref.c`). Consumers will not reproduce unchecked
  `count * sizeof(element)` arithmetic.
- Kept visibility instance lists as arrays of serialized `InstDef`
  references. Retail toggles those four-byte entries between `InstDef *` and
  `Instance *` in place; that operation cannot store an LP64 pointer and is
  not idempotent. Native will retain the immutable `InstDef` reference and
  resolve its runtime instance when consumed.
- Kept `InstDef.ptrInstance` four bytes. On i686 it remains the historical raw
  pointer bits. On LP64 it is an instance-pool index plus one, with zero
  reserved for null. `InstDef_GetInstance` and `InstDef_SetInstance` are the
  only translation boundary.
- Split serialized and runtime visibility-memory representations.
  `LevelVisMemAsset` is the fixed `0x90` LEV table; native `VisMem` retains
  host pointers. LP64 materializes a bounded sidecar and allocates
  native-width BSP list nodes instead of widening retail memory in place
  (`include/namespace_Level.h`, `platform/native_asset_ref.c`).
- `NavHeader.last` remains a four-byte serialized slot, but native derives the
  endpoint from the inline frame array and `numPoints`. It does not write a
  native pointer into the retail header.

The accessor layer is now present, but the 307 callers have not all been
migrated and no i686 validation has been run for this batch. Subsequent
entries will preserve each compiler inventory, rejected validation, and
accepted result.

### First Level/Nav consumer-migration audit

After migrating the model lists, icon groups, spawn tables, instance
definitions, visibility roots, navigation consumers, collision structures,
and most level-drawing paths, the audit was run again:

```sh
./tools/audit-64bit-memory-model.sh
rg -n 'error:' build-64bit-audit/forced-lp64-compile.stderr
```

The result was:

```text
forced LP64 static assertions:        11
non-static compiler errors:           41
pointer-to-integer coordinates:        0
integer-to-pointer coordinates:        0
unique narrowing source lines:         0
```

This reduces the consumer graph from 307 to 41 compiler failures. It is an
intermediate inventory, not an accepted build. The 41 failures include one
syntax regression introduced while rewriting the checkpoint collision
condition in `game/COLL.c`: a mismatched parenthesis caused two parser
diagnostics. That edit will be corrected before the remaining direct
checkpoint and mesh references are migrated. The other failures identify the
unconverted call sites in race UI, lap and pickup logic, vehicle recovery,
warpball/hazard code, and the native state digest.

### Zero-consumer-error Level/Nav audit

The remaining race UI, lap, pickup, recovery, hazard, warpball, and state
digest consumers were moved behind `Level_GetRestartPoints`,
`Level_GetMeshInfo`, `MeshInfo_GetVertices`, `MeshInfo_GetBspRoot`, and
`MeshInfo_GetQuadBlocks`. The collision parenthesis was repaired, and
`RB_Potion_ThTick_InAir` regained the `teethInst` local needed by the newly
centralized `InstDef_GetInstance` expression.

The next audit reported only the 11 known resident executable/data-map static
assertions:

```text
forced LP64 static assertions:        11
non-static compiler errors:            0
pointer-to-integer coordinates:        0
integer-to-pointer coordinates:        0
unique narrowing source lines:         0
```

The serialized Level/Nav portion therefore accounts for 17 additional
resolved layout failures and reduces the original 673-failure census to 11.
This is the compile/audit gate only. No i686 build, replay, retail load, or
ARM64 runtime claim is attached to this result yet.

### Rejected first Level/Nav i686 validation

The first uniquely logged build used
`/tmp/ctrpad-level-nav-build-20260730.log`. It linked and passed all seven
CTest cases in 0.88 seconds, and the extended relocation test reported
`level=checked`. The binary evidence was:

```text
timestamp: 2026-07-30 08:43:15.025952259 +0000
size:      9,108,964 bytes
SHA-256:   077682dbc443868d5251e90ae2c2c9728835aba90066508b758d8c643d42a2b2
```

This run is rejected because it added three warnings to the accepted
diagnostic baseline:

```text
CtrLevelRuntimeVisMem_ResolveLists defined but not used
DrawLevelOvr1P_ResolveTexturePointerChecked defined but not used
DrawLevelOvr1P_ResolveTexturePointer defined but not used
```

The visibility materializer is LP64-only and must be excluded from the i686
translation unit. The two DrawLevel helpers became dead code after mid-texture
resolution moved to `QuadBlock_GetTextureMid`; they will be removed rather
than suppressed. A second build and log are required before this batch can be
accepted.

### Accepted Level/Nav i686 validation

The correction did three things before rebuilding:

- compiled `CtrLevelRuntimeVisMem_ResolveLists` only for pointer widths greater
  than 32 bits;
- removed the two now-unreferenced DrawLevel texture-pointer helpers; and
- corrected the remaining raw DrawLevel byte path so it resolves
  `CtrAssetRef32`, reconstructs the retail PSX mempack address, and reapplies
  low tag bits before extracting a byte.

The audit again reported 11 resident-map assertions, zero non-static compiler
errors, and zero pointer/integer narrowing coordinates. `git diff --check`
also passed. The separate candidate build used
`/tmp/ctrpad-level-nav-build-accepted-20260730.log`:

```text
fresh binary timestamp: 2026-07-30 08:47:31.176164301 +0000
fresh binary size:      9,107,604 bytes
fresh binary SHA-256:   bf0b49f28e3e31ad46e03d35398baece79370a57285b3a483b06eceee49e4b24
i686 tests:             7 / 7 passed
i686 test time:         0.74 seconds
compiler warnings:      2 format-security + 2 maybe-uninitialized (unchanged)
asset/level test:       level=checked
```

The Level self-test covers direct and low-bit-tagged mid-texture references,
the mesh, quad, BSP, and checkpoint arrays, packed visibility tags, and the
LP64 `VisMem` sidecar. This accepts the compile and media-free i686 regression
gate for the batch. It does not accept the remaining resident maps, an ARM64
retail load, checkpoint portability through the new sidecar, or golden replay
parity.

## 2026-07-30 — Resident Data/sData maps: measured LP64 contracts

**Status:** forced-LP64 compilation succeeds; i686 regression validation has
not yet accepted this batch.

The final 11 failures were absolute retail-address assertions in `Data` and
`sData`, plus the size/offset of the `bakedGteMath` entry table. These objects
are resident runtime maps, not serialized file structures. Their real host
pointers must widen on LP64, while the retail ILP32 absolute anchors must
remain enforced.

Apple Clang 21 record-layout output measured:

| Field | ILP32 offset | LP64 offset |
|---|---:|---:|
| `Data.rowsQuit` | `0x381c` | `0x4c38` |
| `Data.menuQuit` | `0x3830` | `0x4c50` |
| `Data.playerIconAdvMap` | `0x5a78` | `0x7050` |
| `Data.bakedGteMath` | `0x7554` | `0x8bb0` |
| `sData.AkuAkuHintState` | `0x0908` | `0x0a28` |
| `sData.lngStrings` | `0x090c` | `0x0a30` |
| `sData.botCrashNavRot` | `0x0a80` | `0x0bf0` |
| `sData.vehicleCollisionImpactStrength` | `0x0a88` | `0x0bf8` |
| `sData.talkMaskXASamplePeak` | `0x0a8c` | `0x0bfc` |
| `sData.talkMaskMaxMouthFrame` | `0x0a90` | `0x0c00` |

`BakedGteMathEntry` is now named and typed as `MatrixND *` plus `int`.
Its asserted size is eight bytes on ILP32 and 16 bytes on LP64. The retail
absolute assertions remain unchanged inside the ILP32 block; the LP64 block
asserts the measured native offsets rather than pretending native memory has
retail absolute addresses.

Two adjacent semantic problems were corrected:

- `sData.PLYROBJECTLIST` is now explicitly a pointer to serialized
  `CtrAssetRef32` entries. The remaining vehicle birth lookup resolves each
  model reference instead of casting the table to `Model **`.
- checkpoint restoration now rebases every `bakedGteMath[].physEntry` pointer.
  The table is part of the restored resident `Data` image, so cross-image-base
  restore could otherwise retain captured-process addresses.

The audit driver was generalized to accept either remaining static assertions
or a successful forced-LP64 compile, while still rejecting any non-layout
compiler error. The resulting audit is:

```text
forced LP64 compile exit:              0
compiler errors:                       0
static assertion failures:             0
pointer-to-integer coordinates:        0
integer-to-pointer coordinates:        0
unique narrowing source lines:         0
warnings:                              2 historical format-security
```

This is the first zero-error forced-LP64 compile in the migration, reducing
the original 673 layout failures to zero. It is not yet an accepted resident
map batch until a fresh i686 build preserves the warning/test baseline.

### Accepted resident-map i686 validation

The uniquely logged build used
`/tmp/ctrpad-resident-maps-build-20260730.log`. Full log inspection confirmed
the same two format-security and two maybe-uninitialized warnings as the
accepted baseline, with no new diagnostics:

```text
fresh binary timestamp: 2026-07-30 08:54:45.852280708 +0000
fresh binary size:      9,109,080 bytes
fresh binary SHA-256:   93748f6826040a3d02c60a522c07163fd0927799500e65b6de3d9eab6381d085
i686 tests:             7 / 7 passed
i686 test time:         0.76 seconds
compiler warnings:      2 format-security + 2 maybe-uninitialized (unchanged)
asset self-test:        model=checked override=checked level=checked
```

This accepts the resident-map batch and closes the forced-LP64 mechanical
compile gate at zero errors, zero layout failures, and zero narrowing
coordinates. The binary is a regression artifact only; actual macOS ARM64
linking, retail loading, replay comparison, and iPadOS work remain open.

## 2026-07-30 — First native ARM64 build and retail startup fault chain

**Status:** the native ARM64 build and a 2,000-frame retail smoke run are
accepted as diagnostic evidence. Deterministic parity, visible-screen review,
play input, save persistence, XA/STR coverage, and M6 are not accepted.

The global CMake gate now accepts only four- or eight-byte pointer targets and
prints an explicit 64-bit-memory-model status on LP64
(`CMakeLists.txt:7-15`). SSE remains conditional on an x86 target and the
portable aliasing/wrap flags remain compiler-probed
(`CMakeLists.txt:123-146`). `CMakePresets.json:117-131` adds a fresh Ninja,
RelWithDebInfo, `arm64` macOS preset. The obsolete
`build-macos-arm64-baseline` evidence directory was not modified.

The first clean configure and build completed on Apple M2 with Apple Clang 21,
Cocoa/CoreAudio/OpenGL, and ARM NEON. The build emitted 32 Apple-Clang
warnings, so it was never described as warning-clean. All seven media-free
tests passed in 0.51 seconds; `file` and `lipo` both identified a thin ARM64
Mach-O. The first binary was:

```text
version:       CTR Native 0.1.0-beta.7.1 (a40a7584c576-dirty)
timestamp:     2026-07-30 03:58:46 -0500
size:          4,114,184 bytes
SHA-256:       c4913cfe1384513951a6747e3a4dca6a1b130b272a7f14ba11f741a37ddc9f07
ARM64 CTest:   7 / 7 passed
self-test:     model=checked override=checked level=checked
```

The isolated launch directory was
`/tmp/ctrpad-macos-arm64-retail-z2MQ1h`. It contained only the binary and an
`assets/ctr-u.bin` symlink to the ignored user-owned NTSC-U BIN. No retail byte
was copied into Git. The launch passed disc validation, created an Apple
Metal-backed OpenGL 4.1 context, compiled every PSX and VRAM shader, opened a
CoreAudio stream, and placed the mempack above 4 GiB. It then exited with
status 1. This first launch was rejected; compilation and CTest did not imply
a working runtime.

### Diagnostic routes and first fault

An initial source search showed that the no-argument game loop should not
return normally. LLDB breakpoints on `exit`, `_exit`, `abort`, and the return
site did not produce usable evidence in this desktop shell: LLDB left its
child debugger-stopped and did not deliver a backtrace. Both debugger
processes created by this investigation were explicitly terminated. This was
a failed diagnostic route, not evidence that the runtime was hung.

A separate diagnostic tree,
`/tmp/ctrpad-macos-arm64-asan-91HnQZ`, was configured with:

```text
CMAKE_BUILD_TYPE=Debug
CMAKE_OSX_ARCHITECTURES=arm64
CMAKE_C_FLAGS=-fsanitize=address,undefined -fno-omit-frame-pointer
CMAKE_EXE_LINKER_FLAGS=-fsanitize=address,undefined
```

Its seven tests passed. The first ASan launch reproduced a write to
`0x000100000008`. Apple's external `atos` helper hung during the first
symbolized report, so that diagnostic process was terminated and rerun with
ASan symbolization disabled. Offline `atos` mapping gave:

```text
LIST_RemoveMember                 LIST.c:96
Channel_AllocSlot                 HOWL_Channel.c:145
cseq_opcode05_noteon              HOWL_CseqOpcode.c:189
Channel_ParseSongToChannels       HOWL_Channel.c:437
howl_PlayAudio_Update             HOWL_Playback.c:91
MainDrawCb_Vsync                  MainDrawCb.c:43
Native_EmitVBlank                 native_platform.c:634
VSync                             native_platform.c:730
StateZero                         MainMain.c:671
CTR_Main                          MainMain.c:103
main                              main.c:278
```

The cause was a literal retail stride in the audio channel free-list:
`ChannelStats` is `0x20` bytes on ILP32 but `0x28` on LP64 because its
intrusive links are native pointers. `Voiceline_PoolInit` now uses
`sizeof(struct ChannelStats)` and the real array length
(`game/HOWL/HOWL_Voiceline.c:22-25`). This keeps the retail count while
preventing links from being written through adjacent records.

### Second and third sanitizer boundaries

After the channel stride fix, UBSan advanced into load stage 5 and reported an
eight-byte pointer read from the four-byte-aligned first MPK word at
`LOAD_TenStages.c:341`. That word is a relocated serialized
`CtrAssetRef32`, not a host pointer. It is now read as four bytes and resolved
through the checked guest-reference boundary
(`game/LOAD/LOAD_TenStages.c:338-350`).

The next run advanced into rendering and reported a misaligned
`RenderBucketEntry` at `RenderBucket_QueueExecute.c:2146`. Native had reused a
byte-oriented RDATA scratch address while each LP64 queue entry contains two
eight-byte pointers. The fix:

- defines aligned native storage sized for the same maximum number of retail
  entries (`game/MAIN/MainInit.c:3-18`);
- points the native queue at that host storage without consuming the
  retail-pressure mempack (`game/MAIN/MainInit.c:286-292`); and
- writes a complete native-pointer terminator instead of clearing only four
  bytes (`game/MAIN/MainFrame_RenderFrame.c:675-708`).

The sanitizer build then remained in the retail game loop for 2,000 frames,
reported 31.24 FPS, and emitted no further ASan/UBSan diagnostic before the
run was deliberately stopped with Ctrl-C. A `screencapture` attempt failed
with `could not create image from display`, so this is runtime/sanitizer
evidence, not visual acceptance.

### Normal ARM64 and i686 acceptance evidence

The normal ARM64 rebuild log is
`/tmp/ctrpad-macos-arm64-runtime-fix-build-20260730.log`. A temporary-bundle
setup command first failed harmlessly because it used a repository-relative
binary path while its working directory was already the temporary bundle.
The corrected command used the absolute binary path and created
`/tmp/ctrpad-macos-arm64-retail-fixed-jmxRuQ`. The normal binary reached the
same 2,000-frame marker at 31.25 FPS before a deliberate Ctrl-C.

```text
ARM64 binary timestamp:  2026-07-30 04:16:26 -0500
ARM64 binary size:       4,114,232 bytes
ARM64 binary SHA-256:    f2ac8e010ac83c5b4894c24d5f12b644a4ddcf1e33f02ff246c823c3f5ef9cc5
ARM64 architecture:      thin Mach-O arm64
ARM64 tests:             7 / 7 passed in 0.37 seconds
ARM64 build warnings:    32
forced-LP64 errors:      0
forced-LP64 assertions:  0
pointer narrowing sites: 0
normal retail smoke:     2,000 frames, 31.25 FPS, deliberate stop
```

The i686 regression build log is
`/tmp/ctrpad-runtime-fix-i686-build-20260730.log`:

```text
i686 binary timestamp: 2026-07-30 09:19:25.564188336 +0000
i686 binary size:      9,109,708 bytes
i686 binary SHA-256:   1e46e4ef61f91e518b313f4feee5d8e14b809b45d27fb98f58a47bdefac04e23
i686 tests:            7 / 7 passed in 0.41 seconds
i686 warnings:         2 format-security + 2 maybe-uninitialized (unchanged)
```

`git diff --check` also passed. These results accept the first native ARM64
link/load/render smoke boundary and preserve the 32-bit media-free regression
gate. They do not prove a particular screen was visible, controller/keyboard
playability, audio/video correctness, save persistence, cross-process
checkpoint rebinding, or equality with the 24,232-frame i686 golden report.

## 2026-07-30 — ARM64 checkpoint v3, invalid seeded run, and cross-process restore

**Status:** checkpoint capture and a 178-frame cross-process bootstrap restore
pass on macOS ARM64. The imported i686 input stream is not a valid ARM64
coverage run because its inputs begin against a different asynchronous
loading schedule. Full 24,232-frame parity remains open.

### Why checkpoint v2 failed before frame zero

The first replay-seeded ARM64 attempt used
`/tmp/ctrpad-macos-arm64-retail-fixed-jmxRuQ` and created report
`debug/reports/20260730/ctr-042341`. It stopped safely before frame zero with:

```text
[CTR State] invalid checkpoint size: 0
[CTR Replay] replay-seeded recording stopped early at frame 0 of 24232
```

Checkpoint version 2 stored address-range starts and the image/code anchor in
`u32`. `NativeCheckpoint_InitHeader` therefore rejected normal ARM64 image and
global addresses above 4 GiB. This was a developer-format width defect, not a
retail loader defect.

Checkpoint version 3 now records the producing pointer width, stores region
starts and the code anchor as `u64`, reads/writes native pointer slots as
`uintptr_t`, validates range arithmetic without narrowing, and relocates image
pointers on either side of the anchor. LP64 `CtrAssetRef32` PMAP slots remain
serialized four-byte references rather than being rewritten as host pointers.
The changes are in
`platform/native_checkpoint.c:19-105,390-559,2181-2265`.

### The visibly stuck 1,900-frame attempt was rejected

The first v3 capture ran in
`/tmp/ctrpad-macos-arm64-checkpoint-v3-mirH98` and produced report
`debug/reports/20260730/ctr-042714`:

```text
frame_count=1900
checkpoint_count=7
checkpoint_size=4434548
checkpoint frames=0,300,600,900,1200,1500,1800
frame-0 checkpoint checksum=0x5e37847c
```

This proved that ARM64 could capture checkpoints at process addresses above
4 GiB. It did **not** prove valid gameplay. The user observed that the visible
game flow was stuck while the internal frame counter continued. The run was
stopped deliberately at frame 1,900 and marked invalid.

Prefix comparison against the fresh complete i686 replay
`/tmp/ctrpad-seeded-current-20260729/debug/reports/20260730/ctr-051139`
showed:

```text
timing:       0 equal, 1900 mismatched (frames 0-1899)
rng:        11 equal, 1889 mismatched (frames 11-1899)
drivers:    11 equal, 1889 mismatched (frames 11-1899)
world:     304 equal, 1596 mismatched
allocation:  0 equal, 1900 mismatched
root:        0 equal, 1900 mismatched
```

Replay version 2 supplies pad snapshots per tracked game frame but omits
VBlanks emitted between tracked frames during asynchronous loading. On a
fresh ARM64 boot, the inputs therefore land on different menu/loading states.
Advancing frame counters are not a substitute for visible flow or state
equality.

The limitation is not ARM64-specific. Comparing the original finalized i686
golden report with a fresh replay-seeded i686 report also showed extensive
fresh-timing drift: only two timing frames and two root frames were equal.
`--record-from-replay` is therefore automation input, not a deterministic
cross-run parity oracle.

### Cross-process restore fault chain

The first attempt to replay report `ctr-042714` in a fresh process failed with
signal 139. macOS diagnostic
`~/Library/Logs/DiagnosticReports/ctr_native-2026-07-30-043045.ips`
identified `GAMEPAD_ProcessHold` reading a controller packet through an old
process address. The outer `sdata.gGamepads` pointer had been relocated, but
the eight nested `GamepadBuffer.ptrControllerPacket` pointers had not.

Restore now relocates those packet pointers and optional racing-wheel pointers
(`platform/native_checkpoint.c:1575-1581`). The aligned render-bucket arena is
static native storage outside the captured retail regions, so it is explicitly
rebound after restore (`game/MAIN/MainInit.c:21-31`;
`platform/native_checkpoint.c:2377-2381`).

A rebuilt 364-frame report, `ctr-043321`, then advanced farther before signal
10. Diagnostic
`~/Library/Logs/DiagnosticReports/ctr_native-2026-07-30-043413.ips`
located a write through a stale `Song.CseqSequences[]` pointer at
`game/HOWL/HOWL_Channel.c:409`. Restore now relocates:

- every `Song.CseqSequences[]` entry;
- every `SongSeq.firstNote` and `SongSeq.currNote`;
- `advHubSongSet.ptrSongSetBits` and `howl_endOfHowl`;
- `Data.audioMeta[].name`, `MetaDataCharacters[].name_Debug`, and terrain
  emitter pointers; and
- the first native-pointer slot in every small/medium/large process-stack
  pool item.

The ownership code is
`platform/native_checkpoint.c:707-731,1368-1501,1544-1628,1886-1888`.

To avoid another crash-by-crash search, restore gained a final safety scan.
It walks aligned native-pointer slots in every restored captured region,
reports values still owned by an old-process range, and rejects the restore
before gameplay. The first scan, on report `ctr-043640`, safely rejected 172
stale pointers and named their region/offsets. DWARF field mapping reduced
those offsets to the owners listed above. The guard is
`platform/native_checkpoint.c:1964-2001,2389-2393`.

### Failed micro-capture and accepted restore evidence

One deliberately short report, `ctr-043301`, was stopped before replay frame
zero and contained no usable checkpoint. It was discarded, not counted as a
pass.

After the relocation correction, report
`/tmp/ctrpad-macos-arm64-checkpoint-v3b-LnvgJP/debug/reports/20260730/ctr-044204`
captured 178 frames and one frame-zero checkpoint under:

```text
record sdata:   0x102e3e318
record gGT:     0x102e48930
record mempack: 0x102f145e0
checkpoint checksum: 0x91252e23
```

A separate launch of the same binary restored it under different addresses:

```text
playback sdata:   0x104b1a318
playback gGT:     0x104b24930
playback mempack: 0x104bf05e0
stale-pointer findings: 0
result: replay finished after 178 frames
exit status: 0
```

The raw whole-checkpoint checksum changes across processes because it includes
host addresses; it remains diagnostic only. The per-frame canonical replay
comparison completed without divergence. This accepts the ARM64 bootstrap
cross-process restore boundary, not full rolling-checkpoint coverage and not
the 24,232-frame retail parity run.

## 2026-07-30 — Visible stall, pool pressure, first race, and physics constants

**Status:** the reported intro stall and the subsequent ARM64 runtime/layout
boundaries are corrected. A fresh, post-correction 2,200-frame run enters
Roo's Tubes, executes 489 race frames, writes eight rolling checkpoints, and
exits 0. This is strong runtime evidence, not acceptance of full retail
parity, visual output, an entire race, saves, or M6.

The user reported that the game appeared stuck and asked that investigation
continue. The source input was the complete fresh i686 report:

```text
/tmp/ctrpad-seeded-current-20260729/debug/reports/20260730/ctr-051139
frames: 24,232
```

All diagnostic ARM64 runs in this section used an isolated tree,
`/tmp/ctrpad-arm64-input-diag-VYgBb3`, with the user-owned ignored
`assets/ctr-u.bin` and an input-only 2,200-frame prefix. No retail byte was
added to Git.

### Reproducing the visible stall at frame 307

Frame-record inspection showed the first game-visible disagreement precisely:

```text
frame 307 input: retail BTN_START on both runs
i686 loading stage: -1 -> -4
ARM64 loading stage: remains -1
```

LLDB at the ARM64 frame established that input was not the cause:

```text
connected controllers: 1
buttonsHeldPrev:        0x0000
buttonsHeldCurr:        0x1000
buttonsTapped:          0x1000
msInThisLEV:            approximately 9,456 / 9,472
minimum skip time:      5,792
gameMode1:              0x20000000 (GAME_CUTSCENE)
loading stage / level:  -1 / 41
```

The process thread buckets contained PLAYER and CAMERA threads but no
cutscene thread. A conditional breakpoint at the cutscene skip branch in
`CS_Thread.c` was never reached. This rejected the initial input/timer
hypothesis and moved the fault to cutscene process birth.

`PROC_BirthWithObject` deliberately accepts an object only when its size is
strictly less than the usable stack-pool slot. The pools still used retail
strides even though their host objects contained widened pointers:

```text
pool       retail usable bytes   demonstrated LP64 object
small      64                    MineWeapon=64, WoodDoor=80
medium     128                   CutsceneObj=144, WarpPad=160
```

The allocation therefore failed safely but silently; no cutscene thread was
born, so valid Start input had no consumer. The correction keeps the retail
item counts but derives the LP64 slot stride from the largest supported
object, including the allocator's strict-less-than byte and alignment. Static
assertions cover every known small/medium object
(`game/MAIN/MainInit.c:24-72`).

The fresh 330-frame report
`debug/reports/20260730/ctr-050035` then performed the expected transition at
the exact source frame:

```text
frame 307: level 41, loading -1 -> -4
exit status: 0
```

This accepted the visible-stall fix, not the full run.

### The widened pools exposed retail-memory exhaustion at frame 1709

The first 2,200-frame extension created report `ctr-050129`. It stopped after
1,709 records while loading level 3 at stage 7 and consumed one CPU core.
Attaching LLDB found the intentional allocation-failure loop:

```text
MEMPACK_AllocMem
JitPool_Init(itemSize=152, Particle)
MainInit_JitPoolsNew
LOAD_TenStages(stage=7)
CTR_Main

free bytes:                 10,888
requested particle pool:   19,456
```

The host-width pool allocations were correct in isolation but had consumed
more of the fixed retail-pressure arena:

```text
pool       maximum LP64 bytes above retail
Thread     4,608
Instance  26,112
Small      3,200
Medium     1,536
Large      1,472
Particle   3,584
Rain         128
total     40,640 = 0x9ec0
```

Simply enlarging the full backing store would hide pressure bugs and give
gameplay more free memory than retail. The accepted design instead:

1. keeps the physical native backing at exactly `0x200000`;
2. moves the LP64 arena start earlier by the maximum `0x9ec0` pool overhead,
   leaving the historical arena end unchanged;
3. computes the actual overhead for the current player/pool configuration;
4. reserves `maximum - actual` before creating pools.

Consequently, widened pools plus the reservation consume the same normalized
budget, and the remaining gameplay allocation pressure is retail-equivalent
for supported configurations. The constant, formula assertion, arena
geometry, and reservation are in `include/platform/native_memory.h:11`,
`platform/native_memory.c:28-29`, and `game/MAIN/MainInit.c:74-135,455`.

Report `ctr-051321` completed 1,711 frames and exit 0. The full Roo's Tubes
loader sequence matched the i686 source exactly:

```text
1684 level 39 stage -4
1685 level  3 stage  0
1686 level  3 stage  1
1687 level  3 stage  2
1688 level  3 stage  3
1689 level  3 stage  4
1690 level  3 stage  5
1691 level  3 stage  6
1705 level  3 stage  7
1709 level  3 stage  8
1710 level  3 stage  9
```

### First race-frame crash and a rejected visibility diagnosis

The 2,200-frame report `ctr-051459` progressed through source frame 1710 and
then crashed before recording frame 1711. Apple diagnostic
`~/Library/Logs/DiagnosticReports/ctr_native-2026-07-30-051615.ips`
symbolized the invalid `0x40` access to:

```text
RenderAllLevelGeometry (MainFrame_RenderFrame.c:909)
MainFrame_RenderFrame
CTR_Main
```

On LP64, `VisMem.visOVertList` begins at offset `0x40`, proving
`gGT->visMem1 == NULL`. The materializer returned null without preserving the
reason. Diagnostic logging was added for the visibility asset, mesh, BSP
root, cache, BSP allocation, and each guest-reference-backed list
(`platform/native_asset_ref.c:1068-1195`).

The instrumented run `ctr-052018` and diagnostic
`ctr_native-2026-07-30-052133.ips` reported:

```text
context=LOAD_TenStages level visibility memory
player=1
list=leaf
ref=0x00000000
words=32
status=null
bspNodes=1011 quadBlocks=1806 waterVertices=3291 sceneryVertices=0
```

This rejected the corruption hypothesis. A one-player retail LEV
intentionally leaves inactive player slots 1-3 null. The LP64 sidecar had
incorrectly required all four destinations. It now resolves and validates
every populated serialized slot while preserving null inactive slots.
`MainInit` separately verifies the lists required by the active player count,
and level rendering defensively returns if the visibility sidecar is absent
(`platform/native_asset_ref.c:1102-1137`,
`game/MAIN/MainInit.c:152-244`,
`game/MAIN/MainFrame_RenderFrame.c:863-883`).

### The next race boundary exposed all misplaced physics constants

With visibility corrected, report `ctr-052401` entered the race, marked
driver 0 active at frame 1711, and wrote checkpoint 6 at frame 1800. It later
ended with `SIGTRAP`. Diagnostic
`~/Library/Logs/DiagnosticReports/ctr_native-2026-07-30-052530.ips`
symbolized the explicit divide guard:

```text
CTR_MipsDiv
VehPhysCrash_WeightedAverage
VehPhysCrash_WeightedVelocity
VehPhysCrash_AnyTwoCars
VehPhysForce_CollideDrivers
MainFrame_GameLogic
CTR_Main
```

The collision-weight divisor was zero because both real
`Driver.const_CollisionWeight` fields were zero. The root cause was the
retail `MetaPhys` table: it stores offsets `0x416..0x484` into the ILP32
`Driver`, and `VehBirth_SetConsts` still applied those offsets directly to
the widened LP64 struct. Pointers before the constant block move that native
block forward by `0x68` bytes. All 65 constants—gravity, jump, acceleration,
speed, steering, turbo, collision weight, and the prototype key—had been
written `0x68` bytes too early, corrupting other runtime fields and leaving
the real constants uninitialized.

The divide semantics were not weakened. `VehBirth` now translates every
retail table offset relative to the named native `const_Gravity` block and
checks the legal retail span. Compile-time assertions prove that collision
weight and the end of the constant block retain their internal retail
relative offsets (`game/Vehicle/VehBirth.c:580-716`).

A new media-free self-test applies all 65 constants for all four engine
classes, checks every byte-sized/halfword/word value at its translated
destination, and rejects out-of-range retail offsets. It is exposed through
`--self-test-vehicle-constants` and CTest
(`game/Vehicle/VehBirth.c:637-682`, `main.c:174-214`,
`CMakeLists.txt:191-194`).

### End-of-race quip offsets closed from the real retail metadata

The source audit found the same ABI mistake in `UI_VsQuipReadDriver`: the
NTSC-U quip metadata supplies retail `Driver` offsets, but the reader added
them directly to the native host struct. This path requires at least two
players and therefore was not exercised by the one-player 2,200-frame
coverage segment.

Rather than invent a mapping from source declarations alone, LLDB was used to
inspect both real metadata tables in `data.data850`. The VS table occupies
`0x170..0x518`, the battle table occupies `0x730..0x850`, and together they
contain 51 records. Forty-six records use the generic driver reader and refer
only to three internally contiguous retail scalar ranges:

```text
retail range   native named anchor
0x4f0..0x4f7  Driver.quip1
0x514..0x56b  Driver.timeElapsedInRace
0x574..0x57f  Driver.NumMissilesComparedToNumAttacks
```

One condition-type-3 record describes the eight-byte
`numTimesAttackedByPlayer` array at retail offset `0x560`; the game already
accesses that array through its named native field and never sends the record
through the generic reader.

`UI_VsQuipResolveDriverField` now translates only those three proven ranges
from their retail anchors to their native named anchors. It accepts only the
one-, two-, and four-byte widths used by the generic reader, rejects a value
that would cross a range boundary, and preserves the signed-halfword behavior
of the retail function. Compile-time assertions prove that the internal
layout of every translated scalar range remains unchanged
(`game/UI/UI_VsQuip.c:3-87`).

The `ctr_native_vs_quip_offsets` regression test walks all 51 real records,
requires exactly 46 generic reads, verifies the special eight-byte record,
round-trips every supported value width, and rejects offsets immediately
outside the legal ranges. It runs without retail media through
`--self-test-vs-quip-offsets`
(`game/UI/UI_VsQuip.c:154-293`, `main.c:179-224`,
`CMakeLists.txt:195-198`).

The first i686 validation attempt is intentionally retained as a rejected
result. CMake registered the ninth test, but Ninja had reused the previous
single-translation-unit executable. That old binary did not recognize the
new flag, started the normal asset path, and failed for missing media. The
isolated verification object's absence and the binary timestamp proved the
stale-artifact cause. Rebuilding `main.c.o` and relinking only
`/tmp/ctrpad-i686-verify-f8TwcX` produced the accepted 9/9 result. The
immutable `build-linux-i686-baseline/ctr_native` was never modified.

### Render-list construction was still writing through retail pointer geometry

Continuing the raw-offset audit found a third instance of the same general
mistake in `RenderLists.c`. The list walkers received native
`DrawLevelOvr1PRenderList` storage but selected list-head pointer slots with
retail ILP32 arithmetic:

```text
target                   old retail calculation   native LP64 location
slot N BSP head          N * 8 + 4                N * 16 + 8
full-dynamic BSP head    0x28                     0x50
```

On LP64, the old eight-byte pointer stores were not merely selecting the
wrong list. For example, the slot-zero store began four bytes into
`ptrQuadBlocksRendered` and straddled it and `bspListStart`; the old
full-dynamic offset selected slot two's normal BSP head. ARM64 permits many
unaligned accesses, which explains why the game loop could survive 489 race
frames, but that did not make the list ownership or visual result valid.

`RenderLists_GetHead` now accepts the native render-list type, checks its slot
range, and returns named `bspListStart` or `bspListStart_FullDynamic`
addresses. Both 1P/2P and 3P/4P walkers use the helper. No serialized offset
is involved in the native operation
(`game/RenderLevel/RenderLists.c:160-172,235-346`).

The new `ctr_native_render_lists` test resolves and writes all five normal
heads plus the full-dynamic head and rejects out-of-range indices. Its
architecture report proves the two intended layouts:

```text
i686:  pointer-size=4 slot-size=8  full-dynamic=0x28
ARM64: pointer-size=8 slot-size=16 full-dynamic=0x50
```

It is exposed through `--self-test-render-lists`
(`game/RenderLevel/RenderLists.c:376-426`, `main.c:184-233`,
`CMakeLists.txt:199-202`).

Because list construction runs on every race-render frame, the earlier
`ctr-054005` report was retained as evidence for the preceding runtime fixes
but superseded as the final post-audit report. The complete segment was rerun
as `ctr-055513`. During live monitoring, `wc` showed 1,602 replay lines at
the moment the process disappeared, so this was provisionally treated as an
early exit. That reading was a buffered-file artifact: finalization flushed
all 2,200 records, `metadata.txt` says `finalized=1`, the log records
checkpoints 0-7 and race activation at frame 1711, and the log closes normally.

Comparing `ctr-054005` with `ctr-055513` showed 2,200/2,200 equal RNG, driver,
world, and allocation digest components. Timing/root match through frame 441
and then diverge as before from host scheduling. Thus the corrected list
addresses do not alter tracked game state in this segment, while removing
untracked pointer corruption from the renderer.

### Red-beaker rain still read widened instances at retail byte offsets

The next raw-address candidate was not a false positive. Red-beaker rain
advanced a cloud `Instance *` by `playerIndex * 0x88` and read transform
halfwords at `+0x8c`, `+0x90`, and `+0x94`. On retail these addresses select
`InstDrawPerPlayer.mvp.t[0..2]`. On LP64, `Instance` grows from `0x74` to
`0xa0` and each draw record grows from `0x88` to `0xb0`; the old player-zero
reads therefore landed among widened instance function/thread fields rather
than in the transform.

The remaining `+0x50` signed-byte read has unusual but deliberate retail
aliasing. With the same per-player stride, player zero selects
`Instance.depthBiasNormal`; players one through three select the low byte of
the preceding draw record's retail-offset-`0xd8` field, now named `lodIndex`.
The correction preserves that ILP32 behavior explicitly:

- `RedBeaker_GetCloudDraw` returns `INST_GETIDPP(instance)[playerIndex]`;
- transform coordinates come from the named `mvp.t[]` fields with the
  original signed-halfword truncation; and
- `RedBeaker_GetCloudOtBias` selects `depthBiasNormal` or the preceding
  `lodIndex` low byte by name.

Both helpers reject invalid player indices. The renderer no longer performs
retail byte arithmetic on the widened instance/draw-record aggregate
(`game/RenderWeather/RedBeaker_RenderRain.c:20-47,215-300`).

The new `ctr_native_red_beaker_layout` test builds an instance plus four draw
records, validates all four native record addresses and transform triplets,
checks the player-zero/later-player bias distinction, and checks bounds. Its
reported layouts are:

```text
i686:  pointer-size=4 instance=0x74 idpp=0x88
ARM64: pointer-size=8 instance=0xa0 idpp=0xb0
```

It runs through `--self-test-red-beaker-layout`
(`game/RenderWeather/RedBeaker_RenderRain.c:304-356`, `main.c`,
`CMakeLists.txt`).

This item-effect path was not activated by the 2,200-frame one-player input
prefix, so the structural test—not the coverage replay—is the evidence for
the correction. Nevertheless, the complete current binary was rerun to rule
out covered-path regressions. Report `ctr-060443` finalized 2,200 frames,
eight checkpoints, and race activation at frame 1711. It matches
`ctr-055513` for all 2,200 frames in all six digest components: timing, RNG,
drivers, world, allocation, and root.

### Passing segment and exact limits of the evidence

The first full report after the runtime-crash fixes was `ctr-052749`.
`ctr-054005` repeated it after the vehicle-constant regression test. After the
subsequent quip, render-list, and red-beaker offset audit, the final evidence
report is:

```text
/tmp/ctrpad-arm64-input-diag-VYgBb3/debug/reports/20260730/ctr-060443
frame count:       2,200
checkpoint count:  8 (frames 0,300,600,900,1200,1500,1800,2100)
race activation:   frame 1,711
race frames:       489
process result:    exit 0
```

The final report and its immediate post-render-list predecessor matched all
six digest components for all 2,200 frames.

The i686-prefix comparison remains deliberately non-passing:

```text
component    equal   mismatched
timing           0        2200
rng             89        2111
drivers       1709         491
world         1924         276
allocation       0        2200
root             0        2200
```

More narrowly, frame counter, main game state, and level ID match all 2,200
frames. Loading stage matches 2,197 frames; only initial boot frames 9-11
differ, while the frame-307 skip and complete frame-1684..1711 race load
align. Driver state is equal through the first 20 active race frames and
diverges at frame 1731 after the fresh-boot RNG streams had already separated.
This reinforces the earlier finding that replay-seeded asynchronous fresh
boots are coverage automation, not yet the cross-architecture parity oracle.

Final build gates:

```text
macOS ARM64: thin Mach-O arm64, 11/11 CTest passed in 0.52 s
Apple build: 32 existing warnings
i686 build:  ELF 32-bit Intel 80386 in /tmp/ctrpad-i686-verify-f8TwcX
i686 CTest:  11/11 passed in 0.46 s
i686 warnings: 2 format-security + 2 maybe-uninitialized (established set)
preserved baseline build-linux-i686-baseline/ctr_native: not modified
forced LP64: 0 errors, 0 assertions, 0 narrowing coordinates
git diff --check: passed
```

The quip metadata path now has structural coverage from the real tables, but
its full end-of-race/VS runtime behavior is still not exercised. Full
24,232-frame parity, visual review, an entire race, multiplayer/end-of-race
play, controller play, memcard persistence, XA/STR coverage, and iOS/iPadOS
work remain open.

## 2026-07-30 — “Stuck” follow-up: sanitizer-led renderer and checkpoint closure

**Source base:** `a40a7584c5761ce24b54bbb9280058a1d84b9dd5`

**Status:** the complete 2,200-frame segment now passes a fresh combined
ASan/UBSan run. Four defects hidden by the ordinary ARM64 run were found and
corrected in sequence. This section intentionally retains every rejected run
and the operator correction that occurred between them.

### Why the ordinary passing run was not accepted as sufficient

Report `ctr-060443` reached frame 2,200, but the preceding LP64 failures had
shown that permissive ARM64 memory behavior can let corrupt render state
survive. A new build was therefore configured in
`/tmp/ctrpad-macos-arm64-asan-final-KgCmb9` with:

```text
-fsanitize=address,undefined
-fno-omit-frame-pointer
ASAN_OPTIONS=symbolize=0:abort_on_error=1
UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1
```

The build and its then-current 11 tests passed. The first attempt to start the
retail replay used the nonexistent path
`ctr-060443/source/input.ctrreplay`; report replay files are stored directly
beside `metadata.txt`. That invocation exited normally with “failed to open
replay seed input” and created no gameplay evidence. The command was corrected
to `ctr-060443/input.ctrreplay`; this was an operator path error, not a product
defect.

### Rejected sanitizer run 1: misaligned audio snapshot before frame zero

```text
report:       /tmp/ctrpad-arm64-asan-run-3eWrOg/debug/reports/20260730/ctr-060956
finalized:    0
frames:       0
checkpoints:  0
failure:      platform/native_audio.c, NativeAudio_CaptureState
```

UBSan rejected a `NativeAudioSnapshot *` at an address ending in `...f34`.
The native state bundle aligns regions to four bytes, while the snapshot
contains 64-bit members and requires eight-byte alignment on ARM64. Ordinary
execution happened to tolerate the cast, but the C access was undefined.

The checkpoint format and version did not need to change. Audio capture and
restore now use a naturally aligned static snapshot as serialization scratch
and cross the arbitrary byte-stream boundary with `memcpy`. Capture copies
under the audio lock so the serialized state remains coherent; restore copies
before validating or accessing any member
(`platform/native_audio.c:2540-2605`). The new
`ctr_native_audio_state_alignment` test deliberately places its blob at
address modulo eight equal to four, then captures and restores it through the
public API. This raised the media-free suite from 11 to 12 tests on both
architectures.

### Rejected sanitizer run 2: LP64 render-list offset mistaken for table index

```text
report:       /tmp/ctrpad-arm64-asan-replay-56Uzoc/debug/reports/20260730/ctr-061439
finalized:    0
frames:       1,500
checkpoints:  6
failure:      DrawLevelOvr1P bucket index 20 outside an 11-entry table
```

UBSan stopped at `DrawLevelOvr1P.c:9689` in that build. Retail render-list
fields are packed 32-bit words, so `retailByteOffset / 4` is the canonical
setup/handler-table index. The already-corrected native LP64 render list uses
eight-byte pointers: full dynamic is at host offset `0x50`, and the old
calculation produced index 20 rather than canonical index 10.

The 1P, 2P, and 4P dispatchers now iterate canonical bucket indices. A bucket
record selects the corresponding named host field, while setup/handler arrays
use the canonical index and scratch `currentBucketOffset` receives
`bucketIndex * 4`, the retail value. The 4P table naturally covers canonical
indices 7 through 0. This corrects both the out-of-bounds access and the
emulated scratch value (`game/226/226_00_DrawLevelOvr1P.c:9725-9754`,
`game/227/227_00_DrawLevelOvr2P.c`, and
`game/229/229_00_DrawLevelOvr4P.c`).

### Rejected sanitizer run 3: water texture reconstructed from low 32 bits

```text
report:       /tmp/ctrpad-arm64-asan-buckets-ITtKQg/debug/reports/20260730/ctr-061954
finalized:    0
frames:       1,500
checkpoints:  6
failure:      water texture read through 0x05028d7c
host pointer: 0x105028d7c
```

After the bucket fix, AddressSanitizer reached the correct water-list handler.
`SeedWaterListState` reconstructed `waterEnvMap` from retail scratch member
`waterEnvMapPtr32`, dropping the high address bits before reading its UV
words. Auditing every `*Ptr32` access in overlays 226-229 found three more
live host cursors with the same risk: clip-record cursor, rendered-list
cursor, and visibility-word cursor.

All four now have full-width host sidecars. Their retail scratch members are
still written exactly as 32-bit words for overlay behavior and canonical
state, but no host dereference is reconstructed from those words. Entry and
viewport setup refresh the sidecars before use, including 1P, 2P, 3P, and 4P
water-map entry points. A follow-up search confirmed that overlays 226-229 no
longer read a host pointer back from any `*Ptr32` scratch member
(`game/226/226_00_DrawLevelOvr1P.c:75-81,881-894,7274-7333,
7990-8015,9024-9051`).

### Rejected sanitizer run 4: clipped OT pointer reconstructed from low 32 bits

```text
report:       /tmp/ctrpad-arm64-asan-sidecars-x1lhYu/debug/reports/20260730/ctr-062437
finalized:    0
frames:       1,500 (metadata last flush)
race active:  frame 1,711 in the log
failure:      raw primitive OT read through 0x03003de0
```

This run crossed the former water crash and activated the race. It then
reached the clipped-primitive consumer, where `DrawLevelOvr1PClipRecord`
correctly retains a retail four-byte absolute `otEntry` but native code cast
that field directly back to `uint32_t *`.

The record format remains unchanged. Its consumer now subtracts the low
32-bit OT base using unsigned arithmetic, validates four-byte alignment and a
bounded index through retail maximum OT entry 1020, and adds that offset to
the current viewport's full-width `pb->ptrOT`. This also handles a low-word
wrap without guessing high address bits. Invalid records stop the overlay
instead of dereferencing an arbitrary address
(`game/226/226_00_DrawLevelOvr1P.c:3074-3131`).

The expanded `ctr_native_render_lists` self-test now covers six named heads,
all 11 canonical bucket roles, the four full-width cursor sidecars, and a
synthetic clipped-record OT rebase. Its accepted output ends with:

```text
dispatch=canonical-index host-cursors=4 clip-ot=rebased
```

### Accepted sanitizer and ordinary runs

The fourth corrected retry completed without any ASan/UBSan report:

```text
sanitizer report: /tmp/ctrpad-arm64-asan-clipot-XvhS0n/debug/reports/20260730/ctr-062801
finalized:        1
frames:           2,200
checkpoints:      8
race activation: frame 1,711
process result:  exit 0
```

The current ordinary binary independently completed:

```text
normal report:   /tmp/ctrpad-arm64-current-normal-HiaVBo/debug/reports/20260730/ctr-062951
finalized:        1
frames:           2,200
checkpoints:      8
race activation: frame 1,711
process result:  exit 0
replay SHA-256:  a356f2e88815babe1f16372387d87e4fc98370b20d343970f817dda25d06d239
```

Relative to pre-sanitizer report `ctr-060443`, the current normal run matches
RNG, drivers, world, and allocation for all 2,200 frames. Timing and the root
match through frame 1,755 and differ for frames 1,756-2,199, after corrected
race rendering begins. The old report is not an oracle for that range: it was
using the invalid bucket index and truncated pointers described above.
Sanitizer output is also not used as the parity oracle because instrumentation
changes asynchronous host timing; it nevertheless matched RNG, world, and
allocation for every frame and proved memory/UB safety for the covered path.

### Final mechanical and cross-width gates

The latest isolated i686 rebuild was forced to recompile the unity object
rather than accepting a stale executable. It produced:

```text
ELF 32-bit LSB PIE, Intel 80386
Build ID: d37c0df7911bd6996714a4d2db76b494e823afe6
CTest: 12/12 passed
immutable build-linux-i686-baseline/ctr_native: not modified
```

Current ARM64 normal and sanitizer suites each pass 12/12. The forced-LP64
audit and whitespace gate report:

```text
forced_lp64_compile_exit=0
forced_lp64_compile_errors=0
forced_lp64_static_assert_failures=0
pointer_to_integer_coordinates=0
integer_to_pointer_coordinates=0
unique_pointer_narrowing_source_lines=0
git diff --check: passed
```

This accepts the 2,200-frame sanitizer closure, not M6 or the project as a
whole. A complete race, all multiplayer renderers, items/effects, visual
comparison, save persistence, XA/STR coverage, and iPadOS packaging remain
open.

## 2026-07-30 — “Keep reviewing”: repeatability rejection and exact-binary checkpoint proof

**Source base:** `a40a7584c5761ce24b54bbb9280058a1d84b9dd5`

**Status:** the apparent game stall is cleared on the covered path. A final
version-3 report records and replays all 2,200 frames across different ARM64
process layouts. This section retains the rejected repeat, two checkpoint
misdiagnoses, the validation correction, and an i686 operator error.

### A second fresh recording rejected determinism assumptions

The normal accepted coverage report `ctr-062951` was seeded again without any
source change. The second report was:

```text
/tmp/ctrpad-arm64-current-repeat-jVyhpJ/debug/reports/20260730/ctr-063215
finalized=1
frames=2200
checkpoints=8
race activation=1711
```

It was behaviorally useful but not byte-for-byte deterministic:

| Component | Equal frames | Mismatched frames |
|---|---:|---:|
| timing/root | 1,018 | 1,182 |
| RNG | 2,130 | 70 |
| drivers | 1,914 | 286 |
| world | 2,145 | 55 |
| allocation | 2,138 | 62 |

This confirms that `--record-from-replay` is coverage automation. Its input
records do not include VBlanks emitted between tracked frames during
asynchronous loading, so an equivalent fresh boot is not the parity oracle.
The repeat is retained as rejected parity evidence.

### Checkpoint investigation 1: 82 reports came from the wrong dirty binary

An initial unchanged-playback attempt appeared to fail at frame zero with 82
stale values in DATA and D230. DWARF/LLDB mapping covered metadata callbacks,
the opcode table, overlay callbacks, `RectMenu.funcPtr`, and cheat handlers.
All named fields were already passed through the image-pointer relocator.

LLDB then showed:

```text
recorded code anchor: 0x1043eccd0
live code anchor:     0x1000b8cd0
```

so ASLR genuinely differed. A breakpoint immediately before validation showed
the named opcode and cheat callbacks already held correct live addresses. The
decisive correction was to launch the exact binary copied beside the report,
not the newly rebuilt repository binary. That exact binary restored checkpoint
zero and completed all 2,200 frames with race activation at 1,711.

Both binaries printed the same identity:

```text
CTR Native 0.1.0-beta.7.1 (a40a7584c576-dirty)
```

The old identity key therefore accepted different unity-build layouts. The 82
reports were not evidence that the producing executable's relocation was
broken; they were evidence that `COMMIT-dirty` is not a binary identity.

### Replay format version 3: fingerprint the executable bytes

Replay header version 3 stores a 64-bit FNV-1a fingerprint of the running
executable in the former reserved words. The identity checksum now includes
it, metadata writes `executable_fingerprint`, and recording/playback fail if
the executable file cannot be identified. Header and frame sizes remain 148
and 440 bytes respectively. Version-2 reports are intentionally historical
artifacts for their original binaries.

The replay-gate self-test now changes only the executable fingerprint and
proves that identity rejects it:

```text
binary-identity=checked
```

### Checkpoint investigation 2: a scalar word looked exactly like a pointer

The first end-to-end version-3 recording was:

```text
/tmp/ctrpad-arm64-current-normal-HiaVBo/debug/reports/20260730/ctr-065011
frames=2200
checkpoints=8
race activation=1711
```

Its exact-binary playback rejected one purported SDATA pointer:

```text
offset=0x538
value=0x1012c1388
```

LLDB field-offset inspection proved that `0x538` is
`RaceFlag_Position`, followed by three more 16-bit race-flag scalars. The
four legal scalar values happened to concatenate into an aligned address
inside the old mempack. Scanning every aligned word in whole captured regions
cannot distinguish this from a pointer and will inevitably produce such false
positives.

The raw scan was replaced with a bounded applied-relocation ledger. Every
successful resident or image relocation records its slot, old value, and
expected live value. After all rebind/repair stages, restore rejects only a
typed slot that reverted exactly to its old value; ledger overflow is also a
hard rejection. The thirteenth media-free CTest deliberately recreates the
address-shaped scalar and a reverted relocation:

```text
[CTR State] pointer-validation self-test passed:
scalar=ignored relocated=reversion-checked
```

### Accepted version-3 record, cross-process restore, and mismatch rejection

Final report:

```text
path:                   /tmp/ctrpad-arm64-current-normal-HiaVBo/debug/reports/20260730/ctr-065640
finalized:              1
replay version:         3
frames:                 2,200
checkpoints:            8
race activation:        frame 1,711
executable fingerprint: b212b34099a6dbca
identity checksum:      0x0d57e31c
input SHA-256:          2486f393b3dc0e1712af2e1f9e797a079c7610f41387ab5f33eb144fa9a081a3
state SHA-256:          c86e4dafaf3ce806a641160d0067070153f7d1d7bd1a052d93fa156bea343f59
```

The producing process used mempack base `0x1009218b8`; playback used
`0x102f298b8`. Checkpoint zero restored, the race activated at frame 1,711,
playback completed 2,200/2,200 frames, and the process exited 0. This is an
exact-binary ASLR/process-layout proof, not cross-build portability.

A still-runnable earlier version-3 binary had the same visible
`a40a7584c576-dirty` label, the same checkpoint size `4434548`, and the same
native-state size `1676316`, but fingerprint `dd9fef13fb4ae04a`. It rejected
the report before checkpoint restore and printed both fingerprints. This is
the intended failure mode.

### Final gates, including a retained i686 operator correction

The first fresh Docker check omitted the established `-m32` compiler/linker
flags. CMake therefore detected a 64-bit Linux target, and SDL stopped because
matching X11/Wayland development libraries were unavailable. This was an
operator configuration error, not a source failure.

The corrected command used a second fresh output directory:

```text
cmake -S /src -B /out -G Ninja -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_C_FLAGS=-m32 -DCMAKE_EXE_LINKER_FLAGS=-m32
cmake --build /out --parallel 8
ctest --test-dir /out --output-on-failure
```

Accepted gates:

```text
macOS ARM64 normal CTest:     13/13
macOS ARM64 ASan/UBSan CTest: 13/13
i686 CTest:                   13/13
i686 binary:                  ELF 32-bit LSB PIE, Intel 80386
i686 Build ID:                f99490697baa07f778fcb5aa8faa37647ee78cbc
immutable baseline binary:    not modified
forced LP64 compile errors:   0
forced LP64 assertions:       0
pointer narrowing coords:     0
git diff --check:             passed
```

This closes the covered bootstrap-checkpoint failure and removes the observed
stall from the 2,200-frame intro-to-race path. It does not close whole-project
parity: non-bootstrap rolling checkpoint restore, a complete race,
multiplayer/end-of-race, save persistence, visual output, XA/STR, touch,
iOS/iPadOS lifecycle, signing, and distribution remain open.

## 2026-07-30 — “The game is stuck”: complete rolling-checkpoint investigation

**Source base:** `a40a7584c5761ce24b54bbb9280058a1d84b9dd5`

**Status:** the observed covered-path stall and all subsequently exposed
rolling-checkpoint restore defects are corrected. Every checkpoint in the
2,200-frame segment restores and reaches frame 2,200 with its exact ordinary
producing binary. Five exact ASan/UBSan frame-1,800 restores also pass,
including a real old/live mempack overlap. This is not whole-game or M6
acceptance.

### Why the review continued

The preceding `ctr-065640` result proved only checkpoint zero. The report
contained seven later records, but the scheduler always restored record zero
and required replay frame zero. Calling the covered game “unstuck” while those
live allocation states were untested would have converted an unverified area
into an assumption.

The scheduler gained:

```text
--replay-start-checkpoint INDEX
```

The index is zero-based. Playback validates the complete checkpoint file,
checks the selected record and frame mapping, computes a bounded replay-file
offset, restores that record, seeks input to its replay frame, and then
continues normally. The replay self-test checks the frame-offset calculation
and reports `checkpoint-start=checked`.

### Rejected diagnostic runs before exact identity was available

The first checkpoint-7 attempt used the older report with header bypass to
preserve diagnostic access after a rebuild. It crashed in
`MainFrame_GameLogic`; LLDB found a recorded-process `Thread.object` in a live
thread. Crash evidence is retained as:

```text
~/Library/Logs/DiagnosticReports/
  ctr_native_rolling_diag-2026-07-30-071442.ips
```

This run was useful for locating a candidate field but was not acceptance
evidence. Two later bypass outcomes made that distinction explicit:

- one reached `PhysLerpRot` with a callback offset resolving to the wrong
  driver function;
- after another code-layout shift, a callback resolved into `sdata_static`
  and execution ended with SIGBUS.

Both were rejected as fingerprint-bypass artifacts. They demonstrate why
version 3 binds native checkpoint state to executable bytes.

One file-copy retry also failed for an operator reason: a relative
`build-macos-arm64/ctr_native` source was evaluated after changing the working
directory to `/tmp`. Repeating the copy with the absolute repository path
succeeded. No source conclusion was drawn from the failed copy.

### Defect 1: live threads are not represented by `taken`

At checkpoint 7, LLDB reported:

```text
JitPools.thread.free.count  = 81
JitPools.thread.taken.count = 0
live threads in buckets     = 15
```

`PROC_BirthWithObject` removes a slot from `free` but does not add it to
`taken`. The old checkpoint walker iterated `taken`, so it relocated zero live
threads even though bucket roots reached fifteen of them. The authoritative
allocator invariant is therefore:

```text
allocated thread = fixed pool slot not present in the free list
```

Thread and thread-object relocation now enumerate that free-list complement.
The pointer-validation CTest constructs three slots, marks two free, and
proves that only the allocated slot's object pointer moves.

An exact report produced after this correction was:

```text
/tmp/ctrpad-arm64-current-normal-HiaVBo/debug/reports/20260730/ctr-072303
executable fingerprint: 7a7d7a0bb94012ed
```

Its exact checkpoint-7 playback passed the former thread failure and then
crashed later in level visibility rendering. That progression accepted the
thread diagnosis but rejected the checkpoint as a whole.

### Defect 2: level visibility and model headers are process-local caches

`gGT->visMem1` addressed a host-width visibility sidecar owned by the recording
process. The level asset references are stable checkpoint data; the heap-backed
`VisMem` tables derived from them are not. Model-header overrides use the same
process-local cache pattern.

Restore now:

1. clears all runtime model-header mappings;
2. invalidates all level visibility cache entries;
3. rebuilds `visMem1` and `visMem2` from the relocated `level1` and `level2`
   assets.

This removed the `MainFrame_VisMemFullFrame` failure without serializing host
heap ownership.

### Defect 3: level instances use the same free-list-complement invariant

Checkpoint 6 next exposed `RB_Banner` through a stale attached level
`Instance`. Dynamic instances use `JitPools.instance.taken`, but
`INSTANCE_LevInitAll` removes retail level instances directly from `free`
without adding them to `taken`. Iterating either only `taken` or only level
roots would cover one allocation path and miss the other.

All instance slots not on the free list are now relocated, including each
active per-player `InstDrawPerPlayer` record. The media-free regression uses
synthetic widened instance-plus-draw storage and proves that the allocated
slot moves while a free slot stays untouched.

Report `ctr-072935` and its exact producing binary then restored checkpoints
0 through 7 and reached frame 2,200 for all eight. This was the first complete
ordinary rolling-checkpoint pass, but it was not yet sanitizer acceptance.

### Defect 4: camera collision traversal retained a recorded quad block

The first sanitizer command used `ASAN_OPTIONS=detect_leaks=1`. The macOS
runtime reported that leak detection was unsupported and exited; the corrected
campaign used:

```text
ASAN_OPTIONS=detect_leaks=0:halt_on_error=1
UBSAN_OPTIONS=halt_on_error=1
```

Exact sanitizer report `ctr-073930` restored checkpoint 6 under a different
mempack layout and trapped in `COLL_FIXED_QUADBLK_TestTriangles`.
`CameraDC.ptrQuadBlock` still named a quad block in the recording process.
Adding that field to the typed camera relocation map cleared the finding. The
self-test now names `camera-quad=checked`.

The next ordinary acceptance report was:

```text
report:      ctr-074323
frames:      2,200
checkpoints: 8
fingerprint: 82d66eaed2589254
input SHA:   c06a7c2acef62a8e28b393632ab86377e17e57ebfc562cdbbf7ccd8a509b7eb8
state SHA:   8e7522d0118091a1f043d080ca89ff3ba637836b2dfb8dfc3b916f02147d791b
```

Its exact checkpoint results were:

| Index | Replay frame | Checksum | Result |
|---:|---:|---:|---|
| 0 | 0 | `0x7d9d816b` | frame 2,200, exit 0 |
| 1 | 300 | `0x4e2f298c` | frame 2,200, exit 0 |
| 2 | 600 | `0x4d06a285` | frame 2,200, exit 0 |
| 3 | 900 | `0x7e52dd64` | frame 2,200, exit 0 |
| 4 | 1,200 | `0x5f6ed31f` | frame 2,200, exit 0 |
| 5 | 1,500 | `0x14f50ad4` | frame 2,200, exit 0 |
| 6 | 1,800 | `0x1b00a195` | frame 2,200, exit 0 |
| 7 | 2,100 | `0x6376c4b8` | frame 2,200, exit 0 |

Full logs were retained at `/tmp/ctrpad-final-checkpoint-{0..7}.log`.

### A repeated sanitizer launch invalidated the first acceptance

Sanitizer report `ctr-075234` finalized 2,200 frames and its first exact
checkpoint-6 restore completed. Recording and restore mempack bases differed:

```text
record:  0x102d5eee0
restore: 0x1062ceee0
```

That was initially treated as sanitizer acceptance. A deliberately repeated
launch of the same exact binary and report then aborted at
`RenderBucket_QueueExecute.c:2055` while traversing the non-level instance
list. The contradictory evidence is retained in:

```text
~/Library/Logs/DiagnosticReports/
  ctr_native_rolling_asan_final-2026-07-30-080125.ips
```

The failing restore used mempack base `0x102d36ee0`, only `0x28000` below the
recording base. The old and live approximately 2 MiB mempack ranges therefore
overlapped.

The failure mechanism was double relocation:

1. the generic JitPool walker relocated an instance link from the recorded
   range to the live range;
2. the semantic instance walker visited the same typed slot;
3. the new live address still numerically fell inside the overlapping old
   range;
4. the second visit interpreted it as recorded state and applied the base
   delta again.

The resulting list cursor eventually became an unaligned data-shaped value,
which UBSan rejected. Widely separated old/live ranges concealed the defect.

An LLDB conditional breakpoint experiment was also retained as a rejected
diagnostic method. Evaluating a source expression on every render-bucket
instance was slow enough to overload audio and was interrupted. Before
interruption it showed a valid 18-entry taken list at replay frame 1,920,
supporting the conclusion that the corruption depended on layout/duplicate
relocation rather than a permanently malformed recorded list.

### Defect 5: typed relocation must be idempotent

The existing applied-relocation ledger already had the correct transaction
identity: slot address, old value, and expected live value. It now has a
fixed-capacity open-addressed index keyed by slot. Resident, image, and mixed
relocation return immediately when that slot was already applied during the
current restore.

This choice preserves:

- allocation-free restore;
- the 65,536-entry hard cap and overflow rejection;
- the typed stale-value validation introduced after the race-flag false
  positive;
- intentionally overlapping semantic walkers for threads, instances, rain,
  drivers, and object payloads.

The thirteenth CTest now constructs old range `0x1000..0x11ff` and live range
`0x1080..0x127f`, places one pointer at `0x1100`, and calls relocation twice.
The accepted result is `0x1180`, not the double-relocated `0x1200`. Its full
status is:

```text
scalar=ignored relocated=reversion-checked overlap-idempotence=checked
pool-allocation=free-list-complement camera-quad=checked
```

The first stress-loop shell used `status=$?`; zsh reserves `status` as a
read-only special parameter, so the loop stopped after its first game process.
The corrected loop used `exit_code`. This was an operator-script failure, not
a game result, and is retained because the historical log must distinguish
unrun repetitions from passing repetitions.

The first combined source patch for the slot index did not match the evolved
file context and was rejected without changing the tree. The change was then
split into reviewable pieces. During that edit, the reset helper briefly called
itself recursively instead of clearing the count and overflow flag. A focused
symbol/line review found and corrected it before any build or test was
accepted. No evidence below was produced by either failed edit state.

### Final sanitizer evidence

Fresh ASan/UBSan build:

```text
build:       /tmp/ctrpad-arm64-asan-overlap-BBfSfx
binary SHA:  eb0abc05b302c17fb4fa74530de5dd8c159d610a6d014b98188a73f1cf95ff70
CTest:       13/13
```

Final sanitizer report:

```text
report:                   ctr-081155
finalized:                1
frames/checkpoints:       2,200 / 8
executable fingerprint:   1d2e0453b2ac5f48
identity checksum:        0xecb840a2
recording mempack base:   0x106ddb3a0
input SHA-256:            84aae8fdd1a5ecb8f6b77293e50f95579064dccae138bc010fcc03ce0a217e5a
checkpoint SHA-256:       25e6a6b15519e41fe7eefbddb5ab94bd16cd62f466cc6d82fe71286cf3d9b083
```

Five exact checkpoint-6 repeats all restored checksum `0x66319a86`, reached
frame 2,200, exited 0, and emitted no ASan/UBSan report:

```text
repeat 1 live mempack: 0x1025573a0
repeat 2 live mempack: 0x1044b33a0
repeat 3 live mempack: 0x10463f3a0
repeat 4 live mempack: 0x10522f3a0
repeat 5 live mempack: 0x106c3b3a0
```

Repeat 5 is the decisive real-overlap case: its live range overlaps the
recorded range beginning at `0x106ddb3a0`, yet the exact replay completes.

### Final ordinary report

```text
report:                 ctr-081618
binary SHA-256:         7282aa566ca1237ca63f539553cc33be4ae8027de2b8c8d8c299d18bdda6c740
finalized:              1
frames/checkpoints:     2,200 / 8
race activation:        frame 1,711
executable fingerprint: 341b92a5333b95dd
identity checksum:      0x270a6b6f
input SHA-256:          f84d01ec1ce77e8da42128cec2120444be56d81d9f55b64dcaa88bbd3e9842d3
checkpoint SHA-256:     d1c6f2c74b36959d6e6f31182df716f22e9164038ef37d25062d7a2b5f2ed44f
record mempack base:    0x1008b98b8
```

The exact producing binary restored all eight ordinary checkpoints without a
header bypass:

| Index | Replay frame | Checksum | Result |
|---:|---:|---:|---|
| 0 | 0 | `0x58c1a6d3` | frame 2,200, exit 0 |
| 1 | 300 | `0x15562009` | frame 2,200, exit 0 |
| 2 | 600 | `0x71353027` | frame 2,200, exit 0 |
| 3 | 900 | `0x2b992c03` | frame 2,200, exit 0 |
| 4 | 1,200 | `0x70473816` | frame 2,200, exit 0 |
| 5 | 1,500 | `0xcff5c3e5` | frame 2,200, exit 0 |
| 6 | 1,800 | `0x72383e92` | frame 2,200, exit 0 |
| 7 | 2,100 | `0xddd34563` | frame 2,200, exit 0 |

Full logs were retained at
`/tmp/ctrpad-final-overlap-checkpoint-{0..7}.log`.

### Final cross-architecture gates

The finalized tree passed all thirteen CTests in the normal ARM64 build, the
fresh ASan/UBSan ARM64 build, and the isolated Docker `-m32` build. The latter
is:

```text
path:        /tmp/ctrpad-i686-final-m32-uGz0OL/ctr_native
format:      ELF 32-bit LSB PIE, Intel 80386
Build ID:    155f5086a4f576fc2b65367915dfa4b59d3832c2
SHA-256:     56c58301e09708c95856f20996f0166e64f2e8f1aace553431202f6d7123ebc9
CTest:       13/13
```

The final forced-LP64 audit reports:

```text
compile exit/errors/assertions: 0 / 0 / 0
pointer-to-integer coordinates: 0
integer-to-pointer coordinates: 0
unique narrowing source lines:  0
i686 object SHA-256:             7ebc25257acc868616a1616d8a1b4479068b6c9ccb27deb7fc2d703db72c357b
layout census SHA-256:           16e54f105a62fce09877e7cf8ef8723707f9acda6da9d2b85e13c1cfb38a762f
```

The protected historical baseline was checked after the final build and
remained unchanged:

```text
path:      build-linux-i686-baseline/ctr_native
SHA-256:   afe7b3d264bd2674192e71485037842ab329d1eca9c0c34ca50da5d4487c76a7
size:      8,888,128 bytes
timestamp: 2026-07-29T17:54:11-0500
```

`git diff --check` also exited 0 after the documentation update.

The complete result is bounded. It closes rolling restore for this
intro/menu/loader/early-race segment. It does not prove a complete race,
multiplayer/end-of-race, save persistence, visual parity, XA/STR, touch,
iOS/iPadOS lifecycle, signing, distribution, or cross-architecture golden
parity.

## 2026-07-30 — Complete VSync capture and cross-width RNG localization

**Scope:** investigate the apparent stall and the remaining i686/ARM64
2,200-frame parity failure using the user-populated ignored retail tree at
`ref/CTR/`. The selected input was
`CTR - Crash Team Racing (USA).bin`; it was mounted or symlinked into build
directories and was never added to Git. The protected historical executable
at `build-linux-i686-baseline/ctr_native` was not rebuilt or modified. All
i686 experiments in this section used the disposable build directory
`/tmp/ctrpad-i686-final-m32-uGz0OL`.

### The visible wait was measured execution, not a game deadlock

The instrumented Apple Silicon runs continued producing checkpoints and
eventually reported approximately 10.5 replay frames per second. The i686
build ran inside an amd64 Ubuntu container on an ARM64 Docker host and was
slower still. Shader compilation, software rendering, synchronous trace I/O,
and architecture emulation made the game look stationary while the replay
frame counter continued to advance. Checkpoint lines at frames 300, 600, 900,
and later were used as the progress signal; no run was accepted merely because
the window stayed open.

### Replay versions 2 and 3 did not cover the loader timing boundary

The existing fixed-size replay frame stored VSync calls made between tracked
`BeginFrame` and `EndFrame`. Asynchronous loading can emit VSync calls before
the next `BeginFrame`, so those calls had no owner. A fresh
`--record-from-replay` process could reproduce every pad snapshot while
executing a different number of loader VSyncs. The resulting comparison was
not evidence of a gameplay defect because the two processes were sampled at
different temporal boundaries.

Replay version 4 keeps the 148-byte header and 440-byte frame record:

```text
vblankPacketCount low 16 bits:  total encoded packet entries
vblankPacketCount high 16 bits: entries emitted before BeginFrame
u16 packet low byte:            emitted VBlank count
u16 packet high byte:           repeat count minus one
```

Calls made with no frame open accumulate as the prefix of the next frame.
Playback emits that prefix before opening the frame, then consumes the
remaining packets at the original call sites. Format validation checks the
prefix/total relationship, nonzero packet values, decoded total, 64-entry
capacity, record checksum, and complete consumption. Readers and
`tools/compare-replay-state-components.mjs` accept versions 2 through 4, but
only version 4 logs `complete-vsync=yes` and can be a strict timing seed.

The first implementation stored every call as a raw entry. Its first i686
report is deliberately retained as rejected evidence:

```text
/tmp/ctrpad-i686-final-m32-uGz0OL/debug/reports/20260730/ctr-134158
frame_count: 0
checkpoint_count: 1
failure: [CTR Replay] too many VSync packets in replay frame 0
```

Frame zero contained more than 64 calls with long runs of equal values. The
format was corrected to run-length encode adjacent equal calls, with up to 256
calls per entry. A media-free replay self-test creates a pre-frame prefix,
exercises a repeated packet, verifies its decoded VBlank total, reproduces the
boundary, and reports `complete-vsync=checked`.

The accepted old-source i686 timing seed was:

```text
report:                 /tmp/ctrpad-i686-final-m32-uGz0OL/debug/reports/20260730/ctr-134748
version:                4
frames/checkpoints:     2,200 / 8
executable fingerprint: 7be34e66f634384d
identity checksum:      0xec30175a
input SHA-256:          8075723058c85126bb7a2415612e7a61f0f7ac0ce9b71ce31e88f91cd743ab1c
state SHA-256:          b5a9085e5bf72b3a4c827adacc6187dbb90e0d3e9576002e93c8b14cd656a8aa
```

### A valid timing comparison isolated real RNG drift

The first complete ARM64 version-4 report was
`build-macos-arm64/debug/reports/20260730/ctr-091111`. Comparison against
`ctr-134748` produced:

```text
timing:     equal=2200 mismatched=0
rng:        equal=70   mismatched=2130 ranges=12-69,116-128,130-137,149-2199
drivers:    equal=2160 mismatched=40   ranges=367-384,1708-1709,2180-2199
world:      equal=2200 mismatched=0
allocation: equal=0    mismatched=2200 ranges=0-2199
root:       equal=0    mismatched=2200 ranges=0-2199
```

Equal timing and world state made the RNG difference actionable. Temporary RNG
caller tracing was added to the disposable investigation builds. The first
caller result was wrong for two independent reasons:

1. the traced function was inlined, so the return address did not reliably
   name the semantic caller; and
2. the first symbolization guessed the Mach-O load base instead of measuring
   the process's actual ASLR slide.

That address initially appeared to resolve to `DrawLevel` and was rejected.
The trace helper was marked no-inline, the running image base was read with
`vmmap`, and the exact slide was used for symbolization. The common
initialization caller then resolved to `CS_Thread_Init`, while the first extra
ARM64 calls resolved to `CS_Thread_UseOpcode`.

One operator-side Node command also failed during this analysis:

```text
buffers.map(fs.readFileSync)
ERR_INVALID_ARG_TYPE
```

`Array.map` supplied its numeric index as `readFileSync`'s second argument.
Replacing the method reference with an explicit callback,
`buffers.map(path => fs.readFileSync(path))`, corrected the diagnostic. No
game result was inferred from the failed command.

### The cutscene decoder widened a retail record and erased `arg1`

Targeted cutscene traces logged initialization, decoded opcode metadata, model
ID, animation range, and RNG calls on both architectures. The decisive
difference was the decoded `CsOpcodeMeta` layout:

```text
                           i686 retail   old ARM64
sizeof(CsOpcodeArg)        4             8
offsetof(arg1)             0x0c          0x10
offsetof(rotStart)         0x10          later
sizeof(CsOpcodeMeta)       0x14          0x20
```

`CsOpcodeArg` incorrectly included `char *ptr`. The bytecode decoder wrote
`arg1.u` at native offset `0x10` on LP64, then cleared the retail
`decodedShorts[8]` and `[9]` rotation slots at bytes `0x10..0x13`. That
sequence erased the argument it had just decoded.

Six intro models demonstrated the gameplay consequence. On i686 their decoded
animation endpoints were:

```text
62, 29, 56, 79, 73, 83
```

Their subsequent RNG calls were spread across game frames 27, 41, 45, 53, 57,
and 59. ARM64 decoded zero endpoints and advanced those animations together at
game frame 13. The RNG generator itself was correct; its consumers were
scheduled differently by corrupted decoded metadata.

The permanent correction in `include/ovr_233.h` removes the host pointer from
the union and asserts, on every architecture:

```text
sizeof(CsOpcodeArg)       = 4
offsetof(arg0)            = 0x08
offsetof(arg1)            = 0x0c
offsetof(rotStart)        = 0x10
offsetof(rotEnd)          = 0x12
sizeof(CsOpcodeMeta)      = 0x14
```

The four pointer-consuming branches in `game/233/CS_Thread.c` now convert the
stored guest word with `(char *)(uintptr_t)opcodeMeta->arg1.u`. A post-fix
temporary ARM trace matched the i686 RNG/cutscene call sequence through game
frame 213 before tracing was removed.

### Post-fix gameplay components matched for all 2,200 frames

The first uninstrumented post-fix ARM report was:

```text
report:                 build-macos-arm64/debug/reports/20260730/ctr-093440
frames/checkpoints:     2,200 / 8
executable fingerprint: a725688b7492b732
identity checksum:      0x1092d7ae
input SHA-256:          1cc06013cbb2526cedeb11b20396e364fc007fa37937abfa05677fc8d8574086
state SHA-256:          fdebfb776cec58c17f688dbe52375caf2e4396a880042f09b13ca94fdf22daf0
```

Comparison against the i686 version-4 seed then produced:

```text
timing:     equal=2200 mismatched=0
rng:        equal=2200 mismatched=0
drivers:    equal=2180 mismatched=20 ranges=367-384,1708-1709
world:      equal=2200 mismatched=0
allocation: equal=0    mismatched=2200
root:       equal=0    mismatched=2200
```

This closed the actual RNG/game-sequence defect. The remaining components
required canonical-digest review rather than more physics changes.

### Both driver mismatch ranges were freed allocator slots

Temporary driver traces recorded all eight table slots only at the mismatch
ranges. ARM report `ctr-094214` showed slot zero at frames 367 through 384
with impossible values such as:

```text
driverID=100 kartState=255 actions=0xffffffff
pos=-1085292736,56,0
```

The i686 diagnostic report
`/tmp/ctrpad-i686-final-m32-uGz0OL/debug/reports/20260730/ctr-144209`
contained different reused bytes at the same frames, for example
`driverID=87` and address-shaped words in actions/position fields.

At frames 1708 and 1709, i686 had four non-null entries containing invalid
driver IDs, impossible kart/lap states, and pointer-shaped values. The replay
logged the actual race driver becoming active at frame 1710. These pointers
named old or free large-stack storage during menu/loader transitions; they did
not name live racers.

Digest schema 2 now uses the same authoritative invariant as checkpoint
relocation:

```text
live process-stack object =
  correctly aligned slot in the current pool
  AND slot absent from the pool free list
```

The state-digest self-test builds one synthetic large-stack driver slot. It
proves a live position mutation changes only `drivers`, then places the slot
on the free list, changes its bytes differently in two trackers, and proves
the complete digests remain equal.

### Canonical allocation state excludes host storage geometry

The old allocation component included `JitPool.itemSize`, `poolSize`, the
expanded LP64 main-pack start/size, and every bookmark array entry. Those are
not all live retail allocation semantics:

- pointer-expanded `Thread`, `Instance`, stack, `Driver`, `Particle`, and rain
  records deliberately use wider LP64 strides;
- `MainInit` reserves the complementary prefix so remaining retail allocation
  pressure is unchanged; and
- bookmark entries above `numBookmarks` are inactive but retain old addresses
  after a pop.

Schema 2 retains pool free/taken populations and retail item capacities,
active mempack identity, initialized-pointer topology, empty/full
relationships, previous-allocation existence, and bookmark depth. It excludes
physical host slot widths, aggregate host pool bytes, main-pack start/size,
low/high cursors, previous physical allocation bytes, and bookmark addresses.

An intermediate schema-2 attempt removed pool byte geometry and inactive
bookmark entries but retained active cursor/bookmark coordinates. A 131-frame
probe still mismatched allocation/root on every frame. Focused frame-zero
logging measured equal pool populations and bookmark depth, then exposed the
noncanonical coordinates:

```text
                           ARM64       i686
last/end offset            0x1ff800    0x1ff800
first-free offset          0x1157e0    0x10d9d4
bookmark 0 offset          0x0bdb48    0x0bfbfc
bookmark 1 offset          0x0bed30    0x0c0de4
bookmark 2 offset          0x0f4560    0x0f6614
previous allocation        12000       12000
bookmark depth             3           3
```

The active bookmarks differed by `0x20b4`; the low-memory cursor differed by
`0x7e0c`. These are real physical host-layout offsets caused by widened
allocations outside the pool population model, not different retail allocator
lifecycle. The temporary frame-zero logger was removed. The next short
cross-width probe matched timing, RNG, drivers, world, allocation, and root for
all 31 available frames before the same binaries continued into the final
2,200-frame run.

### Rejected and interrupted evidence remains explicit

An attempted ARM restore from checkpoint 1 with a rebuilt diagnostic binary
and header bypass exited during restore. It was not used for parity; fresh
boots were used instead.

ARM driver-trace report `ctr-094214` was stopped after the first mismatch range
at replay frame 946 once the needed data was recorded. The i686 trace
`ctr-144209` was stopped after frame 1709. Docker returned status 137 because
the diagnostic container was deliberately stopped; it is not a game crash.

After removing the driver trace from source, one ARM run accidentally launched
the still-linked prior diagnostic executable. Its partial report
`build-macos-arm64/debug/reports/20260730/ctr-095319` was stopped at frame 464,
then the binary was rebuilt, all 13 CTests rerun, and the executable scanned
for `CTR DriverTrace`, `CTR RNGTrace`, and `CTR CSTrace` before a replacement
run began. The partial report is not acceptance evidence.

No retail-derived replay, checkpoint, memcard, executable, log, or disc byte
from these experiments is tracked by Git.

### The first complete six-component prefix comparison passed

The continued short probe completed as two ordinary version-4 reports:

```text
ARM64: build-macos-arm64/debug/reports/20260730/ctr-101244
i686:  /tmp/ctrpad-i686-final-m32-uGz0OL/debug/reports/20260730/ctr-151256
```

Both reports finalized 2,200 frames and eight rolling checkpoints. The
component comparator, invoked with all six components required, reported:

```text
timing:     equal=2200 mismatched=0 ranges=none
rng:        equal=2200 mismatched=0 ranges=none
drivers:    equal=2200 mismatched=0 ranges=none
world:      equal=2200 mismatched=0 ranges=none
allocation: equal=2200 mismatched=0 ranges=none
root:       equal=2200 mismatched=0 ranges=none
```

The report identities were:

| Property | ARM64 | i686 |
|---|---|---|
| Executable fingerprint | `fc02b9137ab25303` | `ae2da3bc16b9b6e2` |
| Identity checksum | `0x3588c24c` | `0xc25cc786` |
| Input SHA-256 | `d9b77e99554db6f4b7741100f4719870182c591c48fd3953272811d4f36de03c` | `d1962ea4e58f69b5f3463787de5f41bb16fb0ca2da1ab2950dabb468a3e61994` |
| State SHA-256 | `85b65ed5f0dafce47133c8600557818822c568d1b9b9a6bea2077641b977cf6c` | `1a2dd1cfcbd3efcb778985105c4d0d5ac6068fc0d0c18e643f35b9d1f943dcf1` |

The different replay and checkpoint file hashes are expected: headers contain
platform/binary identity and checkpoints contain native-width physical state.
The six canonical per-frame digests are the cross-width comparison.

This was not treated as the last gate. A fresh combined ASan/UBSan build had
already passed all thirteen media-free tests, but its 2,200-frame replay was
still running. It subsequently found a separate memory-safety defect, so the
reports above remain valid localization evidence rather than final-source
acceptance.

### Sanitizer exposed a short native instance-name read

The first sanitizer replay report
`/tmp/ctrpad-arm64-asan-v4-0Z3iyC/debug/reports/20260730/ctr-102022`
reached the race and checkpoint 7 at frame 2,100, then aborted while a mask was
spawned:

```text
AddressSanitizer: global-buffer-overflow
READ of size 1
INSTANCE_Birth                 game/INSTANCE.c:26
INSTANCE_Birth3D               game/INSTANCE.c:70
VehPickupItem_MaskUseWeapon    game/Vehicle/VehPickupItem.c:338
VehStuckProc_MaskGrab_Init     game/Vehicle/VehStuckProc.c:579
```

The read was exactly one byte beyond the nine-byte native string literal
`"akubeam1"`. Retail `INSTANCE_Birth` copies 15 bytes because its names live in
one flat PS1 address space. Native callers also pass ordinary, shorter C string
literals (`"beam"`, `"x"`, and others), so blindly retaining that source read
is undefined behavior even though the destination is a 16-byte array.

The native branch now stops when it sees the source terminator and zero-fills
the remainder of `Instance.name`. The non-native branch retains the original
15-byte copy and explicit final terminator. Fixing the constructor rather than
padding the one mask literal closes the contract for every short native
caller. Because this changed the executable after the first exact prefix
comparison, normal ARM64, sanitizer ARM64, and disposable i686 builds and
reports were started again from the same version-4 input seed.

### Final-source reruns and acceptance boundary

The name-copy correction rebuilt all three producers. Ordinary ARM64,
combined ASan/UBSan ARM64, and disposable Docker `-m32` each passed all
thirteen media-free CTests before the reports below were accepted:

```text
ordinary ARM64: build-macos-arm64/debug/reports/20260730/ctr-103032
sanitizer ARM64: /tmp/ctrpad-arm64-asan-v4-0Z3iyC/debug/reports/20260730/ctr-103044
i686: /tmp/ctrpad-i686-final-m32-uGz0OL/debug/reports/20260730/ctr-153116
```

The sanitizer report completed the mask-spawn path, all 2,200 frames, and all
eight checkpoints without another ASan or UBSan report. It matched the
ordinary ARM64 report on timing, RNG, drivers, world, allocation, and root for
every frame.

The final i686 run used llvmpipe under `qemu-i386`. Near the end it reported
2.08 FPS while Docker showed approximately 400% CPU use. Metadata updates occur
at checkpoints, so the terminal again looked quiet for long intervals. The
in-flight replay comparator was run at frames 850, 871, 982, 1,088, 1,216,
1,428, 1,564, 1,650, 1,715, 1,781, 1,853, 1,916, 1,983, 2,038, 2,094, and
2,174. Every available prefix matched all six required components. The log
then recorded race-driver activation at frame 1,710, checkpoint 7 at frame
2,100, and normal finalization at frame 2,200.

The definitive full comparison reported:

```text
timing:     equal=2200 mismatched=0 ranges=none
rng:        equal=2200 mismatched=0 ranges=none
drivers:    equal=2200 mismatched=0 ranges=none
world:      equal=2200 mismatched=0 ranges=none
allocation: equal=2200 mismatched=0 ranges=none
root:       equal=2200 mismatched=0 ranges=none
required components match: timing,rng,drivers,world,allocation,root
```

Final executable identities were:

```text
ordinary ARM64 SHA-256: 7d150121b63307d4f7319938c0680f2f37bcc71cfafc550dc96169138bf5efd0
sanitizer ARM64 SHA-256: 1e43d48c9cab84e30e7ac1c53c7d85b07b193e4fd8187105a9559276d10dfe6c
i686 SHA-256:           93ebb84eee1b30a205818a1749db3b97e81fb2f4172dd9a61bc20eab6871b00a
i686 ELF Build ID:      6191c605c1bebcfd0e4192e1b5131a8ebf291ca2
```

The forced-LP64 audit completed after the final source change:

```text
compile exit/errors/assertions: 0 / 0 / 0
pointer-to-integer coordinates: 0
integer-to-pointer coordinates: 0
unique narrowing source lines:  0
i686 object SHA-256:             7ebc25257acc868616a1616d8a1b4479068b6c9ccb27deb7fc2d703db72c357b
layout census SHA-256:           60cd7bb0f4ec3975aee782949faa76e4769393d300def2a9231b9ea9e1961662
```

The protected historical baseline remained unchanged at SHA-256
`afe7b3d264bd2674192e71485037842ab329d1eca9c0c34ca50da5d4487c76a7`,
8,888,128 bytes, timestamp `2026-07-29T17:54:11-0500`.

The compact final evidence table, report hashes, command, and bounded
acceptance claim are in
`docs/parity/2026-07-30-arm64-prefix-parity.md`. This closes the measured
startup/menu/loader/early-race prefix. It does not convert the historical
24,232-frame version-2 report into acceptance or claim complete race,
powerslide/item/lap/save, multiplayer, persistence, visual, XA/STR, touch,
iOS/iPadOS lifecycle, signing, or distribution coverage.

## 2026-07-30 — Historical full-run seed validation and optimized i686 probe

The next review did not promote the 2,200-frame prefix into a broader claim.
It first established whether the previously recorded 24,232-frame operator run
could safely automate a fresh replay-format-version-4 recording and measured
the cost of doing that under i686 emulation.

### Supplied NTSC-U media and historical automation inputs

The user populated `ref/CTR/` with a new paired BIN/CUE image:

```text
BIN: CTR - Crash Team Racing (USA).bin
size: 605,698,800 bytes
raw sectors: 257,525 x 2,352 bytes; remainder 0
SHA-256: f780bf2331476aabfc00772fa758b12dd95ebfbc907968132cbd3cdd4e2c07c0

CUE: CTR - Crash Team Racing (USA).cue
layout: one MODE2/2352 track
referenced filename: CTR - Crash Team Racing (USA).bin
```

Both files are excluded by the repository asset-ignore rules. No retail byte
was staged or tracked.

The historical report used for full-run automation is:

```text
build-linux-i686-baseline/debug/reports/20260729/ctr-225420
replay format: 2
input frames: 24,232
frame payload size: 440 bytes
rolling checkpoints: 81
input SHA-256: d846451bd078ee48effbfa46debc9e216a2381f4a64a9c06e278e54f81812d2c
state SHA-256: e200f541658b6afa2672118241ee5d575e8dc9145300982e6a851132f1695b35
```

Its `coverage.txt` records `pass` for all eight scripted behavior checks. The
log contains the successful powerslide boost near frame 9,300 and the
operator's final stop at frame 24,232. Its recording-time memory-card result is
`memcard.recording/slot0/BASCUS-94426-SLOTS`, 6,016 bytes with SHA-256
`e52b379c01be17c61844da948234ad927242c220e84bf5cfc2c8e14f06d489b5`.
The input `memcard.seed` is intentionally empty.

This version-2 replay is accepted only as pad/name-entry automation and as the
source of the empty memory-card seed. It does not record the complete VSync
boundary introduced in replay format 4, and its native state hashes are not
cross-width acceptance evidence. `--record-from-replay` consumes its validated
input snapshots while the destination process records its own current timing
and state digests.

### Two rejected performance experiments

The first speed experiment selected `SDL_VIDEODRIVER=offscreen`. The pinned
SDL build was configured with `SDL_OFFSCREEN OFF`, so SDL video initialization
failed. The process then reached uninitialized renderer state and
`qemu-i386` reported a segmentation fault with status 139. This run produced
no parity evidence and was rejected.

Source review explained the secondary crash: `Platform_Init` returns `void`
(`include/platform.h:13`, `platform/native_platform.c:234-269`). It logs and
returns when SDL or renderer setup fails, but `main.c:304-325` cannot observe
that failure and continues into scratchpad, replay, and game initialization.
That successful-startup-independent cleanup defect is retained as an explicit
follow-up; it was not patched while an exact-binary parity run was active.

The second launch attempted to wrap the game with the external `time` program.
The minimal pinned container does not include that executable, so the shell
exited with status 127 before the game started. It also produced no evidence.
Subsequent elapsed-time measurements use the host process start time, report
frame count, and the runtime's own FPS telemetry.

### Disposable optimized i686 build

A disposable Release build was configured at:

```text
/tmp/ctrpad-i686-release-v4-QkBKSJ
container image: ctrpad-linux-i686:ubuntu-24.04
target flags: -m32
CMake build type: Release
```

Configuration took 520.4 seconds under x86 emulation. All 13 media-free CTests
passed. The executable is ELF32 Intel 80386 with Build ID
`fe359b42bee342dcaa844020475ee84f5548174c` and retained DWARF debug
information. Compiler diagnostics included the existing format warnings and
an optimized-only `game/222.c:58-61,230-232` warning that `txtStartX` and
`txtEndX` might be uninitialized. Inspection showed that both are read only
when `tokenAwardTextFrame >= 0`; both branches that make that condition true
assign the coordinates first (`game/222.c:130-151`), so no source change was
made from that warning.

The protected historical baseline was checked again and remained unchanged at
SHA-256
`afe7b3d264bd2674192e71485037842ab329d1eca9c0c34ca50da5d4487c76a7`.

### Completed optimized prefix benchmark

The Release executable was launched under Xvfb and llvmpipe with dummy host
audio, using the already accepted 2,200-frame version-4 report only as input
automation:

```text
source seed:
/tmp/ctrpad-i686-final-m32-uGz0OL/debug/reports/20260730/ctr-134748

destination report:
/tmp/ctrpad-i686-release-v4-QkBKSJ/debug/reports/20260730/ctr-161136
```

At 2026-07-30 11:14:52 -0500, the destination contained 513 frames after
approximately 3 minutes 17 seconds of container lifetime. The required prefix
comparison reported 513 equal and zero mismatched frames for each of timing,
RNG, drivers, world, allocation, and root. Continued in-flight comparisons
remained exact, including after race-driver activation at frame 1,710.

The run finalized normally at 2,200 frames and eight checkpoints at
2026-07-30 11:26:25 -0500, approximately 14 minutes 50 seconds after launch.
The runtime reported 2.73 FPS over its last 2,000 frames. Release optimization
therefore improved the earlier 2.08 FPS Debug observation but did not remove
the dominant qemu-i386/llvmpipe cost.

The definitive comparison reported 2,200 equal and zero mismatched frames for
all six required components:

```text
timing:     equal=2200 mismatched=0 ranges=none
rng:        equal=2200 mismatched=0 ranges=none
drivers:    equal=2200 mismatched=0 ranges=none
world:      equal=2200 mismatched=0 ranges=none
allocation: equal=2200 mismatched=0 ranges=none
root:       equal=2200 mismatched=0 ranges=none
```

Final identities and hashes were:

```text
executable Build ID: fe359b42bee342dcaa844020475ee84f5548174c
executable SHA-256:  686393e84a289220186984b9efe19ceece54bfc4e0978cc1903e7f527b0c45a2
input SHA-256:       e717e5500a36f824da0797257ca9b9e9062b0e015a14689e0de9c7847856ae54
state SHA-256:       4d5239d12037b7623aa274ccf726c58d34247755c7944a0c96c508eb8276775e
metadata SHA-256:    74717a896ade261e6493e625f27a3b35764789606629f8bf9dbfb4ddb40ceabc
log SHA-256:         e78319e96ce22c5ec292899b1146c45b5840368e3da21604df44ae49772bd5af
```

This accepted the optimized binary as an equivalent i686 producer for the
measured 2,200-frame rendered prefix. It did not extend the coverage boundary.

### Full version-4 regeneration sequence

The historical input omits the complete VSync boundary, so independently
recording it once on each architecture would ask each host to reconstruct
timing. Instead, the full sequence was fixed as:

1. ARM64 consumes the historical version-2 pads and empty memory-card seed,
   producing one fresh version-4 report with its own complete timing.
2. The optimized i686 executable consumes that ARM64 version-4 replay,
   pinning pads and every pre-frame and in-frame VSync packet.
3. The component comparator evaluates timing, RNG, drivers, world, allocation,
   and root for all 24,232 frames.
4. Unchanged playback, a deliberate driver mutation, coverage evidence, the
   recording memory card, and artifact hashes are verified separately.

The first stage launched under a macOS idle-sleep guard:

```text
source:
build-linux-i686-baseline/debug/reports/20260729/ctr-225420/input.ctrreplay

destination:
build-macos-arm64/debug/reports/20260730/ctr-112652
```

Its initial metadata correctly reported version 4 for the destination,
24,232 source frames, and `finalized=0`. The log explicitly reported source
version 2 and `complete-vsync=no (coverage automation only)`. No result is
claimed until the report finalizes.

### Comparator transport equality was made explicit

Review during the ARM64 run found that
`tools/compare-replay-state-components.mjs` validated each input's pad
checksum, full-record checksum, and decoded VSync total but reported equality
only for the six canonical digest values. That was enough to prove state
equality, but it did not explicitly prove that both producers consumed
byte-identical pads and version-4 call boundaries.

The analysis-only tool now also reports and can require:

- `pads`: equality of all 48 pad-snapshot bytes per frame; and
- `vsync`: equality of the VBlank total, encoded packet count, pre-frame packet
  count, and every used run-length-encoded packet.

No game source or running executable changed. `node --check` passed, and the
completed Release-i686/ordinary-ARM64 prefix was rerun with all eight signals
required. It reported 2,200 equal and zero mismatched frames for timing, RNG,
drivers, world, allocation, root, pads, and VSync. Two temporary negative
fixtures retained valid internal checksums: one changed a pad byte at frame 0,
and one changed frame 1's VSync call encoding from two repeated one-VBlank
calls to one two-VBlank call while preserving its total. Requiring `pads` and
`vsync` respectively isolated the intended one-frame ranges and returned
status 2. The temporary retail-derived fixture copies were then deleted.

## 2026-07-30 — GitHub checkpoint, visual-capture limit, and late-load OOM

The complete working tree was reviewed before publication. Retail media and
runtime reports remained ignored; 192 source and documentation files were in
scope. The ordinary ARM64 build passed all 13 CTests, the replay-component
tool passed `node --check`, and `git diff --check` was clean.

Commit `2f341999be63250d8cc58d48f585c1fbc3413bff` (`port runtime to ARM64
with parity gates`) was created on `codex/arm64-apple`. The first HTTPS push
returned GitHub HTTP 400 and did not create the remote ref. A retry using Git
HTTP/1.1 and a larger post buffer succeeded; the remote branch hash was
verified equal to the local commit. The repository is private, so connector
PR creation returned 404. Authenticated `gh` fallback created draft pull
request `https://github.com/chrissotraidis/ctrpad/pull/1` against `main`.

### Screen and texture inspection was not claimed

macOS window enumeration identified the running `ctr_native` window, but both
the system screen-capture command and CoreGraphics capture failed under the
Codex app's current Screen Recording permissions. Sending the internal F12
shortcut also produced no `SCREENSHOT.BMP`. Runtime logs prove Apple M2,
OpenGL 4.1/Metal context, shader compilation, and continued rendered-frame
execution, but those are not visual texture evidence. No texture-correctness
claim was made; an in-engine capture or granted screen recording remains
required.

### The first full ARM64 regeneration stopped at frame 22,156

The fresh version-4 producer was:

```text
source:
build-linux-i686-baseline/debug/reports/20260729/ctr-225420/input.ctrreplay

destination:
build-macos-arm64/debug/reports/20260730/ctr-112652
```

It advanced at approximately 29.9 FPS, reproduced the successful boost near
frame 9,300, and followed the historical race/load transitions through its
last rolling checkpoint at frame 21,900. The input file reached 22,156 frame
records, but no file changed afterward and one CPU core remained saturated.
The report retained `finalized=0` and is rejected as parity evidence.

`sample` recorded the live stack in `/tmp/ctrpad-arm64-stall-22156.sample.txt`:

```text
CTR_Main
LOAD_TenStages (loading stage 7)
MainInit_JitPoolsNew
MEMPACK_AllocMem (intentional infinite allocation-error loop)
```

LLDB confirmed level `0x28`, one player, and
`MainDB_GetClipSize(...) == 24000`; the call requested `24000 << 2`, or
96,000 bytes. The allocator had 86,264 bytes free:

```text
firstFreeByte: 0x10140ffc0
lastFreeByte:  0x1014250b8
shortfall:     9,736 bytes
```

The failed process ignored normal termination while spinning in the original
error loop, so it was killed after the sample and LLDB values were safely
captured.

### Same-frame checkpoints isolated the 32,268-byte cause

Checkpoint 73 at frame 21,900 was read from both the failed ARM64 report and
the historical i686 report. Their main packs were:

```text
                         ARM64       i686       difference
arena size             1,371,344  1,330,704       40,640
low bytes used         1,196,704  1,123,796       72,908
free bytes               174,640    206,908      -32,268
bookmark 0 offset          53,272     21,004       32,268
bookmark 1 offset          57,856     25,588       32,268
bookmark 2 offset       1,049,032  1,016,764       32,268
bookmark 3 offset       1,196,704  1,123,796       72,908
```

The first three bookmark differences were exactly constant at frames 0, 300,
1,500, 2,100, 3,000, 9,000, 15,000, 21,000, and 21,900. The additional
40,640 bytes after bookmark 2 exactly matched the already documented maximum
LP64 JIT-pool overhead. This ruled out a cumulative leak.

`sdata->langBufferSize` is `0x3f04`, or 16,132 bytes. The previous LP64
allocation appended a maximum host pointer table:

```text
aligned language bytes:       16,136
4,033 host pointers x 8:      32,264
LP64 allocation:              48,400
i686 retail allocation:       16,132
exact displacement:           32,268
```

That derived pointer table—not game data—was the entire persistent
cross-width loss. `LOAD_LangFile` now leaves the retail-sized language buffer
in MEMPACK and writes the derived table to fixed native storage.
`NativeCheckpoint_Restore` relocates `lngFile`, validates the captured count,
table extent, and every serialized offset, then reconstructs `lngStrings`.
The rebuilt ARM64 binary passed all 13 CTests, `node --check`, and
`git diff --check`; the protected i686 baseline remained byte-identical at
SHA-256
`afe7b3d264bd2674192e71485037842ab329d1eca9c0c34ca50da5d4487c76a7`.

A replacement full ARM64 run started as report `ctr-115352`. Its outcome is
intentionally not recorded here until it passes the former frame-22,156
failure point and finalizes.

### Later correction: in-engine VRAM export exposed visual corruption

The OS-level screen-capture limitation above remains true, but it no longer
prevents visual inspection. While `ctr-115352` was running, LLDB invoked the
game's exported `NativeRenderer_SaveVRAM` on the stopped main thread and
detached immediately afterward. The run resumed normally. The local,
retail-derived evidence is intentionally ignored:

```text
/tmp/ctrpad-vram-115352.tga
format: 1024 x 512 x 16-bit TGA
size: 1,048,594 bytes
SHA-256: 03d799f9baa9362f5cd9f36fe85f5b60ab92e4bf195649997768a93d63a6cfa8

/tmp/ctrpad-framebuffer-top-115352.png
format: 512 x 240 RGB PNG crop
SHA-256: 0525f8feb4f7de7e1d669e5cbc1a132ab1549645b3081a8077b0090271480a5d
```

The export proves that the two display buffers, HUD, kart, minimap, and loaded
texture pages exist. It also visibly shows incorrect colors and high-frequency
corruption across large track surfaces and parts of the framebuffer. An older
local window capture from the earlier i686-seeded ARM64 run exhibits the same
class of corruption, so this is not caused by the language-sidecar change.
Texture presence is observed; texture correctness is rejected and remains a
separate renderer/asset investigation. No screenshot is committed.

### Correction: raw PS1 VRAM is not the presented-window oracle

The preceding corruption conclusion was rejected minutes later. The full
VRAM atlas mixes 4-bit and 8-bit indexed texture pages, CLUT data, both PS1
display buffers, and 16-bit direct-color regions. Interpreting every word as a
standalone RGB555 image makes indexed data look like high-frequency noise and
does not reproduce the renderer's shader/CLUT presentation.

To capture the real screen without macOS Screen Recording permission, LLDB
stopped the main thread, allocated a temporary 800×600 RGBA buffer, called
the already-loaded `glad_glReadPixels` on the default framebuffer, wrote the
bytes out, freed the buffer, and detached. The run resumed normally:

```text
/tmp/ctrpad-window-115352.rgba
size: 1,920,000 bytes
SHA-256: 74a7235e0aa8e9e785d037dbfc1547308cf291bab4cef17257603a6edf06b9d3

/tmp/ctrpad-window-115352.png
format: 800 x 600 RGBA PNG, vertically corrected
SHA-256: 8678bcb0c3cc02b312b346e1685c5072a2e27c5fcdf06b0174c4fc41d4cd5db0
```

The actual presented frame cleanly shows the track-select menu, readable
fonts, layered panels, highlight state, and a coherent textured Roo's Tubes
preview. The raw-VRAM corruption claim is withdrawn. This is positive visual
evidence for that screen, not yet a claim covering every race surface,
effect, video, transparency mode, or multiplayer layout. The retail-derived
PNG remains ignored and is not committed.

### Presented race capture bounds a real level-rendering defect

A second default-framebuffer capture was taken after replay frame 14,291 made
the race driver active again:

```text
/tmp/ctrpad-window-race-115352.rgba
size: 1,920,000 bytes
SHA-256: 91184d86e4fc49eac0e99b02b51b5e871c0368cb2579e9482dfd4582519ac029

/tmp/ctrpad-window-race-115352.png
format: 800 x 600 RGBA PNG, vertically corrected
SHA-256: 299df578488161fa59481ab378637370ea350ff0a63b17a5496a61a97d37b551
```

Unlike the clean menu capture, the race frame shows severe corruption in
track geometry and/or texture coordinates. HUD sprites, rank, lap, crate and
fruit icons, kart, exhaust, and minimap remain recognizable, which narrows the
failure away from general OpenGL presentation and toward the 1-player level
rendering path. Visual acceptance remains open. The next oracle is an i686
capture at a comparable replay checkpoint; no speculative source fix is made
while the allocator regression run is active.

### Replacement ARM64 run crossed the failure and finalized

Report `build-macos-arm64/debug/reports/20260730/ctr-115352` crossed the old
frame-22,156 failure, wrote checkpoints at frames 22,200, 22,500, 22,800,
23,100, 23,400, 23,700, and 24,000, then finalized normally:

```text
replay_version=4
frame_count=24232
checkpoint_count=81
finalized=1
recording_status=finalized
```

The log reproduced the successful powerslide boost near frame 9,300 and ended
with `replay-seeded recording finished after 24232 frames`. The final
artifacts were:

```text
producer copy:
build-macos-arm64/ctr_native-115352-producer
SHA-256 588786e7eaaee5c845dc2c1e6c101178cc93be3a62e94857aea338338cba769e

input.ctrreplay:
10,662,228 bytes
SHA-256 bf022938a8580e91fa06045f0cabb6b58a67fb7cc16bbd4909601b7e1ce93b86

state.ctrstates:
359,201,012 bytes
SHA-256 8fdf6999fd8796228cd9e801f49be1525cd750cf17dda09c395a1fcfeaee0842

metadata.txt:
SHA-256 29b992e1a12a73a825307f7f531daea0fc3f2c71ba42dded5048c0393734305d

ctr-native.log:
SHA-256 24de8fd1f9d395a7aa7599a0158ca6fa04c4714d70e83280c0d713754c16aa22

memcard.recording/slot0/BASCUS-94426-SLOTS:
6,016 bytes
SHA-256 6a01b0f5562ed7a279d8f8e51e3b1874ac39a6120f55db4fe3873288950619a3
```

This accepts the language-table allocator correction. It is not yet the
cross-width full-run acceptance because the historical version-2 input does
not contain complete VSync boundaries. The optimized i686 producer must
consume this new version-4 input before all eight comparator signals can be
required.

The current post-run ARM64 source was rebuilt after preserving the producer
copy. The rebuilt executable has SHA-256
`938ce1b4d2148aa32e25fc4d56d07ad9ac95c5a39f5489e1dac5fba4b15825cf`,
passes all 13 CTests, and leaves the exact producer copy unchanged. The
comparator passes `node --check`, `git diff --check` is clean, and the
protected i686 baseline remains byte-identical at
`afe7b3d264bd2674192e71485037842ab329d1eca9c0c34ca50da5d4487c76a7`.

### Frame-matched i686/ARM64 visual oracle changed the diagnosis

The first i686 Xvfb attempt sent F12 while the emulated producer was still
compiling shaders. It created no screenshot and was rejected. The retry
waited for exact checkpoint 6 to restore. It used the accepted optimized
producer without changing its bytes:

```text
producer:
/tmp/ctrpad-i686-release-v4-QkBKSJ/ctr_native
SHA-256 686393e84a289220186984b9efe19ceece54bfc4e0978cc1903e7f527b0c45a2

report:
/tmp/ctrpad-i686-release-v4-QkBKSJ/debug/reports/20260730/ctr-161136

log boundary:
restored checkpoint 6 / race driver active at frame 1800
F12 screenshot at frame 1802
```

The raw F12 BMP is bottom-up because `Platform_TakeScreenshot` reads the OpenGL
framebuffer directly (`platform/native_platform.c:154-167`). It was flipped
vertically for inspection:

```text
/tmp/ctrpad-i686-visual-3Kt3e9/SCREENSHOT.BMP
size 1,920,138
SHA-256 5a59ff5182ad69e80120509e773395b23a3958e6be4c588f8ccfb442f35bc145

/tmp/ctrpad-i686-race-frame1802-upright.png
SHA-256 c7919b7807d77e0b67a2bb6a0106b7a07a4002068b09901988fac19358e111c7
```

An initial ARM64 attach completed after the short checkpoint playback had
cleared the framebuffer and produced an all-black image. It was rejected. The
retry launched the exact ARM64 producer under LLDB, set a conditional
breakpoint in `NativeReplayScheduler_EndFrame` for replay frame 1,802, called
the loaded `glad_glReadPixels`, and wrote the frame before killing only that
diagnostic playback:

```text
/tmp/ctrpad-arm64-race-frame1802.rgba
size 1,920,000
SHA-256 3f306a2e5b1c6eb4a1da3786e1ff5d07cdc4e92cedd0597ae873de57fb3d92c2

/tmp/ctrpad-arm64-race-frame1802.png
SHA-256 9d13a57ad9076944916059e67742a198110f68fdb1778eba692441401fd275a1
```

The matched images have the same camera, kart placement, and track polygon
boundaries. ARM64 shows high-frequency stripes on a right-side track surface
where i686 shows a flat-colored surface. The i686 llvmpipe image itself has
incorrect cyan/blue coloration, so it is not a retail-correct color oracle.
The evidence rejects the broad geometry-corruption diagnosis and narrows the
open defect toward texture sampling/state, CLUT handling, or UV presentation.
Textureless and wireframe isolation remain the next visual probes. No
speculative renderer change was made from this comparison.

### Exact eight-component parity strengthened the visual comparison

The first ARM64 image above came from the full replacement producer while the
i686 image came from its separately regenerated prefix. To remove any doubt
about the exact game state at the captured frame, the preserved ASan ARM64
prefix was replayed at checkpoint 6 and captured again at frame 1,802:

```text
ARM64 ASan report:
/tmp/ctrpad-arm64-asan-v4-0Z3iyC/debug/reports/20260730/ctr-103044

/tmp/ctrpad-arm64-asan-parity-frame1802.rgba
SHA-256 36d86b3c8117bf3d9ea4c592097872e4cf28ad7db8ae812654487bc0b0c3fcc0

/tmp/ctrpad-arm64-asan-parity-frame1802.png
SHA-256 e6c2b79e1f64e04b6be963249f6b41366777fae3ba4a33610c047403199be1f4
```

`tools/compare-replay-state-components.mjs` compared that report to the
optimized i686 Release report `ctr-161136`. Timing, RNG, drivers, world,
allocation, root, pads, and VSync matched for all 2,200 frames. The new ARM64
capture still contained the striped surface. This is an exact input and
game-state visual mismatch, not a merely similar camera position. Render
primitive bytes and host GL state are not members of the canonical gameplay
digest, so the comparison does not yet distinguish CPU primitive conversion
from indexed-texture sampling.

### Textureless isolation proved that the race geometry is coherent

Three diagnostic attempts were rejected before the useful capture:

1. Enabling textureless mode at `BeginFrame(1802)` captured the preceding
   double-buffered frame and produced the unchanged textured image.
2. The post-run rebuilt executable did not match the preserved producer's
   stale debugger symbol map. The retry used the loaded image address rather
   than treating old symbol addresses as current.
3. A breakpoint conditioned on `BeginFrame(1800)` could not fire because
   checkpoint 6 changes replay frame 0 to frame 1,800 from inside that first
   `BeginFrame` call, after the condition was evaluated. Only that diagnostic
   process was stopped.

The accepted probe stopped at `NativeReplayScheduler_EndFrame` for frame
1,800, wrote `1` to `g_dbg_texturelessMode`, verified the new value in process
memory, and captured at `EndFrame(1802)`:

```text
/tmp/ctrpad-arm64-race-frame1802-textureless-v3.rgba
SHA-256 9a81b82657caf642e5d32819d569e03414806c5379fed091de4ab0d937cd8b2b

/tmp/ctrpad-arm64-race-frame1802-textureless-v3.png
SHA-256 3f057233cf61a4a3b0e48a45bad9db9370c2f7f5a54eeeaeb36ddc9d2681561b
```

The corrupted stripes disappear. The road, kart, camera, and track polygons
remain coherent; textured sprites and fonts become white blocks as expected
when the renderer substitutes its one-pixel white texture. This isolates the
observed defect to the texture/CLUT/UV path and rejects level geometry as the
leading cause. The raw and converted retail-derived captures remain local and
ignored.

### A higher-precision VRAM-storage probe rejected the RG8 hypothesis

A disposable ARM64 build changed the renderer's two-channel VRAM attachment
from normalized `GL_RG8` storage to `GL_RG32F`. The experiment was isolated in
`/tmp/ctrpad-arm64-rg32f-DKSBjH`; the temporary source switch was removed after
the result and is not part of the accepted diff.

Three operational details are part of the record:

1. The first run appeared stopped in an audio semaphore, later advanced, and
   was deliberately killed because it was only a diagnostic.
2. The first exact-frame debugger capture declared `glReadPixels` with eight
   arguments instead of its actual seven and produced no usable image. That
   report was rejected as incomplete at frame 1,802.
3. The corrected readback captured frame 1,801. A later debugger `free` call
   lacked a declared return type, but that happened after the complete buffer
   had been read; the capture itself is valid.

```text
diagnostic producer:
/tmp/ctrpad-arm64-rg32f-DKSBjH/ctr_native
SHA-256 9c34ec67044856f9882bada3769818c262956d90b2c8091e09b9544a8fa3d8c0
(temporary diagnostic only)

/tmp/ctrpad-arm64-rg32f-frame1801-fast.rgba
size 1,920,000
SHA-256 104b0f56fee9009a7738ce88ec0d5159d802b1f341960f2f2d45314a638872be

/tmp/ctrpad-arm64-rg32f-frame1801-fast.png
SHA-256 ffabd742b571d82cabd9a92e43efe4605d9109c0a5ac38a80e847f80f41459fd
```

The striped right-side surface remained. Some flat colors changed, as expected
when changing the attachment representation, but the defining defect did not.
This rejects normalized RG8 precision and host-side dithering as the root
cause and moves the investigation upstream of VRAM storage.

### A canonical CPU render trace separated game state from primitive state

The canonical replay digest deliberately excludes renderer output. A new
playback-only diagnostic, `--render-trace-frame N`, therefore hashes:

- the packed 20-byte `GrVertex` byte stream;
- canonical draw-split state including texture format, blend mode, draw and
  display environments, and vertex ranges;
- semantic white/VRAM/override texture kinds instead of host OpenGL IDs; and
- every `DrawAllSplits` flush plus a frame aggregate.

Host pointers, OpenGL object names, and padding are excluded. The
implementation is in `platform/native_gpu.c`,
`include/platform/native_gpu.h`, and `platform/native_replay_scheduler.c`;
the command is documented in `docs/REPLAYS.md`. The playback-only and invalid
frame-value rejection paths were exercised, and the ARM64 build passed all 13
CTests before evidence collection.

The new ARM64 prefix report finalized normally:

```text
report:
build-macos-arm64/debug/reports/20260730/ctr-124322

producer copy:
/tmp/ctrpad-arm64-render-trace-producer
SHA-256 bac8f3ba77b81f834f8546e114718bd2cdf3a2d9b80362cbf80206ec3d1825f0

input.ctrreplay:
size 968,148
SHA-256 d96ae5967f974d28bfbd2d536f4356c8c25a77f164a7d33747ecf2c618c013fc

state.ctrstates:
size 35,476,672
SHA-256 ba1a951e836fbe6462128d9184f6bbe358ac8055fb589f09f2c7bd0a728b2bdf

metadata.txt:
SHA-256 5520c5f1a545bbadb0a2b4e58addb7107a84577aa0a147cb0520bd58856c0f31

ctr-native.log:
SHA-256 8ed6bd4b2627e6b0a01fb59bc8cc881f469a1daebe53b7dddd075dec0306ba4a
```

All eight replay components match the earlier ASan prefix for all 2,200
frames. Its pre-fix frame-1,802 render trace was:

```text
flush 0  63a7380632d5638c  vertices=2220 splits=65
flush 1  93521c31d474278b  vertices=150  splits=22
flush 2  9118e5e226fea243  vertices=168  splits=27
flush 3  903548b85ad95a70  vertices=360  splits=62
flush 4  1fcbed182bb64484  vertices=66   splits=11
flush 5  9f903af4ae46ef5e  vertices=108  splits=18
flush 6  30edbc4ccf8668dc  vertices=264  splits=38
flush 7  df8bf96380d5d5b7  vertices=30   splits=6
flush 8  ccaec33437cec6f1  vertices=2625 splits=275

aggregate 9df50898e88fb638
flushes=9 vertices=5991 splits=524
formats=4:462,8:37,16:25,rgba:0

log:
/tmp/ctrpad-arm64-render-trace-frame1802.log
SHA-256 f5a4588daf2d33cd4dd34b5c35a2b65871478992a8480cdbcc56c8544631fabb
```

The same total vertex count as i686 suggested coherent geometry, while nine
flushes and 25 16-bit splits implicated erroneous framebuffer-feedback
classification.

### The optimized i686 trace supplied the decisive oracle

The diagnostic i686 build used a clone of an already configured disposable
cache at `/tmp/ctrpad-i686-render-cache-RfAU00`; the immutable historical
baseline was never built into or modified. Starting a second clean configure
was stopped when redundant SDL feature probes under emulation proved
unnecessary. The cloned cache then built successfully and passed all 13
CTests. Its final wrapper command returned nonzero only because a post-build
`chown` tried to change the read-only mounted retail image; compilation and
tests had already succeeded.

```text
/tmp/ctrpad-i686-render-cache-RfAU00/ctr_native
ELF 32-bit Intel 80386, debug_info, not stripped
size 9,198,724
SHA-256 1ac34121e05b5615bf011530b0f9350a465103bb645f4b10ea6f536fc585cef1

report:
/tmp/ctrpad-i686-render-cache-RfAU00/debug/reports/20260730/ctr-175553

input.ctrreplay:
size 968,148
SHA-256 e2f5e6124d8308e78a4484940aea7e1f796390a45f892e66500954981243dc79

state.ctrstates:
size 35,341,376
SHA-256 52af48c4fd48e21a87057e19e6fbd6821ab344babc139263bd3e74c576ddb2b0

metadata.txt:
SHA-256 acfcdb36aabfdda83f621766ca053470e85406baa667ad6c594440ffd5fe0f59

ctr-native.log:
SHA-256 f85cba408834079b39885a72999315f95aadd90527c1646d8db916f7ecfe7a28
```

The first Xvfb readiness check used absent `xdpyinfo`, so it was rejected
before game initialization. For the later exact trace, the original local
image tag had been removed even though the configured container remained.
Two launches that requested the missing tag or forced the committed amd64
container snapshot to `linux/386` were rejected before game start. Reusing a
local snapshot without the incorrect platform override then succeeded.

Timing, RNG, drivers, world, allocation, root, pads, and VSync match ARM64 for
all 2,200 frames. At exact frame 1,802, i686 produced:

```text
flush 0  4d1cbba5c0098b5c  vertices=5991 splits=352
aggregate 76303b9b4c2ee4c2
flushes=1 vertices=5991 splits=352
formats=4:327,8:25,16:0,rgba:0
```

This proves the visible mismatch was already present in the ARM64 CPU render
stream. The Apple OpenGL implementation and retail texture bytes were not the
root cause.

### Framebuffer-copy bounds were correct but their activation was not

Two initial LLDB breakpoint-command constructions retained only `continue`.
They counted nine `NativeRenderer_StoreFrameBuffer` calls but printed no
arguments; the second redundant run was stopped early. The corrected
multi-command breakpoint completed the exact replay:

```text
/tmp/ctrpad-arm64-frame1802-feedback-regions-v3.log
size 13,945
SHA-256 6888987a139aec78ec35767f20ac9dfb77541b11c188e87c04e71a78eef849a3
```

All nine calls copied `(x=0,y=296,w=512,h=216)`. Their active tpage sequence
was `26608,26608,26608,32695,26608,32695,32695,32695,111`. The copy rectangle
was internally consistent. The impossible high pages `0x67f0` and `0x7fb7`,
not a partial framebuffer rectangle, triggered the extra barriers.

An exact conditional trap on the first 16-bit classification stopped at:

```text
NativeGpu_TPageOverlapsActiveDrawPage(tpage=26608)
ProcessGouraudPoly
POLY_GT4 packet=0x00000001004eb334
packet storage=s_mempackMemory+891516
```

The primitive layout static assertions pass on both widths. The invalid page
was therefore produced by DrawLevel, not misread by the generic GPU parser.

### Root cause: an LP64 guest reference was treated as a host pointer

DrawLevel reads a level mosaic texture word from texture-layout data. After
relocation:

- i686 stores a native 32-bit host pointer in that word; and
- LP64 stores the ADR-0001 eight-bit-region/24-bit-offset guest reference.

`DrawLevelOvr1P_IsNativeLevelTexturePointer` cast the fixed-width word through
`uintptr_t` and checked whether that numeric value lay inside MEMPACK. That is
valid for i686 and necessarily false for LP64. ARM64 consequently treated a
valid mosaic reference as a positive inline sentinel, advanced to the wrong
texture-layout record, skipped the required deepest mosaic reload, and copied
unrelated UV bytes into `POLY_GT4.tpage`. Values such as `0x67f0` then looked
like 16-bit framebuffer texture pages to the GPU batching layer, causing the
extra feedback copies and visible stripes.

The accepted correction in
`game/226/226_00_DrawLevelOvr1P.c`:

- resolves the fixed-width word through `CtrAssetRef_ResolveRequired`, which
  preserves i686 direct-pointer behavior and decodes LP64 guest references;
- validates the resolved byte range against the active level MEMPACK span;
- applies the existing plausible-texture check to the resolved host address;
  and
- resolves and bounds-checks all three deepest mosaic source reads instead of
  casting `mosaicBase + sourceOffset` directly.

No retail bytes, shader code, texture formats, feedback heuristics, or
geometry code were changed.

### Fixed ARM64 regeneration, trace, and presented-frame evidence

The first fixed regeneration command used a relative seed path. Because the
application changes its working directory to the executable base, that path
resolved under `build-macos-arm64` twice. It failed before frame zero and
created no valid report. The accepted retry used the absolute seed path and
regenerated from frame zero rather than bypassing the old executable
fingerprint or restoring callbacks from an incompatible unity-build
checkpoint.

```text
report:
build-macos-arm64/debug/reports/20260730/ctr-133039

producer copy:
build-macos-arm64/ctr_native-133039-producer
SHA-256 8b2ddfe4ef5f0c0df23dc26ea2d53da38ec1610895c6a828f5ff079af2eb3b7d

input.ctrreplay:
size 968,148
SHA-256 57a471f4e71823b3942f5483687a586fb0f838d78eafe647cf539936e98371e7

state.ctrstates:
size 35,476,672
SHA-256 96d4898a3a59d5267244a4eac6e7e1a7f4f1c512fdafecd9e730277e72929f2e

metadata.txt:
size 1,043
SHA-256 d3384fee3e0c2f8bec7340cb6e4c9b0ddbac9e79dc99d5befa14ff2375caac4a

ctr-native.log:
size 2,116
SHA-256 1fe3b7d1b1effb245bcfe5652f6cf74ba06fb71585e66dfc2f879e95c5f264a1
```

The report finalized 2,200 frames with eight checkpoints. All eight canonical
components match the pre-fix report for every frame, proving the visual fix
does not alter gameplay timing or state.

The fixed ARM64 render trace now matches the i686 trace exactly:

```text
flush 0  4d1cbba5c0098b5c  vertices=5991 splits=352
aggregate 76303b9b4c2ee4c2
flushes=1 vertices=5991 splits=352
formats=4:327,8:25,16:0,rgba:0
```

Presented-frame capture had two rejected debugger attempts after stopping at
the correct frame:

1. `Platform_TakeScreenshot` had been inlined away in the optimized unity
   build and could not be named by LLDB.
2. The first direct-readback command passed the literal text `\u0024capture`
   instead of LLDB's `$capture` persistent variable, so allocation never ran.

The corrected seven-argument `glad_glReadPixels` call wrote the full default
framebuffer and the replay then exited normally:

```text
/tmp/ctrpad-arm64-race-frame1802-fixed.rgba
size 1,920,000
SHA-256 b11f5585b3e8414db476b39ef544559b8ef1931f685a54f167e3e9da042d7fc3

/tmp/ctrpad-arm64-race-frame1802-fixed.png
800 x 600 RGBA, vertically corrected
SHA-256 03ce067255e843fbb010687bf2390930110a8e3201d99a085f519784dae08cc4
```

Visual inspection shows coherent Crash Cove road and dirt textures, starting
grid checker pattern, sky, karts, exhaust, and `ARCADE`/`CRASH COVE` text.
The high-frequency striped surface is gone. The retail-derived capture remains
local and ignored.

Post-fix regression results:

```text
macOS ARM64 internal: 13/13 CTests
macOS ARM64 ASan:     13/13 CTests
Linux i686 internal:  13/13 CTests
git diff --check:     clean
```

During the final pre-publication rerun, invoking `ctest --test-dir
build-linux-i686-m3` directly from macOS was rejected as an invalid test
environment: that CMake cache was configured inside the i686 container and
records its binary directory as `/out`. It could not create
`/out/Testing/Temporary` on the host. Running `ctest --test-dir /out
--output-on-failure` inside the still-active `ctrpad-i686-debug` container
produced the accepted 13/13 result above.

The protected historical i686 binary remains unchanged:

```text
build-linux-i686-baseline/ctr_native
SHA-256 afe7b3d264bd2674192e71485037842ab329d1eca9c0c34ca50da5d4487c76a7
```

## 2026-07-30 — Current-source full i686 version-4 regeneration launched

The next parity stage uses the finalized 24,232-frame ARM64 version-4 report
as the sole pad and complete-VSync seed. This entry is an in-progress
publication checkpoint, not acceptance of the unfinished i686 report.

### Exact inputs and disposable build boundary

```text
source commit:
53ab70e966b262b42c3f0c00b0c5748405622ad9

ARM64 seed:
build-macos-arm64/debug/reports/20260730/ctr-115352/input.ctrreplay
size 10,662,228
SHA-256 bf022938a8580e91fa06045f0cabb6b58a67fb7cc16bbd4909601b7e1ce93b86

user-owned NTSC-U BIN:
ref/CTR/CTR - Crash Team Racing (USA).bin
size 605,698,800
SHA-256 f780bf2331476aabfc00772fa758b12dd95ebfbc907968132cbd3cdd4e2c07c0

disposable output:
/tmp/ctrpad-i686-full-v4-current-cbRPWn
```

The disposable output began as a copy of the previously configured Release
cache at `/tmp/ctrpad-i686-release-v4-QkBKSJ`. A clean build removed all 261
objects before rebuilding. The protected
`build-linux-i686-baseline/ctr_native` was neither mounted as output nor
modified.

Three pre-launch results were explicitly rejected:

1. Executing the ELF32 producer directly from macOS returned
   `exec format error`; only the container invocation is meaningful.
2. Docker Desktop exposed the read-only `/src` mount root as UID 0 even
   though `.git` remained UID 502. Git rejected the worktree as dubious, so
   CMake labeled the otherwise passing build `unknown-dirty`. That executable,
   SHA-256
   `79b173a3ee81e8427642cc8d1224d50cf44329608c27b82ab27947a384f2982c`,
   was not used.
3. The first negative verifier wrapper tried to assign zsh's read-only
   `status` parameter after correctly finding the missing build manifest.
   The wrapper itself therefore failed and was replaced with a
   task-specific variable; it did not launch the game.

The accepted reconfigure passed a process-local Git safe-directory setting
through `GIT_CONFIG_COUNT`, `GIT_CONFIG_KEY_0`, and
`GIT_CONFIG_VALUE_0`. CMake then embedded the exact source label, rebuilt the
unity object and affected SDL identity object, and passed all 13 CTests:

```text
version:
CTR Native 0.1.0-beta.7.1 (53ab70e966b2)

producer:
/tmp/ctrpad-i686-full-v4-current-cbRPWn/ctr_native
ELF 32-bit Intel 80386, Release with DWARF
Build ID 26b34e7c1b73fd5f087adf67a5b3a26ea6421558
SHA-256 42df6c21f43539248212c9614e2c2fb3f5223d9cc2cfd4af876cbdd6d2f193a3

producer preservation copy:
/tmp/ctrpad-i686-full-v4-current-cbRPWn/ctr_native-full-v4-producer
SHA-256 42df6c21f43539248212c9614e2c2fb3f5223d9cc2cfd4af876cbdd6d2f193a3

toolchain manifest:
/tmp/ctrpad-i686-full-v4-current-cbRPWn/toolchain-packages.txt
SHA-256 7533bb723143fa947795ded64dbde4c82ffb7bb902b246929a11ac725ec22cd6
```

The compiler repeated the two upstream format-security diagnostics and the
previously reviewed optimized-only guarded-coordinate warning in
`game/222.c`. No new warning came from the mosaic-reference correction or
render trace.

### Live regeneration and prefix evidence

The full run launched at `2026-07-30T14:04:57-0500` under `caffeinate`, Xvfb,
llvmpipe, and dummy host audio:

```text
destination:
/tmp/ctrpad-i686-full-v4-current-cbRPWn/debug/reports/20260730/ctr-190458

source timing declaration:
version=4 complete-vsync=yes
```

At the publication checkpoint `2026-07-30T14:23:38-0500`, the in-flight file
contained 3,211 complete frames while metadata had durably checkpointed frame
3,000 and 11 checkpoints. `finalized=0` is expected and intentionally
prevents an acceptance claim. The prefix comparator required and found exact
equality for all 3,211 available frames:

```text
timing, RNG, drivers, world, allocation, root: equal=3211 mismatched=0
pads, VSync:                                  equal=3211 mismatched=0
```

An external monitor sends the host-only F12 screenshot command at 600-frame
metadata intervals. The key is not mapped into the PS1 pad snapshot; pad
equality above also proves that the recording input stayed unchanged.
Accepted visual checkpoints so far are:

```text
frame 300:
/tmp/ctrpad-i686-full-v4-current-cbRPWn/visual-evidence/frame-300-startup.bmp
SHA-256 ae306de912505a33691271c4264878ee7634fea42c5fdb13b882dc146cc6035a
coherent CTR intro

frame 1500:
/tmp/ctrpad-i686-full-v4-current-cbRPWn/visual-evidence/frame-1500-menu-load-upright.png
SHA-256 61ff162f760779f7169d588ee1495fed4db290a37232abb9d0910d49ca050c09
Crash Cove selected in the track menu

frame 2400:
/tmp/ctrpad-i686-full-v4-current-cbRPWn/visual-evidence/frame-2400-upright.png
SHA-256 abd57ccd53b951f1ea23907ef80e9fdc8ced6b9dcfe73a195b42d1a1b04942ab
active Crash Cove race, lap 1/3, HUD, kart, track, and item slot
```

The known llvmpipe cyan/yellow color error makes these structural and coverage
captures, not a color oracle. They remain local and ignored because they are
retail-derived.

### Verifier strengthened before acceptance

`tools/verify-linux-i686-golden-replay.sh` now accepts an explicit disposable
build directory and full expected source commit without overwriting the
protected baseline. It additionally requires:

- a source commit present in the repository and matching 12-character report
  and producer build IDs;
- finalized replay version 4;
- exactly 24,232 frames and 81 checkpoints by default;
- a retail image whose size is a multiple of 2,352-byte raw sectors; and
- exact `replay finished after 24232 frames` lines from both unchanged
  playback processes.

Invalid zero counts and a non-hexadecimal source commit both reject with
status 1. `sh -n` and `git diff --check` pass. The disposable-build invocation
is documented in `docs/parity/NTSC-U-GOLDEN-RUN.md`.

The GitHub implementation remains intentionally unmerged while the product is
unfinished. Commit `53ab70e96` is present on `origin/codex/arm64-apple`, and
the branch is the head of the open draft PR in
`chrissotraidis/ctrpad#1`; the default `main` view therefore does not yet show
these implementation files.

### Live parity, visual, and publication checkpoint at 14:47 CDT

The worktree and remote implementation branch both identified commit
`5f6d3ca2d4b5775afcbdfe105924f34c69bacb30`. A direct GitHub query confirmed
that `chrissotraidis/ctrpad#1` remained open and draft with
`codex/arm64-apple` at that exact head, while `main` remained at
`95417c723518407d6bfe3c81a37606294963efe2`. Consequently, GitHub's default
branch view still did not show the implementation even though it was backed
up in the draft pull request. No merge was attempted while parity acceptance
remained incomplete.

At this checkpoint the current-source i686 recording contained 6,384 complete
frames. The prefix comparator required all eight available components and
reported zero mismatches:

```text
timing, RNG, drivers, world, allocation, root: equal=6384 mismatched=0
pads, VSync:                                  equal=6384 mismatched=0
```

Additional screenshots were inspected rather than accepted automatically:

```text
frame 4200:
/tmp/ctrpad-i686-full-v4-current-cbRPWn/visual-evidence/frame-4200-upright.png
SHA-256 59f7609ec09070c25280d38b04ae949abc227925b2fabaa42f0ae7cc4639be82
active Crash Cove race; coherent kart, HUD, start gantry, road, and minimap

frame 4800:
/tmp/ctrpad-i686-full-v4-current-cbRPWn/visual-evidence/frame-4800-upright.png
SHA-256 f75f4f58d41b1dce16c344d92c739b3c1f847e9ee79e9081c0029161120be266
rejected: black transition/buffer capture

frame 5400:
/tmp/ctrpad-i686-full-v4-current-cbRPWn/visual-evidence/frame-5400-upright.png
SHA-256 250cbf6979b82678922967ee7c9c73109fab5d0601b87a869a50afbcf62b95be
active Crash Cove race; coherent kart, HUD, road, walls, scenery, and minimap

frame 6000:
/tmp/ctrpad-i686-full-v4-current-cbRPWn/visual-evidence/frame-6000-upright.png
SHA-256 f75f4f58d41b1dce16c344d92c739b3c1f847e9ee79e9081c0029161120be266
rejected: black transition/buffer capture
```

The process remained runnable and CPU-active while the file advanced. The
repeated black image hash therefore represented a presented-buffer capture
boundary, not a replay stall or claimed texture regression.

A disposable performance probe tested Mesa's `GALLIUM_DRIVER=noop` against a
clone of the historical i686 report. The bundled Mesa library exposes the
no-op Gallium symbol, but the probe failed during platform initialization:

```text
glx: failed to create drisw screen
X Error of failed request: BadValue
Minor opcode: X_GLXCreateContext
```

This path was rejected before replay and cannot replace the accepted llvmpipe
renderer. The 363 MiB disposable clone and its path marker were moved to the
macOS Trash after the failure; they are recoverable until the Trash is
emptied. The live report, protected baseline, source tree, and retail image
were not modified by the probe.

## 2026-07-30 — Full cross-width divergence: invalid AI restart index

**Status:** the first current-source full optimized-i686 candidate is formally
rejected. It matched all eight replay components through frame 6,779, then
used relocated words beyond the level's restart-node array as a fake node at
frame 6,780. A native-only bounds correction and a dedicated regression test
pass ARM64 Release, ARM64 ASan/UBSan, and i686 Release. New full ARM64 and
i686 reports are required before parity can be accepted.

This section preserves the complete causal path, including visual evidence,
rejected debugger routes, raw memory evidence, instruction-level results, and
the exact limit of the correction. The protected historical i686 baseline was
never rebuilt or used as an output directory.

### Presented-window capture replaced the black F12 captures

The live report remained:

```text
/tmp/ctrpad-i686-full-v4-current-cbRPWn/debug/reports/20260730/ctr-190458
producer:
/tmp/ctrpad-i686-full-v4-current-cbRPWn/ctr_native-full-v4-producer
producer SHA-256:
42df6c21f43539248212c9614e2c2fb3f5223d9cc2cfd4af876cbdd6d2f193a3
source commit:
53ab70e966b262b42c3f0c00b0c5748405622ad9
```

Several internal F12 screenshots were all black with the same hash. The
process was CPU-active and continued appending frames, so these did not prove
a game stall. Inspection of the renderer and screenshot timing showed that
the internal path reads the default back buffer after swap, where the content
is undefined.

An external X11 helper, `/tmp/ctrpad-x11-capture`, instead captured the
visible Xvfb window. This is the authoritative Linux presented-frame boundary.
Captures at frames 7,200, 7,800, 8,400, 9,000, 9,600, 10,200, 10,800, and
11,400 showed live changes. The inspected frame-11,400 image is:

```text
/tmp/ctrpad-i686-full-v4-current-cbRPWn/visual-evidence/frame-11400-presented.png
800 x 600
size 172,364
SHA-256 70fb97770f0ca0b55ba34182949ef08f24340a65abb94bcfbc4f7644e4c9a34a
```

It shows a coherent Crash Cove race view: kart, canyon and waterfall
textures, HUD, racer portraits, and minimap. The established llvmpipe
cyan/yellow color skew remains, so this is a geometry/texture-presence oracle,
not a color oracle. This directly answers the reported “stuck” concern: the
presented window and textures are visible and changing, even though the
canonical parity result later fails.

### Comparator located the first game-state mismatch

The required prefix command remained:

```text
node tools/compare-replay-state-components.mjs \
  --prefix \
  --require timing,rng,drivers,world,allocation,root,pads,vsync \
  build-macos-arm64/debug/reports/20260730/ctr-115352/input.ctrreplay \
  /tmp/ctrpad-i686-full-v4-current-cbRPWn/debug/reports/20260730/ctr-190458/input.ctrreplay
```

All eight components matched through end-of-frame 6,779. At frame 6,780,
only `drivers` and the aggregate `root` changed. Timing, RNG, world,
allocation, pads, and VSync remained exact. The driver digests at that frame
were:

```text
ARM64 expected: 7d42e48e3ebba032
i686 actual:    48f0799f58d94ac9
```

This ordering matters. It rejects timing, input, VSync, allocator order,
world-state, and initial RNG drift as causes of the first mismatch.

The report was not stopped after the failure. It was retained to collect a
complete failure and coverage map. At the documentation checkpoint, it held
16,758 complete frames and 56 durably written rolling checkpoints while
`finalized=0`. The comparator then reported:

```text
timing:     equal=16758 mismatched=0
rng:        equal=16240 mismatched=518
drivers:    equal=12573 mismatched=4185
             ranges=6780-6793,6854-7011,9825-13837
world:      equal=16643 mismatched=115
allocation: equal=15988 mismatched=770
root:       equal=12376 mismatched=4382
             ranges=6780-6793,6854-7011,9825-13837,16561-16757
pads:       equal=16758 mismatched=0
VSync:      equal=16758 mismatched=0
```

This later cascade is diagnostic only. The report is already a failed parity
candidate at frame 6,780 and cannot be passed to the unchanged-process or
mutation acceptance verifier.

### Raw driver snapshots reduced the mismatch to two scalars

Exact checkpoint-22 restores were advanced from replay frame 6,600 to 6,780.
The two isolated evidence trees are:

```text
/tmp/ctrpad-arm64-divergence-6780
/tmp/ctrpad-i686-divergence-6780
```

They contain producer copies, replay/checkpoint inputs, `GameTracker` dumps,
all eight raw `Driver` objects, normalized canonical-field dumps, and
debugger logs. Producer identities are:

```text
ARM64 copied producer SHA-256:
588786e7eaaee5c845dc2c1e6c101178cc93be3a62e94857aea338338cba769e

i686 copied producer SHA-256:
42df6c21f43539248212c9614e2c2fb3f5223d9cc2cfd4af876cbdd6d2f193a3
```

A temporary parser, `/tmp/ctrpad-driver-dump.c`, was compiled at both pointer
widths. Its source SHA-256 is
`f7cfef1e596cb4e46f5161c0ebb1b7f57224c4bea50e1d5227592fb04e95526f`.
It emitted every scalar used by the canonical driver digest and normalized
away only representation fields intentionally excluded from that digest.

Comparing all eight drivers found exactly two unequal canonical fields:

```text
driver 4:
  ARM64 distanceToFinish_curr = 4294962623 (-4673 signed)
  i686  distanceToFinish_curr = 31

driver 7:
  ARM64 distanceToFinish_curr = 4294962613 (-4683 signed)
  i686  distanceToFinish_curr = 12
```

Every other canonical driver scalar was equal. Both affected drivers had:

```text
bot.ai_quadblock_checkpointIndex = 255 (0xff)
```

### Rejected debugger routes before the accepted captures

The debugger setup retained several failures rather than erasing them:

1. The first diagnostic container had GDB but no callable `qemu-i386`, so it
   could not run the ELF32 target explicitly.
2. Explicit `qemu-i386` loaded the PIE near `0x00400000`, while Docker's
   binfmt path loaded it near `0x40000000`. The restored checkpoint then
   correctly rejected a callback identity mismatch:
   `old=0x40a92720 expected=0xe92720`. This was an invalid address-layout
   experiment, not game evidence.
3. A later container run omitted the retail asset mount and failed before the
   replay boundary. The next had assets but no X display and failed SDL
   initialization. The accepted route used the normal binfmt loader,
   `QEMU_GDB`, Xvfb, and the ignored user-owned asset.
4. The first ARM64 LLDB script used `0x10287c60` as the SDATA address. The
   correct address for the copied producer was `0x100287c60`.
5. A symbolic breakpoint intended to perturb a digest helper did not fire
   because the optimized unity build inlined it. The accepted raw dump used
   the end-frame boundary with an exact ignore count.
6. The first ARM64 instruction capture stopped before the relevant `ldp`, at
   `0x100079d64`, so `w8/w9` were stale. Moving the stop to
   `0x100079d68` captured the consumed projection and wrong-way values.
7. The GDB convenience expression printed both selected targets as
   `driver=0` because typed pointer arithmetic applied the diagnostic offset
   twice. The breakpoint conditions themselves selected
   `driverID == 4` and `driverID == 7`; the stopped driver addresses and raw
   dumps prove the targets.

The accepted GDB image is:

```text
ctrpad-linux-i686-gdb:ubuntu-24.04
sha256:200e3129c0695b730965ae5c957e6d126fa1f9bf2761aa10c5c50d10869b63e2
```

### Instruction-level captures rejected arithmetic and GTE drift

The accepted logs are:

```text
/tmp/ctrpad-arm64-divergence-6780/arm64-vehlap-6780.log
SHA-256 1636492385b452ac34d281d9c065a6976b14c98f2192f94f1d364a6ed2be5e7e

/tmp/ctrpad-i686-divergence-6780/i686-vehlap-6780.log
SHA-256 d4e3c57d1b1130faa82c28cea4f1b6882e13fa50b64287edfd5588482665c4b3
```

At the exact progress/remainder calculation:

```text
                         ARM64 driver 4    i686 driver 4
projection                 -28,478,767      -50,662,030
wrong-way dot                  491,646          508,404
track length                    60,016           60,016
selected node distance           2,280          132,432
progress / remainder            -4,673               31
normalized V0 x,y,z       386,1997,3555     221,1145,3926

                         ARM64 driver 7    i686 driver 7
projection                 -28,520,405      -50,740,686
wrong-way dot                  487,017          515,197
track length                    60,016           60,016
selected node distance           2,280          132,432
progress / remainder            -4,683               12
```

Each port's projection, fixed-point shifts, signed division/remainder, and
stored result agree with its own selected node bytes. The failure is therefore
upstream of the MIPS/GTE arithmetic: the ports were given different node
inputs.

### Checkpoint bytes proved an out-of-bounds relocated-word dependency

Checkpoint parsing located the level's MPAK region and restart-node base in
both pointer-width records:

```text
ARM64:
  checkpoint MPAK payload offset 135752
  MPAK runtime base              0x1004118b8
  restart-node MPAK offset       0x1306a0

i686:
  checkpoint MPAK payload offset 118844
  MPAK runtime base              0x408c7bc0
  restart-node MPAK offset       0x13a55c
```

The declared restart nodes align logically and begin with equal serialized
bytes. Index `0xff`, however, is far outside the declared array. The fake
node's `nextIndex_forward` was `239`, which led to another out-of-bounds fake
node. Comparing the extended 256-node window found relocated pointer slots
among the consumed bytes:

```text
ARM64 example relocated word: 0x011c5684 (guest reference)
i686 example relocated word:  0x40a97100 (direct host pointer)

fake node 239 first eight bytes:
ARM64 1c00ff12100a1d01
i686  1c00ff128c24aa40
```

The checkpoint at frame 1,800 already contains these representation
differences, as expected for relocated asset pointers. They first become
game-visible at frame 6,780 when the two bots pass the `0xff` sentinel into
`VehLap_UpdateProgress`.

This also means copying the observed i686 garbage result into ARM64 would not
be a valid correction: a second i686 process can relocate those host pointers
to different addresses under ASLR. Game state must not depend on bytes beyond
the owned restart array.

### Accepted native boundary and regression test

The ASM-verified retail routine is left unchanged for `!CTR_NATIVE`.
`VehLap_UpdateProgress` now applies the native ownership rule before resolving
or indexing the restart array:

```text
0 <= checkpointIndex < level->cnt_restart_points
```

An invalid index performs no progress update, consistent with the function's
existing early returns when a driver, checkpoint, or restart array is not
available. No physics constant, GTE operation, track distance, timing rule,
input packet, asset byte, or PS1 instruction path was changed.

The new media-free entry point
`--self-test-vehicle-lap-checkpoint-bounds` verifies:

```text
first index:       valid
last index:        valid
one-past-end:      rejected
0xff sentinel:     rejected
negative index:    rejected
empty level:       rejected
null level:        rejected
```

Accepted post-correction gates:

```text
macOS ARM64 Release:
  build-macos-arm64/ctr_native
  Mach-O 64-bit executable arm64
  14/14 CTests

macOS ARM64 ASan/UBSan:
  /tmp/ctrpad-macos-arm64-asan-vehlap-OTDxVr
  -fsanitize=address,undefined -fno-omit-frame-pointer
  ASAN_OPTIONS=symbolize=0:abort_on_error=1
  UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1
  14/14 CTests

Linux i686 Release:
  /tmp/ctrpad-i686-vehlap-8IzCKm/ctr_native
  ELF 32-bit LSB PIE, Intel 80386
  Build ID bc70ff1a6c3c42921fbfdf199d1d22102251d314
  14/14 CTests

git diff --check:
  passed
```

The i686 compiler repeated only the established two format-security and two
optimized maybe-uninitialized warnings. The ARM64 builds repeated the
established upstream/native warning set. No new warning names the checkpoint
boundary or its test.

The protected historical executable is still byte-identical:

```text
build-linux-i686-baseline/ctr_native
SHA-256 afe7b3d264bd2674192e71485037842ab329d1eca9c0c34ca50da5d4487c76a7
```

### Publication and acceptance boundary

Before this correction, local `HEAD`, `origin/codex/arm64-apple`, and the
draft pull request head were all commit
`f1c63bf5a6a2048485f65c993c0978f108d40222`. GitHub `main` remained
`95417c723518407d6bfe3c81a37606294963efe2`; this is why the implementation
was not visible in the default branch even though it was backed up in the
draft PR. No merge was attempted.

The next publication checkpoint will commit the native guard, fourteenth
CTest, roadmap update, parity-result update, and this chronological record to
`codex/arm64-apple`, then push that branch. It will not merge the draft pull
request.

The correction is structurally and sanitizer tested but is not yet full-run
accepted. The required next sequence is:

1. let the rejected i686 report finish only for its failure/coverage map;
2. generate a new 24,232-frame ARM64 version-4 report from corrected source;
3. generate a new optimized-i686 report from that corrected input;
4. require all eight components to match for every frame;
5. replay the accepted report unchanged in two separate processes; and
6. require the deliberate game-state mutation to fail at the named frame and
   component.

### First checkpoint publication result

The correction and its complete evidence set were committed and pushed:

```text
commit:
b6aff593968f8257f4c819f1492370821714f0d4
subject:
fix native AI checkpoint bounds

origin/codex/arm64-apple:
b6aff593968f8257f4c819f1492370821714f0d4

draft PR:
https://github.com/chrissotraidis/ctrpad/pull/1
state: OPEN
draft: true
head: codex/arm64-apple
head OID: b6aff593968f8257f4c819f1492370821714f0d4
base: main

origin/main:
95417c723518407d6bfe3c81a37606294963efe2
```

The PR body was updated to report the 14/14 gates, frame-6,780 diagnosis,
presented-window result, invalid-sentinel correction, and corrected full-run
acceptance sequence. It remains draft and unmerged. This preserves the
distinction the user called out: the implementation is backed up on GitHub,
but GitHub's default `main` branch still shows only the earlier merged
foundation/viability work.

## 2026-07-30 — Corrected full ARM64 report and explicit GitHub target audit

### Clean committed producers

Commit `55d3b71c6da56a5fdd3e7e7f206c6673061e96e6`
(`docs: record checkpoint publication`) was used to rebuild both corrected
producers from a clean source state. Their embedded build IDs therefore name
the same source commit:

```text
macOS ARM64:
  build-macos-arm64/ctr_native-corrected-full-producer-55d3b71c6da5
  CTR Native 0.1.0-beta.7.1 (55d3b71c6da5)
  SHA-256 49449fd9313a8a3414617058873dcd3a3f07401af770476b4468b8caa107f844
  CTest 14/14

Linux i686:
  /tmp/ctrpad-i686-vehlap-8IzCKm/ctr_native-corrected-full-producer-55d3b71c6da5
  CTR Native 0.1.0-beta.7.1 (55d3b71c6da5)
  ELF 32-bit LSB PIE, Intel 80386
  GNU Build ID 919868b2b65d04c4d3508eeca72e1cbe8cc33cdf
  SHA-256 e07be52d72e6a2f323587d9302467be366401485cecd29d86ae54846f973528c
  CTest 14/14
```

The corrected i686 producer is deliberately idle until rejected report
`ctr-190458` releases the QEMU/llvmpipe resources. Running both full rendering
jobs concurrently would make the diagnostic completion slower and would
weaken timing observations without adding parity evidence.

### Corrected ARM64 full report

The clean ARM64 producer replayed the original 24,232-frame input into:

```text
build-macos-arm64/debug/reports/20260730/ctr-170507
```

It finalized normally with replay version 4, 24,232 frame records, and 81
rolling checkpoints. It reached the end of the scripted race/save/load
sequence and logged both the successful powerslide event and replay
completion.

```text
input.ctrreplay
  size 10662228
  SHA-256 2c5d9b99a9f33854de8cce4a3cf19c7d5571183883d17115d7bf0379c2accfab
state.ctrstates
  size 359201012
  SHA-256 cbac56813503cd2ffb9e5c514fe33dcacb3d0eafc5c7ec6da666d5f275ba14c8
metadata.txt
  size 1040
  SHA-256 63c6a67e3d21543dccdc91a9d4f1afcb76240307f14a4b630db293c01572808e
ctr-native.log
  size 8336
  SHA-256 ca862cdca66e62213a125bb24272cedacc836ac47bfcbbb5af2d2a2eee93dfc5
memcard.recording/slot0/BASCUS-94426-SLOTS
  size 6016
  SHA-256 6a01b0f5562ed7a279d8f8e51e3b1874ac39a6120f55db4fe3873288950619a3
```

Comparing this corrected report with the superseded pre-guard ARM64 report
shows that input scheduling stayed identical while state descended from the
corrected AI result:

```text
timing:      24232 equal, 0 mismatch
rng:         24232 equal, 0 mismatch
pads:        24232 equal, 0 mismatch
vsync:       24232 equal, 0 mismatch
drivers:     19987 equal, 4245 mismatch
root:        19987 equal, 4245 mismatch
world:       24034 equal, 198 mismatch
allocation:  24030 equal, 202 mismatch

drivers/root mismatch ranges:
  6780-7011
  9825-13837
world mismatch range:
  6814-7011
allocation mismatch range:
  6810-7011
```

These are expected before/after-fix differences, not the cross-architecture
acceptance comparison. The next optimized-i686 report must match the
corrected ARM64 report on all eight components for all 24,232 frames.

Two independent unchanged ARM64 playback processes were then scheduled
sequentially against `ctr-170507`. At the time of this entry, attempt 1 was
active, advancing at the expected approximately 30 game frames per second,
and had produced no divergence. Their final status, log hashes, host-address
samples, and mutation result will be added after both processes finish.

### User-visible rendering at the apparent pause

The rejected i686 diagnostic recording remained compute-bound rather than
stuck. Its metadata advanced to frame 22,800 while the QEMU/llvmpipe process
continued using multiple CPU cores. An external X11 capture at the replay's
name-entry segment provides later visual evidence:

```text
/tmp/ctrpad-i686-full-v4-current-cbRPWn/targeted-visual-evidence/
  target-22380-captured-22380.png
SHA-256:
71cbc6388b973eea21d62842675d60b5201e292a9c908091413181a2bfd477f2
```

The presented window contains the coherent `PLEASE ENTER YOUR NAME` screen,
letter/number grid, Save/Cancel controls, character and kart background, and
loaded textures. Together with the earlier frame-11,400 Crash Cove capture,
this rules out a black-screen or frozen-renderer interpretation. The run
remains rejected for deterministic state parity and is being allowed to
finish only for its complete failure and coverage map.

### Explicit fork, branch, PR, and default-branch audit

The user correctly observed that the implementation was not visible in the
GitHub default branch. A new read-only audit separated branch backup from
merge state:

```text
origin:
  https://github.com/chrissotraidis/ctrpad.git
upstream:
  https://github.com/CTR-tools/ctr-native.git

local HEAD:
  55d3b71c6da56a5fdd3e7e7f206c6673061e96e6
origin/codex/arm64-apple:
  55d3b71c6da56a5fdd3e7e7f206c6673061e96e6
GitHub fork branch API:
  55d3b71c6da56a5fdd3e7e7f206c6673061e96e6

origin/main:
  95417c723518407d6bfe3c81a37606294963efe2

fork PR:
  https://github.com/chrissotraidis/ctrpad/pull/1
  state OPEN
  draft true
  base main
  head codex/arm64-apple
  head OID 55d3b71c6da56a5fdd3e7e7f206c6673061e96e6
```

An initial unqualified `gh pr view 1` resolved PR number 1 against the
configured upstream repository and displayed the unrelated closed
`CTR-tools/ctr-native#1`. No write followed that ambiguous lookup. Repeating
the query with `--repo chrissotraidis/ctrpad`, then verifying the branch with
both `git ls-remote origin` and the GitHub branch API, established the values
above.

Therefore:

- the implementation and documentation are backed up on the fork branch;
- the draft PR points to that exact branch commit;
- `main` intentionally remains unchanged;
- nothing was merged or pushed to upstream; and
- any future PR command must name `--repo chrissotraidis/ctrpad` explicitly.

### Rejected i686 report completed

The source-`53ab70e966b2` diagnostic container subsequently exited normally.
Its report is finalized, not hung:

```text
/tmp/ctrpad-i686-full-v4-current-cbRPWn/debug/reports/20260730/ctr-190458
finalized=1
recording_status=finalized
replay_version=4
frame_count=24232
checkpoint_count=81

input.ctrreplay
  size 10662228
  SHA-256 6dd0217f04fe26d19f6aafd6bfde91121486708f63f725218482a0faaf0c8a8c
state.ctrstates
  size 357831140
  SHA-256 db6f8d0b6f20189b5b2f63c11e4e42f5a8d0cdc6d8f04a202edc55dfacbadc5b
metadata.txt
  size 958
  SHA-256 085acfaf3431f1e856eec4682091e1e63b10b845451eaf7d847293bf7cd9b6cf
ctr-native.log
  size 8771
  SHA-256 d93214e1d3d1b275aef26a89eae09e54911e7530602bafa241baa28d874c909d
memcard.recording/slot0/BASCUS-94426-SLOTS
  size 6016
  SHA-256 6a01b0f5562ed7a279d8f8e51e3b1874ac39a6120f55db4fe3873288950619a3
```

The complete comparison against its matching pre-guard ARM64 reference
confirms that timing, pad snapshots, and VSync transport never changed. The
invalid AI restart-node result propagated through gameplay state:

```text
timing:      equal 24232, mismatch 0
rng:         equal 19066, mismatch 5166
drivers:     equal 19845, mismatch 4387
world:       equal 23618, mismatch 614
allocation:  equal 22929, mismatch 1303
root:        equal 14703, mismatch 9529
pads:        equal 24232, mismatch 0
vsync:       equal 24232, mismatch 0

drivers mismatch ranges:
  6780-6793
  6854-7011
  9825-13837
  17213-17414

root mismatch ranges:
  6780-6793
  6854-7011
  9825-13837
  16561-21405
  22158-22656
```

This finished map is preserved as negative evidence. It is not promoted into
the unchanged-playback verifier and does not weaken the frame-6,780 root
cause: the first mismatch remains only `drivers`/`root`, while timing, RNG,
world, allocation, pads, and VSync are still exact at that point.

### Late visual coverage review

The final targeted X11 captures also completed. Direct visual inspection
established:

```text
frame 10880: Aku mask held in the item HUD
frame 10910: Aku mask still held immediately before Circle input
frame 10930: mask-use visual effect active around the kart
frame 11020: mask-use visual effect remains active
frame 11040: mask-use visual effect remains active
frame 11060: mask-use visual effect remains active

frame 21320: paused active race with complete HUD and minimap
frame 21920: mode-selection menu and trophy model
frame 22220: Crash character/kart selection
frame 22380: name-entry keyboard and Save control
frame 22520: explicit SAVE COMPLETED message
frame 22820: later active race/item-effect scene
frame 23120: N. Sanity Beach track-selection menu
frame 23720: title/mode-selection menu
frame 24220: later active race with mask effect
```

The replay pad records independently contain Circle at frames
`10913-10918` and `11030-11035`, sustained Cross acceleration, both steering
directions, and L1/R1 powerslide ranges. The corrected ARM64 report logs the
retail successful-boost branch. This re-observes item acquisition/use,
steering/acceleration, powerslide/boost, save action, menu return, and later
gameplay; it is not merely inherited coverage text.

One historical coverage claim is not yet re-accepted. Crops of every
available 600-frame race capture from 7,200 through 21,000 all show
`LAP 1/3`, including the final paused race at 21,320. The old report's
`lap_advanced=pass` is therefore not copied to the corrected report without
new proof. Deterministic parity for this input can pass independently, but
full golden gameplay coverage still requires either an extracted state
transition that satisfies the documented lap-line requirement or a new
recording that visibly advances to lap 2.

### Corrected ARM64 two-process and mutation results

Both unchanged playback attempts against corrected report `ctr-170507`
finished all 24,232 frames with exit status 0 and no canonical divergence:

```text
attempt 1:
  raw checkpoint recorded 0x253d5e82
  restored-process checksum 0xb197ac13
  host sdata 0x1028ca318
  host gGT 0x1028d4930
  SHA-256 2d04482e1529e06d265d9eace98cc9d0e65688571a5263631a2a1009b2f874cf

attempt 2:
  raw checkpoint recorded 0x253d5e82
  restored-process checksum 0x31f2c4a1
  host sdata 0x1042fa318
  host gGT 0x104304930
  SHA-256 94a7f80cdae6e402cec6aa8aa1d30d10746b447fb28b189c3e65790ca3b77828
```

The different host addresses and raw restored bytes demonstrate independent
ASLR layouts. Their equal canonical trace demonstrates that process-local
pointer representation is excluded correctly.

The deliberate mutation process changed
`driver[0].posCurr.x` from `-165632` to `-165631` at the automatically
selected first active-race frame 1,711. It exited 2 at that exact frame and
reported `drivers mask=0x00000004` as the first canonical difference:

```text
/tmp/ctrpad-arm64-corrected-playback-mutated.log
SHA-256 0b0b0e6de80557c4f7112ac965f60c39555304154e84cc8a2ee151f4bef31df0
```

The corrected ARM64 report therefore passes the full two-process
determinism/mutation proof for this scenario.

### Corrected i686 full regeneration started

After the rejected container finalized, the pinned corrected i686 producer
started:

```text
/tmp/ctrpad-i686-vehlap-8IzCKm/debug/reports/20260730/ctr-223323
build_id=55d3b71c6da5
producer SHA-256:
e07be52d72e6a2f323587d9302467be366401485cecd29d86ae54846f973528c
seed:
build-macos-arm64/debug/reports/20260730/ctr-170507/input.ctrreplay
```

The disposable build now also carries a freshly queried package manifest,
`toolchain-packages.txt`, with SHA-256
`7533bb723143fa947795ded64dbde4c82ffb7bb902b246929a11ac725ec22cd6`.
At the latest comparison checkpoint its first 4,875 complete frames match
corrected ARM64 on timing, RNG, drivers, world, allocation, root, pad
snapshots, and VSync. The report remains in flight; full acceptance is not
claimed before all 24,232 frames and the i686 two-process/mutation verifier
finish.

### Former frame-6,780 boundary passed

The in-flight corrected i686 report crossed the exact first-failure boundary
from the rejected run. At 6,812 complete frames, the strict prefix comparer
reported:

```text
timing:      equal 6812, mismatch 0
rng:         equal 6812, mismatch 0
drivers:     equal 6812, mismatch 0
world:       equal 6812, mismatch 0
allocation:  equal 6812, mismatch 0
root:        equal 6812, mismatch 0
pads:        equal 6812, mismatch 0
vsync:       equal 6812, mismatch 0
```

This is the real optimized-i686 execution using the same `55d3b71c6da5`
source identity as the corrected ARM64 producer. It establishes that the
native restart-node ownership fix removes the former cross-width divergence
at frame 6,780. It does not substitute for the required final all-frame
comparison.

### Typed lap-coverage audit

The lap coverage concern was also checked against exact checkpoint state,
not only pixels. The exact corrected ARM64 producer restored representative
rolling checkpoints under LLDB and broke at
`NativeReplayScheduler_ObserveGameplayState`. Typed expressions read
`s_replayFrame`, `gGT->numLaps`, `driver[0]->lapIndex`, and
`driver[0]->checkpoint.currentIndex`.

```text
replay frame  lapIndex  checkpoint.currentIndex
1800          0         71
3000          0          3
3900          0          6
4500          0          6
6000          0          9
6900          0         16
8100          0         10
9000          0         18
12000         0         61
13500         0         40
15000         0         12
18000         0         45
21000         0         57
21300         0         57
24000         0          0
```

Every sample has `numLaps=3` and a live player driver. The checkpoint index
changes across real track progress and resets across the scripted
race/restart transitions, while `lapIndex` never leaves zero. Samples near
the end of every active-race segment were included specifically to avoid
mistaking an early-race sample for the complete trace.

The 14 retained LLDB logs are under:

```text
/tmp/ctrpad-lap-checkpoint-evidence-55d3
```

The SHA-256 of their sorted `shasum -a 256` manifest is:

```text
af6bcdc11bc6785823bc579e12eb40a56a5d3852dde925567b135070f869e1e2
```

Combined with the HUD captures, this rejects `lap_advanced=pass` for the
current input. A later coverage recording must visibly and structurally
reach `lapIndex >= 1`; this requirement is now explicit rather than silently
inherited.

## 2026-07-30 — macOS application bundle and direct screen inspection

**Starting branch checkpoint:** `codex/arm64-apple` at `270fd7e2b1b6`

### Why the bare executable was not enough

The exact parity producer is intentionally a bare Mach-O. It launches and
renders, but macOS application control did not list it as a controllable
application, so that route could not provide a repeatable window-inspection
workflow. An idle diagnostic from the bare executable was stopped with
SIGTERM only after this limitation was established; SDL finalized
`build-macos-arm64/debug/reports/20260730/ctr-181359` at 6,390 frames. That
report is not gameplay or parity evidence.

The parity producer remains unchanged. A separate bundle build was added so
GUI lifecycle work cannot silently change the exact producer used by the
cross-width gate.

### Bundle implementation

The implementation added:

- opt-in `CTR_NATIVE_MACOS_BUNDLE` CMake handling, leaving the default and
  `macos-arm64` parity target unchanged;
- a `macos-arm64-app` configure/build/test preset with a distinct
  `build-macos-arm64-app` directory and macOS 11.0 deployment floor;
- `platform/apple/Info.plist.in`, including
  `io.github.chrissotraidis.ctrpad` and SDL's parent base-directory policy;
- a post-link ad-hoc signature over the complete bundle; and
- `tools/run-macos-arm64-app.sh`, which validates the external raw-sector
  image and creates only the ignored
  `build-macos-arm64-app/assets/ctr-u.bin` symlink.

The retail image is not copied into the bundle or Git. The bundle file census
contains only:

```text
CTRPad.app/Contents/Info.plist
CTRPad.app/Contents/MacOS/CTRPad
CTRPad.app/Contents/_CodeSignature/CodeResources
```

### Rejected first bundle and correction

The first bundle linked successfully but retained the host SDK's accidental
`minos 26.0` floor. Its linker-applied executable signature also identified
the code as `CTRPad` and did not bind the finished `Info.plist`. That build
was rejected.

The app preset now sets `CMAKE_OSX_DEPLOYMENT_TARGET=11.0`, and CMake signs
the finished bundle in a post-build command. The replacement validation
reported:

```text
Mach-O 64-bit executable arm64
LC_BUILD_VERSION minos 11.0
CFBundleIdentifier io.github.chrissotraidis.ctrpad
LSMinimumSystemVersion 11.0
CodeDirectory flags=adhoc
Info.plist entries=15
codesign --verify --deep --strict: valid
CTest: 14/14 passed
```

`plutil -lint`, JSON preset parsing, launcher shell syntax, and
`git diff --check` also passed.

### Direct launch, rendering, and lifecycle evidence

The launcher found the user-supplied NTSC-U raw image outside the bundle.
Runtime diagnostics identified:

```text
Video adapter: Apple M2 by Apple
OpenGL version: 4.1 Metal - 90.5
GLSL version: 4.10
4-bit, 8-bit, 16-bit, and RGBA PSX shaders ready
VRAM pipelines ready
```

macOS application control then listed a running `CTRPad` application with
identifier `io.github.chrissotraidis.ctrpad` and captured its presented
window directly. The observed sequence included the Naughty Dog splash,
multiple textured 3D track scenes, karts, item crates, UI text, and the main
mode-selection menu. Geometry, HUD, and textures were coherent under the
native Apple GPU path. The retail-derived screenshots remain ignored local
evidence and were not added to Git.

Synthetic Computer Use key taps are shorter than the retail input snapshot
interval and were not reliable enough to accept gameplay keyboard mapping. A
burst reached the main menu, but individual Cross/D-pad pulses were not
repeatable. F10 is handled on key-up and did reliably request a clean report
stop. The report finalized at the current frame before Command-Q closed the
app:

```text
build-macos-arm64-app/debug/reports/20260730/ctr-184802
build_id=270fd7e2b1b6-dirty
frame_count=15803
checkpoint_count=53
finalized=1
input.ctrreplay:
  7ba4b80889f5001ca77ca0b63ae7c2887bb2db3d9f4c65b6ac2ed8c2b4bad97c
state.ctrstates:
  7c4a5dadac16e969d6e4170d0cd96cb4569b3526382ad78f1e17c31b271fdd54
metadata.txt:
  c460d9110d661bc2a2275124e3f4f969f9d6fcb3e8547f3ea0fc8a74bfa6a421
```

The `-dirty` label is explicit: this was structural/visual validation of the
uncommitted bundle change, not a clean-producer parity artifact. A clean
post-commit rebuild is required before this workflow is called reproducible
from the published source.

### Parallel parity status and decision

The corrected i686 report continued independently during bundle work. At
13,469 complete candidate frames, all eight required components still matched
the corrected ARM64 reference with zero mismatches:

```text
timing, rng, drivers, world, allocation, root, pads, vsync
```

The development bundle workflow is accepted for publication after a clean
rebuild. M6 itself remains open: the i686 run must finish all 24,232 frames,
the full two-process/mutation gate remains, real lap advancement is not
covered by the inherited input, and manual keyboard/controller play plus
save-across-relaunch still need direct acceptance.

### Clean published rebuild

The seven implementation/documentation files above were committed and pushed
to the draft PR branch as:

```text
630e556c15bf134d84f279d2a6b1aab0f78ee3ba
feat: add macOS ARM64 app bundle workflow
```

The app preset was configured and rebuilt again after that commit. This
forced `main.c` and SDL's revision object to pick up the clean source
identity. The resulting executable reported:

```text
CTR Native 0.1.0-beta.7.1 (630e556c15bf)
SHA-256:
b2fd4921940c10eb55bb063047e29e22b7380adeb4b2d5b032e28ba2b9e474a6
```

The clean rebuild again passed 14/14 tests, was a thin ARM64 Mach-O with
`minos 11.0`, and passed strict deep signature verification with the expected
bundle identifier and 15 bound plist entries.

Git and GitHub were queried independently after the push:

```text
local HEAD:                    630e556c15bf134d84f279d2a6b1aab0f78ee3ba
origin/codex/arm64-apple:      630e556c15bf134d84f279d2a6b1aab0f78ee3ba
draft PR #1 head:              630e556c15bf134d84f279d2a6b1aab0f78ee3ba
origin/main (unchanged):       95417c723518407d6bfe3c81a37606294963efe2
```

This is backup on the draft implementation branch, not a merge to `main`.

During the rebuild, the independent corrected i686 run advanced further. A
fresh prefix comparison covered 15,571 complete candidate frames with zero
mismatches on timing, RNG, drivers, world, allocation, root, pads, and VSync.

## 2026-07-30 — Finalized corrected cross-width rejection and typed lap audit

**Starting branch checkpoint:** `codex/arm64-apple` at
`9beead6b86d577872741a8934d0934e2c6c77f5e`

### GitHub and process-state audit

The user's observation that only the viability work was visible on `main` was
correct. Independent local, remote-tracking, and GitHub queries reported:

```text
local HEAD:
  9beead6b86d577872741a8934d0934e2c6c77f5e
origin/codex/arm64-apple:
  9beead6b86d577872741a8934d0934e2c6c77f5e
draft PR #1 head:
  9beead6b86d577872741a8934d0934e2c6c77f5e
origin/main:
  95417c723518407d6bfe3c81a37606294963efe2
```

PR #1 was open and draft, with base `main` and head
`codex/arm64-apple`. The implementation was therefore backed up on GitHub but
not merged. This is intentional while the mandatory parity gate is red; it is
not described as merged work.

The previously inspected `CTRReplayInspector.app` was closed through its
normal window control. A process census found no running CTRPad, replay
inspector, `qemu-i386`, or diagnostic game process. The disposable GDB
container remained available only for the frame-16,561 investigation.

### Corrected i686 report finalized

The corrected optimized-i686 producer finished the entire scenario:

```text
producer:
  /private/tmp/ctrpad-i686-vehlap-8IzCKm/ctr_native-corrected-full-producer-55d3b71c6da5
producer SHA-256:
  e07be52d72e6a2f323587d9302467be366401485cecd29d86ae54846f973528c
report:
  /private/tmp/ctrpad-i686-vehlap-8IzCKm/debug/reports/20260730/ctr-223323
finalized=1
frame_count=24232
checkpoint_count=81
```

The report files are:

```text
input.ctrreplay
  4bada362045669896cdbccf78239ff20cba3ca922531e0dc83eaf4167c9043cb
state.ctrstates
  c4a12812b3369ceab61891266e6a7e210a9292108f4261fdc9ec4a30cea4c17e
metadata.txt
  615b6ec9148e55a56ae9f0a1bb0b701ff2f92122d5913ac03592dadb9a527859
ctr-native.log
  81a5002f7d6f321c1289456f9d19beed5ad71981f59f8277ba3c140799a33ae4
```

This corrected run is not a parity pass. The definitive full comparison
against ARM64 report `ctr-170507` exited 2:

```text
timing:      equal=24232 mismatched=0    ranges=none
rng:         equal=19387 mismatched=4845 ranges=16561-21405
drivers:     equal=24030 mismatched=202  ranges=17213-17414
world:       equal=23713 mismatched=519  ranges=16561-16580,22158-22656
allocation:  equal=23679 mismatched=553  ranges=16561-16580,17213-17227,17231-17275,22158-22630
root:        equal=18888 mismatched=5344 ranges=16561-21405,22158-22656
pads:        equal=24232 mismatched=0    ranges=none
vsync:       equal=24232 mismatched=0    ranges=none
required components mismatched:
  rng,drivers,world,allocation,root
```

The earlier 15,571-frame prefix result was accurate at the time it was
recorded, but it is now superseded by this finalized rejection.

### Exact first-frame bound

Raw end-of-frame replay fields show both architectures at:

```text
frame 16560:
  deadcoed0=0x1b9ef97b
  deadcoed1=0x533fcdb6
```

At frame 16,561:

```text
ARM64:
  deadcoed0=0xa8c39902
  deadcoed1=0x7d5e2621
i686:
  deadcoed0=0xb5fd73cb
  deadcoed1=0xdeb84781
```

Running the exact `RngDeadCoed` recurrence from the common frame-16,560 state
reaches ARM64 after 33 calls and i686 after 38 calls. This proves a five-call
i686 excess in one frame; it is stronger than merely observing two different
hashes.

The exact accepted ARM64 producer was restored from checkpoint 55 and traced
under LLDB. Frame 16,561 made 12 particle initializations:

```text
one wall spark:
  3 deadcoed calls
six ordinary exhaust particles:
  5 calls each
five potion-shatter particles:
  0 calls
total:
  33 calls
```

The exhaust particles were emitted in pairs for three drivers. The potion
path was `RB_GenericMine_ThDestroy` to `RB_Explosion_InitPotion`. Neither
`VehEmitter_Sparks_Ground` nor `RB_FlameJet_Particles` executed on ARM64.

An early theory that “five ground sparks” explained the delta was discarded:
the source constant is ten, not five, and the breakpoint never fired. A
complete flame-jet emission was also statically counted at 14 calls, not
five, and its breakpoint did not fire. The honest remaining bound is one
additional five-field particle-RNG pattern on i686. The exact emitter is not
yet proven.

Ignored local LLDB evidence is retained at:

```text
build-macos-arm64/debug/arm-frame-16561-callers-v2.log
build-macos-arm64/debug/arm-frame-16561-particles.log
```

### Rejected i686 tracing attempts

The following routes were attempted and rejected in order:

1. Native GDB inside the amd64 container failed to control the emulated i386
   process with `Couldn't get CS register: Input/output error`.
2. A diagnostic binary using return-address introspection exited 139 before
   game initialization.
3. Direct QEMU remote debugging encountered one emulation-only address-base
   artifact during checkpoint restore. A debugger-only adjustment made that
   slot internally consistent, but the nested validation/bootstrap path was
   too slow to reach the target frame. It produced no accepted trace and
   modified no report or retail file.
4. A simpler frame-gated diagnostic around `Particle_Init` and the candidate
   emitters was compiled with SHA-256
   `1e5c3b25a1a36c5d16f94f3486698930db7b18aa94b88c100e6e9b8286fd9cfd`.
   Under normal outer Docker emulation, it printed the startup, base, and
   asset paths, then received target SIGSEGV and exited 139 before
   initialization. The untouched producer survived the same command.

All diagnostic source patches were reverted immediately after their binaries
were built. No output from a crashing or header-bypassed diagnostic is
acceptance evidence.

### Rejected lap-extension trial A

The first lap extension retained the inherited input through frame 21,248,
held Cross+Right through frame 21,320, and then held Cross to frame 24,231.
Its seed is ignored local evidence:

```text
build-macos-arm64/debug/reports/20260730/ctr-170507/input-lap-extension-a.ctrreplay
SHA-256:
b5db8c1903e06026052efde33ccbb747a8bb9f6e35677017e7dd338a805544ec
```

The clean bundled producer finalized:

```text
build-macos-arm64-app/debug/reports/20260730/ctr-192711
finalized=1
frame_count=24232
checkpoint_count=81
```

Its files hash to:

```text
input.ctrreplay
  faaf50b56a7c410e031f7dce07722c7b89a5f4dc2918aafc669b6eb872e96090
state.ctrstates
  f75e1a1afa8dbcf52b6e60c4d5efc9b36343ea1f67c4e6b119730730177d3729
metadata.txt
  6df42d2efe5886b1cc7983acd0ae0ef9456377fb855a8373123baa3a3fa0fb35
ctr-native.log
  6851a3082cfe01bc169661c8dc11bd3024910ca1947efd851e7a59d13ab383a8
```

All 24,232 recorded pad snapshots equal the extension seed. A raw typed
checkpoint parser found continued late track progress but `maxLap=0`; trial A
is rejected.

### Reproducible lap inspector

The raw audit was promoted to
`tools/inspect-replay-lap-coverage.mjs`. The tool:

- verifies the state-container header, every payload boundary, and every FNV
  checksum;
- supports checkpoint v2/i686 and v3/i686/ARM64 layouts;
- resolves the player driver through recorded address-owner ranges instead of
  treating recorded host addresses as live pointers; and
- reports every structural `lapIndex` and
  `checkpoint.currentIndex`, plus `maxLap`.

It was syntax-checked and run against four real reports. Current ARM64 report
`ctr-170507`, corrected i686 report `ctr-223323`, and lap-extension report
`ctr-192711` all report:

```text
activeRecords=80
maxLap=0
maxCheckpoint=77
lapAdvanced=no
```

The historical version-2 report
`build-linux-i686-baseline/debug/reports/20260729/ctr-225420` reports:

```text
frame 21000 lap 0 checkpoint 72
frame 21300 lap 1 checkpoint 1
frame 21600 lap 0 checkpoint 0
maxLap=1
lapAdvanced=yes
```

At frame 21,300 the historical and current inherited inputs share player
zero's button mask and analog axes, but not the complete transport record or
the preceding trajectory. The old lap advance is real historical evidence,
not a substitute for a current version-4 lap.

The complete finalized result, exact comparison command, source citations,
and next steps are preserved in
`docs/parity/2026-07-30-full-cross-width-result.md`. The roadmap and parity
index were updated at the same checkpoint. M6 remains open, and iOS remains
gated.

### GitHub publication checkpoint

The inspector, full rejection report, roadmap correction, parity-index
correction, and this chronological record were committed and pushed as:

```text
57cfd653319b93d9f9cf50a40485c608ee885b88
test: archive full cross-width parity rejection
```

Post-push verification independently reported:

```text
local HEAD:
  57cfd653319b93d9f9cf50a40485c608ee885b88
origin/codex/arm64-apple:
  57cfd653319b93d9f9cf50a40485c608ee885b88
draft PR #1 head:
  57cfd653319b93d9f9cf50a40485c608ee885b88
PR state:
  OPEN, draft
base:
  main
```

This publishes and backs up the work on the draft implementation branch. It
does not merge the known parity failure to `main`.

## 2026-07-30 — Frame-16,561 potion-emitter root cause and typed-layout correction

### GitHub state clarified before the fix

The user correctly observed that implementation work was not present on
GitHub's `main` branch. A read-only audit at the start of this continuation
showed:

```text
local HEAD:
  185e9f9b9245589f473149a3299e6834da9f6029
origin/codex/arm64-apple:
  185e9f9b9245589f473149a3299e6834da9f6029
draft PR #1 head:
  185e9f9b9245589f473149a3299e6834da9f6029
origin/main:
  95417c723518407d6bfe3c81a37606294963efe2
```

Thus the implementation and evidence were backed up on the draft PR branch,
but not merged. This distinction is intentional while M6 is red and is now
stated explicitly in status updates. No merge was performed.

### Final rejected instrumentation route

The earlier frame-gated i686 source instrumentation was rebuilt once through
the same CMake unity-build path as the accepted producer, ruling out the
manual object-build method as the sole cause of its failure. The diagnostic
copy was:

```text
/private/tmp/ctrpad-i686-frame16561-NgN10e/
  ctr_native-diag-cmake-particle-16561
SHA-256:
  987c35ef9a5a5b6ef0bba7d3a243a8f82347b6e47f43de83cb2a7002005103cb
```

A fresh boot survived for 60 seconds, but a checkpoint-replay launch exited
139 during validation before frame 16,561. It was rejected. The temporary
logging patch was immediately reverted, and no output from this binary was
used as acceptance evidence.

### Read-only accepted-producer probe

The next route modified neither executable nor replay. Disposable container
`ctrpad-i686-proc-probe` ran the exact accepted producer:

```text
producer:
  /diag/ctr_native-corrected-full-producer-55d3b71c6da5
SHA-256:
  e07be52d72e6a2f323587d9302467be366401485cecd29d86ae54846f973528c
launch:
  --replay /diag/debug/reports/20260730/ctr-223323/input.ctrreplay
  --replay-start-checkpoint 55
```

The host retail BIN was mounted read-only. The container used
`--cap-add SYS_PTRACE --security-opt seccomp=unconfined` so a helper could
read `/proc/66/mem` while `qemu-i386` executed the accepted process. This did
not patch code, change report headers, or bypass executable/checkpoint
identity. The loaded ELF base was `0xb586b000`; relative to the linked guest
base `0x40000000`, the runtime relocation delta was `0x7586b000`.

A polling helper sent `SIGSTOP` at the target replay counter. A second
read/resume/stop step left the process stopped at `s_replayFrame=16562`,
which is the end of recorded frame 16,561. The exact state was:

```text
start of frame 16561:
  RNG common: 1b9ef97b,533fcdb6
  live particles: 34
  free particles: 94

end of frame 16561:
  i686 RNG: b5fd73cb,deb84781
  live particles: 39
  free particles: 89
```

For the ARM64 side, LLDB stopped the exact accepted producer at
`NativeReplayScheduler_EndFrame` with `s_replayFrame == 16561`:

```text
ARM64 RNG:       a8c39902,7d5e2621
live particles:  34
free particles:  94
ordinary head:   0x1006017d8
particle pool:   0x1005ffe50
LP64 item size:  152
```

The complete 19,456-byte ARM64 particle pool was retained as ignored local
evidence at `/private/tmp/ctrpad-arm-frame16561-pool.bin`. The LLDB command
and transcript are:

```text
build-macos-arm64/debug/arm-frame-16561-pool.lldb
build-macos-arm64/debug/arm-frame-16561-pool.log
```

### Exact list comparison

Both ordinary-particle linked lists were decoded using their native struct
layout and pool bounds. The comparison found:

```text
ARM64 active ordinary particles: 34
i686 active ordinary particles:  39
i686 records 5..38:              match ARM64 records 0..33
i686-only list-head records:     5
```

Every i686-only record was a potion-shatter particle:

```text
framesLeftInLife: 19
flagsSetColor:    0x00a1
flagsAxisWord:    0x000003a7
funcPtr:          Particle_FuncPtr_PotionShatter
modelID union:    0x45
```

Their positions differed as expected, but their emitter-derived scalar fields
and axis schema were the same. Because the record begins with a lifespan of
20 and is inspected after its first update, `framesLeftInLife=19` explains
why world and allocation differ for exactly 20 frames, 16,561 through
16,580. Each record also randomizes Y velocity with seed 400, requiring one
`MixRNG_Particles` call. Five correct i686 records therefore explain the
five-call RNG delta exactly.

This supersedes the earlier honest statement that the exact i686 emitter was
not yet observed. That earlier statement remains in the preceding
chronological entry because it accurately records what was known then.

### Source-level root cause

`game/231/RB_Explosion.c` encoded `s_potionShatterEmitter` as 81 raw `u32`
words and cast it to `struct ParticleEmitter *`. The words are nine retail
records of 0x24 bytes each. That is correct for ILP32, where:

```text
InitTypes offset: 0x04
data offset:      0x14
record size:      0x24
```

On LP64 the pointer inside `FuncInit` changes the native layout:

```text
InitTypes offset: 0x08
data offset:      0x20
record size:      0x30
```

ARM64 therefore interpreted valid retail bytes at invalid native offsets and
advanced between entries with the wrong stride. Its five potion
`Particle_Init` calls occurred, as the accepted trace had shown, but did not
leave the five valid 20-frame particles that i686 created.

### Correction

The raw runtime table was replaced with a typed
`static const struct ParticleEmitter[]` containing:

```text
entry 0: function init, color flags 0x00a1, lifespan 20, ordinary type
entry 1: X start 1
entry 2: Z start 1
entry 3: Y start 1, velocity 3800, acceleration -280,
         randomized velocity seed 400
entry 4: scale X start 0x1000
entry 5: red start 1
entry 6: green start 0xc800
entry 7: blue start 1
entry 8: zero terminator
```

`Particle_Init` and its public declaration now take
`const struct ParticleEmitter *`, and `RB_Explosion_InitPotion` passes the
typed table without a cast.

A new CTest entry point,
`--self-test-potion-emitter-layout`, validates all semantic records on both
pointer widths. On 32-bit it additionally compares the entire typed table
against the original 81 `u32` words. This proves the correction preserves the
retail ILP32 representation while allowing the compiler to form the correct
LP64 offsets and stride.

### Verification

The corrected source passed:

```text
macOS ARM64 Release:
  build-macos-arm64/ctr_native
  15/15 CTests
  potion test reports pointer-size=8
  SHA-256 b68c72b7252acd35c6f657b4f7f14bf96a9c8d90b39713e383c2dae620e33ddf

macOS ARM64 ASan/UBSan:
  /tmp/ctrpad-macos-arm64-asan-vehlap-OTDxVr/ctr_native
  ASAN_OPTIONS=symbolize=0:abort_on_error=1:detect_leaks=0
  UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1
  15/15 CTests
  SHA-256 dd9450b111a5e54e21b8b671a50f771cda756486093c0a62ae7828da136c4c9c

Linux optimized i686:
  /private/tmp/ctrpad-i686-vehlap-8IzCKm/ctr_native
  ELF 32-bit LSB PIE, Intel 80386
  Build ID 07c4c9a68db6db738e1531696bc6fac7b0ee88da
  15/15 CTests, including exact 81-word comparison
  SHA-256 28afbee11aaea30afa4a4a4b6240c988f1a8baf460bf27a342f803ffdb7faa0a

git diff --check:
  passed
```

The macOS builds repeated only pre-existing compiler warnings. The i686 build
repeated the established two format-security and two optimized
maybe-uninitialized warnings. No new warning was introduced by the typed
table.

This verifies the source correction and cross-width layout guard. It does not
retroactively make reports `ctr-170507` and `ctr-223323` match. M6 remains
open until clean committed producers regenerate the full trace and all eight
required components match for all 24,232 frames.

### Typed-emitter GitHub publication checkpoint

The correction, regression test, roadmap, parity report, and chronological
record were committed and pushed as:

```text
77d230a0f331675f7d05f79fbe495a61987493a7
fix: preserve potion emitter layout across widths
```

The first `gh pr view 1` audit omitted `--repo`. Because this checkout has
both `origin` and `upstream`, GitHub CLI resolved the number against
`CTR-tools/ctr-native` and returned that project's unrelated closed PR #1.
That result was rejected immediately; it says nothing about CTRPad.

The corrected repository-pinned audit used:

```sh
gh pr view 1 --repo chrissotraidis/ctrpad \
  --json number,state,isDraft,baseRefName,headRefName,headRefOid,url,title
```

It and independent Git refs agreed:

```text
local HEAD:
  77d230a0f331675f7d05f79fbe495a61987493a7
origin/codex/arm64-apple:
  77d230a0f331675f7d05f79fbe495a61987493a7
draft PR #1 head:
  77d230a0f331675f7d05f79fbe495a61987493a7
PR:
  https://github.com/chrissotraidis/ctrpad/pull/1
state/base/head:
  OPEN draft / main / codex/arm64-apple
origin/main:
  95417c723518407d6bfe3c81a37606294963efe2
```

The work is therefore backed up on GitHub and reviewable in the draft PR. It
is still deliberately not merged into `main` while full post-fix parity
regeneration remains pending.

## 2026-07-30 — Clean potion replay and overlay-233 cutscene-emitter correction

### Clean ARM64 regeneration after the first emitter fix

The clean committed ARM64 producer built from `7af15bea2157` was retained as:

```text
build-macos-arm64/ctr_native-potion-fix-producer-7af15bea2157
SHA-256:
d971fce784b8ac0470c5d1603d372a4f2a7f96f7625f861c2868f6328631823a
```

It passed 15/15 media-free tests and regenerated the full inherited input as
`build-macos-arm64/debug/reports/20260730/ctr-211646`. The report finalized
24,232 frames and 81 checkpoints with exit code zero. Its files hash to:

```text
input.ctrreplay
  cc0ac47cffcc0f9944ce75dafcecd0dfab1ae32faeabc239513a2320bfc61e92
state.ctrstates
  33e8accaa5dd59148bc4230a42d3835c21921a07dee9c98664d946067f744931
metadata.txt
  0b0b9eb56fb6f5a8c02056f1cd7e0df8c2d438ccff714ee3238686ecc8dac73f
ctr-native.log
  0a823bfc4ab99938e9bb44f5372b38990c91b0f9c0ed8a1a4718e9af8a79f88a
```

The earlier i686 report `ctr-223323` is stale relative to that fix, so the
comparison is diagnostic rather than M6 acceptance. It nevertheless proves
the correction's boundary:

```text
timing/rng/drivers/pads/vsync: all 24,232 frames match
world/root:                    499 mismatches, frames 22,158-22,656
allocation:                    473 mismatches, frames 22,158-22,630
```

Every old frame-16,561 RNG/world/allocation range and the frame-17,213 driver
range disappeared. The potion-emitter correction is therefore verified
end-to-end. A separate later defect remained.

### Checkpoint-22,200 pool localization

The canonical states at frame 22,200 were decoded using the recorded pointer
width and native field offsets. The i686 state had `numParticles=10` and
22/32 free particle slots. ARM64 had `numParticles=0` and 32/32 free slots.
Every other pool count matched:

```text
thread 27/48
instance free 35, taken 12, maximum 64
small 16/25
medium 0/8
large 0/4
oscillator 32/32
rain 2/2
```

Decoding the ten i686 ordinary-list records found lifetimes 14 through 5,
color flags `0x00a3`, and a one-new-particle-per-frame axis sequence. Their
position, velocity, acceleration, scale, and color fields match overlay-233
group 46 at retail address comment `0x5818`. `R233.particleConfigs[6]` points
to that group with count 1 and model delta 2.

### Second source-level root cause

Seven group headers in `game/233/R233.c` were declared through
`InitTypes.AxisInit` even though `initOffset=12` makes them function
initializers. This reproduces retail bytes by accident on ILP32:

```text
union offset:     4
pointer width:    4
color/lifespan:   offsets 8/10
```

On LP64, the union moves to offset 8 and the function pointer widens to eight
bytes. The axis initializer places color/lifespan at offsets 12/14 inside the
pointer and leaves the real fields at offsets 16/18 zero. The observed group
therefore created no persistent ARM64 particles.

The seven headers at indices `0,10,20,28,38,46,54` now initialize
`FuncInit` directly with null function pointer, named color flags, and named
lifespan. No axis records or config values changed.

### Cross-width regression gate

`--self-test-cutscene-particle-emitter-layout` verifies:

- all seven function records, color flags, lifespans, types, and fillers;
- all seven terminators;
- every config emitter pointer, icon group, frame offset, count, flags, and
  signed model delta;
- the entire nine-word retail representation of each corrected function
  record on i686.

Accepted source-level gates:

```text
macOS ARM64 Release:
  16/16 CTests
  SHA-256 021821a4cddafecd05f9c1ba5fe79db87296618ea70843de9b728027712aeb19

macOS ARM64 ASan/UBSan:
  ASAN_OPTIONS=symbolize=0:abort_on_error=1:detect_leaks=0
  UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1
  16/16 CTests
  SHA-256 0ac33ae7f62b11b842390f4bf43a33a5e70ac655246430eee1952055d1417c57

Linux optimized i686:
  ELF 32-bit LSB PIE, Intel 80386
  Build ID 05a883c8d6efcef3db117611b5ad43ee5172bda5
  16/16 CTests, including exact retail-record comparisons
  SHA-256 9a5af521e2dcf1c97413dbbb1b8fd805a18699b50bbd6acdc2220e02fde13820

git diff --check:
  passed
```

The normal ARM64 build repeated 32 established warnings, the sanitizer build
59 established warnings, and i686 repeated the established two
format-security plus two maybe-uninitialized warnings. The new typed
initializers introduced no warning.

### Concurrent i686 continuity run

An immutable copy of committed producer `7af15bea2157` is independently
regenerating the full i686 report under ordinary Xvfb/llvmpipe execution:

```text
container:
  ctrpad-i686-potion-full
run directory:
  /private/tmp/ctrpad-i686-potion-run-KMjNWQ
report:
  debug/reports/20260731/ctr-023139
documentation checkpoint:
  finalized=0 frame_count=3000 checkpoint_count=11
```

The source rebuild cannot alter that copied executable. This continuity run
is retained to complete the clean first-fix pair, but it predates the
overlay-233 correction and cannot accept the new source. M6 remains open
pending fresh committed producers and a full all-eight-component match after
both emitter fixes.

## 2026-07-30 — Published overlay fix and finalized ARM64 all-component match

### GitHub publication and branch state

The overlay correction, sixteenth test, roadmap, parity report, and preceding
chronological entry were committed and pushed as:

```text
eee2a8df5b9605d27c7b20e943bba76174a4f6fc
fix: preserve cutscene emitter layout across widths
```

Immediately after the push, `gh pr view` briefly returned the previous draft
PR head. That response was not accepted. Repository-pinned REST reads and Git
refs then agreed:

```text
local HEAD:
  eee2a8df5b9605d27c7b20e943bba76174a4f6fc
origin/codex/arm64-apple:
  eee2a8df5b9605d27c7b20e943bba76174a4f6fc
draft PR #1 head:
  eee2a8df5b9605d27c7b20e943bba76174a4f6fc
PR:
  https://github.com/chrissotraidis/ctrpad/pull/1
state/base/head:
  OPEN draft / main / codex/arm64-apple
origin/main:
  95417c723518407d6bfe3c81a37606294963efe2
```

The implementation—not only viability documentation—is backed up on GitHub.
It remains deliberately unmerged while the formal parity gate is open.

### Clean committed producers

Both clean builds report `eee2a8df5b96` and pass 16/16 tests:

```text
ARM64:
  build-macos-arm64/ctr_native-cutscene-fix-producer-eee2a8df5b96
  Mach-O 64-bit executable arm64
  SHA-256 fa9a7d46292ab09b143e0e2b514317daa251af48f6341dc437b1f5d81ded3961

optimized i686:
  /private/tmp/ctrpad-i686-vehlap-8IzCKm/ctr_native-cutscene-fix-producer-eee2a8df5b96
  ELF 32-bit LSB PIE, Intel 80386
  Build ID 14387ee999252f9fb16177bb0fdfb76e2c099f4b
  SHA-256 d2e6f06023ccaedae689f11b36b33e005cb30d7bbc70d2a5e3e036f57b276c8e
```

The complete tree sweep for other particle function headers found every
other `initOffset=12` site already using `InitTypes.FuncInit`. No additional
wrong-union header remains in the particle tables.

### Superseded i686 continuity run

The first-fix-only i686 producer reached frame 4,566. It was then stopped so
its emulated renderer's CPU budget could be reassigned to the clean
same-commit report that can satisfy the gate. The files were retained:

```text
run:
  /private/tmp/ctrpad-i686-potion-run-KMjNWQ
report:
  debug/reports/20260731/ctr-023139
Docker exit:
  137
frames/checkpoints:
  4566 / 16
```

The recorder's signal/shutdown path changed metadata to `finalized=1`, but
that flag means only that headers were closed. The report contains 4,566 of
the seeded 24,232 frames, has no normal process exit record, and came from
source before the overlay correction. It is explicitly rejected as partial
evidence.

### Finalized clean ARM64 report

Clean producer `eee2a8df5b96` regenerated the entire inherited input:

```text
report:
  build-macos-arm64/debug/reports/20260730/ctr-215303
frames/checkpoints/finalized/exit:
  24232 / 81 / 1 / 0
```

The report files hash to:

```text
input.ctrreplay
  dfd06c677f29d9c2155cb01cf00fd958dddfc06127d9b6c67937651039029e09
state.ctrstates
  a0ea4a59e7e26716e99249430aed02bd45794a096c4b929dcfd323e8ecd03632
metadata.txt
  9ed1ba92c55da00135e5329bce682b2d82a8530a4daab3607bdd6b10eb0d9e4d
ctr-native.log
  bea2c87b0c6694f139561ce5f2ba94eba7d46a6daa54b8bce17b39aba3d0de1a
```

A live prefix comparison first crossed the entire former frame
22,158-through-22,656 mismatch range at 23,256/24,232 with all components
equal. After finalization, the complete comparison against immutable i686
report `ctr-223323` was:

```text
timing:      equal=24232 mismatched=0 ranges=none
rng:         equal=24232 mismatched=0 ranges=none
drivers:     equal=24232 mismatched=0 ranges=none
world:       equal=24232 mismatched=0 ranges=none
allocation:  equal=24232 mismatched=0 ranges=none
root:        equal=24232 mismatched=0 ranges=none
pads:        equal=24232 mismatched=0 ranges=none
vsync:       equal=24232 mismatched=0 ranges=none
```

This older i686 report is not the formal same-commit candidate, but the
i686-only byte comparisons prove both new typed tables preserve its retail
records exactly. The result proves both ARM64 corrections remove their full
observed mismatch ranges without changing i686 behavior.

### Clean same-commit i686 run

The clean i686 producer is recording under ordinary Xvfb/llvmpipe execution:

```text
container:
  ctrpad-i686-cutscene-full
run directory:
  /private/tmp/ctrpad-i686-cutscene-run-4hjAQW
report:
  debug/reports/20260731/ctr-025812
documentation checkpoint:
  finalized=0 frame_count=1800 checkpoint_count=7
```

The first 1,879 complete frame records match clean ARM64 report `ctr-215303`
on all eight required components. The approximately 2.5-frame-per-second
rate is the established i386 plus llvmpipe emulation cost, not a stall.

M6 remains open until this same-commit report finalizes all 24,232 frames and
passes the complete comparison. The finalized ARM64 result is a major
correction checkpoint, not permission to skip the remaining clean-pair gate.

## 2026-07-30 — Reproduced current-format lap coverage

### Why the first extension was not enough

The inherited version-4 input and steering extension A both reached
`maxCheckpoint=77` but `maxLap=0`. To keep this from becoming an informal
"the kart looked close" judgment, `tools/extend-replay-input.mjs` was added
in commit `0e524c0c0`. It validates replay layout, record order, pad and
record checksums, packet bounds, VSync totals, and every generated output
record. It refuses to overwrite output and states in its source that copied
state digests are not acceptance evidence.

Trial B extended the automation to 36,000 frames, using trial A as the pad
source and the clean current version-4 report as the timing source:

```text
seed:
  build-macos-arm64/debug/reports/20260730/ctr-215303/input-lap-extension-b.ctrreplay
SHA-256:
  2c98ea3c327770d3d90ba7eed09fd52be13f2b703b344c9cec5102091ac674b6
fresh report:
  build-macos-arm64/debug/reports/20260730/ctr-221412
```

The structural inspector showed lap 0/checkpoint 56 at frame 21,300 and the
same values through frame 28,800. After 7,550 frames without progress, the
process was stopped at frame 28,850. It had 97 recorded checkpoints and had
previously reached checkpoint 77, but never advanced a lap. Although the
shutdown path finalized the file headers, the metadata also records a
36,000-frame input seed and only 28,850 output frames. The run is explicitly
rejected as interrupted and incomplete. Its four retained report hashes are
recorded in `docs/parity/2026-07-30-full-cross-width-result.md`.

### Isolating input from timing

The older finalized i686 version-2 report `ctr-225420` reaches `lapIndex=1`
at frame 21,300. A byte audit found that the historical and inherited current
inputs have identical game-visible PSX pad transport on all 24,232 frames:
for all four pads, bytes 0 through 8 contain the same status, ID, buttons,
analog axes, and connected flag. Only the three reserved bytes differ. In
contrast, per-frame VBlank totals differ on 412 frames, starting at frame
zero.

This rejected the controller-script hypothesis and identified timing
transport as the trajectory-changing variable. Replay version 2 has the
historical in-frame VSync packets and elapsed time, but predates version 4's
pre-frame boundary marker.

Commit `a269843a2` made the extension tool promote validated version-2 and
version-3 sources. It retains the source records and in-frame timing after
frame zero, changes the output header to version 4, and requires an independent
complete version-4 bootstrap. It copies frame zero's entire VSync block and
elapsed time from that bootstrap, recomputes the record checksum, and validates
the entire output under version-4 rules. Missing option values are rejected
rather than being interpreted as paths.

The exact inputs were historical report `ctr-225420` and clean current-format
report `ctr-215303`. The resulting promoted seed is:

```text
build-linux-i686-baseline/debug/reports/20260729/ctr-225420/input-promoted-v4.ctrreplay
version/frames:
  4 / 24232
SHA-256:
  80522b7675089f4bddd6c3d04e09a86c41eb5524e6661b5e65f63d886405fcea
```

The full derivation command is recorded in the parity report.

### Fresh current-build result

Clean ARM64 producer `eee2a8df5b96` recorded from the promoted seed. This was
a real replay-seeded execution, not a header rewrite of the historical output:

```text
report:
  build-macos-arm64/debug/reports/20260730/ctr-223221
frames/checkpoints/finalized/exit:
  24232 / 81 / 1 / 0
```

The typed LP64 checkpoint inspector reports 80 active records,
`maxCheckpoint=77`, and `maxLap=1`. The transition is structural:

```text
frame 21000 lap 0 checkpoint 72
frame 21300 lap 1 checkpoint 1
frame 21600 lap 0 checkpoint 0
```

The state reset after the race does not invalidate the captured lap-advance
checkpoint. Hashes for the accepted input, state, metadata, and log are
recorded in the parity report.

### Making the transport proof rerunnable

The initial semantic transport comparison was a disposable analysis command.
That was insufficient for the requested historical record, so it was replaced
by checked-in `tools/compare-replay-transport-semantics.mjs`. The tool
validates both complete replay files and compares:

- the nine game-visible bytes of every pad snapshot;
- end-of-frame `elapsedTimeMS`;
- VBlank totals;
- RLE-expanded pre-frame VSync sequences; and
- RLE-expanded in-frame VSync sequences.

Comparing the promoted seed with fresh report `ctr-223221` yields zero
mismatches in all five semantic components across all 24,232 frames. The raw
VBlank storage block differs on 23,836 frames because the fresh recorder
run-length-encodes repeated equal packets; after expansion, both pre-frame
and in-frame sequences match on every frame.

This closes the missing current-build version-4 lap-coverage task. It does
not close the formal cross-width gate: clean same-commit i686 report
`ctr-025812` still has to finalize and match all eight state/transport
components before the two-process and deliberate-mutation gates run.

At the final documentation check, that container was still healthy and had
advanced from frame 9,300/checkpoint 32 to frame 9,900/checkpoint 34:

```text
finalized=0
build_id=eee2a8df5b96
frame_count=9900
checkpoint_count=34
```

This is continued progress under slow i386 plus llvmpipe emulation, not a
stalled game.

### GitHub/main distinction

The implementation is published on `codex/arm64-apple` through draft PR #1,
but it has not been merged to `main`. At this checkpoint:

```text
origin/main:
  95417c723518407d6bfe3c81a37606294963efe2
origin/codex/arm64-apple before this documentation commit:
  a269843a2ded81c355f19c2948ae650859c0dddd
PR:
  https://github.com/chrissotraidis/ctrpad/pull/1
state:
  OPEN / draft
```

That distinction is intentional while M6 is open, but it must always be
reported plainly: "backed up on GitHub" does not mean "merged to main."

## 2026-07-30 — Prepared the exact i686 process/mutation gate

Reviewing `tools/verify-linux-i686-golden-replay.sh` before the long report
finished found three assumptions that did not fit the immutable clean run:

1. the executable had to be named `ctr_native`;
2. `toolchain-packages.txt` had to be in the run directory; and
3. `coverage.txt` had to be attached to the same report.

The clean producer is intentionally named
`ctr_native-cutscene-fix-producer-eee2a8df5b96`, its toolchain manifest
remains in the disposable build tree, and report `ctr-025812` is the formal
same-commit parity trajectory rather than the separate structural lap report.
Copying or inventing a coverage form would blur those identities.

The verifier now accepts:

```text
CTRPAD_I686_BINARY
CTRPAD_TOOLCHAIN_PACKAGES
CTRPAD_REQUIRE_COVERAGE=0|1
```

The defaults are unchanged: ordinary golden verification still selects
`ctr_native`, requires the build-local manifest, and requires `coverage.txt`
with all eight pass keys. An explicitly selected binary must remain under the
selected build/run tree so the container mount is unambiguous. The external
manifest is read and hashed into `environment.txt`.

`CTRPAD_REQUIRE_COVERAGE=0` is narrowly named and documented as
process-determinism/mutation verification. It still requires:

- a clean tracked worktree;
- exact full source commit and embedded build ID;
- finalized version-4 metadata with 24,232 frames and 81 checkpoints;
- the complete input, checkpoint, log, and two memcard directories;
- a recorded powerslide event;
- two full unchanged processes with different host-address samples;
- different raw restored checkpoint bytes but identical canonical state;
- an automatically selected active-driver mutation;
- exit status 2 and `drivers` as the first canonical difference; and
- a complete evidence hash manifest.

Only the manually maintained coverage form is omitted. The accepted,
current-build structural lap evidence remains report `ctr-223221` and must be
cited separately. This mode must not be described as a full golden-coverage
pass.

Once `ctr-025812` finalizes and the all-eight-component comparison passes,
the prepared invocation is:

```sh
CTRPAD_REQUIRE_COVERAGE=0 \
CTRPAD_I686_BUILD_DIR=/private/tmp/ctrpad-i686-cutscene-run-4hjAQW \
CTRPAD_I686_BINARY=/private/tmp/ctrpad-i686-cutscene-run-4hjAQW/ctr_native-cutscene-fix-producer-eee2a8df5b96 \
CTRPAD_TOOLCHAIN_PACKAGES=/private/tmp/ctrpad-i686-vehlap-8IzCKm/toolchain-packages.txt \
CTRPAD_EXPECTED_SOURCE_COMMIT=eee2a8df5b9605d27c7b20e943bba76174a4f6fc \
CTRPAD_DISC_IMAGE='/Users/chrissotraidis/GitHub/ctrpad/ref/CTR/CTR - Crash Team Racing (USA).bin' \
tools/verify-linux-i686-golden-replay.sh \
  /private/tmp/ctrpad-i686-cutscene-run-4hjAQW/debug/reports/20260731/ctr-025812 \
  auto
```

This command was run once as a preflight while the report was incomplete. It
resolved the selected retail image and report tree, then exited 1 with:

```text
Golden report metadata is not accepted: missing finalized=1.
```

That rejection is expected and proves the verifier will not start the three
long playback processes from an in-flight report. The full command is
prepared, not yet claimed as passed.

## 2026-07-30 — Measured macOS ARM64 cadence

### Static timing and complete replay audit

The remaining independent M6 work item was to measure cadence against retail
logic and VBlank timing. Source establishes two different quantities:

```text
platform/native_platform.c:
  GPU clock                    53,693,175 Hz
  cycles per NTSC VBlank          897,619
  exact VBlank target          59.817333412 Hz
  two-VBlank frame target      29.908666706 Hz

include/macros.h:
  nominal NTSC FPS             30
  ordinary logic elapsed       32 ms
```

`Native_AdvanceVBlankTarget` carries its rational remainder, so integer
counter conversion does not accumulate drift. The game consumes the separate
32-ms retail logic quantum through `MainFrame_GameLogic`.

`tools/analyze-replay-cadence.mjs` was added so this audit is reproducible. It
validates the file version/layout, every record index, pad checksum, record
checksum, version-aware VSync boundary, RLE packet, and decoded total before
reporting timing.

Clean report `ctr-215303` produced:

```text
frames/version:
  24232 / 4
elapsedTimeMS histogram:
  31:2,32:24216,48:9,64:5
pre-frame VBlank histogram:
  0:24231,857:1
in-frame VBlank histogram:
  0:1,2:24215,3:9,4:2,5:1,6:2,8:2
post-bootstrap frames:
  24231
32-ms plus two-VBlank frames:
  24200 (99.872065%)
```

Frame zero contains all 857 initial asynchronous-loader VBlanks and no
in-frame VBlank. It is valid captured timing but not an ordinary rendered
frame. The remaining transport contains 36 VBlanks above a uniform
two-per-frame schedule, producing a post-bootstrap transport average of
29.886466 Hz. Those explicit loading/stall packets are evidence, not noise to
discard.

The existing log has twelve 2,000-frame wall samples:

```text
31.31, 29.86, 29.83, 29.86, 29.91, 29.88,
29.90, 29.91, 29.91, 29.91, 29.91, 29.90
```

The first is a startup transient. The remaining eleven average 29.889091
with minimum 29.83 and maximum 29.91. Because log windows are not
checkpoint-local, a direct wall measurement was still required.

### Rejected measurement routes

Checkpoint 70 maps to frame 21,000. The 3,232-frame tail contains exactly
6,465 VBlanks: 3,231 two-VBlank frames and one three-VBlank frame. The exact
model duration is:

```text
6465 / 59.817333412 = 108.079040 seconds
```

The first `/usr/bin/time` playback exited 0 in 126.31 seconds. It was rejected
as a cadence result because the interval began at process launch and included
asset validation, shaders, audio, checkpoint validation, and restoration.

A second attempt timestamped filtered child output, but redirected C stdio was
block-buffered. It emitted no trustworthy live markers and was deliberately
stopped. No timing from that run is evidence.

### Accepted line-buffered wall measurement

The same unchanged producer was then launched through `/usr/bin/stdbuf -oL
-eL`. A Node monitor timestamped only lines when they were emitted:

```text
checkpoint restore:
  process wall +16.269339 seconds
replay finish:
  restore wall +108.051815 seconds
process exit:
  0
```

Against the 108.079040-second model:

```text
deviation:
  -0.027225 seconds
  -0.025190 percent
observed frame cadence:
  29.911575 frames/second
```

The built-in `FPS: 31.23` line seen in this checkpoint playback was rejected:
its 2,000-frame counter began before checkpoint restoration, so it does not
describe 2,000 frames inside the measured tail.

Measurement mode was added to the checked-in analyzer so the line-buffered
method can be repeated directly. A short checkpoint-80 self-test covered
232 frames and 464 VBlanks:

```text
expectedSeconds=7.756949
observedSeconds=7.722709
deviationSeconds=-0.034240
deviationPercent=-0.441409
exit=0
```

The 34-ms short-run difference is consistent with the roughly 27-ms
marker-delivery difference in the 108-second run. The long segment is the
accepted sustained-cadence measurement.

The full report, commands, identities, hashes, source citations, and
acceptance boundary are in
`docs/parity/2026-07-30-macos-arm64-cadence.md`. macOS desktop cadence is
accepted for M6. iOS display-link/lifecycle pacing remains unimplemented and
must be measured independently.

## 2026-07-30 — Proved macOS save persistence across relaunch

### Why report output alone was insufficient

Clean current ARM64 reports `ctr-215303` and `ctr-223221` both began with
empty memory-card seeds and wrote:

```text
memcard.recording/slot0/BASCUS-94426-SLOTS
bytes:
  6016
SHA-256:
  6a01b0f5562ed7a279d8f8e51e3b1874ac39a6120f55db4fe3873288950619a3
```

This proves a game-driven write. It does not by itself prove a later process
loaded the bytes, because replay playback deliberately clones the sibling
empty `memcard.seed` into a disposable sandbox on every run.

### Static artifact validation

`tools/inspect-native-memcard-save.mjs` was added to validate the file without
embedding or committing it. It checks:

- exact size 6,016 bytes;
- 0x100-byte `SC` icon header and one-block count;
- 0x1680-byte game payload;
- profile version `-18` (`0xffee`);
- declared `MemcardProfile` size 0x1600; and
- the complete retail CRC recurrence from `MEMCARD_Checksum.c`.

The actual save passes with CRC remainder zero and the expected SHA-256.
Passing the 988-byte report metadata as a negative input exits 1 on the size
gate.

### Exact-producer second process

The ordinary default root `build-macos-arm64/memcards` was confirmed absent.
The report's save was copied to:

```text
build-macos-arm64/memcards/slot0/BASCUS-94426-SLOTS
```

The copied hash remained
`6a01b0f5562ed7a279d8f8e51e3b1874ac39a6120f55db4fe3873288950619a3`.
Git ignore resolution pointed to the existing `build-*/` rule; no save byte
appeared in `git status`.

The immutable producer
`ctr_native-cutscene-fix-producer-eee2a8df5b96` was started with no replay
arguments under LLDB. The renderer initialized on Apple M2 / OpenGL 4.1 Metal
and all PSX shaders/VRAM pipelines reported ready.

Computer Use was considered to advance the title flow, but the raw
non-bundled SDL executable has no bundle identifier and was not exposed as a
targetable app. Attempts using its absolute path and process name both
returned `Invalid app`; no synthetic UI action was accepted from that route.
The process subsequently reached the normal startup memory-card read
automatically, so no input injection was needed.

A breakpoint on `NativeMemcard_ReadSaveData` stopped with:

```text
save_name="bu00:BASCUS-94426-SLOTS"
byte_count=5760
data_offset=256
```

Stepping out returned:

```text
w0=0
nativeResult=NATIVE_MEMCARD_OK
```

Stepping out of the enclosing `MEMCARD_Load` allowed its full
`MEMCARD_ChecksumLoad` loop to run and returned:

```text
Return value: (u8) 0
```

Value 0 is `MC_RETURN_IOE`, the retail success code. The adapter can return it
only after the native read and checksum both succeed. The second process's
live payload header read `0xffee 0x1600`; its stored terminal CRC bytes were
`b1 00`.

This is a real cross-process read/checksum result from the exact producer, not
a file-existence inference or a rebuilt diagnostic binary.

### Cleanup and scope

The process was resumed briefly after the load, then intentionally killed
under LLDB. That exit is not normal lifecycle acceptance. The temporary
default root was moved intact to:

```text
/private/tmp/ctrpad-memcard-relaunch-root-20260730
```

The build's default `memcards` path is absent again, matching its pre-test
state and preventing later tests from silently inheriting the profile. The
authoritative source artifact remains in ignored report `ctr-215303`.

This accepts macOS file persistence and checksum-valid load across process
launches. It does not accept complete manual play, a clean quit for this
diagnostic process, iOS sandbox placement, or device lifecycle persistence.
The detailed evidence is in
`docs/parity/2026-07-30-macos-arm64-save-relaunch.md`.

## 2026-07-30 — Proved ordinary macOS audio output and initial XA decode

### Why the replay report was not audio acceptance

The clean 24,232-frame replay reports exercise deterministic audio state, but
their logs do not prove that an ordinary process opened CoreAudio or submitted
non-silent samples. The next probe therefore kept the exact accepted
`eee2a8df5b96` producer and changed only its runtime sink.

### Real device boundary

An ordinary no-replay launch opened:

```text
driver=coreaudio
src=44100 Hz/2 ch
dst=44100 Hz/2 ch
device=44100 Hz/2 ch
sampleFrames=1024
```

It ran another ten seconds without an audio-output fault report and was
stopped with Ctrl-C. The signal path shut down cleanly with exit 0. This
accepted device creation, not audio content.

### Captured the submitted PCM

The same immutable binary was launched with SDL's compiled-in disk driver,
`SDL_AUDIO_DISK_OUTPUT_FILE` under `/private/tmp`, and a performance output
directory. Ctrl-C after 702 measured game frames produced a clean exit and
summary. The CSV's named `audio_underrun_delta` and
`audio_overflow_delta` columns both summed to zero.

The 6,332,416-byte S16LE stereo capture had SHA-256
`eac89fd2abc7d1d115070faaf847aa1293c4e32addd4272e75fec99a47f14834`.
Both channels were non-silent, 1,530,073 frames differed between channels,
neither channel contained a full-scale sample, and the peaks were about
-4 dBFS. `tools/inspect-native-pcm-s16le.mjs` now performs this inspection
without committing the disposable raw audio. `/dev/null` is rejected as an
empty capture.

The 35.898050-second value calculated from the number of captured samples is
not treated as wall-clock cadence evidence for SDL's diagnostic sink.

### Proved the initial XA path rather than inferring it

An LLDB name breakpoint on `NativeAudio_PlayXATrack` captured the automatic
`StateZero` request:

```text
categoryID=1
xaID=80
volumeLeft=32640
volumeRight=32640
```

The loader returned 1 and activated 52 mono compressed sectors, 209,664
source frames, at 37.8 kHz.

The first attempt to follow
`NativeAudio_XaStreamDecodeNextSectorNoLock` used `thread step-out` from an
optimized inline frame. It appeared to return 0 without moving a counter.
That observation was rejected because the debugger can force an inline return
instead of executing the optimized body.

The replacement breakpoint targeted the actual success boundary at
`native_audio.c:2224`. SDL audio thread `SDLAudioP15` reached it with
`nextSector=1`; one source step changed `decodedFrames` from 0 to 4,032. The
stack continued through XA pseudo-37.8-kHz sampling, zig-zag interpolation,
mixing, frame rendering, and `NativeAudio_StreamCallback`.

The detailed commands, values, caveats, and acceptance boundary are in
`docs/parity/2026-07-30-macos-arm64-audio-output.md`. This accepts the macOS
device/open/output and initial XA decode boundaries. Subjective listening,
broad mix/reverb/track coverage, STR synchronization, and every iOS route and
lifecycle case remain open.
