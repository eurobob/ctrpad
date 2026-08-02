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

## 2026-07-30 to 2026-07-31 — Diagnosed and fixed disappearing quick keyboard taps

### Why the visible game looked stuck

Direct control of the signed ARM64 application had already proved that its
window, renderer, textures, and lifecycle were live. The visible sequence
included the SCEA screen, rotating Naughty Dog crate, fully textured CTR main
menu, Crash Cove demo, and Adventure cutscenes. However, short synthetic
gameplay-key presses did not reliably advance the menu. That observation was
kept separate from renderer evidence and investigated instead of being
treated as either a frozen game or accepted keyboard play.

The retail input path polls at approximately 29.9 Hz. Computer Use emits a
complete key-down/key-up pair quickly. Source inspection showed that
`Platform_PollHostEvents` drained both SDL events, but
`Platform_InputUpdate` later sampled only `SDL_GetKeyboardState`. If both
events fell between polls, the final state was released and no press reached
the game. This explained both the visibly running animation and the apparently
ignored tap.

### Implemented one-snapshot host press edges

The correction added a host-only active-low keyboard latch:

1. `Platform_PollHostEvents` forwards raw mapped key events after updating Alt
   state and before Return/right-modifier shortcut normalization.
2. Key-down clears the corresponding PSX bit in
   `s_keyboardLatchedButtons`; key-up never erases an unconsumed press.
3. The next input snapshot ORs that semantic press into held-key state,
   writes the normal PSX packet, and resets the latch to `0xffff`.
4. Replay-installed snapshots, disabled pad communication, initialization,
   shutdown, and state restore clear the latch.
5. Alt-modified host shortcuts do not enter the latch. Replay/checkpoint file
   formats do not change because this transport edge is intentionally not
   serialized.

The self-test sends `C` and Right down and up before any consume. The first
snapshot must contain both active-low bits; the second must be exactly
`0xffff`. The CTest required-output expression includes
`tap-latch=c+right one-snapshot`.

### Live debugger trace and rejected observations

The diagnostic signed app reported version
`88ae012d5875-dirty` because the source was deliberately tested live before
commit. LLDB stopped on `Platform_InputKeyboardEvent` for one Computer Use
`C` press:

```text
key=6
down=1
latched after handler=0xbfff
```

The subsequent key-up used `down=0` and left the latch intact.
`NativeInput_ConsumeKeyboard` entered with `0xbfff`, returned decimal 49151,
and the completed retail packet was:

```text
00 41 ff bf 80 80 80 80
```

The first visual check after the press appeared unchanged. It was rejected
because LLDB was still paused inside the consume path and the game could not
advance. After resuming and detaching, the application advanced from the
main menu into Adventure. Further no-debugger taps advanced a live cutscene,
but that is weaker evidence because cutscenes also advance with time. The
direct one-tap packet trace is the accepted observation.

The four-file source diff was reviewed after the trace. `git diff --check`
passed, and no source changed before commit. The exact traced source became:

```text
24aff7d88db66e241bb1277fe9f9b316d62a19bc
fix: preserve quick keyboard taps
```

It was immediately pushed to `origin/codex/arm64-apple` as a recoverable
GitHub checkpoint.

### Rebuilt the exact clean commit on all active regression targets

The committed build ID was `24aff7d88db6`.

```text
macOS ARM64 Release:
  16/16 CTests passed
  SHA-256 4181de9b2f55fc6251343e29633558ca779db4746f977b0c2f00d43f00904e57

macOS ARM64 combined ASan/UBSan:
  16/16 CTests passed
  ASAN_OPTIONS=symbolize=0:abort_on_error=1
  UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1
  SHA-256 d0691133460b049627c3389d929483b2bf2330e1accd3367a3a51910431dc8ad

optimized Linux i686:
  16/16 CTests passed
  SHA-256 2e6f2bbbb1945c91ad6742fd68c2e34b35339f24d6b583294a53737daf7a1de5

signed app executable:
  thin Mach-O ARM64
  strict ad-hoc signature valid
  Info.plist valid
  identifier io.github.chrissotraidis.ctrpad
  SHA-256 bab0099c7a9d68a92a0cd7dd96445ce537e98716c1ba726d7303f7baa78ad15e
```

The compiler repeated established warnings but emitted no test or sanitizer
failure. The complete result and acceptance boundary are in
`docs/parity/2026-07-31-macos-arm64-keyboard-tap.md`.

### Kept the GitHub merge boundary explicit

At the checkpoint, GitHub showed draft PR #1 open and clean with head
`codex/arm64-apple` and base `main`. `origin/main` remained
`95417c723518407d6bfe3c81a37606294963efe2`; the input source commit existed
only on the draft branch. In other words, the viability documentation is
merged, while native implementation and evidence are pushed for backup and
review but not merged. No merge was attempted.

The unrelated immutable i686 full-parity capture continued under producer
`eee2a8df5b96`. At 2026-07-31 00:09 -0500 it remained healthy at frame 21,600,
checkpoint 73 of the expected 81, with `finalized=0`. Rebuilding `/out` for
the input test did not alter that capture's copied immutable `/run` producer.
The full parity result remains pending rather than being inferred from its
continued progress.

## 2026-07-31 — Accepted the first full same-commit ARM64/i686 trajectory

### Monitored the immutable i686 capture to normal finalization

Report `ctr-025812` used the immutable optimized-i686 producer:

```text
/private/tmp/ctrpad-i686-cutscene-run-4hjAQW/ctr_native-cutscene-fix-producer-eee2a8df5b96
SHA-256 d2e6f06023ccaedae689f11b36b33e005cb30d7bbc70d2a5e3e036f57b276c8e
```

Docker inspection confirmed that exact file was invoked under `/run`, the
source input was mounted read-only, and the user's disc image was mounted
read-only. The run began at 2026-07-30 21:58:11 -0500. Intermediate metadata
and checkpoint markers advanced normally rather than remaining at one frame.

Read-only prefix comparisons were deliberately labeled partial:

```text
frame 22491: all eight equal, zero mismatched
frame 23143: all eight equal, zero mismatched
```

Neither partial result was promoted. At 2026-07-31 00:21:14 -0500 the process
finished with:

```text
frame_count=24232
checkpoint_count=81
finalized=1
recording_status=finalized
container exitCode=0
oomKilled=false
```

The final log recorded checkpoint 80 at frame 24,000, the expected
24,232-frame finish marker, and `LOG CLOSED`.

### Required all-frame comparison passed

`tools/compare-replay-state-components.mjs` compared i686 report `ctr-025812`
against clean ARM64 report `ctr-215303`, requiring:

```text
timing,rng,drivers,world,allocation,root,pads,vsync
```

All eight reported `equal=24232 mismatched=0 ranges=none`; the comparator
exited 0. `tools/compare-replay-transport-semantics.mjs` independently found
24,232 equal and zero mismatched for pad transport, elapsed time, VBlank
total, raw VBlank blocks, expanded pre-frame VBlanks, and expanded in-frame
VBlanks.

Both reports embed build ID `eee2a8df5b96`, finalize 81 checkpoints, contain
the scripted powerslide marker, and produce the same 6,016-byte save:

```text
6a01b0f5562ed7a279d8f8e51e3b1874ac39a6120f55db4fe3873288950619a3
```

The i686 report evidence hashes are:

```text
input.ctrreplay  2c1d72d8b545982a7293ea02f32addfee909c92aac59794ab822e82c2f58adb1
state.ctrstates  4311a1d1d508cbf13b0d707fdf4849cd9e16911e79638ec5e50716d5607d3664
metadata.txt     e23368c84849fe33dadd81bcea32ff71f3a033a7b4b4c9f96a86ac09677d36d2
ctr-native.log   9f5231a844fe720f0112b31dd747e57cf932046bad67da6ab0714a56624463cc
```

The complete result is recorded in
`docs/parity/2026-07-31-full-cross-width-acceptance.md`.

### Kept the remaining gate separate

This accepts the full clean cross-width trajectory; it does not silently
accept the broader M1 gate. Two independent unchanged i686 processes still
must restore under different host address layouts and match all frames, and
an automatically selected active-driver mutation must exit 2 with `drivers`
as the first canonical difference. Those operations are intentionally
described as process-determinism/mutation verification because their prepared
mode omits the separately documented manual coverage form.

Raw reports, checkpoints, saves, the disc image, and retail assets remain
ignored and untracked.

## 2026-07-31 — Rejected repeating i686 layout and prepared a real alternate mapping

### Stopped the first verifier run before it could waste hours

The finalized cross-width pair allowed the prepared process/mutation verifier
to pass all preflight checks and enter playback 1. Its first checkpoint
reported:

```text
recording:
  sdata=0x403cd040 gGT=0x403d6bf4 mempack=0x408c8bc0
playback 1:
  sdata=0x403cd040 gGT=0x403d6bf4 mempack=0x408c8bc0
raw checkpoint:
  recorded=0xd4c950a8 restored-process=0xd4c950a8 equal=yes
```

The verifier would later require a changed process layout. Continuing the
24,232-frame playback could not satisfy that check, so the disposable
container was stopped after the evidence appeared. The outer command ended
137 because its container was intentionally stopped. That run is rejected,
not a replay failure or lifecycle result.

The ELF is a 32-bit PIE (`Type: DYN`), and the container reports kernel ASLR
mode 2. Nevertheless, this Docker Desktop amd64/i386 execution path assigned
the same addresses to independent direct launches. `setarch -L` and
`setarch -R` both failed with `Invalid argument`, including in an unconfined
probe container, so personality flags were rejected as a usable route.

### Proved a second mapping without rebuilding the producer

Invoking the immutable PIE through its matching i386 dynamic loader changed
the mapping, but the normal loader path made SDL select the loader directory
as the application base. Copying the same loader into the ignored run tree
kept `SDL_GetBasePath()` at `/out/` and preserved normal assets:

```text
loader:
  /private/tmp/ctrpad-i686-cutscene-run-4hjAQW/ld-linux-ctrpad.so.2
SHA-256:
  eccfafa93226e52e32aff3f97f5f779e7d02306cda017eae6d9c358142d4278d
producer:
  unchanged eee2a8df5b96
```

The short probe then reached checkpoint restore with:

```text
sdata=0x3efaf040 gGT=0x3efb8bf4 mempack=0x3f4aabc0
recorded=0xd4c950a8 restored-process=0x46478f61 equal=no
```

The canonical replay began normally. A second loader probe repeated the same
alternate mapping, proving that loader-versus-loader is not randomized.
Both short probes were forcibly timed out after the required early evidence;
QEMU printed a termination-time target-signal message and exit 124. Those
exits are rejected as lifecycle or full-replay evidence.

### Corrected the verifier's actual invariant

The real requirement is two unchanged full processes with:

- different host-address samples;
- different restored raw-checkpoint checksums; and
- identical canonical state through normal exit.

It is not necessary for both restored raw checksums to differ independently
from the original recording. Requiring that accidental stronger condition
would reject the valid direct-plus-loader pair even though the two playback
processes exercise different pointer layouts.

`tools/verify-linux-i686-golden-replay.sh` now accepts optional
`CTRPAD_I686_ALT_LOADER`. Playback 1 remains direct; playback 2 uses the
explicit loader. The loader must be executable and remain under the selected
build tree. The script requires the two restored raw checksums to differ,
requires at least one `equal=no` comparison against the recording, retains
the distinct host-address check, and hashes the loader into
`environment.txt`. Mutation playback still invokes the exact producer
directly.

This preserves and sharpens the process-independence gate without changing
the immutable game binary, replay, checkpoint, or canonical comparison.

## 2026-07-31 — Running project-time checkpoint and active process gate

### Added a separate progress and time ledger

The user requested a transparent running historical document that records
progress and the time it took. The detailed journal already retained the full
technical process, but it was not an efficient status or duration index.
`docs/history/PROGRESS-LOG.md` now provides that front door while this journal
remains the command-, failure-, and evidence-level reconstruction record.

Time sources are labeled rather than blended:

- the user reported a Codex goal timer of
  `1 day, 11 hours, 48 minutes, 20 seconds`;
- Docker/process wall time is measured independently;
- accumulated renderer-worker CPU time is not described as person-hours; and
- an active run is recorded as in progress, not accepted.

The goal timer is a product-level elapsed value and can include tool waits and
background validation. It is not evidence of uninterrupted active labor.

### Captured the active immutable playback without perturbing it

The corrected verifier was launched from pushed clean branch head
`e3a7e79fa4638497094deb912c1bc9c234e7714f`. Playback uses the earlier exact
immutable producer at source commit
`eee2a8df5b9605d27c7b20e943bba76174a4f6fc`; documenting the run does not
change that binary, replay, checkpoint, or container.

Docker recorded the container start as
`2026-07-31T05:38:33.587291339Z`, or 2026-07-31 00:38:33 CDT. At
2026-07-31 01:36:44 CDT:

```text
process elapsed: 00:58:11
multi-worker CPU: 05:36:46
state: running
OOM-killed: false
latest race-active marker: replay frame 9173 of 24232
```

The direct playback restored the same raw address-bearing checkpoint bytes as
the recording:

```text
sdata=0x403cd040
gGT=0x403d6bf4
mempack=0x408c8bc0
recorded=0xd4c950a8
restored-process=0xd4c950a8
equal=yes
```

That equality is expected for playback 1. Playback 2 is configured to invoke
the same producer through the copied i386 loader and must restore at the
alternate mapping proved earlier.

### Distinguished stdout buffering from a game stall

The verifier's redirected stdout ended partway through an audio-stat line and
did not update for several minutes. Process inspection showed the game thread
waiting on eight CPU-active llvmpipe workers rather than sleeping or exiting.
More importantly, redirected stdout is block-buffered while the runtime-owned
`Crash Team Racing.log` is flushed after each `Platform_Log` message.

The flushed log proved forward progress and the same transition sequence as
the accepted recording:

```text
race active:             1711
race inactive / active:  3017 / 3070
race inactive / active:  3920 / 3973
race inactive / active:  4636 / 4689
race inactive / active:  6959 / 7012
race inactive / active:  8107 / 8160
race inactive / active:  9120 / 9173
```

The 8,000-frame FPS marker eventually appeared at 2.42 FPS. The slow section
was therefore not promoted merely from high CPU utilization; frame markers
and expected game-state transitions established actual progress.

The run remains active and unaccepted. Playback 1 must finish normally,
playback 2 must finish normally under the alternate layout, and deliberate
mutation must exit 2 with `drivers` as the first difference. The Git
publication boundary is also unchanged: implementation and evidence are
pushed on draft PR #1, while only the viability documentation is merged into
`main`.

## 2026-07-31 — Recoverable pause and retail STR decode checkpoint

### Paused the expensive verifier without throwing away progress

The user requested that the goal pause at a natural stopping point. The direct
i686 playback had emitted the frame-10,000 FPS marker with no divergence; its
latest 2,000-frame window measured 1.50 FPS. At
2026-07-31 01:52:57 CDT:

```text
docker pause exciting_gagarin
```

reported the container name, and read-only inspection immediately confirmed:

```text
status=paused
running=true
paused=true
exit=0
oom=false
started=2026-07-31T05:38:33.587291339Z
```

The approximately 1-hour-14-minute active interval is derived from those
start/pause timestamps. The process was frozen rather than stopped, so its
address layout and progress remain in memory. The resume command is:

```text
docker unpause exciting_gagarin
```

The outer verifier tool session was still live at pause time. On resume, that
ownership must be checked before relying on automatic playback-2 and mutation
sequencing.

### Finished the already-started STR probe as the natural code boundary

Before the pause request, work had started on the next M6 evidence tool. The
bounded implementation was finished rather than left as an uncompiled
worktree:

```text
1753edbc5
test: add retail scrapbook STR decode probe
```

`--probe-str-scrapbook N` follows normal application startup through
`NativeAssets_Init` and `NativeAssets_Validate`, then opens `TEST.STR` through
`NativeSTR_StartScrapbook`. It reads and CPU-decodes real sectors without
creating an SDL window or GL renderer. Each frame hash includes canonical
little-endian width, height, and all decoded RGB555 pixels. The sequence hash
includes probe index, dimensions, and each frame hash.

The probe is evidence-only and commits no retail bytes or expected
retail-derived frame contents.

### Validation at the pause boundary

ARM64 Release rebuilt successfully with the established compiler warnings and
passed 16/16 CTests. The first ten retail frames produced:

```text
frame 0  cc257394fc1aa6bf
frame 1  c75e60c5237e9883
frame 2  1619948468f8d745
frame 3  9a4c939a61ecb36d
frame 4  6b4b54295b9aab26
frame 5  d18440f9f8499972
frame 6  d7af0dc8fe801a69
frame 7  d91d3b4cd26382fa
frame 8  6a711f4442d7ba8e
frame 9  429eaab6a380c28f
sequence 60dcf4c65986a034
```

Every frame reported 512 by 208 pixels. Optimized Linux i686 then rebuilt
successfully, passed the same 16/16 CTests, and produced the same dimensions,
ten frame hashes, and sequence hash. A strict diff over only `[CTR STR]`
output exited 0.

Four malformed CLI cases were also exercised: missing value, zero, nonnumeric
value, and duplicate probe option. All exited 1 with:

```text
[CTR STR] --probe-str-scrapbook requires one positive frame count
```

`git diff --check` passed, both binaries had the intended Mach-O ARM64 and ELF
i386 architectures, `git ls-files ref/CTR` remained empty, and no changed path
had a retail-media extension.

### What remains deliberately unaccepted

This result establishes actual retail-sector parsing, CPU MDEC decode, and
cross-width RGB555 identity for ten frames. It does not establish:

- ASan/UBSan cleanliness for the new probe;
- full 4,424-frame scrapbook decode;
- VRAM upload and on-screen presentation;
- retail presentation cadence; or
- STR audio/video synchronization.

Those are resume tasks. No new long process, sanitizer build, renderer run, or
movie-wide probe was started after the user requested the pause.

## 2026-07-31 — Second pause checkpoint after goal continuation

The product continued the active goal after the earlier recoverable pause, so
the preserved verifier container was unpaused and allowed to continue the same
immutable playback-1 process. It did not reach the next durable 2,000-frame
marker before the user again asked to pause.

The final read-only inspection found the direct-loader process still running,
with the exact producer
`ctr_native-cutscene-fix-producer-eee2a8df5b96`, no OOM, no divergence, and no
`playback-2.log`. The authoritative flushed runtime log still ended at the
frame-10,000 FPS marker. Redirected `playback-1.log` had grown to 49,349 bytes
and was still receiving audio-stat output, but that is not sufficient evidence
to infer a later replay frame.

At 2026-07-31 02:44:31 CDT, the following bounded stop was taken:

```text
docker pause exciting_gagarin
```

Docker then reported `running=true`, `paused=true`, `oom=false`, and
`status=paused`; a no-stream resource sample reported 0.00% CPU and
229.1 MiB resident memory. No process was killed and no evidence file was
removed. The same resume command and acceptance boundary from the first pause
still apply.

The product's goal API reported a cumulative elapsed time of
1 day, 12 hours, 15 minutes, 35 seconds at this boundary. That number is logged
as product metadata, not substituted for measured replay runtime. Playback 1,
alternate-layout playback 2, and the deliberate mutation stage remain
unaccepted until their scripted completion checks actually pass.

## 2026-07-31 — Goal resume, recoverable finalization, and STR presentation

### Resumed the exact frozen process

After the user explicitly asked to continue, inspection confirmed that
`exciting_gagarin` still owned the original shell, Xvfb, and immutable
`ctr_native-cutscene-fix-producer-eee2a8df5b96` process. The tracked worktree
and remote branch both pointed at `ac6bd768b`. The container resumed at
02:48:43 CDT with `running=true`, `paused=false`, `oom=false`.

The outer terminal session that originally invoked the verifier had expired,
but the complete playback-1 -> alternate-loader playback-2 -> mutation shell
still lived inside Docker. Two non-mutating `docker wait exciting_gagarin`
observers were attached. One streams the eventual numeric status directly to:

```text
/private/tmp/ctrpad-i686-cutscene-run-4hjAQW/
  debug/reports/20260731/ctr-025812/container-exit-status.txt
```

The file remains empty while the container is active; no successful status was
invented.

At 03:01:23 CDT, `Crash Team Racing.log` grew from 2,424 to 2,466 bytes and
emitted:

```text
[CTR Native] FPS: 0.43 (last 2000 frames)
```

That is the durable frame-12,000 marker. The low number includes both explicit
Docker pause intervals because the FPS diagnostic uses host wall time. It is
progress evidence, not active-speed evidence. No divergence or OOM was
observed.

### Made host evidence packaging recoverable

Commit `69c4c9948` adds `CTRPAD_FINALIZE_ONLY=1` to
`tools/verify-linux-i686-golden-replay.sh`. A normal run now writes `running`
before Docker starts and replaces it with `0` only after the container's
internal normal-playback/address-layout/mutation checks exit successfully.
Finalize-only mode launches no game process. It requires that captured 0 and
independently repeats every artifact-level invariant before producing
`environment.txt` and `evidence.sha256`.

The recovery mode rejected an invalid setting with exit 1, and `sh -n` plus
`git diff --check` passed. This fixes terminal-session fragility without
weakening the long verifier or changing its immutable binary.

### Added a production renderer-backed scrapbook probe

The headless STR probe did not call `LoadImage`, update the host VRAM texture,
execute the direct-VRAM presentation shader, or inspect the framebuffer. The
new command:

```text
--probe-str-scrapbook-present FRAME_COUNT OUTPUT.bmp
```

does all four. It reads the default framebuffer as `GL_RGBA`, restores the
previous pack alignment, flips rows to top-left order, hashes the result, and
saves the last frame through SDL. Retail-derived screenshots remain in
`/private/tmp`.

The first one-frame run produced the accepted decoded hash but a black image.
Rather than promote a black lead-in as renderer proof, the probe was advanced
to ten frames. Frame 9 visibly showed the coherent gray scrapbook cover, red
Naughty Dog mark, `Scrapbook`, and `1994-1999`.

Two pre-commit runs and exact clean commit `c44ea7810391` agreed on:

```text
decoded sequence:   60dcf4c65986a034
presented sequence: e85a9203c966c801
frame-9 BMP:        e7366bc9ff8ea4054d7c45e16f0a0eb8539c3bfb0cb8b1bba1c1bc5b3f1888e3
```

All ten decoded frame hashes remained identical to the optimized-i686 result.
All three BMP files were byte-identical. Missing arguments/path, zero,
nonnumeric, duplicate, and combined probe modes each exited 1. Exact clean
ARM64 passed 16/16 CTests.

The earlier sanitizer tree was resumed at low priority but remained slow under
the i686 renderer. It was stopped deliberately after 5/131 objects, preserving
incremental output, so the exact Release result could be rebuilt first.
Sanitizer cleanliness, 4,424-frame completion, real-menu 15-fps cadence,
STR XA synchronization, and iOS/GLES behavior remain unaccepted.

## 2026-07-31 — STR presentation sanitizer campaign and defined GL offsets

### Preserved the resumed long replay while testing independently

The i686 verifier remained isolated in Docker while the host performed ARM64
builds. Read-only checks repeatedly reported `running`, `paused=false`,
`oom=false`, and exit field 0 while active. The redirected playback log
continued to grow. The explicitly flushed root
`Crash Team Racing.log` advanced past frame 14,000 and recorded the expected
race-driver transitions at 13,765 and 14,291. No completion status was
invented: `container-exit-status.txt` remained empty, playback 2 did not
exist, and the acceptance gate remains open.

### Retained three rejected sanitizer routes

The ARM64 sanitizer tree used:

```sh
cmake -S . -B /private/tmp/ctrpad-str-sanitize-83dRkR -G Ninja \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DBUILD_TESTING=ON \
  -DCTR_NATIVE_MACOS_BUNDLE=OFF \
  '-DCMAKE_C_FLAGS=-fsanitize=address,undefined -fno-omit-frame-pointer' \
  '-DCMAKE_EXE_LINKER_FLAGS=-fsanitize=address,undefined'
```

The first suite set `ASAN_OPTIONS=detect_leaks=1`. Apple's ASan runtime
reported that leak detection is unsupported and every process aborted with
status 134. All 16 tests were rejected; none was relabeled as a product
failure or pass.

The supported configuration was:

```text
ASAN_OPTIONS=detect_leaks=0:halt_on_error=1:strict_string_checks=1
UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1
```

It passed all 16 tests and the headless ten-frame STR probe. The first
renderer-backed probe then stopped at `platform/native_renderer.c:1254`.
UBSan correctly rejected `&((GrVertex *)NULL)->x` as null-member access. The
same undefined expression configured all four packed vertex attributes.

A subsequent run after the source correction, but with ordinary SDL HID
enumeration, stopped in an Apple system framework. ASan reported a
heap-buffer-overflow in CoreGraphics `pdf_lexer_scan`, reached through
CoreUI/SwiftUICore/AppKit and SDL's HID enumeration during
`Platform_InputInit`. Renderer initialization had completed, but the STR
decode/upload/presentation loop had not yet executed. This run is retained as
an environment-boundary rejection, not evidence that the game renderer still
failed.

### Corrected and pinned the actual renderer defect

Commit `75b09db17` replaces the four null-member expressions with:

```c
(const void *)(uintptr_t)offsetof(GrVertex, field)
```

at `platform/native_renderer.c:1254-1261`. Static assertions at
`include/platform/native_renderer_types.h:37-41` pin `sizeof(GrVertex) == 20`
and the position, texture, color, and extra offsets at 0, 8, 12, and 16.
This preserves the exact OpenGL byte layout while using defined C semantics.

Before commit, a fresh normal ARM64 build passed 16/16 tests and produced the
accepted screenshot bytes. The correction was committed at 03:26:12 CDT and
pushed to the existing draft branch. Both build trees were then regenerated
so their binaries embedded the exact clean identity:

```text
CTR Native 0.1.0-beta.7.1 (75b09db17d1c)
```

### Exact clean acceptance

The exact clean Release binary was written at 03:27:32 CDT. It passed 16/16
tests and the ten-frame presentation probe. The exact clean sanitizer binary
was written at 03:33:27 CDT. It passed:

```text
16/16 CTests
headless decoded sequence   60dcf4c65986a034
presented sequence          e85a9203c966c801
frame-9 BMP SHA-256         e7366bc9ff8ea4054d7c45e16f0a0eb8539c3bfb0cb8b1bba1c1bc5b3f1888e3
```

The renderer-specific sanitizer invocation set `SDL_JOYSTICK_HIDAPI=0` to
avoid the unrelated Apple framework path. The full sanitized CTest suite did
not use that setting; its `ctr_native_input` test passed. The sanitizer and
Release BMPs compared byte-for-byte equal. No ASan/UBSan diagnostic appeared
in the accepted headless or renderer runs.

The sanitizer tree was created at 02:39:24 CDT and the final screenshot was
written at 03:33:43 CDT. The 54-minute-19-second wall interval includes
compilation, rejected observations, diagnosis, code correction, Release
validation, and the exact clean rebuild. It is process/project elapsed time,
not person-hours. The goal API reported cumulative elapsed
1 day, 13 hours, 5 minutes, 15 seconds at the 03:34 checkpoint.

The bounded STR sanitizer question is now accepted. Full 4,424-frame movie
coverage, retail menu cadence, STR XA synchronization, broad normal-menu
entry/skip/teardown, GLES, and iOS remain open.

## 2026-07-31 — Complete scrapbook decode and presentation coverage

The ten-frame sanitizer gate was extended immediately to the declared
`NATIVE_STR_SCRAPBOOK_FRAME_COUNT` of 4,424. No new code or binary was
introduced: all commands used the exact clean `75b09db17d1c` Release and
sanitizer binaries already accepted above. They ran serially with
`nice -n 15` so the active i686 verifier retained CPU priority.

### Full headless decode

The Release command completed in 61.35 seconds; ASan/UBSan completed in 68.69
seconds. Both emitted exactly 4,424 frame records, all 512 by 208, with
contiguous probe and source indices 0 through 4,423. Both ended with:

```text
[CTR STR] scrapbook probe passed:
frames=4424 sequence-fnv1a64=4b193011f608bc90
```

Hashing only the 4,424 per-frame evidence lines produced the same SHA-256 in
both builds:

```text
6f70283316cf03bc786e3b96847cf04ace34a12dea33a5e3d6a630f7cb63c7e5
```

A strict comparison of those lines exited 0. No sanitizer diagnostic
appeared.

### Full VRAM upload, presentation, and framebuffer readback

The Release command completed in 64.85 seconds; the HID-isolated sanitizer
command completed in 78.75 seconds. Both emitted exactly 4,424 contiguous
records and ended with:

```text
[CTR STR] scrapbook present probe passed:
frames=4424 sequence-fnv1a64=e7e81ffeedaa7b2c
```

The per-frame records include both decoded RGB555 and presented RGBA hashes.
Their filtered SHA-256 manifest was identical across builds:

```text
96d3254f21c70e004e2319886ef72080c5ca3d413f920c4e04a591238e1f480e
```

The final frame is black movie tail. The two final BMPs are byte-identical at:

```text
85b43f1be768060c8abf89f7f9dfc1052cd78249947910302f5a5a19657677aa
```

That frame is not promoted as visual evidence; the coherent, inspected frame
9 remains the visual proof. Temporary logs and retail-derived BMPs remain
outside Git.

The four commands account for 273.64 seconds of measured serial process wall
time. At the 03:44 CDT checkpoint, the concurrently preserved i686 verifier
remained running, unpaused, not OOM-killed, and at or beyond its durable
frame-14,000 marker. The goal API reported cumulative elapsed
1 day, 13 hours, 15 minutes, 21 seconds.

Complete probe-based movie decode and host presentation are accepted. This
does not establish the real menu's 15-fps scheduler, interleaved XA A/V sync,
normal skip/teardown behavior, GLES, or iOS.

## 2026-07-31 — Exercised the production Scrapbook route

### Rejected the first unlock method without blaming the game

The retail Scrapbook cheat requires holding L1+R1 while entering:

```text
Up, Up, Down, Right, Right, Left, Right, Triangle, Right
```

The native keyboard map assigns L1 and R1 to separate left/right Shift keys
and Triangle to `Z`. Computer Use can send chords, but its key-pulse interface
could not preserve both independent Shift holds across the full sequence. Two
attempts left the six-row menu unchanged. This is retained as a UI-automation
limitation, not a product failure and not acceptance evidence.

### Built a reproducible disposable save instead

The accepted relaunch save was inspected against
`include/namespace_Memcard.h`. `MemcardProfile.gameProgress` begins at profile
offset `0x144`; `GameProgress.unlocks` begins four bytes later; bit 36 is bit 4
of unlock word 1. Including the native file's `0x100` icon wrapper, the target
word is therefore at file offset `0x24c`.

`tools/prepare-scrapbook-test-save.mjs` was added to make that operation
auditable. It:

1. requires a 6,016-byte one-block native save;
2. validates `SC`, profile version `-18`, profile size `0x1600`, and a zero
   retail CRC remainder;
3. refuses identical source/output paths;
4. sets only mask `0x10` in the little-endian word at `0x24c`;
5. regenerates the two-byte retail checksum; and
6. validates before writing with exclusive-create semantics.

The helper changed the word from zero to `0x10`. The output passed the
independent save inspector with CRC remainder zero. In-place and
existing-output tests both exited 1, and the original save retained SHA-256
`6a01b0f5562ed7a279d8f8e51e3b1874ac39a6120f55db4fe3873288950619a3`.
The disposable output has SHA-256
`468c43b5b58b4ebe2eb6a207b166d02c36011b27e50b76e207d6009fca5521a4`
and remains under the ignored app-build memcard directory.

### First visual run established the route but not audio

The current signed ARM64 app was closed normally, relaunched, and advanced
through the ordinary startup sequence. The loaded save exposed a seventh
`SCRAPBOOK` row. Arrow-key pulses occasionally landed entirely between the
retail polls, so screen state was refreshed after each bounded batch instead
of assuming an input arrived. `Return` maps to Start; menu confirmation maps
to PSX Circle on `V`.

The real menu route showed multiple coherent movie scenes. Start skipped the
movie, the title transition ran, and the intact seven-row menu returned. This
was enough for visual entry/skip/return, but the log did not expose XA state.
The run was therefore not promoted as audio-lifetime evidence.

### Added narrow telemetry and repeated from an exact commit

`MM_Scrapbook_PlayMovie` now records:

- XA state, selected channel, and cadence at successful start;
- exit reason, number of successful frame uploads, and XA state at exit; and
- XA state before and after the existing stop calls.

The counters and logs are observational. The existing
`Platform_WaitUntilVBlank`, `NativeSTR_UploadNextFrame`,
`NativeAudio_PlayXAFile`, input mask, and teardown order are unchanged.

The helper and telemetry were committed and pushed as
`63b0a0773a00afb12e3fce9152ece4affbcfda68`. CMake was regenerated so the app
embedded exact build ID `63b0a0773a00`. The resulting 4,167,824-byte binary
has SHA-256
`fe61f8531afd244d2f2d984124fc2e3dacb5b5c7a2a313b0002386d01f31b875`,
is thin ARM64, passed strict deep code-signature verification, and passed all
16 CTests.

The exact run repeated normal boot, save load, menu selection, movie playback,
Start skip, title transition, and menu return. Captures at 04:04:23 and
04:04:39 CDT showed clearly different coherent concept-art and E3-1996
display-wall scenes. The frozen runtime evidence was:

```text
[CTR Scrapbook] native playback started: xaActive=1 xaChannel=1 cadenceVBlanks=4
[CTR Scrapbook] native playback ending: reason=input-skip framesUploaded=674 xaActive=1
[CTR Scrapbook] native teardown: xaBefore=1 xaAfter=0
```

This accepts the production menu route, selected four-vblank scheduler,
successful XA preparation/lifetime, skip, and teardown. It does not prove that
the human-perceived XA waveform was synchronized to a particular video frame,
and the run intentionally skipped rather than reaching the movie's natural
end. Those boundaries stay open.

No retail screenshot, movie frame, save, disc byte, or runtime log was added
to Git. The parity report records local screenshot descriptions and SHA-256
values only.

### Timing and concurrent verifier

The helper was written at 03:55:38 CDT; the disposable save followed at
03:56:35. Source commit `63b0a0773` was created at 04:02:11, the exact app was
written at 04:02:57, and the evidence run closed at 04:06:45. The measured
helper-to-close interval was 11 minutes, 7 seconds; commit-to-close was
4 minutes, 34 seconds.

The Codex goal API reported cumulative elapsed 1 day, 13 hours, 37 minutes,
56 seconds at the 04:06 checkpoint. During this work the preserved i686
direct-loader process stayed running, unpaused, and not OOM-killed. Its
flushed log advanced to the ninth 2,000-frame marker, so frame 18,000 is now
durable. The exit-status capture remains empty; playback 1, playback 2, and
mutation remain unaccepted.

## 2026-07-31 — Let Scrapbook finish and measured its silent tail

### First natural run

The same exact `63b0a0773a00` app and checksum-valid disposable save were
restarted through the normal retail sequence. `SCRAPBOOK` was selected and no
skip input was sent. The app uploaded all 4,424 frames, returned through the
ordinary title/menu transition, and showed the intact seven-row menu.

The run started at 04:12:47 CDT and returned at 04:17:43 CDT, about 296
seconds. This is close to the declared 4,424 / 15 = 294.93 seconds without
attempting to subtract transition or observation granularity. A steady
2,000-frame runtime window reported 14.95 FPS. The frozen log ended with:

```text
[CTR Scrapbook] native playback ending: reason=stream-end framesUploaded=4424 xaActive=0
[CTR Scrapbook] native teardown: xaBefore=0 xaAfter=0
---- LOG CLOSED ----
```

Its SHA-256 is
`384f0f6b5b0d6a1df56b5114e1794fc112e1c462c6dfabf21e684c2e5bd15a5f`.
Temporary one-minute visual checks showed advancing coherent scenes, but the
desktop service removed those captures during close. No screenshot hash is
claimed for them.

### Added observation without changing the scheduler

The first natural run showed that XA had ended before video stream exhaustion
but not when. Commit `900f5656b41d` added only:

- the starting VBlank count;
- a one-time active-to-inactive XA transition record; and
- elapsed VBlanks at both transition and stream end.

The upload, `Platform_WaitUntilVBlank`, four-VBlank cadence, input test, and
audio stop order were unchanged. The source checkpoint was committed and
pushed, the exact app rebuilt, and 16/16 CTests passed.

### Second natural run and frame-content interpretation

The exact `900f5656b41d` app repeated normal startup and natural playback. It
started at 04:22:59 CDT and reached stream end at 04:27:55 CDT:

```text
[CTR Scrapbook] native playback started: xaActive=1 xaChannel=1 cadenceVBlanks=4 startVBlank=4315
[CTR Scrapbook] native XA exhausted: framesUploaded=4371 vblanksElapsed=17480
[CTR Scrapbook] native playback ending: reason=stream-end framesUploaded=4424 xaActive=0 vblanksElapsed=17696
[CTR Scrapbook] native teardown: xaBefore=0 xaAfter=0
```

The endpoint equals `4424 * 4` exactly. The XA transition was sampled after
successful upload 4,371 and before that frame's scheduled wait. The stream
then accepted 53 more frames. The existing full-presentation records were
queried rather than assuming those frames were active content: indices 4,371
through 4,393 have changing hashes during a fade, while 4,394 through 4,423
are 30 identical black frames. The black decoded/presented hashes match the
movie's initial black frame:

```text
decoded  cc257394fc1aa6bf
presented 2bf050200a5e1325
```

The 216-VBlank difference between observations is 3.6 seconds. The 53 later
uploads are 3.53 seconds at 15 FPS; the one-frame discrepancy is the expected
sampling position described above. This rejects the hypothesis of cumulative
video scheduler drift and identifies an authored silent fade/black tail.

The app returned to the normal menu and was closed through its window button.
Normal close removed the temporary live log before it could be frozen, so the
second run has no claimed post-close file hash. The exact lines were captured
before close and are recorded as such. Natural end, exact video cadence, menu
return, and the measured authored-tail boundary are accepted; listening
quality and human-perceived synchronization are not.

## 2026-07-31 — Added a practical keyboard layout

### Chose additive aliases

The desktop input layer already mapped arrows, `Z/X/C/V`, Shift/Ctrl,
brackets, Space, and Return directly to PS1 buttons. Those keys worked after
the earlier tap-latch correction, but they were difficult to discover and
awkward for simultaneous accelerate, steer, and drift.

The implementation keeps every old scancode and adds a spatial two-hand
layout:

```text
W/A/S/D  -> D-pad
I/J/K/L  -> Triangle/Square/Cross/Circle
Q/E      -> L1/R1
P        -> Start
Tab      -> Select
```

Alternative fields were added to `NativeInputKeyboardMapping`. Both the
key-down latch translator and current-held-state reader test the original or
alias scancode before clearing the same active-low PS1 bit. No replay,
checkpoint, gamepad packet, or gameplay/physics representation changed.

The media-free self-test enumerates all 12 new key-to-bit expectations, uses a
synthetic held keyboard state to require Cross + Right + R1 for `K+D+E`, and
sends a complete `K+D` down/up pair before a poll to require the alias tap
latch. This specifically tests race-useful chording as well as menu taps.

### Rejected and corrected verification attempts

The first rebuilt suite reported 15/16 because CTest's required-success regex
still expected the old exact self-test sentence. The executable itself
printed a passing result with the new alias evidence. The stable
`tap-latch=c+right one-snapshot` marker was restored unchanged and the alias
evidence appended; this preserved downstream CI compatibility. The rebuilt
suite then passed 16/16.

A manual follow-up invoked a guessed `--internal-test-input` spelling that is
not a supported internal flag. It launched the normal game instead of the
self-test and was interrupted after renderer/audio initialization. The
process closed its log cleanly, but that launch is rejected as test evidence.
CTest's configured `ctr_native_input` invocation is the accepted automated
path.

The first visual run used the rebuilt pre-commit functional diff. `K` advanced
the legal/splash sequence, and the real menu accepted `S` and `W`. The source
was then reviewed with `git diff --check`; retail-path tracking remained
empty. Source, README control table, and the viability map were committed as:

```text
2c10b00b34df4f0eb61aa8b72cbe99588a930ed6
feat: add practical desktop keyboard controls
```

CMake was reconfigured after commit so the app embedded exact build ID
`2c10b00b34df`. The 4,167,248-byte executable has SHA-256
`6e3171d1619fbc34ee1985679604a018107ac62039e6de5d8489b1729224f9e9`,
is a thin ARM64 Mach-O, passes strict deep signature and plist checks, and
passes 16/16 CTests.

The exact app was then launched through normal retail startup. `K` advanced
to the seven-row main menu; at a stable menu, `S` moved the selection from
Adventure to Time Trial and `W` moved it back. Key pulses delivered during
noninteractive title transitions were ignored by retail and bounded retries
were used; the first stable-menu `S` press succeeded. The app was closed
normally. Temporary captures were inspected but removed automatically on
close, so no hash or retail pixel is published.

### Publication, time, and concurrent verifier

The source commit was created at 04:39:42 CDT, the exact executable was
written at 04:40:48 CDT, and visual verification completed by 04:43:44 CDT.
Commit `2c10b00b3` was pushed to `origin/codex/arm64-apple`; local and remote
tips matched. Draft PR #1 remains open and unmerged.

The goal API reported 1 day, 14 hours, 8 minutes, 23 seconds of cumulative
goal time during the runtime checkpoint and 1 day, 14 hours, 17 minutes,
32 seconds immediately before the documentation freeze. This product timer is
not treated as active labor or a benchmark.

The preserved i686 direct-loader container remained running, unpaused, and
not OOM-killed. Its playback-1 log has durable 2,000-frame markers through
frame 22,000 and a driver-inactive event at frame 21,331. The 24,232-frame
finish line and machine-captured exit status are still absent; playback 2 and
mutation have not begun and remain unaccepted.

## 2026-07-31 — Made controller slots own their SDL instances

### Audited the existing path before changing it

After the keyboard aliases were committed, the desktop input path was read
from initialization through hotplug, slot cycling, state capture/restore,
snapshot generation, and vibration. `s_controllerToSlotMapping` already had a
four-slot `-1` initializer and was swapped, captured, restored, and searched.
It was not written by `NativeInput_OpenController` and was not cleared by
`NativeInput_CloseController`.

That made this concrete duplicate-event sequence possible:

1. SDL instance A opens slot 0, but slot 0's mapping remains `-1`;
2. another add for instance A cannot find an owned mapping;
3. slot 0 is occupied, so the free-slot search chooses slot 1;
4. the same instance opens a second handle in slot 1; and
5. one remove event closes the first matching handle and returns, leaving the
   second handle live as a ghost controller.

The correction writes the resolved ID returned by the opened gamepad's
joystick into the selected slot. Close resets both the controller's
`instanceId` and the slot mapping. A failed open still claims nothing.

### Added a real virtual-device integration test

The self-test initializes SDL's gamepad subsystem and attaches a virtual
standardized gamepad with all standard buttons/axes and a rumble callback. It
then invokes the same public hotplug functions used by host events.

The proof requires one open handle after two identical add events. It drives
South, right shoulder, right trigger, left X, and right Y, pumps the virtual
joystick state, and applies the production controller-to-PS1 snapshot mapper.
The exact result is:

```text
buttons=0xb5ff
rightX=0x80 rightY=0xff leftX=0x00 leftY=0x80
rumble input=40 80 callback low=32640 high=16320 calls=1
```

Removal must leave the controller null and mapping `-1`; reconnect must reopen
the released slot. The public test marker is:

```text
virtual-gamepad=buttons+axes+rumble+hotplug
```

This deliberately does not claim a physical wireless/USB device, a complete
race, or an iOS controller route.

### Exact ARM64 and sanitizer checks

Functional commit `2f9bf4eaedd1ca6c781a9654f9851687b4c7fc18` was created at
04:54:36 CDT. The signed app executable followed at 04:55:57 and reports:

```text
CTR Native 0.1.0-beta.7.1 (2f9bf4eaedd1)
SHA-256 0f23ce4c8c8770caddda4c32d28e84ad6fd0c614c20514dea343cb31ca0967c1
Mach-O 64-bit executable arm64
```

It passed 16/16 CTests, strict deep signature verification, and plist lint.
A later fresh rerun passed the same 16 tests in 0.70 seconds.

The combined ASan/UBSan executable was written at 05:00:36, reports the same
exact build ID, and has SHA-256
`4be53768ba67f9e6d38bb677146e9bf1b24481cf406afe5565bb9abb0025ecad`.
The verbose controller test passed in 1.27 seconds. All 16 tests then passed in
2.24 seconds with leak detection disabled and both sanitizers configured to
halt on the first finding. No finding occurred.

The source commit was pushed from `df17f4643` through `2f9bf4eae` on
`origin/codex/arm64-apple`. Draft PR #1 remained open, draft, cleanly
mergeable, and unmerged. `origin/main` remained `95417c723518`.

### Rejected new i686 warnings instead of normalizing them

A new disposable build tree was created at:

```text
/private/tmp/ctrpad-i686-controller-gXAeRV
```

The pinned `ctrpad-linux-i686:ubuntu-24.04` image mounted the repository
read-only at `/src` and the disposable tree read/write at `/out`. Release,
testing, `-m32` compile flags, and `-m32` link flags matched the established
i686 procedure. Configuration took 590.4 seconds under ARM-host emulation and
reported `64-bit: FALSE` plus the SDL virtual joystick backend.

The first unity compile repeated established format and maybe-uninitialized
warnings but also produced two new sign-comparison warnings in the virtual
test. `SDL_JoystickID` is unsigned while the historical mapping snapshot is
signed. Comparing by accidental integer promotion was rejected. Both test
comparisons now cast the stored mapping bits to `SDL_JoystickID`; local commit
`764205d4c` contains only those two type-explicit comparisons. The ongoing
disposable compile will retain its completed SDL objects, then reconfigure and
rebuild the unity object from the final clean commit. No i686 pass is claimed
until that occurs.

### Concurrent long-verifier state

The unrelated preserved i686 acceptance container was never restarted,
paused, or modified. Its direct-loader playback completed all 24,232 frames
at 04:48:06 CDT and the scripted alternate-loader playback began immediately.
At the recorded checkpoint, Docker showed the qemu-i386 process using about
522% CPU and no pause or OOM. `playback-2.log` contained the first 2,000-frame
FPS marker at 2.26 FPS and the expected frame-1,711 driver activation. The
machine-owned exit-status file remained empty. Alternate playback, exit 0,
raw-layout separation, and mutation detection therefore remain open.

At 05:18:22 CDT, the goal API reported 139,736 elapsed seconds: 1 day,
14 hours, 48 minutes, 56 seconds. It is recorded as cumulative product-task
time, not continuous labor or a performance measurement.

### Final clean-tip controller matrix

After the bootstrap i686 build completed all 261 targets, the same disposable
tree was reconfigured from cache against clean synchronized tip `359e8d5a0`.
Configuration took 22.4 seconds and identified:

```text
SDL-3.4.10-beta-7.1-103-g359e8d5a0
64-bit: FALSE
joystick drivers: hidapi linux virtual
```

Only SDL's revision-sensitive object and the game unity object rebuilt. The
two new `SDL_JoystickID` sign-comparison warnings were absent. GCC repeated
only the four already established i686 warnings: two nonliteral format-string
warnings and two maybe-uninitialized warnings in the existing end-event menu.

The final executable reports:

```text
CTR Native 0.1.0-beta.7.1 (359e8d5a0f07)
ELF 32-bit LSB PIE executable, Intel 80386
interpreter /lib/ld-linux.so.2
GNU Build ID e71bd4d9b99bf1efc5214a6d4c250ca22621ef96
SHA-256 d21c04bd129399a28a0e983fd26d187f5c98adcc1506983caf18a029992bfd10
```

The first final CTest invocation mounted `/out` read-only. CTest could not
create `Testing/Temporary/LastTest.log`, ran no tests, and exited 8. That
invocation is rejected as a harness error. The corrected invocation kept
source read-only but mounted the disposable output read/write; all 16 tests
passed in 3.51 seconds, including `ctr_native_input` in 0.76 seconds.

The clean-tip signed macOS app passes 16/16 in 0.61 seconds, passes strict
deep signature and plist validation, and is thin ARM64. Its executable SHA is
`c97953dddfe682962732aea7a2d2e8ebde6f8083a2add1eb7ef47388924ae446`.
The exact combined ASan/UBSan executable passes 16/16 in 3.90 seconds with no
finding and has SHA
`ec4d49d5ca1180dc2711bfd7353d58d84e9e142a724e97947940d14dad12adfd`.
Both identify as `359e8d5a0f07`.

At 05:36:44 CDT the preserved alternate-loader process had crossed durable
frame 4,000, reported a 1.52 FPS second window, and observed driver-inactive
and active transitions at 3,017/3,070 and 3,920/3,973. Docker reported
running, unpaused, and not OOM-killed. The playback log was 20,677 bytes and
the machine status file remained empty. Completion, exit capture, raw-layout
separation, and deliberate mutation remain unaccepted.

The goal API then reported 140,855 elapsed seconds: 1 day, 15 hours,
7 minutes, 35 seconds. This remains cumulative task time rather than labor or
benchmark time.

## 2026-07-31 — Drove the signed app with the new keyboard aliases

The clean-tip signed app `359e8d5a0f07` was launched through direct desktop
control. SCEA presentation text, the Naughty Dog crate, CTR title animation,
and the seven-row main menu all rendered coherently. No debugger, replay,
installed snapshot, or internal self-test was active.

The first attempt used `S` to highlight Time Trial and a single `K` to
confirm. It did not enter the selection state and the normal attract/story
sequence began. An early hypothesis that this screen required Circle was
rejected after source inspection: the character and track selection paths
accept both Cross and Circle. The actual evidence is only that the attempted
single tap did not land in an accepting menu state. `L` returned to the title;
bounded `S` then `L` entered Time Trial. Repeated bounded Cross input selected
Crash and the no-ghost row, leading to Crash Cove.

The first stable track capture showed timer `0:12:21`, lap 1/3, and the kart
stationary beneath the CTR banner. Forty-five isolated automated `K` presses
advanced the timer to `0:35:80` but did not sustain acceleration; this route
is rejected because the UI controller pressed and released too sparsely.

Sending 500 dense `K` characters through the same focused app event path
moved the kart off the line, visibly into the left cliff/tree area, and moved
the minimap marker by `1:05:66`. A dense alternating `K+D` stream changed the
kart's heading/location by `1:23:95`. A dense sequential `K+D+E` stream then
moved it away from the wall toward the exposed rock/ocean section by
`1:40:48`.

This proves the live alias path can drive acceleration and steering during
real Time Trial physics. The text automation serializes events, so it does not
prove all three inputs were held simultaneously or that R1 produced a visible
hop/powerslide. The media-free test's synthetic held state is still the
authoritative simultaneous `K+D+E` result. No lap/full-race claim is made.

The final capture was created at 05:45:21 CDT by the desktop observation
service. It is a 139,137-byte JPEG with SHA-256
`e6e97000036efe26b162cba3022d3a6e3842656c2c63a488b322b3dc532f0644`.
It remained in the service's temporary directory and was not copied into the
repository because it contains retail-derived pixels. The window close button
then closed the app normally; application enumeration reported
`io.github.chrissotraidis.ctrpad` with `isRunning=false`.

At 05:46:33 CDT the goal API reported 141,439 elapsed seconds: 1 day,
15 hours, 17 minutes, 19 seconds. Docker still reported the preserved verifier
running, unpaused, and not OOM-killed. Playback 2 had observed driver
transitions at 4,636/4,689; no frame-6,000 FPS marker or exit status existed.

## 2026-07-31 — Added a deterministic production SPU mixer/reverb oracle

### Audit boundary

The existing macOS audio evidence accepted the real CoreAudio open, ordinary
non-silent PCM, zero short-run output transport deltas, and initial retail XA
load/decode. Its explicit open list still included representative mixer and
reverb behavior. A source search found only the audio snapshot-alignment
self-test; there was no media-free PCM oracle.

The selected seam is inside `platform/native_audio.c`, which owns both the
public SPU API and the implementation under test. This permits a synthetic
ADPCM block to cross the real upload, Key On, streaming decoder, Gaussian
interpolation, volume, reverb, and render paths without SDL device timing or
retail bytes.

### First implementation and observation phase

`NativeAudio_MixerSelfTestStartVoice` authors one 16-byte ADPCM block at SPU
address `0x2000`, uploads it through `NativeAudio_SpuSetTransferStartAddr` and
`NativeAudio_SpuWrite`, assigns voice 0 through
`NativeAudio_SpuSetVoiceAttr`, optionally assigns the reverb send, and calls
`NativeAudio_SpuSetKey(SPU_ON, ...)`. The test stabilizes the already-keyed
voice at unity sustain so the measured output isolates decode, pitch,
interpolation, panning, termination, and reverb.

The dry phase uses Loop Start + Loop End + Repeat, master `0x7fff/0x7fff`,
voice volume `0x6000/0x2000`, pitch `0x1000`, and 4,096 rendered frames. It
requires nonzero stereo PCM and left absolute energy greater than twice right.

The wet phase resets state, chooses Room + Clear Work Area at depth
`0x7fff/0x7fff`, enables voice 0's send, and renders a centered one-shot block
for 24,000 frames. Voice 0 must be inactive at completion and output after
frame 256 must be nonzero.

The initial CTest regex intentionally allowed printed hexadecimal values while
the implementation was observed on independent targets. Results were
identical:

```text
dry=0x132e19d77167fb3d
wet=0x4bdedc91d1293ad8
tail-frames=6905
```

Ordinary ARM64, combined ASan/UBSan ARM64, and optimized i686 all emitted
those values. The values were then promoted to compiled expected constants
and an exact CTest regex. This ordering avoids declaring an ARM64-only first
observation to be a cross-width oracle.

### Pre-commit fixed-oracle gates

The fixed gate passed 17/17 ordinary ARM64 tests in 0.95 seconds and 17/17
sanitizer tests in 3.58 seconds. The targeted normal and sanitizer runs emitted
the exact oracle. `git diff --check` passed. The four source/build files were
committed as:

```text
87f8e7a052c29aa8c01eb76263162e24cf2c5d00
test: lock cross-width audio mixer oracle
```

### Exact clean-commit matrix

The source identity change was not assumed irrelevant. All producers were
rebuilt after the commit:

```text
signed macOS ARM64 app
  build ID: 87f8e7a052c2
  CTest: 17/17 in 0.57 seconds
  strict signature/plist: passed
  architecture: Mach-O 64-bit arm64
  SHA-256: 85c03b1a557ad379a869ded940b1ed37879c918d96401a7c6f860d99e7611d93

combined ASan/UBSan ARM64
  build ID: 87f8e7a052c2
  CTest: 17/17 in 3.96 seconds
  finding: none
  SHA-256: 5382e363bd6c9872674d3a65077c47e7f23dbaf4e96a1f01e82f6b4ff1601dc6

optimized Linux i686
  build ID: 87f8e7a052c2
  CTest: 17/17 in 3.74 seconds
  architecture: ELF 32-bit LSB PIE, Intel 80386
  interpreter: /lib/ld-linux.so.2
  GNU Build ID: eb4975f6df60b41738c657827bbe5ed38038e54a
  SHA-256: abc019636ca9e8f5081584b7fcc12b36b918299148df7118bc2bd0e07394ef48
```

The normal and sanitizer Apple compiles repeated 32 and 59 established
warnings. GCC repeated the established two format-security and two
maybe-uninitialized warnings. The audio test introduced no new warning.

The i686 build used the existing disposable cached tree at
`/private/tmp/ctrpad-i686-controller-gXAeRV`, pinned image
`ctrpad-linux-i686:ubuntu-24.04`, read-only `/src`, read/write `/out`, Release,
testing enabled, and `-m32` compile/link flags. The protected historical
baseline and the executable inside the preserved long verifier were not
rebuilt.

### Acceptance boundary and elapsed time

This test contains authored synthetic sample nibbles and creates no audio
file. It accepts deterministic cross-width SPU ADPCM decode, left/right
panning, nonrepeat termination, the Room reverb send/processing path, and its
wet tail. It does not accept subjective quality, retail multi-voice balance,
all reverb presets, broad XA transition behavior, long device soak, or iOS
audio lifecycle.

The audit ran approximately 05:50–06:24 CDT. The goal API advanced from
141,675 to 143,697 seconds, ending this checkpoint at 1 day, 15 hours,
54 minutes, 57 seconds. The product timer is not an audio benchmark or labor
estimate.

Concurrent alternate-layout playback 2 remained running, unpaused, and not
OOM-killed. Its third and fourth fixed 2,000-frame windows reached frames
6,000 and 8,000 at 1.06 and 1.40 FPS; driver 0 transitioned inactive/active
at 6,959/7,012. `container-exit-status.txt` remained empty. Completion,
captured exit zero, layout separation, and deliberate mutation therefore
remain unaccepted.

### Publication and corrected repository pin

Implementation commit `87f8e7a05` and the first documentation commit
`9ed75956c` were pushed to `origin/codex/arm64-apple`. Local and remote refs
matched `9ed75956cf7078fa759bbb8189ac7f5c20fc5e94`.

The first `gh pr view 1` validation did not pin a repository. Because this
worktree also has an `upstream` remote, `gh` resolved unrelated closed PR #1
in `CTR-tools/ctr-native`. That response is rejected as a repository-selection
error. The corrected query explicitly used `-R chrissotraidis/ctrpad` and
reported:

```text
PR:          https://github.com/chrissotraidis/ctrpad/pull/1
state:       OPEN
draft:       true
merge state: CLEAN
base/head:   main / codex/arm64-apple
head OID:    9ed75956cf7078fa759bbb8189ac7f5c20fc5e94
origin/main: 95417c723518407d6bfe3c81a37606294963efe2
```

No merge was attempted.

## 2026-07-31 — First shared GLES 3 renderer dialect and failure-path hardening

The newly populated `ref/CTR` and existing Android reference were reviewed
before changing the renderer. `ref/ctr-native-android` was clean on
`feature/add-android-support` at `34648097...`; its relevant renderer change
was `a9c805a...`. The useful delta was smaller than the branch: select ES,
emit GLSL ES 300, resolve through SDL, and guard unsupported desktop calls.
The current tree already stored PSX VRAM as `GL_RG8` and had no
`glGetTexImage`, so neither was reimplemented.

SDL source inspection showed that Cocoa chooses its CGL versus EGL window
setup during `SDL_CreateWindow`, while UIKit uses OpenGLES.framework. The
renderer therefore sets context profile/major/minor before creating the
window. No ANGLE `libEGL.dylib` was present locally. The checked-in glad
loader recognizes the `OpenGL ES` version prefix and exposes the shared 3.0
functions, so the ES path uses `gladLoadGLLoader(SDL_GL_GetProcAddress)` and
then rejects an incomplete VAO/framebuffer/read/pixel-store contract.

Commit `4695d9cb340d` added `CTR_NATIVE_RENDERER_GLES`, the explicit desktop
and ES dialect descriptors, GLSL ES 300 precision headers, SDL proc loading,
and guards for polygon mode, debug labels and GPU timer queries. It also added
`--self-test-renderer-dialect` as test 14 of 18. A pre-commit ordinary build,
GLES-configured build and corrected sanitizer build all passed 18/18. The
first sanitizer invocation requested leak detection; Apple ASan declared that
unsupported and aborted all tests, so it was rejected and repeated with leak
detection disabled.

The first bundle launch lacked its retail asset path and was rejected before
renderer evidence. A temporary bundle `assets` symlink then invalidated the
signature as unsealed content; it was immediately removed, after which strict
deep signature verification passed again. The accepted runtime probes used
ignored build-directory asset links only. No retail byte or capture was
staged.

The desktop production renderer reached Apple M2 / OpenGL 4.1 Metal 90.5,
compiled all four PSX shader modes and both VRAM pipelines, and opened the
44.1 kHz stereo CoreAudio stream. Visual inspection showed coherent SCEA and
orange-crate/green-stream presentation textures. The temporary 92,914-byte
JPEG hash is recorded in the detailed report but the retail-derived image was
not tracked. That bounded probe was stopped with Ctrl-C, not reported as a UI
close.

The first exact macOS GLES dependency failure then exposed an exit 139 after
SDL reported its missing OpenGL/GLES library. Shutdown was issuing unresolved
GL deletion calls after SDL-only initialization, and `main` did not stop after
failed platform setup. Commit `78ef952dbecc` introduced an API-ready cleanup
guard, an immediate `Platform_IsInitialized()` check, and two pre-init
shutdown calls in the dialect self-test. The exact rerun produced the same
missing-EGL diagnostic but exited 1. LLDB independently observed exit 1 with
no faulting stopped process.

Every producer was rebuilt from clean `78ef952dbecc`. Ordinary ARM64 and the
macOS GLES configuration each passed 18/18 in 0.70 seconds; ASan/UBSan passed
18/18 in 5.00 seconds with no finding; optimized Linux i686 passed 18/18 in
2.98 seconds. The first Docker attempt incorrectly selected `linux/386` for
the amd64 multilib builder and exited 125 after a rejected registry pull. The
correct pinned amd64 builder produced an ELF32 Intel 80386 executable with
four established warnings. Exact hashes, warning counts and GNU build ID are
in `docs/parity/2026-07-31-shared-gles3-dialect-bringup.md`.

The iPhone Simulator SDK compiled and linked a thin ARM64 executable with
UIKit, OpenGLES, Foundation and AVFoundation. Strings confirmed clean build
ID `78ef952dbecc`, `#version 300 es`, `sdl-proc`, and the pre-init-safe test.
Its generated plist has empty identifier/name/version fields. The binary is
only ad-hoc linker-signed, has no team, does not bind the plist, seals no
resources, and fails strict signature verification. It is compile/link
evidence, not a launchable M8 bundle.

M7 is now in progress, not accepted. Live GLES pixels, representative CLUT /
mask / transparency / feedback comparisons, state parity and cadence remain
open. M8 metadata, lifecycle, signing, Simulator/device execution and all
later import/touch work also remain open.

The checkpoint ran approximately 06:25–07:08 CDT. Goal active time advanced
from 144,159 to 146,335 seconds, ending at 1 day, 16 hours, 38 minutes,
55 seconds. The independent protected i686 alternate-loader verifier was not
modified. Docker still reported running, unpaused and not OOM-killed; playback
2 crossed frame 10,000 at its fifth fixed window (1.47 FPS), while the
machine-owned status file remained empty. No completion, exit-zero, layout or
mutation result is inferred.

## 2026-07-31 — Presented the shared GLES renderer through SDL/UIKit

### Starting boundary

The shared renderer checkpoint could compile a thin ARM64 Simulator executable
but could not install it. The CMake product had empty app metadata, SDL main
ownership was unresolved, no bundle resources were sealed, and neither a live
GLES context nor a pixel had been observed. macOS could not provide the live
GLES step because SDL's Cocoa path requires an absent ANGLE/EGL runtime.

The user-supplied `ref/CTR` tree remained read-only reference material. Retail
execution used only the ignored NTSC-U image already validated by the project;
no reference or retail file entered the implementation diff.

### First temporary packaging and SDL main diagnosis

The first Simulator experiment copied the compile-probe bundle to
`/private/tmp`, manually supplied minimum metadata, added a local asset copy
and applied an ad-hoc Simulator signature. This was intentionally a discovery
vehicle, not a claimed build product. Launch stopped at SDL initialization.

The previous platform log printed only the high-level failure. Adding SDL's
actual error showed that SDL main had not been initialized. `main.c` defined
`SDL_MAIN_HANDLED` on every platform and called the native entry directly.
That is suitable for the established desktop executable but bypasses the
entry/lifecycle wrapper SDL supplies for UIKit. The final change retains the
define outside iOS and lets SDL own the iOS entry.

Once corrected, the process reached normal retail startup, initialized the
renderer and audio, and continued drawing. The Simulator display remained
black.

### Rejected size-only hypothesis

UIKit differentiates logical window points from drawable pixels. The renderer
had read the logical window size. `SDL_GetWindowSizeInPixels` and
`SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED` were added so the presentation path and
viewport operate on the physical surface. Live logs then reported identical
1376-by-1032 point and pixel dimensions in this Simulator configuration.

The screen was still black. This established that the size correction was
necessary platform work but not the presentation root cause. It was not
reverted, and it was not misreported as the black-frame fix.

### UIKit framebuffer ownership

Runtime tracing confirmed the live call chain reached `CTR_Main`,
`RenderSubmit`, `DrawOTag` and `glDrawArrays`. Shader compilation and GL error
checks did not identify a missing draw. Attention moved from CPU/retail draw
generation to the final presentation object.

SDL's UIKit implementation publishes these window properties:

```text
SDL_PROP_WINDOW_UIKIT_OPENGL_FRAMEBUFFER_NUMBER
SDL_PROP_WINDOW_UIKIT_OPENGL_RENDERBUFFER_NUMBER
```

The live values were both 1. The renderer inherited the desktop assumption
that framebuffer 0 is the window presentation target, and explicitly rebound
zero around render/readback work. Under SDL/UIKit, object 1 is the drawable
SDL creates and swaps. All draws were therefore valid but directed away from
the visible UIKit target.

The final implementation stores the presentation framebuffer/renderbuffer
after window/context creation. It uses zero on other platforms, binds the
UIKit FBO for presentation on iOS and rebinds the UIKit renderbuffer before
`SDL_GL_SwapWindow`. That minimal distinction preserves the shared renderer
and the desktop behavior.

This produced visible CTR pixels immediately. No shader, CLUT, primitive or
GPU-link bridge change was needed.

### Orientation distinction

The new plist declared landscape left/right for iPhone and iPad, preferred
landscape right, full-screen display and a hidden status bar. The code also
sets SDL's iOS orientation hint before subsystem initialization. Nevertheless,
the already booted Simulator hardware was portrait. The app created a
landscape drawable inside that host orientation, yielding an apparently
clipped landscape view.

Simulator's Device > Orientation > Landscape Right action corrected the host
device state. The resulting view filled the simulated iPad in landscape. This
is recorded as a test-environment correction, not proof of rotation callbacks
or physical-device orientation handling.

### Reproducible app metadata and presets

The manual discovery package was replaced with repository-owned configuration:

```text
ios-simulator-arm64: iPhoneSimulator SDK, ARM64, iOS 15.0, GLES, RelWithDebInfo
ios-device-arm64:    iPhoneOS SDK, ARM64, iOS 15.0, GLES, RelWithDebInfo
bundle identifier:   io.github.chrissotraidis.ctrpad
device families:     iPhone (1), iPad (2)
```

No Apple team, certificate name, provisioning profile, retail asset or user
path is stored in the presets or plist. The device product is deliberately
unsigned. The raw Simulator CMake product has only the linker's ad-hoc code
signature and no sealed resource envelope, so strict bundle verification
fails as expected. The local execution copy was populated in `/private/tmp`
and ad-hoc signed afterward; that exact copy passed strict deep verification.

Implementation was committed before final acceptance rebuilding:

```text
98ae2c6d86fe527fb4ae4977357cec51d8a5f46f
feat: present GLES through UIKit
6 files changed, 221 insertions, 10 deletions
```

### Exact clean Simulator rerun

The committed Simulator app was copied to
`/private/tmp/ctrpad-ios-clean-8dujJB/CTRPad.app`. The user's ignored retail
image was cloned into that temporary copy as `assets/ctr-u.bin`, after which
the package received a local ad-hoc signature, passed strict verification and
was installed on Simulator UDID
`D80E9862-C29A-4D69-B8E5-D81D396C17D5`.

The exact launch reported:

```text
version                 0.1.0-beta.7.1 (98ae2c6d86fe)
window                  1376x1032 points, 1376x1032 pixels
presentation            framebuffer=1, renderbuffer=1
adapter                 Apple Software Renderer, Apple Inc.
API                     OpenGL ES 3.0 APPLE-23.1.1
shader language         OpenGL ES GLSL ES 3.00
PSX shader modes        4-bit, 8-bit, 16-bit, RGBA; all ready
VRAM pipelines          ready
audio                   CoreAudio, 44100 Hz, stereo, 1024 sample frames
```

Live visual inspection showed the legal screen, intro animation and full
title menu. The final retained local frame showed a full landscape iPad,
checkered background, Crash, the trophy, blue CTR ring, logo and menu with
coherent color, alpha, text and textures. Its evidence metadata is:

```text
path:       /private/tmp/ctrpad-ios-clean-8dujJB/exact-clean-keyboard-title.jpeg
dimensions: 932 x 768
size:       199375 bytes
SHA-256:    77916c2f69de6ef39432006a9aa3f8ca778aa20f261ff764576bd29be30a8a12
tracked:    no; contains retail-derived pixels
```

The app was boundedly terminated with `simctl`, not through a natural UIKit
shutdown. Two `SDL_uikitviewcontroller` begin/end appearance-transition
warnings repeated during the run. The Simulator OS also printed a duplicate
WebCore/WebKit accessibility-bundle class warning. The latter is treated as a
Simulator runtime message; the former remains an app lifecycle risk and is
not waived.

### Keyboard observation

Basic keyboard controls were implemented earlier in commit `2c10b00b3` and
the quick-tap transport was accepted through exact PSX packet, ARM64,
sanitizer, i686 and live macOS tests. The iOS metadata now advertises indirect
input events, so the Simulator run also exercised the path.

With Simulator Capture Keyboard visibly enabled, `C` was sent during the exact
committed app's intro and the title menu appeared afterward. The intro could
have ended naturally during the observation interval, so this is not accepted
as direct key-delivery evidence. On the earlier pre-commit live run, a later
`C` advanced immediately from the title menu to the Adventure intro. Repeated
`C`, alternate Cross `K`, and D-pad Down `S` on the exact clean title menu did
not yield a reliable visible selection change.

The earlier Adventure transition is retained only as diagnostic evidence
because the action did not repeat consistently after the clean rebuild.
Keyboard capture was explicitly released before app termination. No iPad
keyboard delivery is accepted, and no claim is made for a physical Magic
Keyboard, Bluetooth keyboard or full iPad menu/gameplay path.

### Exact clean cross-target matrix

All producers were rebuilt after `98ae2c6d86fe` so the embedded source
identity matches the implementation under test:

```text
macOS ARM64 desktop GL app
  CTest:       18/18 in 3.58 seconds
  signature:   strict deep ad-hoc verification passed
  SHA-256:     ee690c9fc934c4a4735f1373b41a9a2dc5f479c706be136203daec6c4e8bfed1

macOS ARM64 GLES configuration
  CTest:       18/18 in 1.60 seconds
  runtime:     expected clean diagnostic exit 1; Cocoa ANGLE/EGL absent
  SHA-256:     1d9fb361edcf7442cc70338d7c2881aef6a7192ce0b2171d531dabf59f2bbfa4

combined ASan/UBSan ARM64
  CTest:       18/18 in 10.82 seconds
  leak detect: disabled because unsupported by Apple's ASan runtime
  finding:     none
  SHA-256:     33ad829f4f1d0baa51a20ed3f1dad032ef7b04badbc453a32ceb2cf5be4dc700

iOS Simulator ARM64
  platform:    IOSSIMULATOR, iOS 15.0 floor, SDK 26.5
  live:        launch, GLES shaders/VRAM, audio, pixels
  SHA-256:     9e09fb41b41ba63339e26ac733de31fc1b8c196f6a9089d97929d201b1709771

iOS device ARM64
  platform:    IOS, iOS 15.0 floor, SDK 26.5
  live:        not run; unsigned
  SHA-256:     09576e97b9bde31d89f41efcb52388777ed54f7330dcaf668cf9f5202f1d3f43
```

The exact Linux i686 rebuild was intentionally allowed to finish rather than
reusing an older binary:

```text
optimized Linux i686
  CTest:       18/18 in 4.23 seconds
  architecture: ELF 32-bit LSB PIE, Intel 80386
  interpreter: /lib/ld-linux.so.2
  GNU Build ID: dfac03fc776068dfd25ee53f0284975b1d914217
  SHA-256:     4bcc7844e9cd107ea0ddc67e734397a0df420d6b635843454975289742c21611
```

The pinned amd64 multilib builder mounted source read-only and output in the
disposable cached tree. The binary embeds `98ae2c6d86fe`; it is not the binary
inside the protected historical verifier.

The normal Apple/iOS compiles repeated 32 established warnings; the sanitizer
build repeated 59. The implementation introduced no newly accepted warning.

### Acceptance boundary and next dependency

This work accepts the first live Simulator GLES context and full title-menu
presentation, plus the source-owned app/preset boundary. It does not accept
M7 as a whole because representative renderer comparisons, state parity and
cadence are absent. It does not accept M8 because controller play, lifecycle,
display pacing, physical iPad install/execution and a complete race are absent.

The next product dependency is to correct/verify UIKit lifecycle and replace
host spin pacing with a display-driven path without changing the retail
VBlank state model. After that, physical-device controller/audio/video and
sandbox import/save work can be accepted honestly. Touch remains a later peer
input source, not part of this checkpoint.

### Elapsed time and protected concurrent work

The prior documented renderer checkpoint ended at 146,335 goal seconds. The
first documentation read for this slice was 150,428 seconds. The pre-commit
reading at 08:23:51 CDT was 150,887 seconds: 1 day, 17 hours, 54 minutes,
47 seconds cumulative and 4,552 seconds (1 hour, 15 minutes, 52 seconds) since
the prior checkpoint.

The independent historical i686 alternate-loader verifier was not used as the
exact-build matrix producer and was not changed. Docker reported it running,
unpaused and not OOM-killed. Its machine-owned exit-status file remained zero
bytes. No alternate playback completion, process exit, layout separation or
deliberate mutation is inferred.

## 2026-07-31 — UIKit lifecycle/display-loop implementation and exact live audit

### Scope and source boundary

This slice began at clean commit `cbdd58435173d4ba7786599973eea8278a54607c`.
The requested dependency was M8's application lifecycle: the previous iPad
Simulator package launched and rendered, but `CTR_Main` then owned an endless
loop, normal SDL quit called `exit(0)`, and background/foreground had no
explicit input, audio or timing boundary.

Retail execution continued to use the user's ignored file at
`ref/CTR/CTR - Crash Team Racing (USA).bin`. It was copied only into temporary
packages beneath `/private/tmp`; it was never staged. The repository status
was checked before source work and again before publication.

### Event-delivery investigation

The first proposed implementation polled mobile lifecycle events from the
retail host event loop. That was rejected after reading SDL's event contract:
will/did background, will/did foreground, low-memory and terminating events
are sent synchronously to event watches and are not added to the ordinary
queue. A queue-only implementation could therefore look correct in a unit
test and still miss every real UIKit transition.

`platform/native_platform.c` now installs `SDL_AddEventWatch` immediately
after SDL initialization and removes it before `SDL_Quit`. The watch reduces
each notification into a small lifecycle phase/action result. The reducer is
explicitly idempotent for paired `will`/`did`, duplicate notifications and
direct recovery events; host actions execute only when the reduction requests
them (`platform/native_platform.c:124-257,488-495,535-542`).

### Returning control to UIKit

`game/MAIN/MainMain.c` was separated at the existing native loop boundary:

```text
CTR_MainStep  one unchanged retail state-loop iteration
CTR_Main      desktop loop that repeatedly calls CTR_MainStep
iOS callback  one CTR_MainStep per SDL/UIKit animation callback
```

These boundaries are `game/MAIN/MainMain.c:58-99,512-527`,
`main.c:269-288` and `platform/native_platform.c:800-823`.

`main.c` installs the callback with `SDL_SetiOSAnimationCallback`, returns
from standard iOS `main`, pumps while inactive and stops the callback when a
cooperative quit is observed. Desktop behavior remains a native loop. This is
accurately described as a display-driven outer loop: `VSync` may still wait
synchronously inside one retail step, so a fully continuation-based scheduler
is not claimed.

The old SDL quit and window-close `exit(0)` calls were replaced with a
`quitRequested` flag. A desktop diagnostic received Ctrl-C, unwound through
the native loop, closed the log and exited 0. Exact deterministic coverage is
provided by the lifecycle self-test rather than by treating the dirty launch
as final evidence.

### Suspension invariants

The background action performs these host-only changes:

1. pause the SDL audio-stream device;
2. lock and clear queued PCM rendered before suspension;
3. clear quick-key and name-entry transport edges;
4. publish released active-low PSX snapshots, including centered axes; and
5. flush the platform log.

Foreground clears stale output again, clears transient input edges, rebases
the absolute host VBlank deadline and resumes audio. The game-visible VBlank
counter, callbacks, retail timers, emulated SPU/XA state and all retail state
remain unchanged. The next ordinary input poll resamples current keyboard and
gamepad state, preventing a pre-background hold from sticking while still
allowing a physically held control to become active again after resume.
The concrete audio and input boundaries are
`platform/native_audio.c:2436-2472` and
`platform/native_input.c:958-992`.

### Pacing audit and second spin discovery

The first implementation made the explicit iOS final spin window zero. A
second source audit followed the resulting delay call into SDL and found that
`SDL_DelayPrecise` sleeps only to the last sub-millisecond interval, then
busy-spins to its target. Merely setting the project spin constant to zero did
not satisfy the battery/host-cooperation objective.

The final iOS branch calls `SDL_DelayNS`, yielding for the complete remaining
interval. Desktop keeps the accepted `SDL_DelayPrecise` and bounded-spin path.
The rational NTSC deadline, late-frame catch-up rules and game-visible VBlank
model were not changed (`platform/native_platform.c:830-945`).

### Deterministic tests

CTest 19, `ctr_native_lifecycle`, exercises paired, duplicate and direct
background/foreground transitions, low memory, termination, refusal to resume
after termination, cooperative quit, paired audio actions and deadline rebase
with a preserved synthetic game-visible count. Its exact successful marker is:

```text
[CTR Lifecycle] self-test passed: background=idempotent foreground=rebase quit=cooperative audio=paired low-memory=flush
```

The established keyboard test was audited again in response to the request
for basic keyboard controls. No second input path was added: commit
`2c10b00b34df4f0eb61aa8b72cbe99588a930ed6` already maps the additive
two-hand layout into the production PS1 packet path
(`platform/native_input.c:290-389,583-696`):

```text
W/A/S/D    D-pad                 I/J/K/L    Triangle/Square/Cross/Circle
Q/E        L1/R1                 P          Start
Tab        Select                Ctrl/[ ]   L2/R2/L3/R3 as documented
```

The current `--self-test-input` checks all 12 additive aliases, held `K+D+E`
as Cross + Right + R1, and a complete `K+D` press/release latched into exactly
one retail snapshot. The earlier exact signed macOS run visibly moved the main
menu with `S`/`W`, accelerated with `K` and changed steering with `K+D`.
Therefore the requested basic keyboard function already existed and was
retained; the root README remains its user-facing map.

The current `afb5463cc511` ARM64 app then ran `--self-test-input` directly and
printed the complete `aliases=12 held=k+d+e alias-tap=k+d
virtual-gamepad=buttons+axes+rumble+hotplug` marker. The focused CTest repeated
1/1 in 1.44 seconds. README now calls out window focus and Simulator hardware
keyboard capture while keeping real-iPad keyboard acceptance explicitly open.

### Dirty live runs and rejected diagnostics

The first dirty Simulator build identified as `cbdd58435173-dirty` completed
two full Home/resume cycles before the hidden SDL spin was removed. A second
dirty build using the final yielding wait completed one full cycle and one
rapid transition in which UIKit sent will-background directly followed by
did-foreground. The reducer recovered once, matching its direct-transition
test.

LLDB attach stalled and left the app suspended; the orphaned debugger was
terminated and the process resumed. Two later `sample` attempts also blocked
without producing a report. No lifecycle or performance conclusion uses those
attempts.

The yielding diagnostic package was run with `--perf`. Its console-bound
process ended while CSV row 325 was being written, before a shutdown summary.
That incomplete row and the absent summary were rejected. Analysis used only
the 324 complete rows:

```text
average total                         119.983 ms (8.335 FPS)
average work                          109.354 ms
average renderer_draw_triangles_ms    105.157 ms
average VBlank wait                    10.629 ms
average swap                            3.364 ms
average framebuffer store               4.857 ms
maximum total                         519.623 ms
```

This assigns the Simulator miss to Apple Software Renderer's CPU triangle
submission rather than to the yielding wait. It is a diagnosis, not device
acceptance. The complete-row CSV remains local-only with SHA-256
`d0f1e10eb90de0696e74def5cf0cf789a49276147c0430b247ca04958d2fdd7a`.

### Exact clean Simulator process

Implementation commit `afb5463cc5115073be9427658df424a5d9c14092` was
created and pushed before exact-product validation. The exact Simulator app
was copied to `/private/tmp/ctrpad-ios-lifecycle-exact-cNF1CU/CTRPad.app`,
received a temporary retail copy and local ad-hoc signature, passed strict
deep signature verification, installed, and launched as
`io.github.chrissotraidis.ctrpad` on iPad Pro 13-inch (M5) Simulator
`D80E9862-C29A-4D69-B8E5-D81D396C17D5` running iOS 26.5.

It reported build `afb5463cc511`, a 1376-by-1032 UIKit surface, presentation
framebuffer/renderbuffer 1, Apple Software Renderer, GLES 3 / GLSL ES 3,
four PSX shaders plus VRAM pipelines, and 44.1-kHz stereo CoreAudio. Computer
Use drove two complete Home/background/foreground cycles. Each log sequence
was will-background, did-background, will-foreground, did-foreground with
audio suspended then active. After resume, live inspection showed first the
Crash/N. Tropy kart intro and then the Crash/trophy/checkered title scene;
geometry, colors, alpha and textures remained coherent without a black frame.

The 27-line exact lifecycle log remains outside Git with SHA-256
`30f5b5ec516ffae64f05dc23e4570b438cdadb15858189439749572c7d021ec3`.
The locally signed app executable hash is
`bffddcaf98a990c50d67f0b97457af15b271e69f73d3ab660c0b32a13114bb23`.
The final process was stopped with `simctl`; natural terminating-event delivery
is not inferred.

The same two `SDL_uikitviewcontroller` unbalanced appearance-transition
warnings repeated. A WebCore/WebKit duplicate accessibility-bundle warning was
classified as Simulator-runtime output, not an app finding. Rotation and
view-controller acceptance remain open.

### Exact source matrix and publication

Every exact producer was explicitly reconfigured after commit and embeds
`afb5463cc511`:

```text
macOS ARM64 desktop GL    19/19 in 1.07 s; strict deep signature; SHA-256 0804b67d...b697ec
macOS ARM64 GLES config  19/19 in 0.65 s; expected no-ANGLE exit 1; SHA-256 bf60b666...edcf6
ARM64 ASan + UBSan       19/19 in 6.28 s; no finding; SHA-256 6efa286e...7401
iOS Simulator ARM64      iOS 15 floor, SDK 26.5; pre-sign SHA-256 b1161f25...7c84
iOS device ARM64         iOS 15 floor, SDK 26.5; unsigned/unrun; SHA-256 8364ec5c...1546
optimized Linux i686     19/19 in 10.34 s; ELF32 Intel 80386; SHA-256 5f1f8b06...abdc65
```

The normal Apple/iOS compiles repeated 32 established warnings and the
sanitizer compile repeated 59. The i686 compile repeated four established
warnings and produced GNU Build ID
`d032b695e7957142bc16a754a8af8a2946dab2b5`. No new warning was accepted. The
detailed matrix, complete hashes and deliberately open gates are in
`docs/parity/2026-07-31-ios-lifecycle-display-loop.md`.

Source commit `afb5463cc511` was pushed to `origin/codex/arm64-apple` and draft
PR #1. It was not merged. Documentation publication is evidenced by the Git
commit history and the PR head after this journal entry, avoiding a circular
attempt to embed a documentation commit's own hash inside itself.

### Elapsed time and protected verifier

The previous documented checkpoint ended at 150,887 goal seconds. Exact
validation was read at 154,273 seconds: 1 day, 18 hours, 51 minutes, 13 seconds
cumulative and 3,386 seconds (56 minutes, 26 seconds) later. This timer is
cumulative task time, not a benchmark or labor estimate.

The pre-publication documentation, focused keyboard and optimized i686 audit
ended at 155,756 seconds (1 day, 19 hours, 15 minutes, 56 seconds). That
follow-up consumed 1,483 seconds (24 minutes, 43 seconds), making the whole
checkpoint 4,869 seconds (1 hour, 21 minutes, 9 seconds) after the preceding
150,887-second record.

The pre-existing alternate-loader i686 verifier in container
`ec58fcd7069c` was not paused, restarted, rebuilt or terminated. It remained
running and not OOM-killed; its machine-owned status file was zero bytes. The
latest playback log had crossed nine 2,000-frame windows, but completion,
independent-process/layout separation and deliberate mutation remain
unaccepted until that verifier writes its own final status.

## 2026-07-31 — iOS sandbox ownership and Documents-only retail launch

### Scope and initial audit

This slice started from clean, published source
`5d6a3baac1c0e96f8bf42098891b38ee1432c9e7`. The prior UIKit checkpoint could
launch and resume, but `main.c` still changed the process directory to the
asset base. On desktop that is a convenient portable layout; on installed iOS
it points at the read-only application bundle.

The investigation did not stop at the two paths named in the goal. It followed
all ordinary path owners:

```text
platform/native_assets.c           retail image/extracted reads
platform/native_log.c              Crash Team Racing.log
platform/native_memcard.c          memcards/slot*/BASCUS-94426*
platform/native_perf.c             performance CSVs
platform/native_replay_scheduler.c replay reports and sandboxes
platform/native_savestate.c        savestates
platform/native_renderer.c         screenshots and VRAM diagnostics
```

SDL source inspection confirmed that its shipped filesystem implementation
maps `SDL_GetPrefPath` to Application Support on Apple platforms,
`SDL_GetUserFolder(SDL_FOLDER_DOCUMENTS)` to the application Documents
container, and `SDL_CreateDirectory` creates parent directories. That made an
SDL-owned cross-platform boundary preferable to hard-coded Apple container
paths.

### Contract and implementation

The selected ownership policy is:

```text
iOS bundle                          immutable asset fallback
iOS Documents/CTRPad/assets        user-visible preferred retail input
iOS Application Support/.../CTRPad private writes and working directory
desktop selected asset base        assets plus existing portable writes
```

`platform/native_storage.c` owns normalization, directory creation, getters
and the desktop compatibility finalization. `NativeAssets_Init` now receives a
preferred base and validates it before the bundle/desktop fallbacks. If no
asset exists yet, iOS still selects the Documents base so the validation error
points at the location the user can populate. Startup resolves storage before
assets, switches to the writable root, then assigns absolute log and memory-
card roots before validation. This also makes remaining relative diagnostics
private on iOS without rewriting each diagnostic subsystem in the first slice.

`platform/apple/Info-iOS.plist.in` enables
`LSSupportsOpeningDocumentsInPlace` and `UIFileSharingEnabled`. These keys make
the Documents directory available through Apple's file-sharing surface; they
do not create the eventual in-app picker. CTest 16 exercises synthetic bundle,
Application Support, Documents, import, `memcards`, and Windows portable paths
without reading retail media.

The first source review found one indentation error in the self-test argument
dispatch and corrected it before commit. The full diff then passed
`git diff --check`. A live desktop startup using the ignored retail BIN kept
its base/assets/writable paths at `build-macos-arm64-app`, found the existing
save, initialized desktop GL/CoreAudio, and unwound cooperatively on Ctrl-C.
The pre-commit macOS app passed all 20 CTests.

### Diagnostic Simulator runs and rejected evidence

The first dirty Simulator product identified as `5d6a3baac1c0-dirty`. A
disposable package received a cloned local retail BIN and an ad-hoc signature.
A clean Simulator uninstall/install/launch proved that initialization creates:

```text
Documents/CTRPad/assets
Library/Application Support/chrissotraidis/CTRPad/Crash Team Racing.log
```

The log recorded GLES 3, four PSX shader modes, ready VRAM pipelines and the
UIKit display loop. This proved private writes but still used bundle fallback
for media.

The next intended test copied the retail file into Documents and installed the
3.4 MB build output over the app. `simctl` migrated the Documents file to a new
data UUID, but inspection caught that the installed application still retained
the earlier 578 MB bundle asset. Its resource seal also mismatched. That run is
rejected as Documents-only evidence. Recording this failure matters: relying
only on the selected `Base` line would have hidden an incremental-install
artifact.

The corrected procedure was:

```text
ditto <clean app> <temporary app>
codesign --force --sign - <temporary app>
codesign --verify --deep --strict <temporary app>
simctl uninstall <simulator> io.github.chrissotraidis.ctrpad
simctl install <simulator> <temporary app>
simctl get_app_container <simulator> <bundle-id> app|data
copy ignored BIN only to Documents/CTRPad/assets/ctr-u.bin
simctl launch --console-pty <simulator> <bundle-id>
simctl io <simulator> screenshot <local-only PNG>
```

Inspection proved the installed bundle was 3.4 MB and had no asset directory.
The console then selected Documents for `Base` and `Assets`, Application
Support for `Writable data`, and successfully initialized retail rendering and
audio. The 2064-by-2752 screenshot showed the textured Naughty Dog crate,
including wood grain, braces, logo texture, shaded side face and starfield.
The raw Simulator PNG was rotated 90 degrees relative to the landscape app;
this supports the texture claim but keeps orientation open.

### Incremental source publication and exact rebuild

After pre-commit validation, only the seven intended implementation files were
staged. GitHub CLI 2.96.0 was authenticated, and commit
`02a6623f80a0f0999165f97f56c69a7cde64b4aa` was created at 10:06:21 CDT with
message `feat: split iOS assets and writable storage`. It was immediately
pushed to `origin/codex/arm64-apple`; local and remote hashes matched. No
retail/media path appeared in the staged file list.

Every Apple producer was then reconfigured rather than merely relinked:

```text
cmake --preset macos-arm64-app
cmake --build --preset macos-arm64-app --parallel 3
ctest --preset macos-arm64-app

cmake --preset ios-simulator-arm64
cmake --build --preset ios-simulator-arm64 --parallel 3

cmake --preset ios-device-arm64
cmake --build --preset ios-device-arm64 --parallel 3
```

All binaries embed `02a6623f80a0`. The desktop app passed 20/20, strict deep
signature verification and plist lint, and is thin ARM64. Both iOS binaries
are thin ARM64, have iOS 15.0 floors against SDK 26.5, carry the Files keys,
and identify the correct Simulator/device platform. Normal Apple compiles
repeated the established 32 warnings.

Fresh GLES and sanitizer trees were configured independently. GLES passed
20/20, then a retail-backed diagnostic exited 1 cleanly because the host's
Cocoa backend could not initialize its GLES library. Combined ASan/UBSan passed
20/20 with `detect_leaks=0`, `halt_on_error=1`, `abort_on_error=1` and
`print_stacktrace=1`; there was no sanitizer finding. The sanitizer compile
repeated the established 59 warnings. Exact hashes are recorded in the parity
report rather than abbreviated here.

### Exact committed Documents-only repeat

The exact Simulator build was copied to
`/private/tmp/ctrpad-ios-storage-exact.Cb5A7b/CTRPad.app`, locally signed and
strictly verified. Before the update, the Documents import was 605,698,800
bytes at data-container UUID `39CA500F-9766-4130-B86C-709CE96B1EC7`.
Installing the exact app migrated data to UUID
`13DFD41B-6192-4DB1-946C-5964EC8CA796`; the import retained both its size and
inode. The installed app still measured 3.4 MB with no `assets` directory.

The exact console identified `02a6623f80a0`, selected the migrated Documents
file, assigned Application Support as writable data, initialized a 1376-by-
1032 GLES surface, all PSX/VRAM shaders and CoreAudio, and emitted 9.27, 7.15
and 8.61 FPS windows. The private 794-byte log hash is
`6c2d5fc1...b3bb54a`; the exact local-only texture screenshot hash is
`7b194105...d25b88f`. Two unbalanced UIKit appearance-transition warnings and
the Simulator WebCore/WebKit duplicate-class message repeated. Ctrl-C ended
the console-bound process; process lookup confirmed PID 88996 no longer
existed. This remains bounded termination rather than a natural UIKit-event
claim.

### Open boundary and concurrent work

This slice accepts storage ownership, actual Documents preference, private log
creation, Files metadata, and app-update persistence of the imported document
in Simulator. It does not accept document-picker UX, invalid-image UX,
security-scoped access, physical Files behavior, game-driven iOS memory-card
creation/reload, background save atomicity, or M9 as a whole.

The exact optimized i686 build was started through
`CTRPAD_BUILD_JOBS=4 tools/build-linux-i686-baseline.sh`, which mounts source
read-only and uses a separate output directory. At this intermediate journal
point it remained in the long unity compile and was not counted as passing.
The historical verifier container `ec58fcd7069c` was not paused, restarted,
rebuilt, terminated or used as the new matrix producer.

The independent exact producer then completed all 20 tests in 3.85 seconds.
It emitted an ELF 32-bit LSB PIE for Intel 80386 with GNU Build ID
`e3fab55f8a436052e856dd313a72e08ef78f84b5`, embedded clean source identity
`02a6623f80a0`, and had SHA-256
`96158b047af41542bbe1797e1c16d4de5ecc0c51b358ba02ca06a5780b5bdd33`.
The build repeated only the four established i686 warnings. This is the new
matrix producer, not the executable inside the historical verifier.

The previous documentation checkpoint ended at 155,756 goal seconds. The
final exact-matrix reading at 10:26:43 CDT was 158,249 seconds, or 1 day,
19 hours, 57 minutes, 29 seconds cumulative. The interval was 2,493 seconds
(41 minutes, 33 seconds). This is the product-task timer requested for the
historical record, not a labor estimate or benchmark.

At that read-only audit, Docker reported historical container `ec58fcd7069c`
running, unpaused and not OOM-killed. Its machine-owned exit-status file was
still zero bytes. Playback 2 had emitted ten 2,000-frame FPS windows, most
recently 0.48 FPS, and its log had last changed at 10:18:13 CDT. These are
progress observations only; completion, alternate-layout separation and the
deliberate mutation remain unaccepted until the verifier writes final status.

## 2026-07-31 — Fresh-install iOS Files import and same-process launch

### Scope and prerequisite audit

This slice began from clean, published storage checkpoint
`02a6623f80a0f0999165f97f56c69a7cde64b4aa`. The iOS app could already create
`Documents/CTRPad/assets`, keep private state in Application Support and launch
from a manually placed retail image, but a fresh media-free install still
returned after its console validation message. The goal required a usable
first-launch import experience without weakening the production loader or
placing retail bytes in the bundle.

The audit followed three ownership problems before editing:

1. SDL owns iOS process entry and UIKit's long-lived application loop, so the
   picker cannot be implemented as a blocking call inside `SDL_main`.
2. `NativeAssets_Init` and `NativeAssets_Validate` held the only complete
   interpretation of raw sector format, disc identity and required content, so
   the UI must call that path rather than reproduce a partial checker.
3. `NativeDiscImage_Init` retained an open `FILE *`; a staged file cannot be
   moved reliably until that handle has an explicit release boundary.

The selected state machine split launch into asset selection and runtime
startup. A valid startup still enters the same path immediately. A media-free
iOS startup stores its launch options, creates a native coordinator, returns
from `SDL_main`, and lets UIKit remain responsive. Import success reselects the
now-installed Documents asset and calls the ordinary runtime startup routine;
it does not relaunch the app or create a second game initialization path.

### UIKit bridge and install transaction

`platform/apple/native_ios_import.m` is compiled with ARC only for iOS and
linked against UIKit and Uniform Type Identifiers. It creates a landscape
onboarding window with a short ownership/format explanation, a blue **Choose
CTR disc image** button, progress state, and stable accessibility identifiers.
The picker requests one generic data file with `asCopy:YES` so the original is
not modified.

The bridge balances `startAccessingSecurityScopedResource` when access is
granted and coordinates the Files read with `NSFileCoordinator`. Copy work is
dispatched away from the main thread. The selected file first lands at:

```text
Documents/CTRPad/.ctrpad-import-<UUID>/assets/ctr-u.bin
```

That unique path is on the same volume as the final destination. C then opens
the staged file through `NativeAssets_Init`, requires a readable raw image,
checks `SCUS_944.26`, and runs the full asset validator. The new
`NativeDiscImage_Shutdown` closes and clears the validation handle before the
bridge installs the file. An invalid stage is deleted and the existing import
is left unchanged. A valid first import is moved; a valid re-import uses
`replaceItemAtURL`; either route removes the staging root. Copy, coordination,
validation and final-install errors all return to an enabled chooser.

The user-facing outcomes are intentionally distinct:

```text
cancel             no file selected; choose again when ready
unreadable format  select raw MODE2/2352 BIN, not CUE/archive
wrong region       report detected identity; require NTSC-U SCUS-94426
incomplete image   required files missing; preserve existing import
copy/install error retain the localized filesystem failure
```

Only cancel, invalid format and valid input were exercised live in this slice.
The other implemented branches remain open rather than inheriting acceptance
from code inspection.

### Diagnostic Simulator interaction

The previous evidence Simulator was shut down without deletion. A separate
iPad Pro 13-inch (M5) Simulator named `CTRPad Import Validation`, UDID
`1D19A61F-20B7-46B0-AB52-B3A3406952E2`, was created on iOS 26.5. Its first boot
took 3 minutes 54 seconds because the Simulator performed first-use data
migration. This setup delay is recorded so it is not misattributed to CTRPad.

The first no-media launch visibly showed the navy onboarding view, white/yellow
instructions and blue chooser. The raw screenshot surface initially appeared
portrait-sized; rotating the Simulator produced a normal full landscape app
view. This is useful UI evidence but does not accept physical orientation
behavior. Computer Use selected the button and displayed the real Files picker.

Cancelling returned the exact ready-to-retry status. A local-only 118-byte
`not-a-disc.bin` fixture then exercised the invalid-format branch through
Files. The app showed the raw-MODE2/2352 correction, installed no destination,
and left no `.ctrpad-import-*` directory. A cold no-media relaunch returned to
the onboarding screen.

The ignored user source
`ref/CTR/CTR - Crash Team Racing (USA).bin` was made available in Simulator
Files without copying it into Git. It measured 605,698,800 bytes and SHA-256
`f780bf2331476aabfc00772fa758b12dd95ebfbc907968132cbd3cdd4e2c07c0`.
Selecting it through Files copied, staged, validated and installed the exact
same byte count and hash. PID `93200` remained alive from the no-media view to
the first game presentation; no relaunch or staging residue occurred. A later
cold launch selected the installed image directly.

Private runtime output identified a 1376-by-1032 surface, Apple Software
Renderer, GLES 3 / GLSL ES 300, the four PSX shader modes, ready VRAM pipelines,
the UIKit display loop and CoreAudio. Visible pixels were coherent. Simulator
cadence remained approximately 5–9 FPS and is not extrapolated to hardware.

### Implementation publication and exact rebuild

Only the six intended implementation files were staged. A repository audit
found no tracked disc/media extension, reference input, app package, screenshot
or generated build product. Commit
`7872f7e61ad658d1bdd941d0d8a8088740a61a12` was created at 11:07:21 CDT with
message `feat: import iOS retail media through Files` and pushed immediately to
`origin/codex/arm64-apple`. The draft PR remained the publication boundary and
was not merged.

The exact clean Simulator executable before signing had SHA-256
`aac8ff1a3903cf1ee083c82e830685ce072c1d43459c41e3936349000bbfa6e1`.
It was copied to `/private/tmp/ctrpad-ios-import-exact.N9bdqQ/CTRPad.app`,
ad-hoc signed, passed strict/deep signature verification, and installed with
signed executable SHA-256
`44e66cfc1383852a33fa5af4962e80a6da6af4979a2fae637b008c0869de209a`.
The installed product matched that hash, embedded `7872f7e61ad6`, was thin
ARM64 and reported IOSSIMULATOR platform 7, iOS 15.0 minimum and SDK 26.5.

Before exact import, the prior destination was preserved outside Git under a
different filename. Installing the exact app migrated its data container to a
UUID beginning `A62A57C4`. The exact onboarding and picker imported the retained
user source. Destination size/hash matched, no staging root remained, and PID
`99595` remained continuous into the first game view. The retained local-only
2064-by-2752 capture visibly showed coherent checkered-flag texture and has
SHA-256 `e83b12b1551a9f5e62915f6ed4e401433da0070fe11c1e556cfc609040be36d9`.
Its portrait raw orientation is not a landscape/rotation acceptance claim.

An exact cold console launch selected Documents and Application Support,
reported build `7872f7e61ad6`, initialized the same GLES/VRAM/audio path, and
emitted a 7.01-FPS first window. Ctrl-C bounded the console session; PID `1868`
was confirmed absent afterward, so natural UIKit termination is not inferred.
The Simulator was shut down without deleting its retained app data.

The exact source matrix at this point was:

```text
macOS ARM64 app       20/20 in 0.72 s; strict/deep signature; SHA-256 f488dc74261d...53b7e
ARM64 ASan + UBSan    20/20 in 3.88 s; no finding; SHA-256 c28e83e96807...36cb
iOS Simulator ARM64  iOS 15 floor, SDK 26.5; signed; SHA-256 44e66cfc1383...209a
iOS device ARM64     iOS 15 floor, SDK 26.5; unsigned/unrun; SHA-256 6350c11ca34a...eff
```

The normal Apple/iOS compilation repeated 32 established warnings. The
Objective-C bridge introduced no new warning. The device executable is
architecture/package evidence only. The independent optimized i686 producer
was started in new ignored directory `build-linux-i686-import` with the source
mounted read-only; it did not use or modify the protected baseline output or
historical verifier.

### Warnings, evidence limits and next dependency

Simulator output retained the missing-scene-configuration and minimal-bundle
`Assets.car` messages, the future `UIRequiresFullScreen` warning, two unbalanced
appearance-transition warnings, and the WebCore/WebKit duplicate accessibility
class warning. Files' `asCopy:YES` behavior also created its normal Inbox copy
before CTRPad staged and installed the selection. None is concealed or treated
as physical-device evidence.

This checkpoint accepts fresh onboarding, picker presentation, cancellation,
invalid-format rejection, a full valid production-loader import, validate-
before-replace behavior, same-process startup, cold relaunch and visible
Simulator textures. It does not accept physical Files/signing, interrupted or
backgrounded import, live wrong-region/incomplete/inaccessible failures,
game-created iOS memory cards, background save atomicity, hardware cadence,
controller/device play, touch controls or sideloadable distribution. M9 is
therefore still in progress. The next storage dependency is a game-driven iOS
memory-card create/relaunch/background-persistence test.

The independent optimized Linux i686 producer then completed. Configuration
under amd64 emulation took 1,694.9 seconds because SDL compile-probed libc,
math, CPU and platform facilities individually. The build used the existing
`ctrpad-linux-i686:ubuntu-24.04` image but a new ignored
`build-linux-i686-import` output directory and read-only source mount; it never
touched the protected producer or its live container. All 20 tests passed in
4.04 seconds. The result is an ELF32 Intel 80386 PIE with GNU Build ID
`063a0ff1b696ce52a52333233d92b1cbdf4a6c7c`, embeds clean identity
`7872f7e61ad6`, and has SHA-256
`361d313ea607bc971dacda8da2661eea161d759780913b4b7599c5b85fabab12`.
The compile repeated only the four established i686 warnings. Toolchain was
GCC 13.3.0, CMake 3.28.3 and Ninja 1.11.1; the package manifest remains beside
the ignored build.

## 2026-07-31 — Historical alternate-layout verifier completed naturally

### Completion appeared without intervention

After the isolated import matrix finished, a read-only `docker inspect` for
the protected container returned “No such object.” A complete `docker ps -a`
also contained neither ID `ec58fcd7069c` nor name `exciting_gagarin`. No stop,
pause, restart, rebuild, delete or termination command had been issued in this
slice. The verifier used `docker run --rm`, so disappearance was consistent
with normal shell completion but was not accepted until its machine-owned
status and artifacts were inspected.

The status path preserved when the original terminal watcher was attached was:

```text
/private/tmp/ctrpad-i686-cutscene-run-4hjAQW/debug/reports/
  20260731/ctr-025812/container-exit-status.txt
```

It changed from the previously documented zero-byte file to exactly two bytes,
`0\n`, at 12:05:03 CDT. `playback-1.log`, `playback-2.log`,
`playback-mutated.log` and `mutation-frame.txt` all existed. This is the
script's successful terminal state, not an inference from container absence.

### Two unchanged processes and address-layout separation

Both ordinary playback logs ended with
`[CTR Replay] replay finished after 24232 frames` and a closed-log marker. The
first process restored the same raw address-bearing checkpoint checksum as the
recording. The copied i386 loader produced a different raw checksum and
different host addresses:

```text
direct raw:     recorded=0xd4c950a8 restored=0xd4c950a8 equal=yes
direct hosts:   sdata=0x403cd040 gGT=0x403d6bf4 mempack=0x408c8bc0

alternate raw:  recorded=0xd4c950a8 restored=0x46478f61 equal=no
alternate hosts:sdata=0x3efaf040 gGT=0x3efb8bf4 mempack=0x3f4aabc0
```

The distinct raw representation proves that playback 2 did not merely repeat
the first process layout. Both still completed all canonical comparisons, so
host address/raw-checkpoint differences remained excluded from game-visible
parity as designed.

The scripted mutation automatically selected the first frame with an active
driver, frame 1,711. It changed `driver[0].posCurr.x` from `-165632` to
`-165631`, reported divergence at exactly frame 1,711, and named
`drivers mask=0x00000004` as the first canonical difference. Timing, RNG,
world, allocation, pad checksum and VBlank packet semantics remained equal at
that boundary. The internal verifier required that process to exit 2 before
writing final status 0 for the full three-operation shell.

### Finalize-only recovery verification

The report intentionally lacked the manual coverage form because that scenario
coverage was already accepted separately. The repository's
`CTRPAD_FINALIZE_ONLY=1 CTRPAD_REQUIRE_COVERAGE=0` recovery path was therefore
run from a clean detached `7872f7e61ad6` worktree with explicit immutable
producer, copied loader, toolchain manifest, source commit and report paths.
Finalize-only launched no game. It rechecked source/binary identity, 24,232
frames, 81 checkpoints, status 0, two normal completions, distinct raw and host
layouts, automatic mutation frame and `drivers`-first divergence. It exited 0:

```text
Replay process-determinism and mutation verification passed.
```

It then wrote only hashes/environment manifests into the ignored evidence
directory. The temporary detached worktree was removed after success. Exact
identity is:

```text
source commit           eee2a8df5b9605d27c7b20e943bba76174a4f6fc
producer SHA-256        d2e6f06023ccaedae689f11b36b33e005cb30d7bbc70d2a5e3e036f57b276c8e
alternate-loader SHA    eccfafa93226e52e32aff3f97f5f779e7d02306cda017eae6d9c358142d4278d
playback-1 log SHA      26a801935f3b0ed3749c77047b982d9fa0a6f59c4fd9830e8bf7871b27589249
playback-2 log SHA      a579d85653751c322a3232a73aa6d4785ca3ffd4525929f7ecf8ea2cee24f728
mutated log SHA         affa27f8e1f1673dbff38d2f3a160b0948e0bbe5a255a0de2547fd64b1f02c8c
evidence manifest SHA   19fce2859213c28f9cc5ad6c7a28981b6f9ac1c9db7abf0c52dffc3a54bfdcdf
container image         sha256:633753dde557f377e580536a32acefb4d11ac6fc8621644602acbddc6f1c6cb5
```

This finally accepts the independent-process/layout and mutation portion of
M1. Together with the already accepted manual coverage form and full
same-commit ARM64/i686 all-eight-component match, M1 is complete. M6 remains
open for broader human/audio/renderer/physical-controller/full-race,
savestate, and natural-quit product evidence. Raw reports, retail bytes,
memory-card contents and verifier logs remain local ignored evidence.

### Elapsed-time ledger

The previous published storage checkpoint ended at 158,249 goal seconds. The
final import/matrix/verifier/documentation reading at 12:16:04 CDT was 164,821
seconds, or 1 day, 21 hours, 47 minutes, 1 second cumulative. This checkpoint
therefore added 6,572 seconds (1 hour, 49 minutes, 32 seconds). The elapsed
figure is the Codex product-task timer requested for the repository's historical
record; it is not a build benchmark, active CPU duration or labor estimate.

## 2026-07-31 — Atomic memory-card writes and clean iOS save campaign

### Resumed from the published import/parity checkpoint

The branch began this slice clean at pushed commit `5fc4237fe`. The user asked
for continued work, frequent GitHub backup, complete process history, visible
game status, and basic keyboard testing. The practical keyboard layout was
already present at pushed commit `2c10b00b34df`: `WASD`, `IJKL`, `Q/E`, `P`
and Tab supplement the original input map. README and the automated input
oracle already describe the aliases, one-snapshot tap latch, and simultaneous
`K+D+E` chord. No separate keyboard physics path exists.

The next unclosed storage risk was not path ownership but update atomicity.
`NativeMemcard_WriteSaveData` opened the final save with `"wb"`, wrote icon and
payload, and then closed. That sequence truncated an earlier valid save before
the new data was durable. Backgrounding or termination at the wrong point
could therefore convert a recoverable old save into a partial or empty file.

### Implemented an old-or-new replacement boundary

The production writer now creates
`.ctrpad-<final filename>.tmp` in the same directory as the final save. It
writes icon and payload bytes, calls `fflush`, then uses:

```text
Apple       fcntl(F_FULLFSYNC), with fsync fallback
POSIX       fsync
Windows     _commit(_fileno)
```

After a successful close, POSIX uses `rename`; Windows uses
`MoveFileExA(MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)`. Any open,
write, flush, close, or replacement failure removes the temporary path and
leaves the existing final path alone. Negative sizes, missing required
buffers, invalid paths and temporary-path overflow fail before opening a file.
Zero-length icon/data sections avoid passing null buffers to `fwrite`.

CTest 17 owns an isolated memory-card root and performs initial write/read,
replacement/read, no-temp verification, injected temporary-path open failure,
old-payload preservation, fixture removal, successful retry, and full cleanup.
The injected failure uses a directory at the exact temporary filename, so the
production `fopen` returns `NATIVE_MEMCARD_OPEN_FAILED` while the final file is
still readable. The required marker is:

```text
[CTR Memcard] atomic-write self-test passed: write=flushed replace=atomic failure=preserves-existing temp=clean retry=checked
```

The first draft selected SDL temporary/process helpers that are not in the
vendored 3.4.10 API. That draft did not compile and was replaced with the
platform primitives above. A later direct i686 compile found strict-C17 libc
had hidden the `fileno` declaration. Adding the explicit non-Windows POSIX
prototype made the exact flags pass with
`-Werror=implicit-function-declaration`. Both corrections happened before the
commit and remain documented rather than silently discarded.

### Clean validation and publication

The final matrix was:

```text
macOS ARM64             21/21 in 2.00 s
ARM64 ASan + UBSan      21/21 in 10.63 s, fail-fast, no finding
i686 writer compile     clean under exact optimized flags plus implicit-error
iOS Simulator ARM64     linked, 32 established warnings
iOS device ARM64        linked, 32 established warnings
```

The ordinary macOS compile repeated 32 established warnings and sanitizers
repeated 59. No Windows compiler was installed, so only the Win32 API/source
path was audited. A direct self-test run emitted the exact marker and left no
root or temporary residue.

The source audit contained only `CMakeLists.txt`,
`include/platform/native_memcard.h`, `main.c`, and
`platform/native_memcard.c`: 207 insertions and 6 deletions. Commit
`4b078065ff03b71e02ce7b8a5b03351a777e2830` (`fix: write memory cards
atomically`) was pushed immediately. Local HEAD and
`origin/codex/arm64-apple` matched and the worktree was clean. A transient TLS
handshake timeout affected one explicitly repository-scoped PR metadata read;
it did not affect the push or mutate GitHub. An earlier unscoped `gh pr view 1`
looked up an unrelated upstream closed PR and was discarded as context, not
treated as the downstream review state.

### Built and inspected the exact iOS product

The clean Simulator product embedded `4b078065ff03`. Pre-sign executable
SHA-256 was
`e7e3faac7e0e719a0e4e2dbf823ca63212a2b41497fe048c84b5ac2c8655c83f`.
The disposable app at
`/private/tmp/ctrpad-ios-atomic.CR1jWC/CTRPad.app` was ad-hoc signed, passed
strict/deep verification, and had signed executable SHA-256
`848c18d5692634413b47b553f5fb2e9887dba12ce8bf8e1474b7ca87684568ac`.
It was thin ARM64 and installed with the same executable hash. The retained
Simulator data migrated between app-container UUIDs while preserving the
605,698,800-byte imported BIN.

Visible Computer Use inspection showed coherent Naughty Dog crate,
checkered-flag, trophy/Crash title/menu and attract-mode frames. This directly
answered the user's screen/texture question: exact-product presentation is
working. Software-renderer cadence remained roughly 5–9 FPS and is not a
physical-device performance claim.

### Bounded the Simulator hardware-keyboard question

Simulator's **Capture Keyboard** and **Connect Hardware Keyboard** options were
enabled. System logging showed SDL/UIKit recognized a `Generic Keyboard`, and
the bundle already set `UIApplicationSupportsIndirectInputEvents=true`.
Computer Use sent aliases and originals (`S`, `K`, `P`, `C`, arrows and
Return) to the Simulator and to its exact active application path. No menu or
game response followed.

A temporary iOS-only log in `Platform_PollHostEvents` then counted events at
the production SDL boundary. The same key attempts produced zero diagnostic
events. The log line was removed immediately, the exact clean package was
reinstalled, and the worktree returned to clean. No diagnostic source or
commit remains. This proves only that Computer Use host-key injection did not
reach SDL in this environment. The mapping remains accepted through automated
and live macOS evidence; physical-iPad keyboard delivery remains unaccepted.

### Rejected the incompatible checkpoint shortcut

The accepted clean ARM64 parity report has a checkpoint at frame 22,200, just
before its game-created save at approximately frame 22,392. A local diagnostic
copied that replay/checkpoint into the Simulator sandbox and attempted
`--replay-start-checkpoint 74 --replay-bypass-header` to avoid waiting through
the software renderer.

The runtime explicitly logged a producer/live identity mismatch:

```text
producer build eee2a8df5b96
live build     4b078065ff03
checkpoint/native-state sizes equal
identity and executable fingerprints different
```

It also reported raw checkpoint checksum `0x5c97bc36` versus restored-process
checksum `0x8b45044a`. `docs/REPLAYS.md` already says header bypass is
diagnostic-only and does not make address-bearing checkpoints portable across
rebuilt binaries. The route was therefore outside acceptance before gameplay
continued.

The process then received `SIGSEGV` before any memory-card write. Crash report
incident `68888543-7A98-49CE-9588-D153506B11B6` faulted at address
`0x23be8dfc` in `VehBirth_SetStartlinePosition +172`, specifically the
`level->DriverSpawn[spawnIndex].pos.x` read. That low old-process-shaped
address is consistent with the forbidden checkpoint reuse. The local report
path is
`/Users/chrissotraidis/Library/Logs/DiagnosticReports/CTRPad-2026-07-31-131048.ips`
and its SHA-256 is
`929abb4dfaac6bee74989bff92d37c5bbcb7d24429f808bb4ca7a9db8594fd7c`.
No playback memory-card file existed. This is a rejected shortcut, not a save
failure or ordinary current-source iOS crash.

### Started a clean frame-zero iOS save run

The replacement uses `--record-from-replay`, which consumes only the accepted
24,232-frame pad stream and generates fresh current-process checkpoints and a
fresh isolated memory card. It restores no old checkpoint. At 13:14:59 CDT,
exact PID `36490` started and created report
`debug/reports/20260731/ctr-131503` inside private Application Support.
Startup selected the retained Documents BIN, initialized a 1376-by-1032 UIKit
surface, Apple Software Renderer, GLES 3.0 / GLSL ES 300, all four PSX shaders,
VRAM pipelines and CoreAudio. It created empty `memcard.seed` and
`memcard.recording` directories and fresh rolling checkpoints.

At 13:17:35 CDT the process was still alive. Checkpoints 0 through 3 covered
frames 0, 300, 600 and 900. Early cadence settled from 20 FPS during initial
screens to roughly 6–9 FPS under the software renderer. The game-driven save,
temporary-file residue check, background/foreground cycle, cold read, final
hashes and process duration remain in progress at this intermediate journal
entry. No completion is inferred from process liveness.

### Elapsed-time intermediate reading

The previous published timer was 164,821 seconds. The clean source/remote audit
at the start of this continuation read 168,298 seconds: 1 day, 22 hours,
44 minutes, 58 seconds cumulative, adding 3,477 seconds (57 minutes,
57 seconds). The reading includes the paused/resumed task lifetime and is not
a benchmark or person-hour estimate. A final reading follows the live run and
documentation publication.

## 2026-07-31 — Save completion, native touch implementation and live cadence diagnosis

### Let the clean frame-zero persistence run finish

The exact `4b078065ff03` process continued without checkpoint restore or
identity bypass. At 13:54 CDT it visibly reached Roo's Tubes in the textured
track menu. The local-only screenshot was 743 by 1018, 144,651 bytes, and had
SHA-256
`4beff345e9861db4c1c4f3a600b5767f5d3c18a842be06e4e9afa4938b1d41e3`.

At 14:22:16 CDT, after checkpoint 74/frame 22,200 and before checkpoint
75/frame 22,500, the game created
`debug/reports/20260731/ctr-131503/memcard.recording/slot0/BASCUS-94426-SLOTS`.
The accepted properties were:

```text
size                 6016 bytes
icon/payload         256 / 5760 bytes
blocks               1
profile version      -18
profile size         0x1600
CRC remainder        0
SHA-256              6a01b0f5562ed7a279d8f8e51e3b1874ac39a6120f55db4fe3873288950619a3
temporary residue    none
```

The app was sent Home only after the save appeared. PID `36490` remained alive
at approximately 3.3 percent CPU; inode, length, modification time and hash did
not change. The log emitted `will-enter-background` and
`did-enter-background` with audio suspended. Reopening the app recorded the
foreground transition and later active audio. The resumed frame visibly
contained coherent kart, terrain, particles, HUD and minimap. Its local-only
743-by-1018 JPEG was 124,957 bytes with SHA-256
`ce9c3a0a8d5f0421434ee000c60c8940ff41daa2041b57c021b94dd48419a901`.

The run ended naturally at 14:27:21 CDT. Metadata recorded 24,232 frames, 81
checkpoints, `finalized=1`, `recording_status=finalized` and build
`4b078065ff03`; the log reported the 24,232-frame replay-seeded finish and then
closed. Final local artifact hashes were:

```text
save       6a01b0f5562ed7a279d8f8e51e3b1874ac39a6120f55db4fe3873288950619a3
input      a471aa692c39a6d62813cd9d41acec9a3fb6e4893ac29ecb50ce6164112fb411
states     e52598538ef0e8b490560dd38024279289c46ec13e7c7021882e32af45e92965
metadata   90421662a92a44668c268a17fe75930b2a7caf63b12a703852e58c5db4211e91
log        02b5f482d548a1c870e653cdb66b0211c260ae3cff4c2f18184d95e75cf923ed
```

The UIKit main loop kept the completed process alive and idle, so it was
terminated through `simctl` only after the report's closed state and hashes
were inspected. No source or data was mutated by that termination.

To isolate the production cold-reader boundary, the game-created bytes were
then copied explicitly from the isolated recording root to the default private
memory-card root. Size and SHA remained identical; the inode changed, as
expected for the copy. This is a disclosed test fixture step. It does not claim
that reports automatically promote saves or that a natural non-report run had
written the default path. Later app installation migrated the Simulator data
container while retaining the imported BIN and this default save unchanged.

### Added a native touch peer instead of a second gameplay path

The first touch source checkpoint, `c496c27f04c878fdd27af78749ed82ea3618341e`,
adds a UIKit overlay only on iOS/iPadOS. It attaches after SDL establishes its
root controller, uses safe-area constraints, keeps the center transparent, and
provides accessibility labels/identifiers. Controls are an analog stick, Gas
(Cross), Brake (Square), Item (Circle), View (Triangle), L/R Drift/Boost
(L1/R1), Pause (Start) and Select.

The portable input API exposes enable, button, left-stick and reset calls.
Touch intent is composed after controller and keyboard mapping into slot zero:
buttons AND in active-low form, while an active touch stick replaces only left
axes. Physical-controller buttons and right axes remain intact. Replay input
remains authoritative. Touch contacts reset on disable, suspend/resume,
shutdown, checkpoint/state restore and replay installation so UIKit state
cannot leak into deterministic artifacts.

The first Objective-C compile rejected five calls to
`NSLayoutDimension constraintEqualTo:`. They were corrected to
`constraintEqualToAnchor:` before source acceptance. The initial exact matrix
then passed:

```text
macOS ARM64 Release     21/21 in 4.60 s
ARM64 ASan + UBSan      21/21 in 6.38 s, no finding
iOS Simulator/device   both linked thin ARM64, iOS 15.0 / SDK 26.5
```

Base executable hashes were `d0e80d00...7495` for macOS Release,
`fd1cee98...72d3` for sanitizer, `3ea608d1...d75e` for unsigned Simulator and
`5c0ea2fc...34a` for unsigned device. The ad-hoc signed Simulator executable
was `e0bf890e...6576`. Established warnings were 32 ordinary/iOS and 59 under
sanitizers; no new finding was introduced.

Computer Use inspection of that exact app showed the transparent overlay over
coherent Sony, Naughty Dog crate, CTR title and seven-row menu content. Gas
selected Adventure. Short stick drags did not move the menu. That observation
was retained as a failed product test, and source inspection confirmed the
retail menu reads D-pad bits rather than analog steering.

### Diagnosed the still-missed menu edge at the retail packet boundary

The stick gained a direction ring at 68 percent of its radius. Crossing the
ring emits active-high D-pad intent, with release/press deltas, while the
continuous signed analog vector remains active for races. A first live trial
still missed a bounded Down tap, so LLDB was attached to the running Simulator
process instead of guessing.

The touch callback entered `Platform_InputTouchButton(buttonMask=64, down=1)`.
At the inlined `NativeInput_ConsumeTouchButtons`, registers after the OR held
`w8=0x00000040` and `w10=0x00000040`. Immediately after
`NativeInput_ApplyTouch`, the eight slot-zero bytes were exactly:

```text
00 73 bf ff 80 80 80 80
```

That is a valid analog pad with active-low Down. At the next
`GAMEPAD_ProcessHold`, however, retail-visible controller bytes were again
`0xff 0xff`. A neutral host/display snapshot had overwritten the correct
one-shot edge before retail polled. This rejected both “the game is stuck” and
“UIKit did not deliver touch” as diagnoses; the defect was host-to-retail edge
duration.

The correction adds `s_keyboardLatchedButtonsNext` and touch
`latchedButtonsNext`. A press is exposed to two host snapshots, then removed
unless the button remains held. The retail-free oracle now requires both
snapshots to contain the tap and the third to be neutral. Keyboard aliases use
the same corrected contract; replay/state semantics remain unchanged.

The first CTest after this code change failed only because its PASS regex still
required the old `one-snapshot` wording. The self-test executable had emitted
success. CMake was updated to require the new two-snapshot/touch marker, after
which the suite passed. This was a contract-string mismatch, not a code-test
failure.

A concurrent macOS and iOS rebuild was cancelled after the compiler driver
appeared stationary at zero percent. Inspection showed the child `cc1` process
was actually CPU-active on the large unity translation unit. Later sequential
builds finished normally, disproving the initial lock suspicion; no source or
artifact was lost. A separate direct i686 compile first hit the project's
`internal` macro colliding with an SDL field. Pre-including vendored
`SDL3/SDL.h` before the project header matched the established compile route
and passed with strict implicit-declaration errors. `clang-format --dry-run`
was not promoted to a gate because its Objective-C mode was unavailable and it
also rejected baseline repository C format; diff, build and test gates were
used instead.

### Published and revalidated the exact correction

Commit `c783eda740c4cbfec538c276fa39220674810f64` (`fix: make touch menu
gestures reliable`) contains the direction ring, two-snapshot latch and updated
oracle. It was pushed to `origin/codex/arm64-apple`; local and remote-tracking
identities matched. Together with base commit `c496c27f04c8`, it remains on
draft PR #1 and is not merged to `main`.

Exact final validation was:

```text
macOS ARM64 Release     21/21 in 0.88 s; SHA c77d84b1...e642
ARM64 ASan + UBSan      21/21 in 5.98 s; SHA 69115f77...2cf4; no finding
i686 input TU           passed exact optimized C17 flags and implicit-error
iOS Simulator ARM64     SHA 27fc3a7d...bd6c3; thin; iOS 15.0 / SDK 26.5
iOS device ARM64        SHA 4211ceb7...a819; thin; iOS 15.0 / SDK 26.5
```

The exact local-only package is
`/private/tmp/ctrpad-ios-touch-exact.N9hvQT/CTRPad.app`. Its post-ad-hoc-sign
executable SHA is
`60a1373d13dcb66040b8ec504e6cd2970f7067f57c115ee116a1ecf7fab329f3`.
Strict/deep verification passed with identifier
`io.github.chrissotraidis.ctrpad` and no TeamIdentifier. This is a Simulator
test signature, not a sideloadable physical-device signature.

The exact app visibly repeated coherent presentation under the overlay. An
edge delivered immediately across a retail screen transition could be ignored,
so bounded retries were used only at transitions. On the stable submenu, Down
moved New -> Load on its first tap and Gas opened Load on its first tap. The
touch-only route displayed `CHOOSE A GAME TO LOAD`, profile `A`, Crash's icon
and populated counters, with other slots `EMPTY`. This proves the production
reader consumed the deliberately seeded accepted save bytes after an app
update.

Exact local-only evidence hashes were:

```text
main-menu JPEG      aa31f5d6a73523ff30516d4874a8907ebd9c22db9a68897a8dcd78eab1cd214a
load-profile JPEG   a1c5135dee001ead4b68d39d619157a45632c610df0b7ae20874efd4c01f700b
runtime log         99f5a1710c5adbcfc67c44f223f1b3496749adccac6d5d4c4c44e051c71a344a
```

Simulator cadence remained approximately 6–9 FPS under Apple Software
Renderer. The exact app was terminated after evidence collection. No retail
image, save, screenshot, log or app package was committed.

### Open device boundary and elapsed-time ledger

The controls are now functional and testable in Simulator, and the user can
also test the already published desktop keyboard aliases. Remaining M8/M9/M10
acceptance is physical: Apple development/distribution signing, actual iPad
installation, device cadence/thermal/rotation behavior, touch-target and
contrast review, a complete held-input touch race, and practical L/R
drift-boost execution. The goal remains active.

A read-only availability audit returned `No devices found` from
`xcrun devicectl list devices`, zero valid identities from
`security find-identity -v -p codesigning`, and no file in the standard local
provisioning-profile directory. No device, account or credential was mutated.
The existing unsigned ARM64 device product can enter the next gate only after
a physical iPad and Apple development signing identity/profile are available.

The previous published timer was 168,298 seconds. The pre-publication
documentation reading was 176,394 seconds: 2 days, 0 hours, 59 minutes,
54 seconds cumulative, adding 8,096 seconds (2 hours, 14 minutes, 56 seconds).
It includes paused/resumed task lifetime and is not a build benchmark or
person-hour estimate.

## 2026-08-01 — Custom controls, keyboard discoverability, and visibility-cache diagnosis

### Control-settings implementation and first backup

The continuation began from clean published documentation tip `e8e8e0e1f`.
The existing UIKit overlay had no preferences surface. A new modal form sheet
was implemented in `platform/apple/native_ios_touch.m` with clamped integer
preferences for left/right steering, three size choices and three opacity
choices. Rebuilding first neutralizes touch input, removes old controls, reads
preferences and recreates the overlay. Steering handedness mirrors the stick
and face-button constraints; both independent drift controls stay fixed.

The user's new keyboard request triggered an input audit before any second
event path was added. `NativeInput_DefaultMappings`, `NativeInput_ReadKeyboard`
and `NativeInput_ApplyKeyboard` already supplied the desired arrows/WASD,
ZXCV/IJKL, Q/E, P/Return and Tab/Space controls through the shared active-low
PS1 packet. The README and the exact 1,869-frame iOS keyboard report already
accepted that path. The correct incremental change was a visible/spoken key
legend in **CONTROLS**, not a UIKit keyboard bridge.

The first settings layout fit iPad but needed a scroll view and explicit
minimum heights for short iPhone landscape. The finalized sheet uses safe-area
frame/content guides; segmented controls and Reset are at least 44 points and
Done at least 50. Reset removes the three keys, updates the controls and posts
an accessibility announcement. Present/done/disappear each neutralize touch.

Dirty iOS Simulator and device builds compiled. The disposable
`CTRPad Import Negatives` clone alone was updated. Its pre-install canonical
identities were BIN inode `111450682`, 605,698,800 bytes, SHA-256
`f780bf2331...07c0`, and save inode `111309627`, 6,016 bytes, SHA-256
`6a01b0f556...619a`.

Computer Use opened the real sheet and selected Steer right, Large and High by
screenshot-grounded segment coordinates when segment children were absent
from AX. The underlying cluster mirrored, grew and brightened immediately.
After Done, Simulator focused protected `CTRPad Import Validation`; it was read
only and rejected as evidence. The Window menu refocused the disposable clone.

The refocused screen initially appeared to omit all trailing-anchored controls
in both orientations. This was not waved away. An LLDB attach stopped the app
without usable expressions; `SIGCONT` restored it, and that route was rejected.
A diagnostic include first named nonexistent `platform/native_platform.h` and
failed the build; it was corrected to `platform/native_log.h`. Live logging
then measured scene/window/parent/overlay `1376x1032`, safe frame
`(0,0 1376x1012)`, right stick `(1154,790 195x195)` and right drift
`(1195,11 161x60)`. All frames were inside. Fresh focused Computer Use state
showed all eleven controls; the apparent clipping was stale/wrong-window state.
All logging was removed.

The keyboard legend was visible and spoken. The sheet scrolled from its short
form to expose Reset and Done in AX. Reset restored left/standard/standard and
left an empty preferences dictionary. Done returned to animated gameplay.
An F9/P/F10 attempt was rejected because the ordinary launch lacked
`--record --toggle`; the accepted earlier keyboard report remained canonical.

Dirty Simulator/device/macOS builds passed. Desktop and ASan/UBSan matrices
each passed 22/22. Preservation hashes/inodes remained exact. The one-file
implementation was reviewed, committed as `828d095809fc` and pushed. Exact
directories were configured with that clean build ID. Simulator and iPhoneOS
products completed, but the five-target command was interrupted on the macOS
desktop compile when the user reported slowness and two Simulator devices.

### Resource correction

The disposable clone was shut down; only protected `CTRPad Import Validation`
remained booted. All compilers stopped. Remaining validation was retried with
nice priority 15 and one job. The compiler yielded at roughly 1–4% CPU, but
`vm_stat` later showed only about 64 MB of free pages, so it too was stopped.
Subsequent inspection showed Docker/virtualization and Jump Desktop activity,
but no unrelated user process was killed or changed. Work continued with
low-memory reads and patches only.

### User-visible missing geometry and exact failure signature

The user then asked whether the iPad screen clearly had many non-rendering
assets. The first protected screenshot was an expected checkerboard Loading
transition and was not counted. The next live demo frame retained kart/HUD
instances but showed the incomplete-scene symptom. A read-only query resolved
the protected app data container and audited its production log.

The 2,623-line log has exactly 268 AssetRef lines. Normalization/counting found
134 `level visibility cache exhausted` errors from `LOAD_TenStages` and 134
from `MainInit`, all capacity eight. There are no other missing-asset, model,
texture or asset-reference failure lines.

Source tracing established the causal chain:

1. LP64 `Level_GetVisMem` creates a host-sized `VisMem`/BSP sidecar in the
   fixed eight-entry `s_levelRuntimeVisMem` table.
2. `LOAD_Callback_LEV` invalidates only the exact new destination address.
3. `MEMPACK_PopToState`, `PopState` and `ClearLowMem` discard allocation
   ranges without removing sidecars keyed by old, differently placed levels.
4. The ninth distinct level returns null at both load and MainInit.
5. `MainInit_VisMem` leaves `gGT->visMem1` null.
6. `RenderAllLevelGeometry` immediately returns for null `visMem1`, suppressing
   BSP terrain/water/scenery while separate instances can remain visible.

The first correction draft invalidated all at ordinary load and the inactive
level at hub load. A complete call-site audit found `LOAD_Level`,
`CS_LoadBoss` and bookmark rollback paths, so the draft was superseded before
acceptance. The final correction adds `LevelRuntime_InvalidateRange` and calls
it from the three allocator operations before their pointer/bookmark mutation.
Address comparisons use `uintptr_t`; only sidecars within the discarded range
are freed. Hub replacement clears dangling `visMem2`; exact-destination and
global invalidation remain.

The asset-relocation self-test now registers nine level fixtures, fills all
eight cache slots, recycles by exact key, refills/recycles by contiguous range,
proves the five entries outside that range retain their exact sidecars, then
recycles all. CTest now requires the literal marker
`cache-recycle=targeted+range+all` so an early-returning test cannot pass on the
old prefix alone.

### Current boundary

The correction is source-complete and `git diff --check` passes, but its unity
compile was deliberately stopped for system headroom. It is provisionally
published as `eeaf2c72c` but is not described as accepted. The protected
Simulator remains untouched. Required next work is a low-impact build/test
window, then a disposable single-Simulator run through more than eight level
allocations with zero exhaustion lines and visual course-geometry evidence.
M7, M10 and the overall goal remain active.

A narrower validation route was considered after the stop: build only a
standalone `platform/native_asset_ref.c.o`. Ninja rejected the request with
`unknown target 'CMakeFiles/ctr_native.dir/platform/native_asset_ref.c.o'`
because this configuration compiles the project as one unity `main.c.o`.
No compiler launched and no extra memory pressure was created. The only
meaningful compile remains the deferred single-job unity target.

Timer reading at this interim boundary: 217,521 seconds, or 2 days, 12 hours,
25 minutes, 21 seconds cumulative. The prior published boundary was 212,262;
the interval is 5,259 seconds (1 hour, 27 minutes, 39 seconds). Goal time is
cumulative and includes pauses, diagnostics, resource throttling and this
record; it is not a person-hour estimate.

Before provisional source publication the goal API read 218,058 seconds, or
2 days, 12 hours, 34 minutes, 18 seconds cumulative. This is 537 seconds
(8 minutes, 57 seconds) after the interim reading and 5,796 seconds (1 hour,
36 minutes, 36 seconds) after the preceding published boundary. The reviewed
source/test/report scope was committed as `eeaf2c72c` (`fix: recycle level
visibility sidecars`) and pushed to `codex/arm64-apple`; publication is a
recoverability/review boundary, not substitute evidence for its pending unity
compile or live geometry gate.

### Remote publication and hosted-runner decision

The remaining controls/history documentation was committed as `2951c459e`
and pushed. A first `gh pr create` wrongly resolved the repository from the
`upstream` remote and failed with blank base/head and no-commit diagnostics;
the origin refs themselves were healthy. Retrying with explicit repository
`chrissotraidis/ctrpad` resolved already-open draft PR #1. Its pre-existing
description still asserted broadly coherent rendered textures, so it was not
left stale after the contradictory Simulator evidence.

The first REST description update used `--input -`; the non-interactive stdin
closed empty and GitHub returned HTTP 400 (`Body should be a JSON object`). No
PR mutation occurred. Retrying with a safely quoted `body` field succeeded.
The readback proved draft state, exact head `2951c459e`, the 268-error defect
text and the pending unity-build language.

Remote CI was evaluated as an alternative to pressuring the Mac. Repository
API reads found Actions enabled and zero self-hosted runners. Current official
GitHub runner documentation lists `macos-15` for private repositories as a
three-core M1 ARM64 VM with 7 GB RAM, but private jobs consume the account's
included allowance and can become billable. The billing endpoint was not
readable with the existing token and requested a `user` scope expansion. No
credential scope was expanded, workflow created or potentially paid job
triggered without user authority.

The final local resource reading was still only 3,943 free 16-KiB pages
(about 65 MB), with 11.79 GB swap used. Exactly protected
`CTRPad Import Validation` remained booted; no CMake, Ninja or Clang process
was running. The goal timer then read 218,564 seconds: 2 days, 12 hours,
42 minutes, 44 seconds cumulative, adding 506 seconds (8 minutes, 26 seconds)
after the prepublication reading and 6,302 seconds (1 hour, 45 minutes,
2 seconds) after the preceding published boundary.

### Recovered only in-scope container headroom

A later clean-tip audit rechecked the blocker instead of assuming the earlier
reading persisted. Head and origin were both `79f4b1bb2`; exactly the protected
Simulator was booted; no compiler ran. Free pages briefly measured 9,151
16-KiB pages (about 150 MB), with 11.69 GB of swap used. The largest single
process was a roughly 1.5-GB Virtualization VM.

Docker inspection separated ownership before mutation. Four healthy
`buzz-prod` relay/Redis/Postgres/MinIO containers were unrelated and left
untouched. Goal-owned `ctrpad-i686-debug` had been up for two days, but
`docker top` proved it held only `sleep infinity`, Xvfb and a stale diagnostic
shell looping on Xvfb. No recorder, playback, test or compiler process existed.
Its `/src` bind was read-only; writable `/out` mapped to host directory
`build-linux-i686-m3`; the retail BIN bind was read-only. Stopping this one
container therefore preserved source, output and media and remains reversible
with `docker start ctrpad-i686-debug`.

The four unrelated containers remained running after the stop. Free pages
settled near 5,864 (about 96 MB) and swap use near 11.66 GB. That remains too
little headroom for the unity `main.c.o`, so no compiler was started. The
protected Simulator and unrelated user applications/services were not changed.
At this boundary the goal timer read 218,822 seconds: 2 days, 12 hours,
47 minutes, 2 seconds cumulative, 258 seconds (4 minutes, 18 seconds) after
the publication-close reading.

The final `docker ps -a` state was `Exited (137)`: Docker escalated after the
idle Rosetta/Xvfb process group did not exit within the stop timeout. This does
not change the preservation conclusion—there was no verifier, source/media
were read-only and writable output was a host bind—but records the exact stop
result. Free pages then read 5,313 (about 87 MB) with 11.60 GB swap used. At
218,932 goal seconds (2 days, 12 hours, 48 minutes, 52 seconds), the same
build-headroom condition had recurred for the third consecutive goal turn.

## 2026-07-31 — Touch-only Time Trial, rotation and direct analog delivery

### Re-established exact starting state

The branch and remote-tracking branch both began at pushed documentation commit
`da151bfefb180b575d8d99f3713f4ed31dc6ec0e`, with a clean worktree. The
installed exact `c783eda740c4` touch executable still matched signed SHA-256
`60a1373d13dcb66040b8ec504e6cd2970f7067f57c115ee116a1ecf7fab329f3`.
Before UI work, the retained files were rehashed:

```text
retail BIN   605698800 bytes  f780bf2331476aabfc00772fa758b12dd95ebfbc907968132cbd3cdd4e2c07c0
default save      6016 bytes  6a01b0f5562ed7a279d8f8e51e3b1874ac39a6120f55db4fe3873288950619a3
report save       6016 bytes  6a01b0f5562ed7a279d8f8e51e3b1874ac39a6120f55db4fe3873288950619a3
```

The BIN and save inodes were `111131200`, `111222179` and `111221646`.
Later package installations migrated the data-container UUID several times but
preserved all three inode, size, timestamp and hash tuples.

### Entered Crash Cove with the overlay only

The exact app launched from the Simulator Home screen. View advanced the
presentation. At the stable main menu, a downward outer-ring stick drag moved
Adventure to Time Trial. The first two coordinate Gas clicks did not select;
after refreshing accessibility state, the exact `ctrpad.touch.cross` AX button
did. The same Gas control selected Crash, Crash Cove and No Ghost. View skipped
the fly-in and the app reached the normal Crash Cove start with lap 1/3, timer,
kart, HUD and minimap coherent.

This correction is operationally important: stale/coordinate actions were not
reported as input defects after a fresh AX press succeeded. Computer Use's AX
indices were re-derived after state transitions as required.

### Bounded acceleration rather than a fake human hold

Computer Use supplies click and drag actions, not separate long-lived pointer
down/up streams for two simultaneous human contacts. Twelve Gas AX taps took
about 67 seconds of wall time under the software renderer and advanced the
retail timer to `0:16:86`, but the kart remained close enough to the line that
the result was rejected as held-acceleration evidence.

Ten one-pixel in-button Gas drags advanced to `0:23:26` and moved the kart
slightly. Thirty additional contacts advanced to `0:44:53`. A 60-contact
bounded sequence then reached `1:02:13`; Crash had left the grid, the start
banner had moved overhead/out of the original framing and the minimap marker
had advanced. This is visually decisive forward movement through the real
touch button.

Twenty right-stick drags after the kart slowed did not show decisive heading
change. Thirty cycles alternating Gas and right-stick contacts continued the
world/minimap movement to `1:32:26`, but the final heading remained visually
ambiguous. Sequential desktop gestures were therefore not promoted to
simultaneous steering or drift evidence. The existing deterministic test is
still authoritative for analog snapshot values and multi-button/controller
composition.

### Pause, rotation and resume

The Pause AX button opened the retail Pause menu. The device had launched from
a portrait Home screen, so the first paused screenshot was a 743-by-1018 frame
with the 4:3 landscape game surface letterboxed inside portrait. The Simulator
Rotate control was then invoked while paused. UIKit produced a 932-by-768
landscape device; the game filled the screen and Auto Layout moved the stick,
face cluster, drift shoulders and top controls to their correct safe-area
edges. Gas selected the highlighted Resume entry and the timer continued.

Evidence stayed local-only:

```text
portrait Pause      102560 bytes  23a7f8d6944277c634d11879426fd6428abe69ba0d2a8c38d6c8de397cc6f2db
landscape Pause     148909 bytes  41949c131a1ffb0c64f92bc6f7b290d3cd84b6b3730ceeb0764b6767dd5fed80
movement 1:02:13    185907 bytes  d63fc9d41d70c0ed1a489d8c0ceff79554d7e544b7f591a94c65324ba62b3646
movement 1:32:26    177504 bytes  1d188f44b4bc8e4ecb40cc6df83b77094b29e66d464a37c6b09277231dc55a5f
```

### Rejected automatic-orientation experiments

The plist already permits only LandscapeLeft/LandscapeRight, sets
`UIInterfaceOrientationLandscapeRight`, requires full screen, and the platform
sets SDL's orientation hint before video initialization. Because a portrait
cold launch still letterboxed, a public UIKit helper was tested locally:

1. `UIWindowSceneGeometryPreferencesIOS` with the landscape mask immediately
   before attaching the overlay;
2. the same request from overlay `viewDidAppear`; and
3. the visible-stage request with only `UIInterfaceOrientationMaskLandscapeRight`.

Each route called `setNeedsUpdateOfSupportedInterfaceOrientations`, used
`requestGeometryUpdateWithPreferences:errorHandler:` on iOS 16+, and retained
the public `attemptRotationToDeviceOrientation` fallback for iOS 15. Simulator
and device ARM64 compiled each relevant variant with the 32 established
warnings and no new warning. No error handler fired. None rotated the portrait
cold launch.

The entire helper, log include, call and lifecycle override were removed with
`apply_patch`. `git diff --check`, empty `git diff` and empty `git status`
proved exact source restoration before acceptance work continued. The dirty
packages and public-API experiment are rejected evidence; only manual rotation
reflow is accepted.

### Exact rebuild and live analog callback

The clean branch tip was explicitly reconfigured so generated identity did not
retain a prior `-dirty` cache. The iOS Simulator build linked with 32
established warnings. Its unsigned executable SHA-256 was
`476ccfca0b052f5c0a2f0b6a510d5b813807ee5e83f2214a8c65efe7ec45f8a6`.
A disposable app at
`/private/tmp/ctrpad-ios-touch-race-exact.YmDYP2/CTRPad.app` passed ad-hoc
strict/deep verification and had signed executable SHA-256
`896a13d7f16ff0219730d8e319a9ef3a9585fbeddfce69870bc70bec5798457b`.
The installed executable matched and embedded `da151bfefb18` without `dirty`.

LLDB attached to exact PID `58958`. The first breakpoint command was
misconfigured with only an automatic `continue`; its four hits were discarded.
An interactive command list then printed ARM64 argument registers at the
out-of-line `Platform_InputTouchLeftStick` entry. A center-origin drag showed a
near-neutral active value and release. A click at the visible right edge gave:

```text
w0 x       0x00007ffe = 32766
w1 y       0x000000fa = 250
w2 active  0x00000001
release    w0=0, w1=0, w2=0
```

An out-of-line `NativeInput_ApplyTouch` line breakpoint received zero hits
because the optimized production update uses its inlined location. A later
conditional inlined breakpoint did not overlap the automation's very short
contact. Those routes are not packet evidence. The callback register trace is
accepted only as UIKit-to-native analog delivery; the media-free oracle remains
the held-contact-to-snapshot proof. LLDB detached after reporting one packet-
read error and then `Process 58958 detached`; `simctl` terminated the exact app
after installed/data hashes were rechecked.

### Remaining boundary and elapsed time

This slice accepts a complete touch-only path into an actual Time Trial,
forward race movement, Pause, rotation reflow, Resume, and direct live analog
callback values. It does not accept automatic initial orientation, human held
Gas plus steering, a drift/boost chain, lap or race completion, performance on
hardware, or physical-device signing. The goal remains active.

The preceding published timer was 176,394 seconds. The pre-publication
documentation reading was 179,389 seconds: 2 days, 1 hour, 49 minutes,
49 seconds cumulative, adding 2,995 seconds (49 minutes, 55 seconds). It
includes paused/resumed task lifetime and is not a build benchmark or
person-hour estimate.

## 2026-07-31 — Current-tip keyboard-controls revalidation

The user explicitly requested basic keyboard controls for testing. An audit
found that this requested capability was already implemented and published in
commit `2c10b00b34df4f0eb61aa8b72cbe99588a930ed6`, rather than merely described
in a plan. `NativeInput_DefaultMappings` defines the additive `WASD`, `IJKL`,
`Q/E`, `P` and Tab layout while retaining the original map
(`platform/native_input.c:308-341`). `NativeInput_KeyboardButtonBit` maps both
layouts to the same active-low PS1 button bits
(`platform/native_input.c:370-405`). The complete user-facing table and a basic
race recipe are in `README.md:205-233`.

To avoid relying only on the earlier accepted live run, the published branch
tip `8490b3126081f0cb012c6364e3e66690599d00e7` was explicitly reconfigured
with the `macos-arm64` preset and rebuilt. The compile completed with the 32
already documented warnings and no error. The product was a thin ARM64 Mach-O:

```text
build-macos-arm64/ctr_native
SHA-256 46e70980e10de096472314fb4c641b8122d890271f9d183d6c3e6a61a94e6382
```

The direct command `./build-macos-arm64/ctr_native --self-test-input` emitted
the accepted marker containing:

```text
two-host-snapshots aliases=12 held=k+d+e alias-tap=k+d
```

The oracle checks every alias individually, a held accelerate-plus-right-plus-
drift chord, a complete quick accelerate-plus-right down/up pair, and the
shared controller/touch composition path (`platform/native_input.c:1607-1707`).
The complete macOS ARM64 CTest matrix then passed 21/21 in 0.82 seconds with
zero failure.

No source line was changed. Adding a second copy or changing the accepted map
would have created conflicting controls without additional capability. This
checkpoint instead establishes that the requested implementation exists on the
current GitHub branch, builds, and remains covered. Earlier live macOS evidence
already accepted stable-menu navigation, Time Trial entry, acceleration and
steering with these keys. Simulator host-key injection still does not reach
SDL, and no physical iPad is connected, so hardware-keyboard acceptance on
iPadOS remains an explicit open boundary.

The preceding published timer was 179,389 seconds. The pre-publication
documentation reading was 179,718 seconds: 2 days, 1 hour, 55 minutes,
18 seconds cumulative, adding 329 seconds (5 minutes, 29 seconds). It includes
paused/resumed task lifetime and is not a build benchmark or person-hour
estimate.

## 2026-07-31 — Reframed iPadOS 26 orientation as adaptive scene support

The touch-only race checkpoint deliberately left automatic initial orientation
open after three public `requestGeometryUpdateWithPreferences:` variants did
not rotate a portrait cold launch. This continuation did not retry those
variants. It queried the Simulator's unified log and found the decisive runtime
instruction: update the plist because `UIRequiresFullScreen` will soon be
ignored and all orientations will soon be required.

Apple's current `UIRequiresFullScreen` reference says that iPadOS 26 supports
windowing and dynamic resizing, deprecates the full-screen requirement, and may
leave a scene visually unrotated after an interface-orientation preference
change. TN3192 says to support all orientations and identifies
`UIRequiresFullScreenIgnoredStartingWithVersion` as the compatibility boundary.
These primary sources changed the product interpretation: portrait is a valid
scene geometry, not proof that CTR's renderer or SDL orientation hint failed.

The minimal implementation was:

1. retain `UIRequiresFullScreen=true` for older behavior;
2. add `UIRequiresFullScreenIgnoredStartingWithVersion` with string value
   `26`;
3. retain the two iPhone landscape orientations;
4. declare portrait, portrait-upside-down and both landscapes in the iPad list;
5. retain the SDL landscape hint, but correct its comment to state that
   iPadOS 26 scenes may be portrait or dynamically resized.

The result was committed and pushed as `db45004f909db645863c99b90fc161db68342f37`
(`feat: adapt iPad layout for iPadOS 26`). No renderer or touch-coordinate
special case was necessary: SDL's window-size path already resizes the game
surface, and the overlay already uses safe-area Auto Layout.

### Live layout and runtime validation

A pre-commit signed Simulator build first established that the existing layout
was actually flexible. In a 743-by-1018 portrait app view it kept the 4:3 game
surface centered with shoulder/system controls above and stick/face controls
below. In a 932-by-768 landscape app view the game filled the available scene
and every control moved to its safe-area anchor. A runtime-issues query after
the plist change returned zero configuration rows; the earlier
`UIRequiresFullScreen` warning was gone.

The branch was then committed before the acceptance rebuild so generated
version identity could be exact. The clean iOS Simulator ARM64 product linked
with the 32 established warnings. Its unsigned executable was:

```text
SHA-256 4898a9af1b028d4bc75e8e9be11bd3a23131851bb255389aca5c1ef314b2beb0
identity db45004f909d; no dirty suffix
```

The disposable ad-hoc signed copy at
`/private/tmp/ctrpad-ipados26-exact.yveFiC/CTRPad.app` passed strict/deep
verification. Signing changed the executable SHA-256 to
`408514002b4ae26c749c66be24d72c2925047dfd20364e995f4ce3cd8f9a0b96`;
the installed executable matched that value. The exact app repeated coherent
portrait cold launch, landscape reflow and return to portrait. A process-
scoped `com.apple.runtime-issues` query returned only its header.

Local-only native-resolution captures were recorded, inspected and excluded
from Git:

```text
portrait   2064x2752   988250 bytes   a09a91713d1e007ba116d3a314583a2289b5e8eef6e2d13a70517b265e27d4b3
landscape  2064x2752  1129467 bytes   af3c031496b2d5db973a4f43a71a68f4462e3bc2fa36b4b21c250de0bf77ff2a
```

The landscape native screenshot retained the Simulator framebuffer's portrait
pixel dimensions while its content was rotated. A separate accessibility-
driven 932-by-768 capture was used to inspect the human-facing landscape
composition; the fixed native dimensions were not misreported as device
geometry.

### Cross-target exact matrix and preservation check

The exact device ARM64 product linked with the same 32 established warnings,
was thin ARM64, embedded `db45004f909d` without a dirty suffix, and had SHA-256
`483ae8c6f21943a01153c4746c25371a1518147878dd7fe423e202b74d74fff0`.
Its generated plist contained the version-26 compatibility key, the two phone
landscapes and all four iPad orientations. It was not signed or installed;
physical identity/profile/device availability remains unchanged.

The exact macOS ARM64 product reported
`CTR Native 0.1.0-beta.7.1 (db45004f909d)`, was a thin ARM64 Mach-O at
SHA-256 `404528725455bcb52ffa12d1337b23758fa5e19549194a3ffe8a92387633fca1`,
and passed all 21 CTests in 1.66 seconds. This repeats input, lifecycle,
storage, durable memory-card and renderer regressions after the plist/comment
change.

The final installed Simulator container retained the original retail BIN inode
`111131200`, size `605698800` and SHA-256
`f780bf2331476aabfc00772fa758b12dd95ebfbc907968132cbd3cdd4e2c07c0`.
The memory card retained inode `111222179`, size `6016` and SHA-256
`6a01b0f5562ed7a279d8f8e51e3b1874ac39a6120f55db4fe3873288950619a3`.
Thus exact install/launch/rotation evidence did not overwrite the user's import
or persistence state.

### Corrections and evidence hygiene

The first exact-build wrapper returned control before its Ninja child began.
A retry accidentally started a second identical build. Process IDs and parent
relationships distinguished them; only the newer duplicate was terminated and
the original completed. This was a scheduling correction, not a build failure.

During Simulator rotation, an accessibility element index captured before the
game tree changed was reused after Pause controls appeared. That stale index
clicked the in-game Pause control rather than the Simulator Rotate toolbar.
The observation was discarded, the tree was refreshed, and only the refreshed
toolbar action plus resulting view geometry entered the evidence record.

An exact portrait accessibility screenshot showed a white region below the
app because the desktop Simulator window extended beyond that capture route's
bounds. The native `simctl io screenshot` image showed the complete device
content and was used for portrait acceptance. No white cropping was attributed
to the application.

### Remaining boundary and elapsed time

This checkpoint accepts adaptive portrait/landscape Simulator scene layout and
removes the obsolete forced-landscape question from the software gate. It does
not accept a real iPad's windowing/rotation feel, thermal/performance behavior,
Apple signing, hardware-keyboard delivery, natural simultaneous Gas/steering/
drift contacts, a repeated boost chain or a complete touch-only race. The goal
remains active.

The preceding published timer was 179,718 seconds. The pre-publication
documentation reading was 181,770 seconds: 2 days, 2 hours, 29 minutes,
30 seconds cumulative, adding 2,052 seconds (34 minutes, 12 seconds). It
includes paused/resumed task lifetime and is not a build benchmark or
person-hour estimate.

## 2026-07-31 — Built a reproducible iOS sideload package and guarded signing path

**Commits:** `6db6116fe67a8cd09d0067c4e8d75e0158db488e`,
`a37cdf2aa5af460c20cd4950f450ab133248077c`, and
`207121134a057e83189a192de5f35706868e03da`

### Gap audit and design

The device preset already produced `CTRPad.app`, but that bundle was not a
release artifact: it had only the executable and generated plist, no embedded
license/notices/Installation Information, no reproducible IPA wrapper, and no
validated late-signing route. CMake now embeds `LICENSE`,
`THIRD_PARTY_NOTICES.md`, and `docs/INSTALL-IOS.md` in Apple bundles
(`CMakeLists.txt:117-132`). The checked-in guide documents source availability,
unsigned and signed builds, direct device installation, compatible user-side
re-signing, retail import, update persistence, and the remaining hardware
acceptance boundary (`docs/INSTALL-IOS.md:1-134`).

The retained packager keeps credentials late-bound. It pairs identity/profile
arguments, optionally builds the device preset, stages only the supplied app,
and removes any inherited signature/profile (`package-ios.sh:81-123`). It then
requires an `APPL`, thin ARM64 Mach-O with an iOS build command, verifies all
three distribution resources, and rejects retail-like files and runtime data
directories (`package-ios.sh:125-163`). Unsigned mode stops there. Signed mode
validates the current keychain identity, CMS profile, iOS platform, expiry,
exact/wildcard App ID, optional UDID, team and application prefix; it constructs
minimal app/team/keychain/`get-task-allow` entitlements, embeds the profile,
requests DER entitlements, strictly verifies the bundle, and reads the signed
application identifier back (`package-ios.sh:165-226`). The script never
exports or copies a signing private key.

Apple TN3125 was used for the provisioning-profile boundary, Apple's current
signature-format guidance for DER entitlements, and Apple's registered-device
distribution documentation for the direct install route. AltStore Classic's
official documentation was used only to describe the alternative user-side
re-signing route and its current constraints. The resulting workflow remains
sideload-only; it does not plan App Store distribution.

### Iterations that were rejected or corrected

1. An exploratory signing-disabled Xcode-generator configure entered slow,
   one-by-one SDL compiler feature probes and had not completed by the `atan`
   check after several minutes. It was interrupted without a source change.
   The working Ninja device preset plus explicit late signing remained the
   reproducible route.
2. The first packaging implementation passed its input checks but created an
   archive rooted at `CTRPad.app/`. Its own post-package verifier rejected it
   because `Payload/CTRPad.app/Info.plist` did not exist. Adding `ditto
   --keepParent` at `package-ios.sh:255-266` preserved the mandatory top-level
   `Payload/` directory.
3. Two corrected-layout archives contained the same files but `cmp` differed
   at byte 11. Inspection isolated the difference to temporary `Payload/`
   mtimes. Commit `a37cdf2aa5af` normalizes every staged inode to
   `SOURCE_DATE_EPOCH`, defaulting to the current Git commit time and then a
   2000-01-01 non-Git fallback, while rejecting invalid/pre-ZIP values
   (`package-ios.sh:228-239`). The subsequent independent archives were
   byte-identical.
4. A disposable app containing an empty `ctr-u.bin` was rejected. A call with
   an identity but no profile was also rejected. Neither negative-test artifact
   was retained or staged.
5. The first local DER-entitlement probe failed before signing because `plutil`
   treated `com.apple.developer.team-identifier` as a nested key path. Commit
   `207121134a05` uses `/usr/libexec/PlistBuddy` for literal dotted keys and
   validates the substituted identifier characters first
   (`package-ios.sh:198-214`). The failed probe was not counted as acceptance.
6. `shellcheck` was not installed. `bash -n`, positive and negative behavioral
   probes, exact target builds, archive extraction, `git diff --check`, and
   direct plist/signature readback are the recorded checks; no shell-linter
   result is claimed.

### Exact clean artifact evidence

After the fixes were committed, the device preset was explicitly reconfigured
and rebuilt so its generated version identity was exact. Commit
`207121134a05` produced:

```text
device executable  build-ios-device-arm64/CTRPad.app/CTRPad
SHA-256             62e8148d0e32697d09f8f59d40ab34a477ca4cc05df0530c4a1fc264e7e64be9
architecture        thin arm64
platform/minimum    IOS / 15.0
SDK                  26.5
build identity       207121134a05; no dirty suffix
```

Two packaging commands targeting different output filenames produced identical
archives; `cmp` succeeded and both SHA-256 values were:

```text
78b93b01ae92ebb79fb6e92e1b3b29d9cae5984fa7fa5980a96ff0489abe0c63
```

`unzip -t` passed. The archive contained exactly the `Payload/` and
`Payload/CTRPad.app/` directories plus the executable, plist, license, notices,
and Installation Information. The three resources compared byte-for-byte with
their source files. No retail-like extension, `Documents`, `Application
Support`, `memcards`, `_CodeSignature`, or `embedded.mobileprovision` appeared.
The ignored IPAs and SHA sidecars remained local.

Because no valid Apple identity/profile was available, signed-mode structure
was tested only with a disposable ad-hoc Simulator app and artificial
`TESTTEAM` values. The corrected run passed strict/deep verification, emitted
CodeDirectory v20400, and read back the exact app, team, keychain-group, and
`get-task-allow=true` entitlement values. `TeamIdentifier=not set` correctly
proved it was not an Apple signature. This accepts local construction and
readback plumbing only—not device authorization or installability.

An exact Simulator parent build at `a37cdf2aa5af` installed and launched at the
retail copyright screen with the touch overlay intact; its local screenshot
SHA-256 was `dff60f6d...be3da8d`. The Simulator changed the data-container UUID
but preserved the imported 605,698,800-byte BIN and 6,016-byte memory card by
inode and exact SHA-256. The exact current-tip macOS ARM64 binary at
`9e90e9ad...fb246` passed all 21 CTests in 1.17 seconds.

The full artifact inventory and hashes are retained in
`docs/parity/2026-07-31-ios-sideload-package.md`. All three source commits were
pushed to `origin/codex/arm64-apple` and draft PR #1; none is merged to `main`.

### Remaining boundary and elapsed time

This checkpoint accepts the complete non-secret, reproducible package path. It
does not accept an Apple-authorized signature, physical-iPad install, real
Files import/update persistence, iPad hardware-keyboard delivery, natural
simultaneous touch steering/Gas/drift/boost, a full touch-only race, cadence,
thermal behavior, or performance. Those require the user's connected iPad and
matching identity/profile. The goal remains active.

The preceding published timer was 181,770 seconds. The pre-publication
documentation reading was 183,956 seconds: 2 days, 3 hours, 5 minutes,
56 seconds cumulative, adding 2,186 seconds (36 minutes, 26 seconds). It
includes paused/resumed task lifetime and is not a build benchmark or
person-hour estimate.

## 2026-07-31 — Exercised live Files region/content failures and recovery

**Starting source:** `2f3f24c6e6d1467c390fda442fcb998b75e62a63`

This continuation targeted M9's remaining software-side negative import
coverage. No runtime source was edited: the implemented branches behaved as
designed, so the durable changes are evidence and roadmap corrections only.

### Isolation and rejected clone metadata

The accepted `CTRPad Import Validation` device was first terminated and shut
down without deletion. Its source BIN was inode `111131200`, 605,698,800 bytes,
SHA-256 `f780bf23...07c0`; its production save was inode `111222179`, 6,016
bytes, SHA-256 `6a01b0f5...619a`.

`simctl clone` created `CTRPad Import Negatives` at UDID
`26F3DEE8-8840-446D-85FE-C882009C9C06`. Although the clone had its own files,
`listapps` initially returned absolute bundle/data URLs under the source UDID.
That result was rejected rather than treated as isolation. The exact signed app
was copied to a unique temporary directory, passed strict/deep verification,
and was installed onto the clone. The resulting URLs were rooted under the
clone and its data migrated to container
`FBB4DE38-856C-43D6-BC82-0D2D1777BAAE`. The migrated accepted BIN/save hashes
and new clone inodes were verified before testing.

The signed executable SHA-256 was
`ed53ba9f26eba0a8e501f1e02aaabead1016e3b729751ce97d34c0b28879c11c`
and embedded `a37cdf2aa5af`. The only later implementation changed standalone
packaging entitlement construction, so runtime behavior was current. The app
was not misreported as an Apple-signed device product.

### Local fixtures and UI sequence

The user's ignored archived PAL image was copied only into the clone's Files-
visible Documents root. It measured 740,179,104 bytes and SHA-256
`84aeb6f9...3f41a`. A second local-only fixture copied the first 40,000 complete
raw sectors of the accepted NTSC-U image: 94,080,000 bytes, SHA-256
`1c1fe771...67508`. This retained readable MODE2/2352 and `SCUS_944.26`
identity data while truncating required production content. The accepted image
inside only the clone was renamed to `ctr-u.bin.accepted-before-negative`,
preserving inode `111309681` and its exact hash while allowing media-free
onboarding.

Computer Use observed the native onboarding, opened the real document picker,
navigated **On My iPad → CTRPad**, and selected:

1. the PAL image, which reported detected `SCES_021.05` and required
   `SCUS-94426`;
2. the truncated NTSC-U image, which reported required files missing/unreadable;
3. the full NTSC-U image, which validated, installed, and entered the game.

The initial accessibility action opened Files but the first state capture
returned before its presentation completed. A subsequent coordinate click was
therefore not accepted as the cause. The sequence was repeated with a fresh
tree, a discrete accessibility click, a separate Files-state read, and then
fresh screenshot-derived picker coordinates. Those are the actions retained as
evidence.

### Preservation and recovery evidence

After the PAL and incomplete selections separately:

```text
accepted backup  inode 111309681   605698800 bytes   f780bf23...07c0
memory card      inode 111309627        6016 bytes   6a01b0f5...619a
destination      absent
staging roots    zero
chooser          enabled
```

The final full-image selection installed destination inode `111313696` with
the exact 605,698,800-byte size and `f780bf23...07c0` SHA-256. PID `77827`
remained continuous from onboarding through both rejected selections and the
rendered Sony presentation. A cold relaunch at PID `78515` bypassed onboarding
and visibly rendered the copyright screen and touch overlay. All four native
screenshots and hashes are recorded in
`docs/parity/2026-07-31-ios-files-import.md`; none was staged.

The disposable app was terminated and its Simulator shut down without
deletion. The original `CTRPad Import Validation` device was booted and
relaunched. Its original BIN/save inodes, sizes and hashes were unchanged. Git
remained clean throughout runtime testing; no retail or generated fixture was
ever within the repository staging surface.

This accepts live Simulator detected-region failure, expected-region but
incomplete-content failure, cleanup, retry, valid recovery, same-process
startup and cold relaunch. It does not accept inaccessible-provider/copy-
interruption behavior, termination during the 605 MB copy, active-image
re-selection UX, physical Files behavior, Apple signing, or device runtime.

The preceding published timer was 183,956 seconds. The pre-publication
documentation reading was 185,118 seconds: 2 days, 3 hours, 25 minutes,
18 seconds cumulative, adding 1,162 seconds (19 minutes, 22 seconds). It
includes paused/resumed task lifetime and is not a build benchmark or
person-hour estimate.

## 2026-07-31 — Made interrupted iOS imports recoverable on the next launch

**Implementation commit:** `c745390a55ebbbef08bba1b81f982914eabecc31`

### Why this was the next software gap

The live Files matrix had accepted cancel, invalid raw input, detected PAL,
truncated NTSC-U, full validation, same-process startup and cold relaunch. A
fresh source audit then separated two meanings of “interrupted import.” The
existing importer removes its unique stage after a coordinated-copy error,
validation rejection, install failure or success. Those are handled returns.
If iOS kills the process during the background copy, none of those returns is
guaranteed to execute. The validated destination is still safe because it is
not installed until the copy and production-loader validation finish, but the
partial `.ctrpad-import-<UUID>` directory can survive indefinitely.

That recovery state was actionable without a physical device or external Files
provider. The implementation deliberately did not scan recursively, delete all
dot directories, or remove every name that merely looked similar. A shared
file-scope prefix now drives both stage creation and cleanup. Startup lists only
the direct children of `Documents/CTRPad`; it requires the reserved prefix, a
nonempty suffix and directory type before removing an entry. A missing base is
a normal first-launch no-op. Other listing/removal errors are logged. Successful
removals are counted, and onboarding uses singular/plural status text while
leaving the chooser enabled (`platform/apple/native_ios_import.m:31-32,142-184,199-230,305-310`).

### Implementation and build sequence

The first diagnostic build was intentionally run with the source edit still
dirty. Simulator and device products linked and the macOS regression suite
passed 21/21, proving the Objective-C change compiled before a checkpoint was
created. The established 32 legacy C warnings repeated and the importer added
no Objective-C warning. The source was then committed as
`fix: recover interrupted iOS imports`.

All three presets were explicitly reconfigured and rebuilt after that commit so
the acceptance binaries carried clean identity
`SDL-3.4.10-beta-7.1-135-gc745390a5` rather than a dirty suffix:

```text
Simulator ARM64  019cf0a495696d30cca0bc9be2564c4e117c28ddd4bc9e52875d8e404c825fce
device ARM64     1cc45ac04a6b23136f10500ed3db194c314619b4c39071df3866b321f83f1919
macOS ARM64      ea2c719c9565d3b6181086f2aab337e7748a4294ea4d09cd8e9f5f59d66ef934
```

`lipo` reported thin `arm64` for every product. `vtool` reported
`IOSSIMULATOR` and `IOS`, iOS 15.0 minimum and SDK 26.5. The macOS suite passed
21/21 in 0.94 seconds. A unique temporary Simulator bundle copy was required
because bundle resource assembly invalidates the linker's placeholder
signature. Before signing it matched the matrix executable hash; after ad-hoc
signing it was
`a879cae7c58e200097da431f369c24f7bb9a37a33f4a0e976c635795404c94a3`,
and `codesign --verify --deep --strict` passed. No Apple-authorized signature is
claimed.

### Isolation, seeded durable state and negative controls

The accepted `CTRPad Import Validation` Simulator was terminated and shut down
without deleting or renaming any of its data. The disposable `CTRPad Import
Negatives` clone was still available from the prior validation work, so it was
reused rather than creating another multi-gigabyte copy. Its current valid
destination was renamed inside only the clone to
`ctr-u.bin.accepted-before-recovery`, preserving inode `111313696`, size
605,698,800 and SHA-256 `f780bf23...07c0`. The clone save was inode
`111309627`, size 6,016 and SHA-256 `6a01b0f5...619a`.

Two cleanup-positive fixtures were created directly under the clone's import
base. `.ctrpad-import-interrupted-one` contained a nested 118-byte
`assets/ctr-u.bin` marker; `.ctrpad-import-interrupted-two` was empty. Three
cleanup-negative controls made overbroad behavior observable:

```text
.ctrpad-keep-control          directory; no reserved prefix
.ctrpad-import-               directory; prefix but no suffix
.ctrpad-import-control-file   ordinary file; prefix and nonempty suffix
```

The exact signed app installation migrated the Simulator data-container UUID
from `FBB4DE38-856C-43D6-BC82-0D2D1777BAAE` to
`E6915D41-574B-4380-9FCB-2EA255109A3D`. That did not invalidate the test: the
new path was resolved from `simctl listapps` before launch, and the retained
BIN/save still had their original clone inodes and hashes. Treating the stale
path as current would have been rejected evidence.

### Live observations and restoration

PID `81703` launched the exact build. Both positive fixture directories were
absent afterward. The nonmatching directory, bare-prefix directory and
same-prefix 118-byte file remained with their expected types. No
`assets/ctr-u.bin` destination appeared, so cleanup did not manufacture or
install partial content. The accepted backup and memory card retained their
inodes, sizes and exact hashes. The 2064-by-2752 screen visibly showed the
enabled chooser and exact plural message:

```text
Recovered 2 interrupted imports. No partial image was installed; choose your
NTSC-U raw BIN to retry.
```

The local screenshot hash was
`405fd31047bf323ad71a1f325b8c3d154c1c9958ab21adfac983f0d47e464500`.

The app was terminated, the accepted backup moved back to its normal filename
without changing inode, and PID `81856` cold-launched. It bypassed onboarding
and visibly rendered the retail copyright presentation and complete safe-area
touch overlay. The controls, retail BIN and save still survived unchanged. The
local screenshot hash was
`dff60f6d43e36aff4c252c6848030df2065bd29a20ce407da39484c27be3da8d`.

The clone was terminated and shut down. The original validation device was
booted and relaunched at PID `82204`. Its source retail image remained inode
`111131200`, 605,698,800 bytes and `f780bf23...07c0`; its save remained inode
`111222179`, 6,016 bytes and `6a01b0f5...619a`. Thus the isolated mutation and
Simulator install never changed the preserved source evidence. Retail data,
fixtures, temporary signed bundles, app containers and screenshots remained
outside the repository and staging surface.

### Publishing correction and remaining boundary

The source commit was pushed immediately after acceptance. An unqualified
`gh pr view 1` read followed an upstream repository context and displayed the
unrelated closed `CTR-tools/ctr-native` PR #1. It was a read-only result and was
rejected. A repo-qualified query against `chrissotraidis/ctrpad` confirmed the
actual draft PR #1 is open from `codex/arm64-apple` into `main`; no upstream PR
was changed.

This checkpoint accepts the next-launch state machine after an interrupted
copy: narrow stage removal, visible retry, similar-name preservation, asset/save
preservation and normal subsequent startup. The staging state was seeded rather
than produced by killing a live 605 MB Files transfer. A real inaccessible
provider, coordinated read failure, background termination during copy,
physical-iPad behavior and Apple signing therefore remain open. The goal stays
active.

The preceding published timer was 185,118 seconds. The pre-publication
documentation reading was 186,029 seconds: 2 days, 3 hours, 40 minutes,
29 seconds cumulative, adding 911 seconds (15 minutes, 11 seconds). It includes
paused/resumed task lifetime and is not a build benchmark or person-hour
estimate.

## 2026-07-31 — Killed a real Files import and accepted next-launch recovery

**Starting source:** `dab6ef3d3074124465cb505368a0509c3400861a`

The preceding checkpoint intentionally accepted cleanup from a seeded durable
state and left actual process termination open. This continuation used the
same exact `c745390a5` Simulator runtime and disposable clone to close that
software-side evidence gap. Production source did not change.

### Isolation and provider warm-up

The original `CTRPad Import Validation` Simulator was terminated and shut down
before the clone changed. In the clone, accepted destination inode `111313696`
was renamed to `ctr-u.bin.accepted-before-live-termination`; it retained
605,698,800 bytes and SHA-256 `f780bf23...07c0`. Save inode `111309627`
remained 6,016 bytes and `6a01b0f5...619a`. The three cleanup-scope controls
from the prior acceptance also remained in place.

Computer Use was required because the experiment had to traverse the actual
iOS document picker. The first presentation produced an empty white sheet
rather than a file hierarchy. Simulator logs showed the document-service scene
reach ready state, then File Provider local storage report
`NSFileProviderErrorDomain -1002` and retry. No URL was delivered to the app.
The sheet was dismissed through its accessibility dismiss region, and the
importer returned to `No file selected` with the chooser enabled. A fresh
presentation then displayed the expected **On My iPad → CTRPad** files. This
sequence documents provider flakiness and app cancellation recovery, but it is
not counted as the distinct inaccessible-file importer outcome.

### Attempts rejected as interruption evidence

The first bounded shell monitor polled for a reserved stage and called `simctl
terminate` when it saw one. It detected the UUID directory while the asset size
was still zero. However, `simctl terminate` was cooperative enough that the app
finished the copy, validation and install before stopping. The next launch
found a new destination inode `111333984`, 605,698,800 bytes and the exact
accepted hash. This was a successful import followed by termination, so it was
explicitly rejected as process-death evidence and preserved under
`ctr-u.bin.completed-graceful-attempt` inside only the clone.

A second shell monitor removed its sleep and tried to observe the staged file.
The local Files/APFS path completed too quickly for directory polling, the game
started normally, and no stage observation was produced. That result was also
rejected. The still-running monitor was sent Control-C before any subsequent
test, and the completed destination was retained as
`ctr-u.bin.completed-fast-attempt`. Neither output was treated as recovery.

### Event-driven diagnostic

To remove polling latency without altering production source, a local-only C
helper registered `EVFILT_VNODE`/`NOTE_WRITE` events through `kqueue` on the
clone's `Documents/CTRPad`. Once a nonempty-suffix importer directory appeared,
it busy-waited only inside the disposable helper for
`assets/ctr-u.bin`, printed the size/inode, and sent `SIGKILL` to the one exact
app PID. It had a 120-second outer deadline and a 15-second asset deadline.

The 155-line helper compiled with `clang -std=c11 -Wall -Wextra -Werror` as a
native ARM64 Mach-O. Its local-only identities were:

```text
source SHA-256  5483b6a5e63cc408cbf02c7cc077eb2c53bab64fbba4a7694a65499a7b467d3f
binary SHA-256  22940c2ff0626d72caea7876287bd734f0278ea405ef49da0d6f77f2eecf8970
```

The helper was not linked into CTRPad, copied into an app bundle, or staged in
Git. It exists only under `/private/tmp` as reproducible test instrumentation.

### Accepted kill, durable state and recovery

The clone launched media-free at PID `93005`, and Computer Use selected the
full NTSC-U source through the real picker. The event-driven helper observed
reserved stage `.ctrpad-import-77A87CDE-A75D-4FC7-93D2-F0480E7F4075`, then
asset inode `111335815` at the full 605,698,800-byte size. It immediately sent
uncatchable `SIGKILL` to PID `93005`; a read after the signal still found the
same staged inode and size.

Independent post-kill checks proved PID `93005` absent, the UUID directory and
full stage still present, and final `assets/ctr-u.bin` absent. Hashing the stage
produced exact accepted SHA-256 `f780bf23...07c0`. The retained backup and save
still had clone inodes `111313696`/`111309627`, original sizes and hashes. This
distinguishes a kill before validation/install cleanup from a stop after the
game had already started.

Exact app PID `93222` then launched. It removed the full UUID stage, left the
final destination absent, preserved the nonmatching directory, bare-prefix
directory and same-prefix ordinary file, and retained the accepted backup/save
identities. The enabled onboarding screen visibly reported the singular
recovery message. Local screenshot
`/private/tmp/ctrpad-live-sigkill-recovery.png` is 2064 by 2752 pixels and has
SHA-256 `9f2af441...213a6`.

After terminating the importer, the original clone backup was moved to the
normal destination without changing inode. PID `93310` cold-launched the Sony
presentation and complete touch overlay with no stage. Local screenshot
`/private/tmp/ctrpad-after-live-sigkill-normal.png` has SHA-256
`e6026c03...07b5`; the restored BIN/save still had exact accepted hashes.

The disposable clone was terminated and shut down without deletion. The
original validation device was booted and relaunched at PID `93637`. Its retail
image remained inode `111131200`, 605,698,800 bytes and `f780bf23...07c0`; its
save remained inode `111222179`, 6,016 bytes and `6a01b0f5...619a`. The clone's
two successful control-import files, helper, picker inputs, screenshots, app
bundles and containers remain local-only and outside the repository.

This closes the actual Simulator process-death and next-launch recovery gate
for a full same-volume Files stage. The local APFS copy exposed the staged file
at full size rather than a partial byte count, so partial-byte provider transfer
interruption is not claimed. An inaccessible URL delivered to the importer,
physical-iPad background/termination, Apple signing and explicit in-app active-
image re-selection remain open. The goal stays active.

The preceding published timer was 186,029 seconds. The pre-publication
documentation reading was 187,126 seconds: 2 days, 3 hours, 58 minutes,
46 seconds cumulative, adding 1,097 seconds (18 minutes, 17 seconds). It
includes paused/resumed task lifetime and is not a build benchmark or
person-hour estimate.

## 2026-07-31 — Recovered interrupted stages even when the game was already installed

**Implementation commit:** `8c177e8327f3a97068c6733bb57f57849b4e334c`

### Contradiction review found a post-install lifecycle hole

The real Files/SIGKILL checkpoint proved that a surviving importer stage is
removed on the next media-free launch. Reviewing that conclusion against the
startup call graph exposed a different ordering: cleanup lived inside
`CTRPadImportCoordinator.start`, and `start` is reached only when
`NativeApp_SelectAndValidateAssets` reports no valid asset. If iOS terminates
the process after the validated destination is installed but before the now-
empty unique stage is removed, the next launch selects that destination and
starts the game without ever constructing the coordinator. The good asset is
safe, but the importer-owned directory could remain indefinitely.

This was not treated as a reason to broaden cleanup. The established matcher
already accepts only direct child directories named
`.ctrpad-import-<nonempty-suffix>`, preserves the bare prefix and ordinary
files, and uses one shared constant for creation and recovery. The correction
made that one operation callable from both startup branches.

### Source change and build order

`include/platform/native_ios_import.h` now declares
`NativeIOSImport_RecoverStaleStages`. Its Objective-C implementation validates
the supplied base path, creates a temporary coordinator with that path, and
invokes the existing `removeStaleStagingDirectories` method
(`platform/apple/native_ios_import.m:424-439`). After a valid asset selection,
the iOS block in `main.c:675-687` calls it before
`NativeApp_StartRuntime`. An inspection error is logged but does not reject a
known-good game; a nonzero count is logged and flushed. The no-asset branch is
unchanged and still enters the UIKit importer, which runs the same cleanup and
can show its singular/plural retry message. The patch was 33 insertions across
the header, main entry point and Objective-C importer.

The first diagnostic matrix intentionally ran while the source was still
dirty. Simulator and device products linked; the established 32 C warnings
repeated, no new Objective-C warning appeared, and macOS passed 21/21 in 1.26
seconds. The source was committed as `fix: recover iOS stages before runtime`
and immediately rebuilt after explicit reconfiguration of every preset. This
prevented a dirty or stale object from becoming acceptance evidence.

The clean build identity embedded in every product was
`SDL-3.4.10-beta-7.1-138-g8c177e832`:

```text
Simulator ARM64  e9cb3919afd182a0121fdc5218704eebcc1a9d3c1448cb34f7c2c7000d4dee96
device ARM64     f747d24b4222fba575a4e23925b007426d310a79afca6bca210de94616008ab8
macOS ARM64      02a834202b0ebef29a1168f86a2e01387c39c7938a0befefbaeaa1fed75d891b
```

All were thin `arm64`. `vtool` reported `IOSSIMULATOR`, `IOS`, and `MACOS`; the
iOS outputs used deployment minimum 15.0 and SDK 26.5. The clean macOS suite
passed 21/21 in 1.10 seconds. A uniquely named bundle copy under
`/private/tmp/ctrpad-runtime-recovery.TeNnYn` matched the unsigned Simulator
hash, was ad-hoc signed to executable SHA-256
`c3e6a5d7b89bc0e032fe6e5ff2b8923a221cd60601279c2a4bb6691bc9b97033`,
and passed `codesign --verify --deep --strict`. This is local Simulator signing,
not Apple authorization.

### Durable valid-asset fixture and installation migration

The original `CTRPad Import Validation` device stayed booted with its accepted
app/data untouched. Only the disposable `CTRPad Import Negatives` clone was
mutated. Before boot, it held the valid destination at inode `111313696`, size
605,698,800 and SHA-256 `f780bf23...07c0`; its memory card was inode
`111309627`, size 6,016 and SHA-256 `6a01b0f5...619a`.

One empty cleanup-positive fixture was added directly beneath
`Documents/CTRPad`:

```text
.ctrpad-import-installed-destination-leftover  directory; inode 111345484
```

The earlier negative controls remained alongside it:

```text
.ctrpad-keep-control          directory; inode 111324482
.ctrpad-import-               directory; inode 111324483
.ctrpad-import-control-file   regular file; inode 111324489; 118 bytes
```

Boot-status/install commands took longer than the first bounded terminal yield,
so their live session was polled instead of assuming completion. Installing the
exact signed bundle migrated the data container from
`E6915D41-574B-4380-9FCB-2EA255109A3D` to
`41E2CACF-C11A-4D54-84D9-CC821820885B`. The new container and bundle paths were
resolved with `simctl get_app_container`. Before launch, all fixture inodes were
still present, the BIN/save identities were unchanged, and the installed
executable matched the signed hash `c3e6a5d7...97033`. Evidence tied to the old
container path would have been rejected.

### Exact launch, narrow recovery and visible runtime

PID `95815` launched the exact app. Five seconds later the positive fixture was
absent. The nonmatching directory, bare-prefix directory, and same-prefix
ordinary file retained their types and inodes. The installed BIN remained inode
`111313696`, 605,698,800 bytes and `f780bf23...07c0`; the save remained inode
`111309627`, 6,016 bytes and `6a01b0f5...619a`. The app did not present the
chooser. A 1376-by-2064 framebuffer visibly showed the game plus the complete
touch overlay, proving the valid-asset path continued into runtime. Local-only
screenshot `/private/tmp/ctrpad-installed-asset-stage-recovery.png` has SHA-256
`8ef2773bc7efa876e8ce973c008a8f4f03e7fc750903079ba86b6bdfc822d181`.

The launch requested stdout and stderr redirection to unique `/private/tmp`
paths. Simulator produced neither host file. Consequently the textual
`Recovered 1 interrupted import before runtime startup` output is not claimed
as observed evidence. The accepted basis is the exact installed executable,
one positive deletion, three negative-control preservations, unchanged
BIN/save identities, and visible runtime frame. This keeps a failed diagnostic
route in the historical record rather than silently replacing it with an
inference.

The app was terminated and relaunched as PID `95992`. After five seconds the
stage remained absent, so cleanup neither recreated it nor made startup depend
on one-time transient state. The clone was terminated and shut down without
deletion. The original validation app was brought to the foreground as its
existing PID `93637`; before and after that action its source retail image
remained inode `111131200`, 605,698,800 bytes and `f780bf23...07c0`, and its
save remained inode `111222179`, 6,016 bytes and `6a01b0f5...619a`.

Retail files, the fixture, temporary signed bundle, app containers and
screenshot stayed outside the repository. Source commit `8c177e832` was pushed
to `origin/codex/arm64-apple` before documentation editing so the functional
checkpoint was already backed up. This accepts every-launch reserved-stage
recovery for both media-free onboarding and a valid installed asset. A partial-
byte provider transfer, inaccessible URL delivery, physical-iPad termination,
Apple development signing, explicit active-image re-selection and full device
play remain open. The goal continues.

The preceding published timer was 187,126 seconds. The pre-publication
documentation reading was 188,010 seconds: 2 days, 4 hours, 13 minutes,
30 seconds cumulative, adding 884 seconds (14 minutes, 44 seconds). It includes
paused/resumed task lifetime and is not a build benchmark or person-hour
estimate.

## 2026-07-31 — Revalidated the complete iPad package at the published implementation tip

**Exact implementation tip:** `560f6dd20963dbb7f5fa160de6cd91a9910be6d0`

### Why the original package evidence was no longer sufficient

The deterministic packager was accepted at `207121134a05`, but later commits
changed adaptive scene metadata, touch behavior, live Files failure/recovery,
and iOS startup cleanup. The packaging script itself had not regressed, yet its
old IPA hash could not prove that the current published app still assembled,
excluded retail data, and launched. This continuation tested the actual branch
tip rather than treating a historical package as transitively valid.

No production source change was presumed necessary. The worktree matched
`origin/codex/arm64-apple` at `560f6dd20`. Before starting, three external gates
were read directly:

```text
security find-identity -v -p codesigning  -> 0 valid identities found
standard provisioning-profile directory  -> no profile files
xcrun devicectl list devices --timeout 10 -> No devices found
```

Therefore a true signed-device package was impossible from current local state.
The unsigned/late-signing contract remained actionable, and no example identity,
synthetic profile, ad-hoc device claim, or Simulator signature was substituted
for Apple authorization.

### Parallel exact build matrix

The macOS, iOS Simulator and iOS device presets were explicitly reconfigured
and built in separate build directories concurrently. All configuration output
reported clean SDL/source identity
`SDL-3.4.10-beta-7.1-139-g560f6dd20`. Both iOS builds linked with the established
32 C warnings and no new Objective-C warning. The macOS suite passed 21/21 in
1.11 seconds. Exact executable hashes were:

```text
Simulator ARM64  a84eb77d9f170372e732a44f3e752acaeb14eac65ae80ecc61d61677a891a4b3
device ARM64     667048abf37ccfdb079fde1e859256b6ec71973088447ff7a9b0abb7d751c712
macOS ARM64      82c911c86bb170d9b14677e9e00f6060b44877529442fcf0285b9365786a4521
```

The device app reported bundle ID `io.github.chrissotraidis.ctrpad`, version
0.1.0 (1), platform `IOS`, thin `arm64`, iOS 15.0 minimum and SDK 26.5.

### Reproducible package and direct extraction audit

Two independent `package-ios.sh` invocations wrote new names beneath unique
directory `/private/tmp/ctrpad-current-package.XB9p3b`. Each reported unsigned
mode, excluded retail media, and required LICENSE, notices and Installation
Information. Both outer files were 1,433,744 bytes and had exact SHA-256:

```text
ad8736cd1d1ae82714f3f9be8fec106295696dedcc36862686bb74ea61d8d3fa
```

`cmp` returned success. The two outer file mtimes differed because the commands
ran separately, but all seven internal entries normalized to source commit
epoch `1785541371` (2026-07-31 23:42 UTC). That distinction was inspected rather
than assuming equal archive hashes proved the intended timestamp source.

`unzip -t` accepted every member. The extracted tree contained only:

```text
Payload/
Payload/CTRPad.app/
Payload/CTRPad.app/LICENSE
Payload/CTRPad.app/THIRD_PARTY_NOTICES.md
Payload/CTRPad.app/INSTALL-IOS.md
Payload/CTRPad.app/CTRPad
Payload/CTRPad.app/Info.plist
```

Direct `cmp` checks proved the executable and each legal/install resource equal
to their input files. `lipo`, `vtool`, `strings`, `plutil`, and SHA-256 readback
proved the extracted executable was the exact device build with current
identity. Case-insensitive scans found no retail-like extension and no
Documents, Application Support, or memcards directory. No
`embedded.mobileprovision` or `_CodeSignature` existed. `codesign --verify`
failed with `code object is not signed at all`, the required observable result
for this unsigned package rather than a verification success.

Four negative routes were repeated against current source. A disposable app
copy with LICENSE mechanically copied to the retail-like name `ctr-u.bin` was
rejected before an IPA existed. Supplying only `--identity`, supplying
`--device` without identity/profile, and selecting the already-existing first
output path were also rejected with their specific contract messages. Every
negative output remained absent. The injected file contained GPL text, not
retail data.

### Same-source Simulator runtime correlation

The device IPA cannot run in Simulator, so runtime evidence used the exact
Simulator sibling from the same configuration identity. A unique copied app at
`/private/tmp/ctrpad-current-runtime.t4ME9r` matched unsigned hash
`a84eb77d...a4b3`, was ad-hoc signed to `df2d6249...41ee`, and passed strict/
deep local verification. This signature is explicitly not device authorization.

Only the disposable `CTRPad Import Negatives` clone was updated. Before install,
its BIN remained inode `111313696`, 605,698,800 bytes and
`f780bf23...07c0`; its save remained inode `111309627`, 6,016 bytes and
`6a01b0f5...619a`. The clone boot took 66 seconds to reach terminal status, so
the yielded terminal session was polled to completion rather than treating the
first 30-second return as an install. Exact installation migrated the data
container from `41E2CACF-C11A-4D54-84D9-CC821820885B` to
`1F53906D-0653-4C99-A0A8-EF38A69CA3E4` and the app bundle to
`E17AB993-3224-4DCE-8825-5C173ABD312B`. The new paths were resolved, the
installed executable matched signed hash `df2d6249...41ee`, and strict/deep
verification passed before launch.

PID `97960` launched and visibly rendered the retail copyright presentation
with the complete safe-area touch overlay. Local-only screenshot
`/private/tmp/ctrpad-current-package-runtime.png` has SHA-256
`dff60f6d43e36aff4c252c6848030df2065bd29a20ce407da39484c27be3da8d`.
The clone BIN/save retained their inodes, sizes and hashes after launch. The app
was terminated and the clone shut down without deletion.

Finally, the untouched `CTRPad Import Validation` Simulator returned to the
foreground as its existing PID `93637`. Its original image remained inode
`111131200`, 605,698,800 bytes and `f780bf23...07c0`; its save remained inode
`111222179`, 6,016 bytes and `6a01b0f5...619a`. The IPAs and SHA sidecars,
extraction tree, negative fixtures, signed Simulator app, containers, retail
files, save and screenshot all stayed outside the repository.

This checkpoint accepts current-published-source build, deterministic unsigned
packaging, direct archive/source identity, retail/runtime exclusion, negative
contract behavior, and same-source Simulator update/runtime preservation. It
does not turn an unsigned IPA into a sideloadable signed deliverable. A real
identity/profile and connected target iPad are still required, followed by
on-device import/update/save, physical touch/keyboard, full-race and performance
acceptance. The goal remains active.

The preceding published timer was 188,010 seconds. The pre-publication
documentation reading was 188,683 seconds: 2 days, 4 hours, 24 minutes,
43 seconds cumulative, adding 673 seconds (11 minutes, 13 seconds). It includes
paused/resumed task lifetime and is not a build benchmark or person-hour
estimate.

## 2026-07-31 — Traced live Gas and steering at the current implementation tip

### Scope and immutable product identity

The user asked for continued work, visible game status, frequent GitHub backup,
and a complete historical process record. The worktree and remote branch both
started at clean documentation tip `db78d5a25`. The production implementation
had not changed since `560f6dd20`, so the already audited sibling products were
the exact executable evidence for this continuation:

```text
Simulator unsigned  a84eb77d9f170372e732a44f3e752acaeb14eac65ae80ecc61d61677a891a4b3
Simulator installed df2d6249f96497817af14382bfcc122b1795123a388c32bfaa8f95ead2cc41ee
```

The second hash is an ad-hoc Simulator signature, never Apple device
authorization. No production source was edited during this experiment.

The requested practical keyboard layout was also audited in the repository:
published commit `2c10b00b34df` maps `WASD` to D-pad, `IJKL` to the four face
buttons, `Q/E` to L1/R1, `P` to Start and Tab to Select through the ordinary
PS1-shaped input path. macOS automation and live movement remain accepted;
physical-iPad hardware-keyboard delivery remains open.

### Rejected live-attach route and corrected debugger setup

The first attempt attached LLDB normally to already-running PID `99375`. It
remained hung for approximately 90 seconds, so it was explicitly stopped and
not used as evidence. The clone app was terminated and relaunched with
`simctl launch --wait-for-debugger`; that produced PID `1218` and allowed a
deterministic attach before application startup.

The first breakpoint targeted optimized `NativeInput_ApplyTouch` with a
condition involving `touchButtons` and `s_touchState`. The optimized local was
unavailable and LLDB had split the aggregate global; condition evaluation
failed, every input poll stopped, and startup appeared as a slow white frame.
The breakpoint was deleted. No product failure was inferred.

The corrected boundary was `Platform_InputTouchLeftStick`, which resolved to
three locations. The app image slide was `0x02b90000`; file symbol
`_s_touchState.0` at `0x100839e74` therefore placed the runtime state at
`0x1033c9e74`. An auto-continuing Python breakpoint command printed `w0`, `w1`,
`w2` plus the 16-byte state before each call. This avoided stopping the game
and made held-button state observable alongside live stick movement.

### Normal touch-only route into a race

With the corrected debugger active, the overlay advanced the presentations,
used outer-ring stick Down to select Time Trial, and used Gas to select Time
Trial, Crash, Crash Cove and No Ghost. View skipped the fly-in. The app reached
a live Crash Cove starting grid with timer, lap 1/3, HUD, minimap, textured
world and every touch control coherent.

The first full-resolution framebuffer was 2064 by 2752 pixels, 1,460,372
bytes, SHA-256
`f6a357590df2c7353004e72b63694251dd53cebc7e390d5911d2e6db33fb5ac8`.
It remained local at `/private/tmp/ctrpad-live-multitouch-before.png`.

### Bounded multi-touch attempts

The first intended Simulator two-contact sequence tried to position the
Option/Shift touch-pair center. Computer Use rejected an Alt/Shift-only key
request because its key contract requires a non-modifier. A corrected
Alt/Shift/U sequence completed; `U` is not a CTRPad mapping and Alt events are
excluded from the ordinary keyboard-to-pad route. The subsequent Option-
assisted drag from the Gas side produced no native stick callback and no
visible movement. This route was rejected.

The next bounded fallback overlapped a right-stick drag with twelve Gas UI
actions. Crash visibly advanced beneath the start banner. The debugger printed
the stick sequence:

```text
x=341   y=683 active=1  prior_state=00000000000000000000000000000000
x=32763 y=516 active=1  prior_state=0000000000005501ab02000001000000
x=0     y=0   active=0  prior_state=000020002000fb7f0402000001000000
```

Decoding the final prior state gives `held=0x0000`, `latched=0x0020`,
`latchedNext=0x0020`, `leftX=32763`, `leftY=516`, `active=1`. The `0x0020`
value is outer-ring Right, not Cross. Cross would be `0x4000`, which would
begin the little-endian memory dump with bytes `0040`; the held field began
with `0000` for every callback.

A final pair of concurrent Gas/stick drag requests again produced a stick
begin/release sequence with `held=0`. The UI automation service therefore
serialized these requests. Forward movement and analog callbacks are accepted
as separate live-control results; simultaneous UIKit contact is not.

The after framebuffer was again 2064 by 2752 pixels, 1,442,161 bytes, SHA-256
`4e1ac6f19133a42bcc2a439713523b45d0677789a0aee5a98b0e5d6dd325b421`.
It remained local at `/private/tmp/ctrpad-live-multitouch-after.png` and visibly
showed the advanced kart/camera position.

### Cleanup and acceptance boundary

LLDB was interrupted, detached from PID `1218`, and quit. The app continued;
the first stable Pause touch opened the retail Pause menu. The disposable clone
then retained:

```text
BIN   inode 111313696, 605698800 bytes, f780bf2331476aabfc00772fa758b12dd95ebfbc907968132cbd3cdd4e2c07c0
save  inode 111309627,      6016 bytes, 6a01b0f5562ed7a279d8f8e51e3b1874ac39a6120f55db4fe3873288950619a3
```

The clone app was terminated and the Simulator was shut down without deletion.
The protected validation Simulator remained booted with existing PID `93637`;
its BIN/save inodes `111131200`/`111222179`, sizes and hashes were unchanged.
No retail file, save, screenshot, debugger command, app container or temporary
signed bundle entered Git.

This accepts current-implementation Gas movement, live analog stick range,
neutral release and post-debugger responsiveness. It does not accept held
Gas-plus-steer, held drift-plus-steer, three-boost ergonomics, a complete
touch-only race, physical keyboard delivery, device signing, or physical-iPad
performance. The goal remains active.

The preceding published timer was 188,683 seconds. The pre-publication reading
was 191,189 seconds: 2 days, 5 hours, 6 minutes, 29 seconds cumulative, adding
2,506 seconds (41 minutes, 46 seconds). It includes paused/resumed task lifetime
and is not a build benchmark or person-hour estimate.

## 2026-07-31 — Corrected and accepted iOS hardware-keyboard ownership

### Request, initial audit and observable failure

The user asked for basic keyboard controls so the game could be tested without
touch. The practical aliases were already in published commit `2c10b00b34df`:
WASD D-pad, IJKL faces, Q/E shoulders, P Start and Tab Select. They were covered
by the media-free input self-test and documented in the README, but physical
iPad delivery remained open and the preceding Simulator note incorrectly said
Computer Use produced no SDL keyboard event.

A bounded live investigation used the disposable `CTRPad Import Negatives`
clone while preserving the source-validation Simulator. A normal LLDB attach
to a live process hung for about 60 seconds and was killed. Two subsequent
wait-for-debugger launches remained on a white frame with zero game CPU;
interrupting LLDB placed the main thread in dyld's external-state notification
trap before input startup. Detaching let the app initialize. These attempts
were rejected as debugger/dyld perturbation rather than game hangs.

The first immediate `--record --detailed` report, `ctr-195059`, also failed at
frame zero with `too many VSync packets in replay frame 0`: slow iOS bootstrap
exceeded the fixed 64-run VSync packet capacity. The supported delayed-start
route `--record --toggle --detailed` was used instead. F9 armed report
`ctr-195304`; F10 finalized 1,106 frames.

Direct parsing of that report proved key delivery. Slot zero was the connected
touch/analog `0x73` pad and stayed neutral. Slot one was a connected digital
`0x41` keyboard pad: P produced Start `0xfff7` at frame 467 and S produced Down
`0xffbf` at frame 640. The UI did not respond because retail menus read player
one. The bounded K tap missed the low-FPS sample.

Source inspection found the exact cause. Opening any SDL gamepad in a slot
unconditionally moved the keyboard to the next slot. That preserves useful
separate-player desktop behavior, but iOS touch is deliberately applied only
to slot zero. Simulator controller enumeration therefore turned the hardware
keyboard into player two.

### Correction, self-test and first GitHub checkpoint

`platform/native_input.c` now makes the ownership decision explicit. iOS
shares keyboard, touch and controller input in the primary PSX-shaped pad;
other platforms still move an overlapping keyboard to the next slot. The
generic assignment helper is directly tested in both shared and separate
modes, while the SDL virtual-controller test derives the platform's expected
slot. Input composition order and retail game code were not changed.

The same live report exposed a second metadata defect: `__APPLE__` labeled iOS
as `macos`. `platform/native_replay_scheduler.c` now checks
`SDL_PLATFORM_IOS` first and emits `ios`, leaving macOS unchanged.

Dirty diagnostic builds linked macOS, iOS Simulator and iOS device ARM64.
macOS passed 21/21 CTests; the verbose input test reported
`primary-share=keyboard+touch+gamepad`. Only the two intended source files were
staged. Commit `e6ba535a9c73` (`fix: share iOS keyboard with primary input`) was
pushed to `origin/codex/arm64-apple` before exact runtime acceptance, so the
work was already backed up while the longer test continued.

### Exact clean matrix and update preservation

All three presets were explicitly reconfigured after the source commit and
rebuilt with clean identity `e6ba535a9c73`. Exact executable SHA-256 values:

```text
iOS Simulator  1cef6404aa3c2bf094c3357e71eb1069e81ed3e6307b972d043bfe222c2c62cb
iOS device     4ca08e9bbc2095f1a05d533e15af688da23f244a464164c24e4265557a199db1
macOS          cfd3d9b420e8e9592a622112bd368b20857ba427d68c92fab58bd8578d741dca
```

The exact macOS binary again passed 21/21 CTests. Both iOS applications were
thin ARM64. The linker-signed Simulator executable installed without a second
signature transform and retained hash `1cef6404...c62cb`.

Before install, clone BIN/save identities were inodes
`111313696`/`111309627`, sizes 605,698,800/6,016 and hashes
`f780bf23...07c0`/`6a01b0f5...619a`. Installation migrated the data container
from UUID `1F53906D-0653-4C99-A0A8-EF38A69CA3E4` to
`501F6DA4-63D0-49D2-B4FC-C0A1F5B7EBC7`; it was re-resolved rather than reused.
Both files retained their exact inodes, sizes and hashes after migration.

### Keyboard-only route and packet acceptance

PID `11165` launched the exact app with delayed detailed recording. F9 began
report `ctr-201917` after the Crash-box presentation and complete touch overlay
were visible. Computer Use then sent only documented aliases:

```text
P        presentation -> main menu
S        Adventure -> Time Trial highlight
K        select Time Trial
K        select Crash
K        select Crash Cove
K        select No Ghost
```

The exact app visibly reached the Crash Cove Time Trial starting grid with
retail textures, kart and touch overlay. Local-only 655-by-903 evidence frames
for the selected Time Trial and starting grid hashed to
`b50085dd...6d37` and `09764b01...96dc`; they did not enter Git. F10 finalized
the 1,869-frame, seven-checkpoint report. Metadata now says
`build_id=e6ba535a9c73`, `platform=ios`, fingerprint `e9d9b4240372487c`.
Replay/checkpoint hashes are `07fe7a7e...a7fd` and `524d2aa0...53e5`.

Direct decoding of every 12-byte pad snapshot found slot zero's single stable
identity `status=0x00/id=0x73/connected=0x01`. Its complete non-neutral runs
were Start at frame 361, Down at 528, and Cross at 906, 1,249, 1,376 and 1,562.
Every run lasted one frame and the next frame was neutral. Slots one through
three remained disconnected `0xff/0xff/0x00`, neutral for all 1,869 frames.
This is the decisive player-one correction relative to `ctr-195304`.

### Exact replay, cleanup and boundary

The accepted app was terminated and the same installed binary launched as PID
`12141` with the finalized report. It validated all seven rolling checkpoints,
restored frame-zero state and used the isolated playback memory-card sandbox.
Without further input injection, Computer Use observed the same Time Trial
highlight and Crash Cove grid. The log closed with
`replay finished after 1869 frames`; no canonical divergence occurred. Its raw
frame-zero checkpoint diagnostic differed because it contains excluded host
addresses, as labeled by the log.

After recording/replay, clone BIN/save identities and hashes were unchanged.
The source-validation BIN/save also retained original inodes
`111131200`/`111222179`, sizes and hashes. The disposable clone was shut down
without deletion; existing source PID `93637` returned to the foreground.
Retail files, saves, reports, screenshots, app containers and products all
remain local-only and outside Git.

This accepts iOS Simulator hardware-keyboard delivery, player-one ownership,
menu/content selection to a race grid and explicit release. A physical iPad
keyboard, signed installation, physical Files/update/save behavior, natural
multi-touch, a completed race and device performance remain open. The goal
remains active. Exact evidence and rejected routes are centralized in
`docs/parity/2026-07-31-ios-hardware-keyboard.md`.

The preceding published timer was 191,189 seconds. The pre-publication reading
was 194,894 seconds: 2 days, 6 hours, 8 minutes, 14 seconds cumulative, adding
3,705 seconds (1 hour, 1 minute, 45 seconds). It includes paused/resumed task
lifetime and is not a build benchmark or person-hour estimate.

## 2026-07-31 — Current iOS GLES versus macOS desktop-GL equivalence

### Continuation audit and choice of next gate

The continuation began from clean synchronized branch
`codex/arm64-apple` at documentation commit `23029b2c75e9`; implementation tip
remained `e6ba535a9c73`. The goal objective attachment was reread, followed by
all 511 lines of `docs/ctr-native-viability.md`. The current roadmap showed the
software stack largely implemented while three kinds of work remained:

1. physical signing, install, lifecycle and performance gates requiring a real
   iPad and Apple development credentials;
2. human multi-touch drift/boost ergonomics requiring real simultaneous
   contacts; and
3. M7's still-open live-GLES representative frame, deterministic state and
   cadence comparison.

The third item was selected because it could be completed with current
authoritative artifacts without weakening either physical exit criterion. The
existing renderer trace at `platform/native_gpu.c:141-280` hashes packed
vertices and semantic draw-split state while excluding GL object IDs and host
pointers, making it the correct cross-API comparison boundary.

### Rejected mid-session cross-platform checkpoint playback

The accepted keyboard report remained in the disposable Simulator container:

```text
debug/reports/20260731/ctr-201917
frames=1869 checkpoints=7 build=e6ba535a9c73 platform=ios
```

Its report was copied to
`/private/tmp/ctrpad-cross-render-macos.edJWGc/report`; the original was not
modified. Current macOS executable `cfd3d9b4...1dca` launched with:

```sh
./ctr_native \
  --replay /private/tmp/ctrpad-cross-render-macos.edJWGc/report/input.ctrreplay \
  --replay-bypass-header \
  --render-trace-frame 1590
```

The log truthfully reported:

```text
replay platform=ios   identity=0x6ea1d76d executable=e9d9b4240372487c
live   platform=macos identity=0xaa46653b executable=8edd476c95a09be7
```

The diagnostic bypass validated seven checkpoint records, cloned an isolated
playback memory-card root, restored checkpoint zero and printed the expected
raw-checkpoint mismatch caused by excluded host addresses. It then failed;
macOS opened a crash report for `ctr_native`. This is rejected evidence.
`docs/REPLAYS.md` explicitly warns that bypassing a replay header does not make
checkpoints portable between different executable identities. No product bug
or game hang was inferred from violating that documented boundary.

The supported solution was to generate fresh native reports from a common
boot-origin pad/VSync seed rather than transport one platform's host-bearing
checkpoint into another process.

### Construction and validation of a bounded seed

Accepted clean version-4 report `ctr-215303` was chosen because its input begins
at the native frame-zero boundary and reaches the active-driver transition at
frame 1,711. A local Node invocation copied the first 2,000 full records and
changed only the replay header's declared frame count from 24,232 to 2,000.
The resulting input remained local:

```text
/private/tmp/ctrpad-gles-gl-seed.yRPz1f/report/input.ctrreplay
size:   880148
SHA-256 6a26357cc517ee57e10f9bd55a3a8f48db13edd93ae2a6fa1a225f5685aabf49
```

The original report's isolated `memcard.seed` was copied beside it. The strict
typed comparator ran in prefix mode and required all eight components. Every
one of the 2,000 records matched the source:

```text
timing/rng/drivers/world/allocation/root/pads/vsync:
equal=2000 mismatched=0
```

This established that truncation preserved complete version-4 record
boundaries. The inherited end-state fields were not treated as current output;
`--record-from-replay` consumes only pad snapshots and the complete VSync
transport, then captures fresh current-process state.

### macOS desktop-GL producer

The exact clean implementation executable was:

```text
build-macos-arm64/ctr_native
build ID e6ba535a9c73
SHA-256 cfd3d9b420e8e9592a622112bd368b20857ba427d68c92fab58bd8578d741dca
```

It ran:

```sh
./ctr_native --record-from-replay \
  /private/tmp/ctrpad-gles-gl-seed.yRPz1f/report/input.ctrreplay
```

Apple M2 desktop GL initialized all PSX and VRAM pipelines. Report
`build-macos-arm64/debug/reports/20260731/ctr-204509` finalized normally:

```text
platform=macos
identity_checksum=0xaa46653b
executable_fingerprint=8edd476c95a09be7
frame_count=2000
checkpoint_count=7
input.ctrreplay e02687209a6f4133b97afa78e4783d56777a2ecf186f6ff64d88b417ec3efd3d
state.ctrstates 1cc4229b323916d2d49b557607a347bf55f38fc46b3de49be057ed0152a44a9e
```

The log recorded the active race-driver transition at frame 1,711 and closed
with `replay-seeded recording finished after 2000 frames`.

### iOS UIKit/GLES producer

Disposable Simulator `26F3DEE8-8840-446D-85FE-C882009C9C06` was booted. Its
installed exact executable retained:

```text
build ID e6ba535a9c73
SHA-256 1cef6404aa3c2bf094c3357e71eb1069e81ed3e6307b972d043bfe222c2c62cb
```

The seed was copied into the app's private Application Support debug tree. The
clone's imported BIN and default memory card were measured before launch. PID
`16596` started with `--record-from-replay` and produced report `ctr-204914`.

The app initialized UIKit framebuffer/renderbuffer 1, Apple Software Renderer
GLES 3.0, GLSL ES 3.00, all four PSX shaders, both VRAM pipelines, the touch
overlay and the CADisplayLink-backed lifecycle loop. Durable observations were:

```text
checkpoint 0 at frame 0
checkpoint 1 at frame 300
checkpoint 2 at frame 600
checkpoint 3 at frame 900
checkpoint 4 at frame 1200
checkpoint 5 at frame 1500
race driver active at frame 1711
checkpoint 6 at frame 1800
normal finish at frame 2000
```

Software-rendered gameplay varied from roughly 5 to 11 FPS after the faster
presentation prefix. This kept the run visibly slow but did not change the
replayed timing packets. Final metadata was:

```text
platform=ios
identity_checksum=0x6ea1d76d
executable_fingerprint=e9d9b4240372487c
frame_count=2000
checkpoint_count=7
input.ctrreplay e6307c77e19b600c5658f0a1e73804e105ef22b0eb67787ba12438d13cb76d1f
state.ctrstates 0bb86d689edcb23638bc4c853d68c971449247d4ddd0c22be44dd4344442de8e
```

The iOS process remained idle under UIKit after `LOG CLOSED`; this is expected
for the application-owned lifecycle and was terminated explicitly before the
next launch.

### Full typed comparison

The current macOS and iOS report files have different raw hashes because their
headers carry truthful platform/executable identities and native checkpoint
metadata. `tools/compare-replay-state-components.mjs` compared their semantic
records and required every component:

```text
timing:     equal=2000 mismatched=0
rng:        equal=2000 mismatched=0
drivers:    equal=2000 mismatched=0
world:      equal=2000 mismatched=0
allocation: equal=2000 mismatched=0
root:       equal=2000 mismatched=0
pads:       equal=2000 mismatched=0
vsync:      equal=2000 mismatched=0
```

Two separate comparisons required pad and VSync equality between the common
seed and each output. Both also matched all 2,000 frames. Thus the live GLES
platform preserved current game state and deterministic retail timing for this
bounded scenario.

### Frame 1,813 representative trace and framebuffer

Each native report was copied to a disposable trace directory. Exact producer
playback restored checkpoint 5 at frame 1,500 and traced frame 1,813. Both
macOS desktop GL and iOS GLES emitted:

```text
render-trace frame=1813 flush=0 hash=4971e1a64577397f
vertices=5958 splits=346 formats=4:319,8:27,16:0,rgba:0
render-trace end frame=1813 hash=15c8c5410da67002 flushes=1
```

`diff -u` across the complete trace lines was empty. Both playbacks reached
frame 2,000 without canonical divergence.

The iOS recorder framebuffer captured near that point was 2064 by 2752 pixels,
SHA-256
`c7ec9117fa5dd550aab4e9d6def3270e4f1b199870d4df40fa4a2b09a469a24e`.
It showed Arcade Crash Cove selection/preview with coherent kart textures,
colors, exhaust transparency, track/checker preview, readable menu text and the
full touch overlay.

The macOS full-desktop screenshot attempt was rejected because the earlier
diagnostic crash reporter obscured the SDL window. The exact desktop render
trace and playback completion remained valid; the obscured image was not used.

### Frame 1,802 and the historical feedback distinction

The documentation's trace example uses frame 1,802. Exact checkpoint-6
playbacks were therefore repeated on both renderers. Each emitted:

```text
render-trace frame=1802 flush=0 hash=85435f06301f654a
vertices=6000 splits=338 formats=4:313,8:25,16:0,rgba:0
render-trace end frame=1802 hash=63f8c781f85e4358 flushes=1
```

Again, direct trace diff was empty and both playbacks finished normally.

Historical documentation also contains a pre-fix frame-1,802 trace with nine
flushes and 25 16-bit splits. That was a defect, not a desired framebuffer-
feedback oracle: an LP64 guest reference was misread as an inline texture word,
creating false 16-bit feedback pages and striped Crash Cove textures. The
accepted post-fix i686/ARM64 oracle required one flush and zero 16-bit splits.
The current result agrees across desktop GL and GLES and must not be described
as missing expected 16-bit coverage.

### Evidence preservation, cleanup and M7 boundary

Local-only playback logs were preserved under
`/private/tmp/ctrpad-gles-gl-evidence.6JjCbs`. Their relevant hashes:

```text
macOS frame-1813 log 6bbf0ea73ad73f79d9f44848ccd67d8906137ee94183f18fa648c154451aee6e
iOS frame-1813 log   acc9044f5da8c3abf8f03d4a89b5d940e5ce461c57d6798cc96590b65216195f
macOS frame-1802 log a758735d961141b7318ccedffaed64b849badd5ce1c90f2fad56af02d495de81
iOS frame-1802 log   101314bd28fcf5e1b4a956e8e9e1ab707c31d6e8bb9f55c2e85dcd0794e7045f
```

After the final iOS playback, clone storage was unchanged:

```text
BIN   inode 111313696, 605698800 bytes,
      f780bf2331476aabfc00772fa758b12dd95ebfbc907968132cbd3cdd4e2c07c0
save  inode 111309627, 6016 bytes,
      6a01b0f5562ed7a279d8f8e51e3b1874ac39a6120f55db4fe3873288950619a3
```

The clone app was terminated and Simulator shut down without deletion. The
protected source-validation PID `93637` remained running. The specific macOS
crash reporter created by the rejected bypass diagnostic was dismissed. Current
macOS `ctest --preset macos-arm64 --output-on-failure` passed all 21 tests in
0.47 seconds. Git remained clean until documentation edits began.

This work accepts a 2,000-frame live-GLES equivalence slice and two
representative command traces. It does not make M7 complete. Remaining M7
work is live Cocoa GLES/ANGLE or an equivalent reproducible ES host, the full
24,232-frame live-GLES golden scenario, additional targeted pixel edge cases
where needed, and physical-iPad wall cadence/energy. M8 through M11 retain
their physical signing, controller, Files/save and natural multi-touch gates.
The overall goal remains active.

The preceding published timer was 194,894 seconds. The pre-publication reading
was 196,487 seconds: 2 days, 6 hours, 34 minutes, 47 seconds cumulative, adding
1,593 seconds (26 minutes, 33 seconds). It includes paused/resumed task lifetime
and is not a build benchmark or person-hour estimate.

## 2026-07-31 — Full current-build iOS GLES golden run and keyboard audit

### Preserved the in-flight producer

The continuation resumed while exact iOS implementation `e6ba535a9c73` was
already consuming the complete accepted 24,232-frame version-4 pad/VSync seed
through `--record-from-replay`. The user's later request for basic keyboard
testing was additive, not a reason to terminate this hour-long producer.

Simulator `26F3DEE8-8840-446D-85FE-C882009C9C06`, installed executable
SHA-256 `1cef6404...c62cb`, generated private report `ctr-211128`. UIKit
framebuffer/renderbuffer 1, Apple Software Renderer GLES 3.0, GLSL ES 300, all
PSX/VRAM pipelines, touch overlay and the cooperative lifecycle loop had
initialized before frame capture began. Monitoring sampled file-complete frame
records and native checkpoint records every 45 seconds. Two monitor tool
requests had malformed polling syntax and were corrected immediately; neither
sent input to PID `20290`, restarted it, or changed its continuously growing
files.

The run crossed checkpoint 71 at frame 21,300 and the late race-driver
inactive event at frame 21,331, reached checkpoint 80 at frame 24,000, and
closed naturally after all 24,232 frames. The monitor elapsed reading was
1:07:38. Final replay/state files were 10,662,228 and 359,201,012 bytes with
hashes `056866ff...c60d` and `94eca26b...05e`; metadata finalized with 81
checkpoints. Later Simulator cadence ranged roughly 4-11 FPS and remains a
software-renderer observation only.

### Compared first, then validated the checkpoint file

The strict all-eight comparator first compared iOS `ctr-211128` with accepted
macOS oracle `ctr-215303`. Timing, RNG, drivers, world, allocation, root, pads
and VSync each reported 24,232 equal and zero mismatches.

A copy of the iOS replay/state pair was prepared for checkpoint-80 playback so
the producer report remained immutable. The first `simctl launch` mistakenly
used stale identifier `com.chrissotraidis.CTRPad`; SpringBoard rejected it as
`NotFound`. Inspection resolved the installed identity as
`io.github.chrissotraidis.ctrpad`. The corrected exact launch validated all 81
records, restored frame 24,000 with recorded checksum `0xe32367e8`, and reached
the normal frame-24,232 finish. The differing post-restore raw checksum was
explicitly diagnostic-only and host-address-bearing, not a canonical
divergence.

Checkpoint coverage inspection succeeded structurally but returned
`activeRecords=80 maxLap=0 maxCheckpoint=77 lapAdvanced=no`. That limit is
recorded rather than inferred away. This report does not supersede current lap
oracle `ctr-223221` or the open human completed-race gate.

### Generated the missing same-build desktop report

No 24,232-frame macOS report existed for current build `e6ba535a9c73`. Because
the historical oracle predates substantial platform/renderer work, the exact
current macOS executable (`cfd3d9b4...1dca`) regenerated the entire seed under
desktop GL. Report `ctr-222546` finalized 24,232 frames/81 checkpoints with
exit 0 in 13:31 and later throughput 29.90-29.91 FPS. Replay/state hashes were
`78ef4251...32a` and `7595dafa...4c9`.

The decisive same-build comparison again required all eight components. Every
one reported 24,232 equal and zero mismatches. This upgrades the renderer
state/transport result from a bounded slice to the complete scenario.

Disposable checkpoint-80 copies then traced frame 24,001 in each exact native
producer. Both emitted 3,186 packed vertices, four 4-bit draw splits, flush
hash `2381a1fe20c00a91` and aggregate hash `d1765e952537c48b`; direct trace
diff was empty. Both playbacks finished at frame 24,232. Trace-log hashes were
`dc5d9c30...7034` and `a789d242...5985`. The observed format count does not
claim absent 8-bit/16-bit/RGBA coverage.

### Verified the already implemented keyboard controls

Source inspection found the requested controls already present in
`NativeInput_DefaultMappings`, routed by SDL key events into the same
active-low pad snapshot used by controllers/touch, and documented in the
README. The current dedicated input test passed in 0.17 seconds: all 12
aliases, quick `C+Right` and `K+D` taps, two-host-snapshot retention, held
`K+D+E`, primary sharing, touch composition and virtual gamepad composition.
The final full macOS suite passed 21/21 in 1.01 seconds. No redundant source
change was made; the new requirement and fresh validation are recorded here
and in the parity report.

### Preservation, cleanup and boundary

Clone retail BIN/save retained inodes `111313696`/`111309627`, sizes
605,698,800/6,016 and hashes `f780bf23...07c0`/`6a01b0f5...619a`. The exact
reports, playback copies, state files and trace logs stayed local-only outside
Git. The disposable Simulator was shut down without deletion. Source-validation
PID `93637` remained alive.

M7's complete renderer-choice state/cadence criterion is accepted. Live Cocoa
GLES remains unavailable without ANGLE/EGL; explicit pixel cases not present
in the captured frames and physical-iPad cadence remain open. Physical signing,
keyboard/controller delivery and natural multi-touch full-race/save acceptance
also remain open. The overall goal stays active.

The preceding published timer was 196,487 seconds. The pre-documentation
reading was 202,463 seconds: 2 days, 8 hours, 14 minutes, 23 seconds cumulative,
adding 5,976 seconds (1 hour, 39 minutes, 36 seconds). It includes paused and
resumed task lifetime and is not a build benchmark or person-hour estimate.

## 2026-07-31 — Targeted renderer pixel semantics and GLES VRAM readback correction

### Resumed from a clean published checkpoint

The continuation reread the controlling goal objective and found branch
`codex/arm64-apple` clean and synchronized at documentation tip
`c05aba78a032`. Implementation tip remained `e6ba535a9c73`. The immediately
preceding work had accepted all 24,232 renderer-state frames but explicitly
left pixel/mask/feedback combinations absent from those frames open. Physical
iPad signing, natural multi-touch and device cadence still required unavailable
external hardware/credentials, so the strongest locally closable work was M7's
targeted pixel semantics.

The source audit first bounded the claim. CTR's production `setDrawStp` macro
emits only E6 bit 0, `SetPSXMaskState` consumes only bit 0, and no game-facing
call site requests destination-mask rejection. The test therefore targets the
implemented output-mask bit and does not claim bit-1 behavior merely because a
separate SDK header can describe it.

### Built a real production-pipeline oracle

A new `--self-test-renderer-pixels` route now runs after sandbox path
initialization but before disc selection. This was necessary for UIKit's
storage-root setup while guaranteeing the media-free test cannot open the
retail image or memory card. Duplicate selection fails explicitly.

The 32-by-16 scene writes controlled 4-bit, 8-bit and 16-bit textures and both
CLUTs into production VRAM, then feeds ordinary `POLY_FT4`, `DR_STP` and `TILE`
packets through `ParsePrimitivesLinkedList` and `DrawAllSplits`. It validates:

- transparent index zero preserving the blue background;
- 4-bit and 8-bit red, STP-green and blue CLUT entries;
- 16-bit direct red, STP green, zero and STP white;
- the semi-transparent two-pass distinction: zero discard, non-STP opaque and
  STP average blend;
- exact alpha/STP output;
- forced output-mask bit on an untextured red tile;
- a 16-bit page overlapping the already drawn framebuffer, forcing the real
  feedback flush/copy path; and
- production framebuffer packing followed by exact RGB5551 VRAM readback.

Selected RGBA channels use only narrow raster tolerances while alpha is exact.
Packed VRAM words are exact. A full 2,048-byte FNV-1a hash catches changes away
from the selected pixels. Apple desktop CTest registers the oracle, increasing
the full local suite from 21 to 22 tests; iOS invokes the identical path through
UIKit and `simctl`.

### Kept both failures instead of rewriting history

The first fixture used tile input red 248 and expected red 248 / RGB5551
`0x801f`. Production 16-bit modulation/quantization truthfully produced 241 /
`0x801e`. This was not a mask failure. The input was corrected to 255 so the
case isolates output-mask behavior and then passed on desktop GL.

The first live iOS run was more important. Every selected RGBA pixel passed and
the full image already matched desktop hash `851169f2644a1675`, but every
post-pack VRAM word read as zero. The production GPU-to-CPU VRAM path used
`GL_RG/GL_UNSIGNED_BYTE` from an RG8 framebuffer. Desktop GL supports it;
Apple GLES returned `GL_INVALID_OPERATION`. The old path neither checked that
error nor retained GPU ownership, so it exposed stale zeros to the CPU mirror.

The GLES branch now reads through the guaranteed
`GL_RGBA/GL_UNSIGNED_BYTE` pair into a bounded temporary buffer, repacks R/G
into each 16-bit word, checks errors and clears GPU-newer dirty tiles only on
success. Desktop retains the direct RG path with corresponding error-aware
ownership. The corrected UIKit run passed every exact word and retained the
same full-frame hash as desktop.

One precommit sanitizer attempt enabled `ASAN_OPTIONS=detect_leaks=1`.
Apple's arm64 ASan rejected unsupported LeakSanitizer before any test process
could exercise code. The unsupported option was removed; supported ASan/UBSan
passed 22/22. This is logged as a rejected harness configuration, not a product
failure.

### Published implementation before exact validation

The four-file source scope passed `git diff --check` and contained no retail
data or unrelated edits. It was committed as:

```text
818bc0e161d3736ba6d8fffa371408cde0a9fe56
fix: verify GLES pixel semantics
```

The commit was immediately pushed to `origin/codex/arm64-apple` and existing
draft PR 1. Build directories were then explicitly reconfigured so embedded
build IDs described that clean commit rather than the previous documentation
tip.

The first exact desktop command chain correctly configured and compiled, but
then named nonexistent `build-macos-arm64/ctr`. zsh stopped the `&&` chain
before tests. The product is named `ctr_native`; rerunning through that path
reported build ID `818bc0e161d3`, passed the pixel marker with hash
`851169f2644a1675`, and passed all 22 tests.

### Repeated the gate live under UIKit/GLES

Exact Simulator product `f9a97d5e...f274` was copied to a temporary directory,
ad-hoc signed and verified; signing produced executable hash
`e5d515cf...0f98`. Disposable `CTRPad Import Negatives`
(`26F3DEE8-8840-446D-85FE-C882009C9C06`) took roughly 67 seconds to boot. The
app installed into a new normal Simulator bundle/data container and launched
with only `--self-test-renderer-pixels`.

UIKit reported 1,032-by-1,376 points/pixels and nonzero presentation FBO/RBO 1.
Apple Software Renderer exposed GLES 3.0 APPLE-23.1.1 and GLSL ES 3.00. All
four PSX and both VRAM pipelines compiled. The app emitted the identical GLES
semantic marker and full hash. Existing SDL/UIKit unbalanced appearance-
transition warnings followed after the app-owned test returned; the app was
explicitly terminated.

An overly broad preservation loop started hashing every retained negative-
import copy, including multiple 600-740 MB files. The canonical BIN was already
proven, so the redundant traversal was interrupted. A later
`simctl get_app_container` call after shutdown failed with CoreSimulator error
405 because that API will not resolve a container for a shutdown device. The
already printed exact path supported the final read-only save hash. Neither
diagnostic changed app data.

Canonical clone identities remained:

```text
BIN  inode 111313696, 605698800 bytes,
     f780bf2331476aabfc00772fa758b12dd95ebfbc907968132cbd3cdd4e2c07c0
save inode 111309627, 6016 bytes,
     6a01b0f5562ed7a279d8f8e51e3b1874ac39a6120f55db4fe3873288950619a3
```

The disposable clone was shut down without deletion. Protected
`CTRPad Import Validation` remained booted and untouched.

### Completed the exact cross-target matrix

Exact clean products and outcomes:

```text
macOS desktop GL     0e46d0f6...5980  oracle pass, 22/22 CTest
iOS Simulator ARM64 f9a97d5e...f274  live UIKit/GLES oracle pass
iPhoneOS ARM64       75df7a60...2ec0  compile/link, arm64 Mach-O
macOS GLES config    56f35dfc...901e  compile/link, exact build ID
macOS ASan/UBSan     5837b1e3...f9ac  22/22 CTest
```

The macOS GLES product cannot be launched through Cocoa without a local
ANGLE/EGL implementation, so no desktop live-GLES claim was invented. Exact
evidence, commands, hashes and failure chronology are published in
`docs/parity/2026-07-31-renderer-pixel-semantics.md`.

M7's representative pixel-semantics criterion is now accepted alongside the
already accepted complete renderer-choice state/cadence criterion. M7 remains
in progress for live macOS shared-GLES execution and physical-iPad cadence/
energy. Physical signing/install, natural multi-touch/controller/keyboard
delivery, completed-race playtesting and device Files/save lifecycle remain
open. The overall goal stays active.

The preceding published timer was 202,463 seconds. The documentation-close
reading was 205,801 seconds: 2 days, 9 hours, 10 minutes, 1 second cumulative,
adding 3,338 seconds (55 minutes, 38 seconds). It includes paused and resumed
task lifetime and is not a build benchmark or person-hour estimate.

## 2026-08-01 — Balanced UIKit view replacement without losing the GLES drawable

### Why this was the next local gate

The renderer pixel-semantics checkpoint left the repeated
`SDL_uikitviewcontroller` appearance-transition warning as the clearest local
M8 lifecycle defect. Physical iPad signing, MFi/Bluetooth controller delivery,
true simultaneous touch and device cadence still require hardware or
credentials not present on this Mac. The Simulator warning was reproducible,
bounded and potentially coupled to the view hierarchy that presents the game,
so it was selected before moving to another broad feature.

The user's later keyboard request did not require a second input system. The
published tree already maps arrows/WASD, C/K gas, X/J brake, V/L item, Q/E
drift, P/Enter Start and Tab/Space Select into player one. That route had live
iOS and desktop evidence, and exact CTest 6 revalidated it in both the ordinary
and sanitizer matrices during this checkpoint.

### Reproduced before changing source

An ordinary production launch of exact implementation `818bc0e161d3` on the
disposable ARM64 iPad Simulator printed two unbalanced appearance-transition
warnings. Its deliberately short renderer self-test printed three after
`SDL_main` returned. The ordinary warnings appeared before renderer
initialization, and neither warning involved the imported BIN or save.

Both call paths converged on
`externals/SDL/src/video/uikit/SDL_uikitview.m`. When SDL exchanged the window
view, the add and remove paths each assigned the existing root controller to
nil and immediately installed the same controller again. The source comment
described this as protection for orientation on iOS 7 and below. CTRPad's
deployment target is iOS 15. A read-only check of current upstream SDL found
the same historical code, so no nonexistent upstream fix was claimed.

The count was compelling: two nil/reset sites matched the two warnings on an
ordinary launch. The self-test's extra warning was kept separate because its
`SDL_main` returns immediately instead of entering the production display
loop.

### Rejected the first superficially successful attempt

The first experiment removed only the two nil assignments. It left the later
assignment of `data.viewcontroller` to `rootViewController` in place. This
looked promising in logs:

- ordinary warnings fell from two to zero;
- short-test warnings fell from three to one;
- GLES initialized; and
- all PSX shaders, VRAM pipelines and the display loop ran.

The screen was completely black. The 104,142-byte screenshot
`/tmp/ctrpad-uikit-root-test.png` hashed to
`97de7803f8e9c1186c3097079455d17a36720cdeb3d59668c36bbae7f3b30a5b`.
UIKit treats reassignment of the same root controller as a no-op, so the
controller's replacement view had not been reattached to the window. This is
exactly why the visible-screen check accompanied the warning count.

The experiment was fully reverted with `apply_patch`. It was neither committed
nor used as the basis for exact validation.

### Preserved the controller and repaired the view hierarchy

The accepted implementation keeps the installed root view controller when it
is already correct. After replacing its view, the code checks whether that view
has a superview and uses `[data.uiwindow addSubview:...]` only when attachment
is missing. When the desired controller is not the root, normal root-controller
installation still occurs. The same rule covers both addition and removal.

The source comment makes the compatibility decision explicit: CTRPad targets
iOS 15 and later, while the clear/reinstall sequence served iOS 7 and below.
This avoids a silent fork whose reason would otherwise be lost.

The first dirty-source production launch proved that this approach differed
from the rejected one. It emitted zero ordinary warnings and produced coherent
title/demo/touch pixels. Screenshot
`/tmp/ctrpad-uikit-root-test2-live.png` was 964,291 bytes with SHA-256
`fddf49e3aba96237086ff07e86a057bd51c7d909b73053951b9421e64fea3d83`.
The live GLES oracle still passed hash `851169f2644a1675`.

That dirty process then exercised actual Simulator UI controls through the
visible app: Home, the SpringBoard CTRPad icon, and Rotate. Background and
foreground lifecycle markers remained ordered, audio suspended and resumed,
and the overlay reflowed. No appearance warning appeared. This was retained as
precommit evidence only.

Desktop GL CTest 22/22, an iPhoneOS ARM64 link and ASan/UBSan 22/22 also passed
before commit. The one-file source scope passed diff review and was committed
and pushed as:

```text
6b268157888fbe66b8a4910ae6bf02db6b095d2b
fix: balance UIKit view transitions
```

### Rebuilt and repeated from the exact clean commit

Every final build directory was reconfigured after the commit. SDL and CTRPad
both embedded `6b268157888f`, preventing the documentation tip or a prior dirty
tree from masquerading as the implementation under test.

The unsigned Simulator product was ARM64 Mach-O and hashed to
`5024f16b...427`. It was copied to a temporary directory, ad-hoc signed and
verified for disposable installation; signing produced executable hash
`7146c5bb...33e4`. No retail media entered the app bundle.

The exact renderer self-test initialized UIKit at 1,376 by 1,032, selected
presentation framebuffer/renderbuffer 1, compiled four PSX shaders and both
VRAM pipelines, and passed the complete GLES semantic marker with hash
`851169f2644a1675`. One unbalanced warning followed only after this immediate
self-test returned. It is logged as an open teardown residual.

The exact ordinary production launch printed no appearance warning. Its
initial black/slow interval was diagnosed instead of guessed at: process
`47918` was CPU-active and `lsof` showed it reading canonical 605,698,800-byte
inode `111313696`. The work was the established asset-validation scan. It then
logged `UIKit display loop active`, rendered the title/demo and produced exact
live screenshot hash `fb8fefc8...765e`.

The actual Simulator Home button produced:

```text
will-enter-background / audio=suspended
did-enter-background  / audio=suspended
```

The actual SpringBoard icon resumed the same process and produced:

```text
will-enter-foreground / audio=suspended
did-enter-foreground  / audio=active
```

The actual Rotate control then changed geometry while the game remained
animated and coherent. The native overlay reflowed, and the rotated screenshot
hashed to `eaaa7ee1...081`. The attached production console remained free of
appearance warnings through startup, background, resume, rotation and bounded
`simctl terminate`.

This visible-control exercise was intentionally separate from `simctl` log
inspection. It proved the state transitions occurred through the Simulator UI
that a tester uses rather than by manufacturing lifecycle log records.

### Preservation and exact regression matrix

The exact app was terminated only after renderer, lifecycle and rotation
evidence closed. Canonical disposable-clone data retained:

```text
BIN   inode 111313696, 605698800 bytes,
      f780bf2331476aabfc00772fa758b12dd95ebfbc907968132cbd3cdd4e2c07c0
save  inode 111309627, 6016 bytes,
      6a01b0f5562ed7a279d8f8e51e3b1874ac39a6120f55db4fe3873288950619a3
```

The disposable `CTRPad Import Negatives` simulator was shut down without
deletion. Protected `CTRPad Import Validation` remained booted and was not
installed to, launched, inspected or controlled.

Exact artifacts and outcomes were:

```text
macOS desktop GL       5583d39d...80cc  version exact, CTest 22/22
iOS Simulator ARM64   5024f16b...427   GLES oracle + production lifecycle
iPhoneOS ARM64         3da318db...04ab  compile/link, arm64 Mach-O
macOS GLES config      57af21a0...1f24  compile/link, arm64 Mach-O
macOS ASan/UBSan       2f93ed66...b25d  version exact, CTest 22/22
```

The compiler repeated known legacy conversion/deprecation warnings; no build
or test failed, and ASan/UBSan reported no runtime finding across 22 tests.

### Acceptance boundary and elapsed goal time

The ordinary Simulator appearance-warning boundary is accepted. Exact Home,
resume and rotation are also accepted locally. The one immediate-test teardown
warning is not hidden, and M8 remains in progress for physical signing/install,
natural termination and low-memory delivery, device background/save behavior,
physical keyboard/controller/multi-touch play, a completed race, audio/XA/STR,
cadence and energy.

The preceding published timer was 205,801 goal seconds. The
documentation-close reading was 208,828 seconds: 2 days, 10 hours, 0 minutes,
28 seconds cumulative, adding 3,027 seconds (50 minutes, 27 seconds). This is
cumulative goal lifetime including pauses, not a build benchmark or person-hour
estimate.

## 2026-08-01 — Explicit disc re-selection without live-media mutation

### Selected the next locally closable M9 gate

The continuation began by reading all 511 lines of
`docs/ctr-native-viability.md` again, then comparing the milestone status to the
current source and prior evidence. M8/M10's remaining physical signing,
controller, keyboard, true simultaneous touch, completed-race, cadence and
energy gates cannot be closed honestly without the user's iPad and Apple
credentials. M9 still named explicit active-image re-selection as a software-
side gap, so that became the bounded checkpoint.

The user's keyboard request was not ignored. The repository already has the
basic keyboard route requested: arrows/WASD, C/K gas, X/J brake, V/L item, Q/E
drift, P/Enter Start and Tab/Space Select. It feeds the same player-one input
composition used by touch and had prior live iOS/desktop evidence. Replacing it
with a second keyboard layer would have created conflicting ownership. Exact
CTest 6 revalidated all aliases, quick taps and held composition during this
checkpoint in both ordinary and sanitizer matrices.

### Rejected an unsafe hot swap

The importer validation callback selects and opens assets through global
`NativeAssets`/`NativeDiscImage` state. The running game also continuously uses
that state. Therefore two superficially convenient implementations were
rejected before editing:

- copying/replacing `ctr-u.bin` while the display loop continued; and
- validating the stage in-process while leaving the old game alive.

Both could race active reads or mutate the global disc under gameplay. There is
also no established retail reboot/reset entry point that proves all game state
is reconstructed. The accepted design stops the game first and requires a cold
relaunch after successful replacement. This is less seamless but matches the
real lifecycle that the code can currently guarantee.

### Implemented a main-thread stop-and-reselect handshake

Five source files changed:

- `native_ios_import.h` gained import-purpose and completion-result enums;
- `native_ios_touch.h` gained a bounded disc-reselection callback;
- `native_ios_touch.m` added accessible `ctrpad.touch.disc`, confirmation text,
  and callback cleanup;
- `main.c` records the request and consumes it at the next UIKit display-loop
  boundary; and
- `native_ios_import.m` reuses the existing transaction with replacement-
  specific copy and a relaunch-required completion state.

The confirmation explains that CTRPad must stop, memory-card saves are kept and
unsaved race progress is lost. Its accepted action waits 0.35 seconds for alert
dismissal before requesting teardown. The next display callback stops the
display link, ends the overlay, shuts down platform/audio/renderer state,
closes the disc and opens the replacement screen. The importer remains on the
main thread while its coordinated copy and validation remain on the existing
worker queue.

On success the importer disables its button and requires full app-switcher
close/reopen. Initial onboarding still starts in the same process; that proven
path did not regress. Callback state is cleared when touch ends or attachment
fails, preventing a stale UI callback from surviving overlay teardown.

### Dirty-source proof before publication

ARM64 iOS Simulator, iPhoneOS and macOS builds linked, desktop GL passed 22/22,
and `git diff --check` passed before live validation. A clean copy of the dirty
Simulator app was ad-hoc signed and installed only on disposable **CTRPad Import
Negatives** (`26F3DEE8-8840-446D-85FE-C882009C9C06`). Protected **CTRPad Import
Validation** remained booted and untouched.

The live game reached `UIKit display loop active` and visibly rendered the
animated title/demo. Accessibility exposed `Change retail disc image`. The
first confirmation was canceled and the subsequent screenshot showed a later
animation frame, proving gameplay remained active. The second confirmation
produced ordered log markers:

```text
[CTR Import] confirmed runtime disc re-selection request
[CTR Import] stopping the current game before disc re-selection
```

The replacement screen appeared coherently and the canonical disc/save still
had inodes `111313696`/`111309627` and exact hashes
`f780bf23...07c0`/`6a01b0f5...619a`.

The first real Files presentation repeated the known local provider's blank
white sheet. Dismissing the picker through its accessibility dismiss region
returned the app to an enabled cancel state saying the current disc remained
installed. A second presentation loaded the retained fixtures. Selecting the
full 605,698,800-byte NTSC-U image produced a relaunch-required state, disc
inode `111448009` with the same exact hash, and the unchanged save inode/size/
hash. Cold relaunch rendered CTR from that new inode.

Review of the successful flow exposed one messaging weakness: copy, format or
region failures did not all explicitly reassure a replacement user. A central
re-selection-only suffix was added to every importer error, and the temporary
verification status was changed from `Starting Crash Team Racing` to
`Installing the replacement`. Both iOS products rebuilt cleanly after this
refinement.

### Published the implementation before final acceptance

The worktree contained only the five intended files. GitHub CLI 2.96.0 was
authenticated as the repository owner. The explicit files were staged,
reviewed and committed as:

```text
300499d7cd00050d83dbb2dc2236a25b0529473b
feat: add safe iOS disc reselection
```

The branch `codex/arm64-apple` pushed successfully. A direct GitHub API read of
draft PR #1 reported open/draft, base `main`, head `codex/arm64-apple`, and head
SHA `300499d7cd00050d83dbb2dc2236a25b0529473b`. This closed the requested remote
backup before the longer exact matrix.

### Exact five-product rebuild and tests

All final directories were regenerated after commit so Git-derived version
source contained `300499d7cd00`. The key procedure was:

```text
cmake -S . -B <each existing configured build directory>
cmake --build <directory> --parallel 8
ctest --test-dir build-macos-arm64-app --output-on-failure
ctest --test-dir build-macos-arm64-sanitizers --output-on-failure
```

The five large optimized `main.c` translations ran concurrently and stayed
CPU-active. Four configurations completed first; ASan/UBSan completed last.
The bounded wall interval was about 12.5 minutes, not a stuck build. Rerunning
each build reported `ninja: no work to do`, proving all jobs had completed.

Exact unsigned executable results:

```text
iOS Simulator ARM64   a09932ffc3ad9ca6759c89778cefc4595133ca73c4e7791f643f5a9327be2c08
iPhoneOS ARM64        20dfcf892ffecdd2c23e602acc451aebe77cd9c4bec55fa5ce1f5aba431e5c11
macOS desktop GL      89eb5d224542819c261c490f902391c06231937efbfb40d415b39ec13a9140f3
macOS GLES config     2fc21b1008e4ff30d1b26435585610373618d9b9e91fa03a7157bf995cbd194c
macOS ASan/UBSan      1900f3ea57d6989cad1db9532031ce039e5b1cc9eedfdb9058c8c32a0d6a9f46
```

Every product was ARM64 Mach-O and contained the exact build string. Desktop
GL passed 22/22 in 4.28 seconds. ASan/UBSan passed 22/22 in 10.50 seconds with
no runtime finding. Known legacy conversion/deprecation warnings did not become
new failures.

The 3.5 MB Simulator bundle contained no assets, only the executable,
`Info.plist`, GPL license, installation instructions and notices. An isolated
copy was forced to an ad-hoc signature and passed deep/strict verification. Its
signed executable hash was `688865c7...f2e`. Updating the disposable Simulator
changed its bundle/data container UUIDs but retained disc inode `111448009` and
save inode `111309627` plus their exact sizes/hashes.

### Exact real-UI acceptance

Computer Use was used because this gate depended on the actual Simulator alert,
Files document picker and visible renderer. Accessibility state was refreshed
after every action; screenshot-grounded coordinates were used only for Files
items because the document-service file cells were absent from the returned
accessibility tree.

The exact signed app reported version `0.1.0-beta.7.1 (300499d7cd00)`, opened
presentation FBO/RBO 1, compiled all PSX/VRAM pipelines, activated the UIKit
display loop and visibly rendered coherent textured animation. The accessible
disc button opened the correct warning. Cancel returned to a later live frame.

The accepted confirmation cleanly transitioned to the replacement screen. The
118-byte `not-a-disc.bin` fixture then exercised the newly refined error path:

```text
That file is not a readable raw MODE2/2352 disc image. Select the BIN data
track, not a CUE or compressed archive. Your current disc remains installed.
```

The chooser re-enabled. Disc inode `111448009`, size 605,698,800 and hash
`f780bf23...07c0`; save inode `111309627`, size 6,016 and hash
`6a01b0f5...619a`; and the absence of a new reserved stage all remained exact.

The real picker then selected the full NTSC-U fixture. The success screen
disabled the picker and required relaunch. Atomic replacement created disc
inode `111450682` with the same size/hash. The save retained its old inode,
size, modification time and hash. No new `.ctrpad-import-*` directory survived.

After explicit bounded termination, the exact signed executable cold-launched,
read the new disc inode, activated UIKit/GLES and visibly rendered the Naughty
Dog/title animation plus touch overlay. The local-only 610,956-byte PNG hashed
to `a33b033bab59e1ff6e31d4b6e48b5828272da36395a891c2c349dce4c901ca70`.
Final disc/save identities were unchanged from the post-replacement values.

The attached console contained the Simulator runtime's duplicate WebCore/
WebKit accessibility-bundle warning, which is outside CTRPad, but no unbalanced
UIKit appearance transition through game stop, importer presentation,
replacement or relaunch. Fixtures, app copies, signatures, screenshots and
container data stayed outside Git.

### Acceptance boundary and elapsed time

Explicit asset re-selection and save preservation are accepted on Simulator.
Cancel does not interrupt gameplay; invalid media cannot replace the current
disc; valid media replaces atomically; the memory card is not rewritten; and a
cold launch consumes the replacement. The provider's blank-sheet presentation
was already documented and canceled safely, but a true inaccessible-file URL
callback remains open. Physical Files/signing/save/background/termination,
real hardware input/race/audio/video, cadence and energy also remain open.

The preceding published timer was 208,828 goal seconds. The documentation-open
reading was 211,878 seconds: 2 days, 10 hours, 51 minutes, 18 seconds
cumulative, adding 3,050 seconds (50 minutes, 50 seconds). The documentation-
close reading was 212,262 seconds: 2 days, 10 hours, 57 minutes, 42 seconds,
adding 3,434 seconds (57 minutes, 14 seconds) from the preceding published
checkpoint and 384 seconds (6 minutes, 24 seconds) during the closing audit.
Goal time is cumulative across pauses and resumes and is not a build benchmark
or person-hour estimate.

## 2026-08-01 — Resumed exact acceptance of the level-visibility correction

### Resume and machine-safety boundary

The prior pass ended at a real machine-resource boundary, not at an accepted
renderer fix. The reviewed source was remotely recoverable, but the user had
reported system slowness and instructed the work to use at most one Simulator.
The goal timer had reached 218,932 seconds when that boundary was recorded.

The user then explicitly unblocked and resumed the goal, while asking for more
care with Simulator. Goal state read active. Before compilation, both named
devices were confirmed shut down:

```text
CTRPad Import Negatives    26F3DEE8-8840-446D-85FE-C882009C9C06  Shutdown
CTRPad Import Validation   1D19A61F-20B7-46B0-AB52-B3A3406952E2  Shutdown
```

No Simulator was booted for compilation. The protected validation device was
never installed to, launched, reset or controlled during this acceptance. The
stale goal-owned i686 diagnostic container remained stopped; four unrelated
`buzz-prod` containers remained untouched. Builds were deliberately serialized
at nice priority 15 and Ninja parallelism one.

### Exact ARM64 desktop and sanitizer rebuilds

The ordinary Apple Silicon configuration was regenerated and built with:

```text
nice -n 15 cmake --preset macos-arm64
nice -n 15 cmake --build --preset macos-arm64 --parallel 1
```

The large unity translation completed. The executable reported:

```text
CTR Native 0.1.0-beta.7.1 (4a4b148dd8d1)
```

Focused `ctr_native_asset_relocation` CTest passed 1/1 in 0.04 seconds. Direct
execution emitted the required final field:

```text
cache-recycle=targeted+range+all
```

The complete ordinary suite passed 22/22 in 2.66 seconds.

ASan/UBSan was then reconfigured rather than relying on stale generated build
metadata:

```text
nice -n 15 cmake -S . -B build-macos-arm64-sanitizers -G Ninja \
  -DBUILD_TESTING=ON -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DCMAKE_OSX_ARCHITECTURES=arm64 \
  '-DCMAKE_C_FLAGS=-fsanitize=address,undefined -fno-omit-frame-pointer' \
  '-DCMAKE_EXE_LINKER_FLAGS=-fsanitize=address,undefined'
nice -n 15 cmake --build build-macos-arm64-sanitizers --parallel 1
```

Its focused test passed in 0.21 seconds and the full suite passed 22/22 in 7.19
seconds with no sanitizer finding. Both executables were ARM64 Mach-O and
embedded exact build ID `4a4b148dd8d1`. Known legacy conversion and API
deprecation warnings repeated; they were not misreported as new failures.

Exact desktop product identities were:

```text
desktop    7fe474c5e1299445e97ba0bd3d64a38346d0097f6b6509e51180a9a210786c56
ASan/UBSan da62529752f29f231f1566afc69035b0a32ae78bfec84441366238b81ecf1d5f
```

### Sequential iOS compile/link and packaging observation

With zero booted devices, `ios-simulator-arm64` and `ios-device-arm64` were
each reconfigured and built separately at nice 15 / parallel 1. Both UIKit/GLES
targets compiled and linked as ARM64 Mach-O. Exact executable hashes were:

```text
iOS Simulator  6c0189fafa44de71ce8aadf01c53a186bccd862ec8244f428aedbbde2468d78a
iPhoneOS        7dc667e7523e761806b488356824082515eef43604872c8d8f3badc1a93f63f5
```

Both contain version `0.1.0-beta.7.1`, build ID `4a4b148dd8d1` and SDL source
marker `SDL-3.4.10-beta-7.1-157-g4a4b148dd`. Each bundle contains only its
executable, `Info.plist`, `LICENSE`, `INSTALL-IOS.md`,
`THIRD_PARTY_NOTICES.md` and `_CodeSignature/CodeResources`; no disc image or
derived retail media is bundled. The identifier is
`io.github.chrissotraidis.ctrpad`, minimum iOS is 15.0 and both device families
are present.

The linker's ad-hoc signature was inspected rather than assumed valid.
`codesign -dv` reports identifier `CTRPad`, no team identifier and no sealed
resources. Deep/strict verification fails with:

```text
code has no resources but signature indicates they must be present
```

The Simulator accepts that development bundle, but it is not evidence of a
properly resource-sealed, team-signed physical-device package. That open gate
is retained explicitly.

### Exact disposable install with preserved data-container state

Only disposable **CTRPad Import Negatives** was booted for the runtime gate.
Protected **CTRPad Import Validation** remained shut down. Before install, the
active imported disc and save identities were:

```text
BIN   inode 111450682, 605698800 bytes, mtime 1785358888,
      SHA-256 f780bf2331476aabfc00772fa758b12dd95ebfbc907968132cbd3cdd4e2c07c0
save  inode 111309627, 6016 bytes, mtime 1785525736,
      SHA-256 6a01b0f5562ed7a279d8f8e51e3b1874ac39a6120f55db4fe3873288950619a3
```

Installing exact `build-ios-simulator-arm64/CTRPad.app` migrated the data
container from UUID `7C46DC73-D4C0-4CFD-9C40-317D44C655F7` to
`F5B6B88F-640E-440B-888D-FC9514311E11`. The BIN/save inodes, sizes,
modification times and hashes were unchanged after migration.

The first launch command mistakenly used the unsupported argument
`--terminate-running` and was rejected as `Invalid device` without launching or
changing state. The corrected flag was `--terminate-running-process`; launch
then succeeded. This failure is recorded because silently dropping it would
make the procedure harder to reproduce.

### Real UI inspection and careful rotation handling

Computer Use was used for visible Simulator inspection because shell logs
cannot prove pixels. The plugin wrapper was loaded in its persistent Node REPL.
Targeting `com.apple.iphonesimulator` was ambiguous because two Xcode installs
expose that identifier, so the retry used the explicit application path
`/Applications/Xcode.app/Contents/Developer/Applications/Simulator.app`.

The first exact live frame showed the legal/title scene, Crash, trophy,
textured checkered floor and walls, plus the touch overlay. The Simulator window
was sideways. One accessibility-indexed Rotate action was issued. Its immediate
screenshot refresh failed because a helper binding was missing; the action was
not repeated blindly. Fresh app state then confirmed the rotation had already
occurred. The device was left in that state.

Subsequent fresh frames visibly showed:

1. the complete CTR title/menu with logo, character, background and text;
2. a **Race Today** scene with sky, grass, foliage, sign and course geometry;
3. Crash and a kart in a forest with textured ground, trees, foliage, building,
   sky, character/kart instances and complete touch controls; and
4. a later Crash/kart scene with textured props, wheels, crates and background.

These views directly contradict the pre-correction failure mode in which
`RenderAllLevelGeometry` skipped BSP terrain/scenery while instances remained.
They do not imply a whole-game visual-parity claim.

Two screenshots were captured for visual QA. The clearer 655-by-903 Simulator
window frame hashes to
`5637af8065ed6f29acbfe89045e9088cc27ff25aeb6df4575df8f13059626a87`.
A raw 2,064-by-2,752 device image was rotated 180 degrees to make the captured
content human-readable and hashes to
`dc75cc1947c5097aa143ea0a0813bb79655b559cf57e0f25f6f2e5c25c99ea67`.
Both were reviewed locally, then moved outside the worktree so retail-derived
pixels do not enter the GPL source repository.

### Attached-console churn and error audit

The exact app was cold relaunched with an attached console using
`simctl launch --console-pty --terminate-running-process`. It reported the
expected exact version, disposable sandbox paths, Apple Software Renderer,
OpenGL ES 3.0, all four PSX shaders, VRAM pipelines, touch overlay, UIKit
display loop, memory-pack arena and CoreAudio. The only unrelated diagnostic
was the established duplicate WebCore/WebKit accessibility-class warning from
the Simulator runtime.

The active file log reached 42 lines: 14 initialization lines followed by 28
periodic 120-frame FPS samples. On Apple Software Renderer the samples ranged
from 3.37 through 7.72 FPS. A complete exact-pattern audit found none of:

```text
[CTR AssetRef]
visibility cache exhausted
cache exhausted
ERROR
FATAL
```

The attached console likewise showed no visibility/materialization error before
termination. The deterministic nine-fixture test proves slot recycling and
out-of-range preservation; the live run proves the repaired production app can
render the observed title/menu/demo scenes without the old error signature.
No new production-only materialization counter was added merely to inflate the
claim surface.

One attempted evidence command began SHA-256 traversal of every document
fixture, including multiple 605–740 MB files. It produced the stat identities
but no useful digest before it was interrupted to avoid needless machine load.
The canonical active BIN's unchanged inode, size and mtime across install and
runtime, combined with its already established SHA-256, are the stronger and
cheaper preservation evidence. The active save was rehashed and remained exact.

### Natural stopping point and acceptance boundary

After evidence capture, exact bundle ID termination ended the app and attached
console cleanly. The disposable Simulator was shut down without deletion.
Final `simctl` state showed both disposable and protected devices shut down,
so no Simulator load was left behind.

The eight-entry LP64 level-visibility sidecar lifetime defect is accepted as
corrected at its deterministic host boundary and in this observed UIKit/GLES
title/menu/demo runtime. The original 268-error signature did not recur,
whole-scene geometry and textures are visible, ordinary and sanitizer matrices
pass, iOS targets compile, and disc/save state is preserved. This restores the
specific M7 visual gate that the user's observation reopened.

The acceptance remains bounded. It does not close physical-iPad team signing,
strict resource sealing, hardware cadence/energy, true simultaneous touch,
controller/keyboard hardware delivery, completed-race play, every scene/asset,
or remaining device lifecycle/Files/save gates. The overall goal remains
active and moves back to those dependencies.

The resumed-runtime timer reading was 220,500 seconds: 2 days, 13 hours,
15 minutes, 0 seconds cumulative. The documentation-close reading was 221,029
seconds: 2 days, 13 hours, 23 minutes, 49 seconds. That adds 2,097 seconds
(34 minutes, 57 seconds) from the recorded 218,932-second blocker boundary,
including 529 seconds (8 minutes, 49 seconds) for the closing documentation
audit. Goal time includes pauses/resumes and is not a build benchmark or
person-hour estimate.

## 2026-08-01 — Current iPhoneOS package reproducibility and signing inventory

The next dependency after visual acceptance was the signed sideload boundary.
The raw device CMake bundle had compiled, but its linker-created ad-hoc
signature was not a complete resource-sealed app signature. Source inspection
found that this is not the release path: `package-ios.sh` copies the device app
to an isolated `Payload/CTRPad.app`, removes `_CodeSignature` and
`embedded.mobileprovision`, validates the thin ARM64/iOS/legal/retail-free
contract, then either emits an explicitly unsigned IPA or applies a supplied
identity/profile, DER entitlements and strict verification.

Fresh read-only host inventory returned:

```text
security find-identity -v -p codesigning  0 valid identities found
standard provisioning-profile locations  no files
xcrun devicectl list devices              No devices found
CTRPad Import Negatives                   Shutdown
CTRPad Import Validation                  Shutdown
```

Therefore no Apple-authorized signature or physical install could be performed
honestly. No keychain, Xcode account, profile or device state was changed.

The already accepted iPhoneOS product embedding `4a4b148dd8d1` was passed to
two separate packager invocations at nice priority 15. Both used the same
explicit source timestamp and unique output names under
`/tmp/ctrpad-package-current.QbdUlr`. The complete procedure was:

```text
SOURCE_DATE_EPOCH=<8eeebdd48 commit time> ./package-ios.sh \
  --app build-ios-device-arm64/CTRPad.app --output <first unique IPA>
SOURCE_DATE_EPOCH=<same> ./package-ios.sh \
  --app build-ios-device-arm64/CTRPad.app --output <second unique IPA>
cmp <first> <second>
unzip -t <first>
```

Both outputs had exact SHA-256:

```text
fb6840647df8ce59fee6f7b8eef0173961b3bd93a1d45d531ff9b1304430d7d6
```

`cmp` returned success. `unzip -t` reported no errors. The seven archive
members were only the standard `Payload/CTRPad.app` directories, executable,
`Info.plist`, GPL license, notices and installation guide. The extracted
executable remained thin ARM64, platform iOS, minimum iOS 15.0, SDK 26.5 and
embedded exact version `0.1.0-beta.7.1`, build ID `4a4b148dd8d1` and SDL
identity `SDL-3.4.10-beta-7.1-157-g4a4b148dd`.

Direct scans found no retail-like extension, runtime data directory,
`embedded.mobileprovision` or `_CodeSignature`. This is intentional unsigned
output for later user-side or profile-aware signing, not a misleading
installable claim. It also resolves the raw-bundle signature observation: the
incomplete linker ad-hoc signature does not survive packaging, and the signed
branch must replace it with the supplied authorization before it can pass.

The IPAs, SHA sidecars and extracted tree remain local-only. No Simulator was
booted for this package audit. Current-source unsigned package reproducibility
is accepted; a compatible Apple identity, provisioning profile and connected
iPad remain required for the signed physical-install gate.

## 2026-08-01 — Reopened and corrected current iPad portrait clipping

### The geometry fix exposed a separate layout failure

The accepted level-visibility run showed complete textures and world geometry,
but its fresh Computer Use frame also showed the iPad shell in portrait with
the game clipped into the upper region. The right View/Item/Brake/Gas cluster
was absent and a large black region remained below. The renderer log explained
the shape: it still initialized a `1376x1032` landscape window inside the
portrait shell.

This was not another missing-asset failure. The scene itself contained the
forest, building, ground, foliage, kart and character. It was a UIKit/SDL
orientation-policy mismatch, and the prior broad claim that every portrait
control reflowed was reopened instead of protected by documentation inertia.
The lifecycle fix's balanced appearance transitions and retained drawable were
not invalidated.

### The plist and runtime hint contradicted one another

The generated iPhone plist declares only both landscape orientations. Its iPad
override declares portrait, upside-down portrait and both landscape
orientations, as required by the already documented iPadOS 26 windowing policy.

Immediately before `SDL_Init`, however, `Platform_Init` still called:

```text
SDL_SetHint(SDL_HINT_ORIENTATIONS, "LandscapeLeft LandscapeRight")
```

SDL's UIKit controller intersects this hint with the plist mask. The result is
appropriate for iPhone but reduces iPad to landscape-only too. On current
iPadOS, the outer scene may be portrait while that controller remains
landscape, producing the exact clipped dimensions observed.

The correction advertises all four orientations in SDL's hint. The target-
specific plist remains authoritative, so iPhone still intersects to landscape
while iPad retains all four. No device-specific Objective-C bridge, forced
scene request, transform or second renderer layout was introduced.

### Dirty-source proof before commit

Both Simulators remained shut down for compilation. iOS Simulator and
iPhoneOS were built sequentially at nice priority 15 and one Ninja job. Both
linked with the established 32 legacy C warnings and no new Objective-C
warning. Candidate executable hashes were:

```text
iOS Simulator  23edbc1ad3f969ccc0194a9e1f63ea296488acb01b3e0dfbf0ca8ead457180a8
iPhoneOS        790727855a50471b9a2e8defb249bf209f5e5660f10d9e9de6de6a95cc9791cb
```

The Simulator candidate embedded `faa32dc04ffc-dirty`. Only disposable
`CTRPad Import Negatives` was booted; protected `CTRPad Import Validation`
stayed shut down. Install migrated the data container from
`F5B6B88F-640E-440B-888D-FC9514311E11` to
`A5E280A9-E7D6-4E99-AF2A-0CF9DC890CF8` while preserving disc/save identities.

Cold launch in the already portrait device now reported `1032x1376`. The legal
screen filled the portrait surface and showed both drift buttons, utilities,
stick and all four right actions. One refreshed Computer Use rotation produced
a full landscape Crash/trophy/checkered scene. One newly addressed return
rotation produced a complete portrait title/menu with all 11 control
identifiers. This directly exercised the route that the previous build failed.

The candidate was boundedly terminated and the disposable device shut down.
The active log contained no asset/lifecycle error and the canonical save still
hashed to `6a01b0f5...619a`.

### Published source and exact clean builds

The one-file implementation was committed and pushed as
`2c78c040bf2a766d3a12e8fdbff11306e04eb517` before exact rebuilding. Both iOS
presets were explicitly reconfigured so CTRPad and SDL embedded the clean
identity `2c78c040b`. Compilation again ran sequentially, at nice priority 15
and one Ninja job, with both Simulators shut down. Exact executable hashes
were:

```text
iOS Simulator  87188026542dfee99e7e060f64ef77bff6a81924b477fec704f4ba729ba06b9c
iPhoneOS        3dc6e3c7706e996d59668ffb2c91024473d22b068c0565317fabdc87ccd9ebc1
```

Both are thin ARM64 Mach-O products, report version `0.1.0-beta.7.1`, and
embed SDL identity `SDL-3.4.10-beta-7.1-160-g2c78c040b`. Generated-plist
inspection confirmed that iPhone still has only both landscape entries while
iPad has portrait, upside-down portrait and both landscape entries. The 32
established legacy C warnings repeated without a new Objective-C warning.

### Exact portrait/landscape/portrait acceptance

Only disposable `CTRPad Import Negatives` was booted for the exact run;
protected `CTRPad Import Validation` stayed shut down and untouched. Installing
the clean app migrated the disposable data container while retaining the exact
BIN inode, size and modification time and the exact save inode, size,
modification time and SHA-256 `6a01b0f5...619a`.

The attached console reported build `2c78c040bf2a`, portrait window
`1032x1376`, Apple Software Renderer, GLES 3.0, all PSX/VRAM pipelines, the
touch overlay, UIKit display loop and CoreAudio. Computer Use refreshed state
before every action. Cold portrait filled the legal scene and retained all
controls; one fresh Rotate action filled landscape with the textured Naughty
Dog/title scene and all controls; one newly addressed return rotation filled
portrait and exposed all 11 control identifiers in the accessibility tree.

Exact local-only frame hashes were:

```text
portrait cold    43e62bf243f7f89a9f1e7b61c486ad5773b50b2cbb990e487d09f9c2619a1ec7
landscape        092ccc06911f02f115d32c192535438fef162a713f30405d7a0436ac2bb0852a
portrait return  0953cef917839a96a05c1904d6a8b469dba80952beccc239ae66c555430d478a
```

They remain outside Git so retail-derived pixels are not published. The active
file log contained no `[CTR AssetRef]`, cache-exhaustion, `ERROR`, `FATAL` or
unbalanced-appearance line and hashed to
`e18608c228102db26e901efe76c5f6108b50869e4aed2d2363bbff35c3cfa9b6`.
The attached Simulator console emitted its known duplicate WebCore/WebKit
accessibility-class warning and one Foundation
`NSMapGet(...): map table argument is NULL` diagnostic after the first
rotation. The Foundation line occurred once in both candidate and exact runs,
did not recur on the return rotation, and did not interrupt pixels,
accessibility, periodic FPS output or bounded termination. It remains an
explicit Simulator-side diagnostic rather than a hidden success condition.

After termination the BIN and save identities were still exact. The disposable
Simulator was shut down, leaving both named devices shut down. The current
Simulator clipping defect is accepted as corrected without relabeling this as
physical-iPad acceptance. Live-device orientation/window resizing, Stage
Manager behavior, cadence, energy and simultaneous human touch remain open;
M10 and the overall goal remain active. Full evidence is in
`docs/parity/2026-08-01-ios-orientation-hint.md`.

## 2026-08-01 — Re-audited user media and release hygiene without new load

The user asked that the newly populated `ref/CTR/` inputs be checked while the
machine remained slow. A read-only inventory found six ignored entries totaling
about 1.3 GB: the current 605,698,800-byte BIN and 95-byte CUE, `.DS_Store`,
and the older 740,179,104-byte IMG, 788-byte CCD and 30,211,392-byte SUB.
`.gitignore` excludes the entire directory and `git status --ignored` reported
only `!! ref/CTR/`; none of those files entered the index.

The current CUE still names the BIN as one `MODE2/2352` track at
`INDEX 01 00:00:00`. Its BIN size is exactly 257,525 raw sectors and matches
the already accepted NTSC-U `SCUS_944.26` fixture identity and preservation
records. The older CloneCD metadata describes 314,702 sectors; its documented
runtime identity remains PAL `SCES_021.05`, so it is a negative region fixture,
not an input for the NTSC-U build. The canonical BIN had already been hashed
and exercised through import, rendering and save-preservation acceptance; a
new multi-file full hash was deliberately not repeated.

A tracked-file extension scan found no BIN, CUE, IMG, CCD, SUB, ISO, CHD, PBP,
BIG, HWL, XA, STR, TIM, save, memory-card, IPA, provisioning-profile,
certificate or private-key artifact. `LICENSE`, `THIRD_PARTY_NOTICES.md`,
`docs/INSTALL-IOS.md`, the top-level build instructions and the packager's
retail/runtime exclusion checks are tracked. `bash -n package-ios.sh` passed;
CMake listed all four Apple configure presets; the existing macOS build exposed
the complete 22-test manifest through `ctest --show-only`. These were syntax,
inventory and manifest checks, not a relabeled compile or test run.

The audit caught two stale prose issues. README still listed live wrong-region
UI as open even though the dedicated Files follow-up accepted it; the actual
open software case is an inaccessible-provider callback plus physical-device
behavior. M10 also repeated its older landscape/full-screen clause. Both were
corrected without changing an implementation or acceptance boundary.

Resource inspection found only 4,687 free 16-KiB pages, about 73 MB, with a
heavily occupied compressor. No simulated device was booted and no compiler or
full-media hash was started. A final process audit did find an idle Simulator
app showing a shut-down `OpenRCT2 Touch Invalid Import` window. Computer Use
refreshed its accessibility state, opened the Simulator menu and selected the
normal Quit action. Final process and `simctl` reads showed zero Simulator
processes and zero booted devices. The next clean rebuild remains deferred
until it can run without making the user's computer less responsive; this is a
resource decision, not a build failure or new goal blocker.

## 2026-08-01 — Added and accepted an exact corresponding-source artifact

### Chose release progress that did not overload the host

The next roadmap action was a current-head clean build, but resumed inspection
found only 3,743 free 16-KiB pages, roughly 58 MB, and 10.60 GB swap in use.
Both Simulators remained closed and no compiler was running. Rather than repeat
the previously documented unsafe unity-build attempt, work moved to M11's still
open corresponding-source release artifact, which required Git/archive I/O but
no compilation or Simulator.

The repository and draft PR already exposed the tracked GPL source, yet there
was no deterministic release tarball tied to the app's embedded commit. New
`package-source.sh` packages one committed Git object rather than traversing
the working tree. It requires the build system, platform/game/include/tools and
vendored SDL source, licenses, Installation Information, modification records
and both packagers. Archive-list scans reject retail/runtime media, saves,
binary packages, profiles, certificates, key-like files and generated/runtime
trees before gzip publication.

The README and iOS Installation Information now direct a distributor to create
the source archive from the same clean commit as the IPA and publish both
sidecars. The source packager refuses tracked changes and existing outputs,
uses a commit-derived root plus timestamp/name-free gzip output, and verifies
the completed tar/gzip/member contract.

### The first exact run exposed an incomplete-publication window

Before commit, `bash -n` and help passed. A run against the dirty tracked tree
failed before output with the intended commit-first error. The implementation
and documentation were committed and pushed as `95dcb67f177b`.

The first clean archive then completed. During the second sequential command,
the execution session ended after the 17-MB tarball moved to its requested
path but before `shasum` filled the sidecar. The local directory therefore held
one complete pair plus a second tarball and zero-byte checksum. That state was
rejected, not normalized into a reproducibility claim.

Although the interruption was external, it revealed that the script published
its archive too early. The correction calculates and validates the digest,
writes a basename-relative sidecar, and verifies both inside private staging
before moving the complete pair and checking it again at the destination. It
was committed and pushed as `4091b602ab2a`.

### Exact corrected package and extraction proof

Two separate nice-15 runs from exact clean commit
`4091b602ab2abc74db584a56de06faa23905a96a` completed in about 15.42 and
17.84 seconds. Each output was exactly 17,482,944 bytes with 3,233 members and
root `CTRPad-source-4091b602ab2a/`. Both hash to:

```text
d1c4b4fb119605ab56475d2a6861555a71bceac146ea2b9004c498adf5e64df1
```

`cmp` succeeded. Both basename-relative sidecars passed independent
`shasum -a 256 -c`; gzip and tar validation passed. The exact member list had
no prohibited extension/path and retained executable modes on both packagers.
An invalid-ref run failed before creating output.

One exact tarball was extracted into a fresh temporary directory. It contained
no `.git` tree. Both packagers passed `bash -n`; its own CMake metadata listed
`macos-arm64`, `macos-arm64-app`, `ios-simulator-arm64` and
`ios-device-arm64`; a second extracted-filesystem restriction scan was empty.
The archives, checksums, interrupted diagnostic output and extracted tree stay
local-only.

Final resource state still had only 3,693 free 16-KiB pages and about 10.55 GB
swap used. No clean compile was attempted and no Simulator was opened. This
accepts deterministic complete source-artifact generation and extraction, not
a clean-machine build, legal review, same-identity IPA, signing or physical
device acceptance. M11 and the overall goal remain active.

### Final credential-policy hardening and documented-head repeat

A last guard audit found no secret in either archive but did find incomplete
future suffix coverage: Apple `.p8` and `.pfx` keys, alternate provisioning-
profile names and common certificate encodings were not named. `.gitignore` and
the source member rejection were extended together, and the archive now
requires its own ignore policy. This landed as `21fb81296bd0` before the first
acceptance documentation commit `71b68f118b18`.

That fully documented pre-report head was then packaged twice, again
sequentially at nice priority 15. Runs took about 14.53 and 13.10 seconds. Each
archive had 3,234 members, exact size 17,488,503 bytes, root
`CTRPad-source-71b68f118b18/` and SHA-256:

```text
1681c4587c03e51618a42b4bee793d2dd62f3655f16b66b8dbc06f518cb553f3
```

`cmp`, both sidecars, gzip, tar and the expanded Apple credential/profile/key
scan passed. Fresh extraction again had no `.git`, passed both shell syntax
checks, exposed all four Apple presets and had no prohibited member. Final
headroom remained unsuitable for compilation at roughly 62 MB free and
10.51 GB swap used, so the clean-build boundary remains unchanged.

## 2026-08-01 — Added isolated signing-keychain support and bounded synthetic proof

### External signing inputs remained absent

Resumed inventory at clean GitHub head `65c3f4ab3` found zero valid code-signing
identities, no profile in either standard Xcode directory and no connected
device. Free VM pages had improved only to 8,648, about 135 MB, with 10.50 GB
swap still used. No Simulator or compiler was started.

The signed packager also searched only the default keychain list. An optional
`--keychain` input now requires identity/profile signing, requires the path to
exist, restricts `security find-identity` to that unlocked keychain and passes
the same path to `codesign`. It does not unlock the keychain or change the
default/search list. Installation Information documents the isolated-keychain
route.

Candidate syntax/help checks passed. A keychain without identity/profile and a
present but untrusted identity both failed before IPA output. The ordinary
unsigned path still produced a valid seven-member retail-free archive. The
two-file implementation was committed and pushed as `37a5e16760ba` before
exact repetition.

### Synthetic identity routes stayed explicitly non-Apple

All fixtures lived in one private temporary directory. OpenSSL 3's default
PKCS#12 encoding failed macOS import with a misleading MAC/password message;
regenerating only the container with `-legacy` imported the same synthetic key
and self-signed code-signing certificate. X.509 basic policy saw it but reported
`CSSMERR_TP_NOT_TRUSTED`; code-signing policy exposed zero valid identities.

Adding user trust would have required an authorization interaction. No prompt
was accepted, later trust inspection was empty, and the real packager retained
its valid-identity requirement. Exact `37a5e1676` therefore repeated the
untrusted failure and created no IPA or checksum.

The synthetic CMS profile decoded through `security cms -D` with iOS platform,
matching synthetic App ID/team/device and future expiration. Its SHA-256 was
`b0cc01e9...c8ca`. A separate downstream diagnostic embedded that profile,
constructed the same four entitlements and applied an ad-hoc DER signature.
Deep/strict verification passed and the extracted entitlements hashed to
`518d2312...716e`. CodeDirectory v20400 simultaneously reported ad-hoc flags,
ad-hoc signature and no TeamIdentifier, keeping the proof correctly bounded.

### Exact unsigned regression and cleanup

Two exact clean-head unsigned packages used the same commit-time
`SOURCE_DATE_EPOCH`. They compared byte-for-byte, passed ZIP validation,
contained exactly seven legal/app members and excluded retail/runtime/profile/
signature state. Both hash to:

```text
582b8491ecab23cd0ebb944a806a21beb8fba8eb7270cd2153121762f9e2b929
```

The temporary keychain was deleted with `security delete-keychain`. The user's
search list again showed only the original login keychain, valid code-signing
identities remained zero and trust settings remained empty. Synthetic keys,
profiles, IPAs, the diagnostic app and entitlement files stayed outside Git.

This accepts explicit isolated-keychain mechanics, fail-closed identity
handling and unchanged unsigned behavior. It does not accept a real signed IPA,
Apple authorization, physical installation or device execution. M11 and the
goal remain active.

## 2026-08-01 — Refreshed the exact current-head Apple matrix

### Resource gate and ordinary ARM64 acceptance

Work resumed at clean local/upstream/draft-PR head
`bbb17478c76d7f8868ebfd3a6bf1b6d4cc90bfa5`. Initial raw free pages were low,
but `memory_pressure` reported 53% system-wide free and zero throttled pages.
There was no Simulator application and `simctl` reported no booted device.

`macos-arm64` was explicitly reconfigured, then built at nice 15 with one job.
The large unity translation completed with 32 established legacy warnings and
no error. Free pages dipped into the low thousands during compilation, but no
sample showed throttling and swap did not grow. Exact product identity:

```text
file     Mach-O 64-bit executable arm64
version  CTR Native 0.1.0-beta.7.1 (bbb17478c76d)
SDL      SDL-3.4.10-beta-7.1-169-gbbb17478c
SHA-256  279a7965a616995f1c4525322568ad81a4a66519890fb349fb73c05fcfa4aea2
```

The complete ordinary suite passed 22/22 in 3.52 seconds; `/usr/bin/time`
measured 3.69 seconds wall, 0.56 user and 0.94 system.

### Sanitizer route and exact result

The first convenience command,
`cmake --preset macos-arm64-sanitizers`, was rejected before compilation
because that preset does not exist. Available presets were inspected rather
than assuming a typo was a product failure. The established sanitizer cache
specified RelWithDebInfo, ARM64, AddressSanitizer plus UndefinedBehaviorSanitizer
and frame pointers. It was explicitly reconfigured with those flags and built
at nice 15 / one job.

That unity compile took several minutes and repeated 59 established warnings.
Sampled system-wide free memory stayed at 42–47%, throttled pages stayed zero,
and no Simulator was opened. Exact product identity:

```text
file     Mach-O 64-bit executable arm64
version  CTR Native 0.1.0-beta.7.1 (bbb17478c76d)
SHA-256  5172e794d772adc76bffd397f96790fe999cb0bd6a479f7a057ea869df3ca28d
```

With `ASAN_OPTIONS=symbolize=0:abort_on_error=1:detect_leaks=0` and
`UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1`, all 22 tests passed in
6.53 seconds. The outer wall time was 6.59 seconds. Leak detection remains off
only because Apple's ARM64 runtime does not implement LeakSanitizer; there was
no address or undefined-behavior finding.

### Sequential iOS compile/link evidence

`ios-simulator-arm64` and `ios-device-arm64` were each reconfigured and built
sequentially at nice 15 / one job. Both compiled UIKit/GLES Objective-C
sources, linked a thin ARM64 executable, repeated the 32 established Apple
warnings and embedded exact version, source and SDL identities.

```text
iOS Simulator  47984fe3f417a3c19060ec4a69d3c26093f24110100dbc1fcc31fefc54c81333
iPhoneOS        7229ffd73d3e86f45f3c853bfee8270467e68f3a77e9e078c85f0f9088112490
```

Both plist products report `io.github.chrissotraidis.ctrpad`, iOS 15.0 and the
intended iPad orientation mask. The about-3.5-MiB bundles contain only the
executable, plist and GPL/notices/Installation Information. No retail file is
present by design: the Files workflow imports the user-owned BIN to
Documents/assets/ctr-u.bin.

The Simulator executable is linker-ad-hoc signed with identifier `CTRPad` and
no team. Deep/strict bundle verification repeated the known
`code has no resources but signature indicates they must be present` result.
The Simulator accepts that development state; it is not a physical signature.
The iPhoneOS bundle correctly reported `code object is not signed at all`.

The first shell wrapper for those statuses used zsh's read-only `status`
parameter and exited before a result. It was repeated with task-specific
variables. A cleanup attempt later passed multiple files to macOS `unlink` and
removed none; the private temporary files were enumerated and unlinked
individually instead. These operational corrections changed no product or user
data.

### Exact unsigned packaging and boundary

Two sequential unsigned packages used the exact device app and fixed commit
epoch `1785581547`. Runs took 14.32 and 5.02 seconds. `cmp`, ZIP validation and
both checksum sidecars passed. Each seven-member IPA was exactly 1,449,260
bytes and SHA-256:

```text
05ff4601973413f279b7295f1fd885f74b52ddf011360aae7adb2a8f41c97dab
```

The members were only `Payload/`, `Payload/CTRPad.app/`, executable, plist and
the three distribution documents. There was no retail/runtime/profile/signature
state. The temporary IPAs and sidecars were deleted after acceptance.

Closing resource state was 47% system-wide free, zero throttled pages and
9,836.94 MiB swap used, slightly below the start. Final process reads still
showed zero Simulator applications and zero booted devices.

This accepts the current-head ordinary/sanitizer/iOS/unsigned-package refresh,
not a clean extracted-source build, current macOS app bundle, Apple signature
or physical-iPad behavior. Exact commands and all hashes are in
`docs/parity/2026-08-01-current-head-apple-matrix.md`. The goal remains active.

## 2026-08-01 — Closed the fresh extracted-source build gate

### The first archive exposed a build-identity gap

After the exact Apple matrix, `package-source.sh` produced a 17,505,157-byte,
3,236-member archive from documented head `34ea4415cda8` at SHA-256
`369152ea...d80d`. It passed gzip/tar, extraction, no-`.git`, shell-syntax and
four-preset checks. An independent `shasum -c` first ran from the repository,
so its basename-relative sidecar could not find `source.tar.gz`; rerunning from
the output directory passed.

Static inspection before compilation found a real release problem. Existing
CMake only asked Git for the source hash and dirty status. With `.git`
intentionally absent, the extracted application would embed `unknown-dirty`,
breaking the documented promise that its build identity names the source
archive. The route was rejected before a long compile.

### Identity correction and negative controls

CMake now accepts an optional `CTR_NATIVE_SOURCE_COMMIT`, validates 12–40 hex
characters and verifies that any override in a real checkout matches the
actual checkout. Git is authoritative only when its top level equals the CMake
source root, preventing an archive nested under an unrelated repository from
inheriting the parent identity. Without root Git, CMake accepts the exact
12-hex generated archive-root suffix or a validated explicit override for a
renamed tree. Other roots remain `unknown-dirty`.

A candidate Git configure used the real `34ea4415cda8` commit and retained
`-dirty` for the three tracked changes. Override `000000000000` failed with a
checkout mismatch; `not-hex` failed the format rule. Neither generated a build
graph. README and Installation Information now document normal and renamed
extraction.

Commit `4673e1f9fcab` was pushed before exact archive/build proof.

### Exact archive, build and test

Two sequential source packages from full commit
`4673e1f9fcabbb27526f8422dc9c35aaf356f50e` compare byte-for-byte. Each is
17,505,454 bytes with 3,236 members and SHA-256:

```text
5cdbbf9d8939b4aa63b157bdcd1e865328531909da3763862aa9e37bcb51a2cd
```

Both sidecars passed from their containing directory. The packager's archive,
required-member and retail/runtime/package/profile/key checks passed. Fresh
extraction had no `.git`, both packagers passed `bash -n`, and all Apple
presets enumerated.

The untouched `CTRPad-source-4673e1f9fcab` root configured with the standard
macOS preset at nice 15. It emitted exact source identity `4673e1f9fcab` from
the corresponding-source root and completed in 73.09 seconds. The fresh
one-job build compiled 242 targets in 209.27 seconds, repeated 32 established
warnings and linked:

```text
Mach-O    64-bit executable arm64
version   CTR Native 0.1.0-beta.7.1 (4673e1f9fcab)
SHA-256   a645cdece83a697ff1cd628ec93dee4c393fe353fa8476f3dd6b471e80bfade2
```

All 22 CTests passed in 3.06 seconds; the outer wall measurement was 3.29
seconds. No retail media was present or read.

SDL's separate revision helper emitted `SDL-3.4.10-HEAD-HASH-NOTFOUND` because
its vendored tree also lacked Git metadata. The exact app source ID and hashed
complete SDL source remain sufficient for this gate; the diagnostic difference
is retained as optional polish rather than rewritten as parity.

### Renamed root, resources and cleanup

A second extraction was renamed to `renamed-source`. With the full explicit
commit it configured in 76.72 seconds, emitted the dedicated override identity
message and generated exact compile definition `4673e1f9fcab`. The same source
had already completed the full build, so a redundant unity compile was skipped.

All work was sequential, nice 15 and one job. No Simulator application opened
and no device booted. Closing memory state was 48% system-wide free, zero
throttled pages and 9,700.94 MiB swap used. Three exact task-owned temporary
trees totaling about 279 MiB were deleted after evidence collection; no
repository, retail or Simulator-container path was targeted.

This accepts same-host fresh extracted-source configure/build/test and exact
archive-root/override identity. An independently provisioned clean Mac, current
macOS app/iOS products, real Apple signature and physical device remain open.
Full evidence is in `docs/parity/2026-08-01-extracted-source-build.md`; M11 and
the overall goal remain active.

## 2026-08-01 — Reopened Simulator stability before physical-device work

### Why earlier evidence was insufficient

The user made the acceptance boundary explicit: no physical iPad until the
Simulator is stable, visibly complete and diagnosable. Earlier evidence had
proved the corrected visibility cache in bounded scenes, but it had not proved
long scene churn, reliable keyboard/accessibility input or a usable diagnostic
history. A current baseline therefore had to be run instead of treating the
previous checkpoint as permanent acceptance.

The exact `43245107c279` baseline was compiled with one job at nice 15 while all
Simulator processes and devices were shut down. Only the disposable import-
negative device was later booted. Title, menu, character, track, Crash Cove and
pause frames were coherent, and the complete 88-line app log contained none of
the known asset/cache/error markers. This did not close the gate: runtime
frequently fell to roughly 4-8 FPS, some short inputs needed repetition, and
the logger had already destroyed the preceding session by opening with `w`.

### Durable evidence and input semantics

The native logger now rotates four prior sessions before opening the current
file. Persistent entries carry UTC wall time, elapsed session time and severity
and are flushed after every call. Startup identifies the version, build,
compiler, platform target, base/assets/writable paths and current/previous log
paths. Native input records only mapped key and touch transitions, including
the effective mask; it does not dump retail data or invent UI text input.

UIKit's accessible game-button action previously animated a button without
guaranteeing the `UIControlEventTouchDown` transition that owns the emulated
pad state. The dedicated game-button subclass now sends a touch-down followed
by a delayed touch-up-inside through the existing targets. This preserves the
same state machine as physical touch and leaves onboarding/utility buttons on
ordinary UIKit behavior.

Two isolated renderer self-test invocations proved log rotation and retained
both sessions while the renderer pixel hash stayed `851169f2644a1675`. The
ordinary macOS build passed 22/22 CTests. A live iOS diagnostic then showed
single accessible Start/Cross actions causing the expected retail transitions
and logged their exact press/release pairs. The same PID survived
Home/background/foreground and retained name-entry state. Both the new current
log and retained baseline were free of the targeted asset, cache, error, fatal
and unbalanced markers.

### Rejected claims and remaining route

The diagnostic iOS binary came from dirty source while its existing CMake graph
still embedded the previously configured clean commit. It is useful behavioral
evidence but cannot identify the correction exactly and therefore cannot be
release evidence. Similarly, coherent pixels in the inspected screens do not
prove every track, kart, HUD, effect or recycled memory-pack state. Anticipated
physical-device performance does not waive poor Simulator usability.

The next accepted checkpoint must start from a committed clean source identity,
compile with no booted Simulator, boot exactly one disposable device, exercise
presentation, menus, Adventure/Load, Time Trial and several scene/level
transitions, and correlate keyboard/touch, rotation and Home/resume against the
complete current and rotated logs. It must report frame behavior and every
visual or diagnostic defect honestly. Only that can reopen physical-device
work. Full current evidence is in
`docs/parity/2026-08-01-simulator-stability-logging.md`.

### Exact replay exposed a VSync/retail-consumer boundary

After committing and pushing `7bcc51a790a1`, both Apple build graphs were
freshly configured with no booted device. The exact macOS build passed all 22
tests; the exact iOS product was fully ad-hoc signed in a temporary copy and
installed as an update on the sole disposable device. Four generations of
real logs rotated correctly.

The graphical claim strengthened: Adventure Load and its saved slot, character
garage, name keyboard, hub, Crash's kart, Aku Aku, HUD/minimap, orientation
changes and same-PID lifecycle recovery all rendered coherently without the
known asset/cache diagnostics. Full-resolution inspection resolved an
initially suspicious large feathered object as the correctly textured Aku Aku
mask rather than corrupt geometry.

The same run invalidated the control claim. SDL ingress logged `P`, `K` and `S`
with the expected Start, Cross and Down masks. Start advanced, but later quick
Cross/Down did not affect stable retail menus; accessible Cross and a UIKit
stick drag did. Repeating Down did not make it deterministic. The observation
was not relabeled as an automation issue because the product promises quick
keyboard taps and the mapped events were demonstrably inside the app.

Code order explained the miss. `MainDrawCb_Vsync` polls host events, writes a
PSX-shaped packet and immediately calls `GAMEPAD_PollVsync`, but menu code does
not derive held/tapped buttons until the later `GAMEPAD_ProcessHold`. The old
two-native-snapshot latch was therefore measured in VSync callbacks, not in
retail consumers. With game logic at 4-8 FPS, two pressed packets could be
followed by neutral before the menu sampled them.

An arbitrary millisecond timeout or larger fixed snapshot count was rejected:
either can still expire before a stalled frame or remain held long enough to
trigger retail repeat behavior. Instead, mapped quick keyboard and touch edges
now persist until `GAMEPAD_ProcessHold` has sampled all retail pads. A native-
only acknowledgement then clears the transport latches and logs the consumed
masks. Physical held state still comes from SDL/UIKit, while replay installs,
disabled pad communication, suspend, restore and shutdown retain their
fail-closed clearing behavior.

The self-test now proves persistence through repeated host reads, explicit
retail acknowledgement and immediate neutral release for both keyboard aliases
and touch taps. Its isolated CTest passed. Simulator and device were fully shut
down before compilation. Exact committed builds and the live keyboard route
remain the next acceptance step.

## 2026-08-01 — Accepted retail-consumer input, retained the Simulator gate

### Exact compilation without Simulator load

The retail-poll acknowledgement was published as
`ba80d153ae558cc74d1660043e32e58ce8baad46`. Both named devices and the
Simulator GUI were shut down throughout compilation. Sequential nice-15,
one-job builds produced thin ARM64 macOS and iOS Simulator executables at
SHA-256 `55070327...c2ed2b` and `6f5f7152...eb923` in 67.25 and 69.93 seconds,
respectively. Both embedded `ba80d153ae55` and repeated the 32 established
warnings. The macOS suite passed 22/22 in 2.41 seconds. A copied Simulator
bundle was fully ad-hoc sealed, strictly verified and transformed only its
executable identity to `6d944fed...c5c55`; this was never labeled a device
signature.

### Exact keyboard-only two-level route

Only disposable `CTRPad Import Negatives` booted. Its complete 605,698,800-byte
BIN and 6,016-byte save retained hashes `f780bf23...7c0` and
`6a01b0f5...19a3`. The published keyboard map navigated presentation to Time
Trial, selected Crash, loaded Crash Cove without a ghost, skipped its fly-in,
applied Gas, paused, moved the pause selection, resumed, paused again, opened
Change Level, selected Roo's Tubes, skipped its fly-in and reached that track.
All 20 logged keyboard ingress edges have 20 ordered consumer lines naming the
same masks about 0.10-0.21 seconds later. This is the first exact slow-
Simulator run in this checkpoint where every stable-menu quick key reached the
retail `GAMEPAD_ProcessHold` consumer without repetition.

Full-resolution Computer Use inspection covered complete character tiles,
Crash Cove preview/fly-in/grid/race/HUD/minimap and Pause, then distinct Roo's
Tubes preview/fly-in/grid/tunnel/banner/speedometer pixels. Portrait,
landscape and portrait remained coherent. An explicit Home/resume cycle kept
PID `65296`, course state and pixels and wrote all four ordered lifecycle
events at +866.705 through +878.179 seconds. Representative local simctl
screenshots hash to `43828ed5...ab644` and `eb9b565c...bb5c`; no retail-
derived image entered Git.

### Five-generation log and system diagnosis

The final current file contains 128 lines / 13,642 bytes at SHA-256
`fa832418...bbcc1`. Its four archives are 5,685, 9,370, 10,396 and 3,704 bytes
at the previously recorded `aa12c685...9353`, `8ec554ad...d8687`,
`17f50a77...dc5` and `12e8a9bf...fc07` hashes. A single targeted scan over all
five found no AssetRef, cache-exhaustion, app error/fatal, unbalanced-render,
assert, signal or crash marker.

The unified log contained five error-level framework diagnostics: one
CoreFoundation plug-in-factory registration and four CoreAudio unavailable-
hardware-profile lines. No new CTRPad crash report existed. These are retained
as Simulator limitations rather than hidden or misattributed to the renderer.

### Rejected full-stability claim and resource cleanup

The 63 FPS samples ranged 4.61-22.80 and averaged 7.37, with later readings
commonly 4.61-5.30 FPS. CTRPad used about 86% CPU and 221,264 KB RSS at one
sample. Input correctness at that speed is accepted; Simulator usability is
not.

The app produced logs for about 19 minutes 38 seconds and survived the tested
lifecycle cycle. The first cleanup command targeted
`com.chrissotraidis.ctrpad`, while the actual built plist identifier is
`io.github.chrissotraidis.ctrpad`. Its `found nothing to terminate` response
therefore says only that the wrong identifier was not running; it does not show
that PID `65296` exited. This was discovered by reading the built plist before
the next diagnostic and corrects the earlier pushed interpretation.

A narrowly filtered headless RunningBoard query had already outlived its
initial wait, but it was unnecessary once the identifier mismatch was found.
The device was shut down rather than left consuming system resources. Final
reads showed both CTRPad devices shut down and no CTRPad/Simulator GUI process.
Two verified task-owned temporary signed-app directories, 3.5 MB each, were
deleted by exact path; the final text log and two hashed screenshot files were
retained in `/tmp`.

This accepts the exact consumer latch, the inspected two-track graphical churn,
rotation/Home recovery and retained logging. Poor software-renderer cadence,
broader level/effect coverage, an observed correct-ID or natural termination,
human multi-touch and every physical-iPad gate remain open. Full proof and
hashes are in `docs/parity/2026-08-01-simulator-stability-logging.md`.

## 2026-08-01 — Profiled and staged final Simulator presentation

The exact keyboard/graphics replay made the next product problem measurable:
the Simulator stayed alive and visually coherent but was too slow to serve as
a practical stability gate. Rather than guess at the renderer, an opt-in dirty
diagnostic added separate packed-VRAM restore/present buckets and draw-call,
vertex, split and semitransparent-split counters. Its preserved 2,772-frame CSV
showed Crash Cove averaging 230.729 ms total, 198.501 ms non-wait work,
137.739 ms in split submission and 47.201 ms in final packed-VRAM presentation.

Inspection of the final path showed that the packed integer VRAM shader decoded
the 320×240-ish logical output over the complete 1032×1376 Simulator surface.
The replacement resolves once at the logical dimensions into a reusable RGBA
FBO and uses `glBlitFramebuffer(..., GL_NEAREST)` only for host scaling. The
GLES entry-point contract now rejects a loader without blit support. The prior
direct path remains private to the renderer pixel self-test.

The pixel oracle was deliberately strengthened before the live run. A 32×16
logical fixture is presented into a 64×32 host window through both paths,
captured and compared byte-for-byte. It passed with the unchanged logical hash
`851169f2644a1675` and staged presentation hash `a7798c5a6ddee965`. The macOS
build linked in 68.06 seconds; the focused oracle passed in 3.64 seconds and all
22 CTests passed in 5.66 seconds. With both Simulators off, the iOS build linked
thin ARM64 in 74.03 seconds with only the established 32 warnings. Unsigned and
strictly verified disposable signed executable hashes were
`48c03822...d534` and `3b54a227...e51`.

Only disposable device `26F3DEE8-8840-446D-85FE-C882009C9C06` was booted.
Computer Use enabled keyboard capture, then `S K K K K I` navigated from the
mode menu through Time Trial, Crash, Crash Cove, No Ghost and fly-in skip. The
new path visibly retained the complete logo/menu, all portraits, Crash model
and kart, track list/preview, ghost prompt, fly-in, lights, banner, HUD,
minimap, track, cliffs, sky, water and touch controls. The persistent log
correlated the route at the retail poll. A later K/D tap stream was explicitly
treated only as ingress/consumer evidence, not sustained driving.

Correct bundle ID `io.github.chrissotraidis.ctrpad` terminated PID `77788`.
The preserved optimized CSV contains 3,000 frames, 990,063 bytes and SHA-256
`e1036716...e034`; the flushed 99-line, 11,145-byte app log hashes to
`9a0b455e...603a`. Five rotating app logs scanned clean of known asset,
visibility-cache, application ERROR/FATAL, render-balance, assert, signal and
crash markers. Unified logging repeated only one CFBundle and four CoreAudio
Simulator limitations; no new CTRPad report existed.

Crash Cove final present improved from 47.201 to 7.737 ms, non-wait work from
198.501 to 150.674 ms and total from 230.729 to 181.024 ms. This is an 83.6%,
24.1% and 21.5% reduction respectively, but 181.024 ms still fails a 33.333-ms
30-FPS budget by about 5.4×. The optimized run now attributes 131.012 ms to an
average 116.64 splits, including 72.64 semitransparent two-pass splits. The
next renderer work is therefore constrained to submission/state reduction that
preserves ordering, STP/blend, mask and framebuffer-feedback behavior. Both
devices and the Simulator GUI were shut down. Full evidence, exact hashes and
the deliberately open clean-replay/broader-churn/device boundary are in
`docs/parity/2026-08-01-simulator-renderer-profile.md`.

The evidence-analysis goal reading was 239,829 seconds (2 days, 18 hours, 37
minutes, 9 seconds), 2,835 seconds after the previous published boundary. The
goal remains active.

## 2026-08-01 — Kept the exact game visible while auditing the next renderer cut

Published `ff26c0815a04` was rebuilt exactly with every Simulator off and
installed into the existing disposable data container. Only
`CTRPad Import Negatives` booted; `CTRPad Import Validation` remained off. The
user-facing viewer stayed open rather than being sacrificed to an overlapping
compile. Its session identifies `ff26c0815a04`, iOS, OpenGL ES 3.0 and Apple's
software renderer. A live 1,553-second snapshot showed coherent Crash Cove
pixels and successful retail consumption of keyboard and touch masks. Its
130-line / 13,371-byte log hashed to `c7e02716...6816` and had no targeted
asset, texture, cache, error, failure, assertion or crash line. The later
visible `9:59.99` clock was checked against `UI_DrawRaceClock`: ordinary
non-seven-lap races intentionally retain that maximum display while the game
and FPS logging continue. It is not evidence that the session froze.

That run remains a rejection for performance: steady race samples are near
6 FPS. The preceding profile explains why. Crash Cove averages about 192 draw
calls for only about 5,535 vertices; roughly 73 semitransparent textured splits
are issued twice because PS1 ABE leaves non-STP texels opaque and blends STP
texels. Reordering or merging those primitives was rejected because it would
change retail overlap behavior.

The current uncommitted prototype instead selects coherent
`GL_EXT_shader_framebuffer_fetch` only when GLES advertises it. A fragment then
reads the existing destination and evaluates the PS1 average, add, reverse-
subtract or quarter-source equation only for sampled STP pixels; sampled
non-STP pixels write opaque source color. The desktop and unsupported GLES
route keeps the established two draws. A dedicated counter will prove which
route ran rather than inferring it from extension availability.

The Khronos extension definition provides the needed ordering contract:
coherent fetch reflects prior overlapping samples in API primitive order. It
also defines ES3 user-declared `inout` outputs and explicitly leaves ordinary
blending orthogonal. Apple's current iPhoneOS and iPhoneSimulator ES3 headers
both expose the extension token. This justifies attempting the optimization;
it does not prove Apple Simulator shader acceptance or pixel equivalence.

The pixel self-test now draws all four PS1 semitransparency modes over a common
blue destination and checks opaque non-STP plus blended STP pixels. A separate
bilinear sample exercises the exact half-STP/half-non-STP boundary where the
old path writes both passes; this prevents accepting one-pass texture halos.
Its marker will include a separate blend hash and the route actually used.
Static review caught two pre-build defects. The generated blend helper appeared
after its consumer macro, and shutdown cleared the availability flag before the
marker was printed. Dependency order is now direct, and the actual active-path
result is saved before shutdown.

No build has been run from these dirty sources while the user is inspecting
the exact viewer. The acceptance sequence is intentionally strict: close the
sole Simulator, compile the portable desktop fallback, pass the focused oracle
and all 22 CTests, compile the iOS Simulator product, require the GLES shader
and enabled-path oracle to match the fallback, then profile and visually churn
one Simulator. Failure or an absent runtime extension means fallback or revert,
not a performance claim.

The in-progress goal reading was 242,344 seconds (2 days, 19 hours, 19 minutes,
4 seconds), 2,515 seconds after the previous evidence reading. The goal remains
active.

The exact viewer ultimately remained open about 37 minutes, 27 seconds before
the next build boundary. Correct-ID termination flushed a final 166-line,
16,467-byte log at SHA-256 `d07493d5...0f6`; the targeted renderer, asset,
cache and application-fault scan stayed empty.

## 2026-08-01 — Proved coherent GLES framebuffer fetch against the fallback

The staged Crash Cove profile made the next experiment precise: roughly 79
textured semitransparent splits in one stable state each issued an opaque/non-
STP draw followed by a blended/STP draw. Reordering those primitives would
violate retail ordering. The candidate instead detects coherent
`GL_EXT_shader_framebuffer_fetch` through the ES 3 indexed extension list and
compiles PSX fragment outputs as `inout`. Each STP fragment reads the current
destination and evaluates average, add, reverse-subtract or quarter-source;
visible non-STP output remains opaque. Unsupported GLES and desktop GL retain
the established two-pass route. A CSV counter records every split that actually
uses fetch.

The first iOS oracle failure was not in the new blend code. SDL/iOS ignored the
requested 64×32 test window and supplied the real 1032×1376 Simulator surface.
Both direct and staged readback failed before capture because their fixed
buffers described the requested size. Rebuilding exact clean `ff26c0815a04`
as a control reproduced the same failure. The test now validates the actual
host dimensions, checks overflow and allocates both presentation captures from
that size. Desktop still exercises 64×32; iOS exercises every byte of
1032×1376.

The corrected host test exposed a subtler failure: all selected average/add/
subtract/quarter pixels passed while the complete blend hash differed. A
stronger GLES oracle now disables the runtime capability, draws the complete
fixture through the portable path, reenables it, draws through fetch in the
same context, and reports the first byte mismatch. It stopped at `(21,7)` red,
expected 0 and actual 62. Bilinear combined visibility exceeded one half even
though STP and non-STP contributions were each below one half. The portable
path discarded both passes; the candidate had written quarter-source. The
one-pass path now discards when both individual contributions are below the
same threshold. The full buffers then matched byte-for-byte at
`f6dc5a2e558bc7b5`.

Both named devices and the Simulator GUI remained shut down for every compile;
all builds ran sequentially at nice 15 with one job. The final macOS increment
linked in 75.79 seconds, printed logical `851169f2644a1675`, blend
`f6dc5a2e558bc7b5`, portable oracle match and staged-presentation
`a7798c5a6ddee965`, then passed 22/22 CTests in 1.90 seconds. The final iOS
increment linked in 65.07 seconds. Both repeated the same established 32
warnings. The unsigned thin-ARM64 iOS executable hashed to `76473dce...a54`;
the strict/deep-verified disposable signed copy hashed to `ff1a87b4...a08`.
Its enabled marker retained the logical and blend hashes, reported
`blend-oracle=match`, and tested the actual 1032×1376 presentation surface at
hash `172d49a34571b64c`. The immediate-self-test UIKit appearance and duplicate
accessibility-loader warnings remain documented harness/framework diagnostics.

Only disposable device `26F3DEE8-8840-446D-85FE-C882009C9C06` booted for the
live dirty run. The protected device stayed off. The signed app updated the
existing container and launched with `--perf --perf-dir
perf-fetch-dirty-ff26`. Computer Use found Simulator keyboard capture disabled,
enabled it explicitly, and used `S K K K K I` to select Time Trial, Crash,
Crash Cove, No Ghost and skip the fly-in. Repeated K taps drove from the grid;
paired D/K taps turned along the first coastal opening. The app log correlated
individual edges and combined `0x4020` masks at the retail poll.

Every inspected screen retained its retail content: CTR mode menu, Crash and
all portraits, track preview, ghost prompt, fly-in, grid, kart lighting, HUD,
banner, cliffs, horizon, animated water, fence and touch overlay. The bounded
route does not claim every asset or track. The final session exceeded 720.887
seconds, producing 5,172 frame rows. Correct-ID termination flushed the
1,722,887-byte CSV at `f8fbef54...ee5` and the 173-line / 18,813-byte log at
`096615ca...c76`. Its targeted error/failure/missing/corruption/assertion/
texture/shader/pipeline scan returned zero. After device shutdown removed the
controllable app window, a normal SIGTERM closed only the explicitly verified
Simulator GUI PID. Both named devices were rechecked off.

The before/after comparison selects identical logical geometry rather than
unlike track positions: 123 splits, 79 semitransparent splits and 5,405.94
split vertices. The preceding two-pass sample has 279 frames; fetch has 700.
Draw calls fall exactly 79, from 205 to 126, and submitted renderer vertices
fall 5,885.87 to 5,423.94. Total time improves 187.614 to 172.324 ms, non-wait
work 154.872 to 147.464 ms, renderer triangles 144.520 to 137.016 ms and split
submission 135.412 to 127.764 ms. Reciprocal throughput rises 5.33 to 5.80
FPS. The 8.15% total improvement is real but insufficient: the frame remains
5.17 times the 30-FPS budget. A later coastal scene averaged 7.67 FPS over 942
frames, but has no matched two-pass position and is recorded only as runtime
characterization.

This dirty gate justifies retaining the code for exact commit/replay, not
accepting Simulator or moving to the device. The full chronology, hashes,
markers and gate decision are in
`docs/parity/2026-08-01-ios-framebuffer-fetch.md`. The evidence-analysis goal
reading was 245,597 seconds (2 days, 20 hours, 13 minutes, 17 seconds), 3,253
seconds after the previous boundary. The goal remains active.

## 2026-08-01 — Converted the fetch prototype into an exact published checkpoint

After the dirty profile passed review, the implementation, new parity report,
roadmap and both history ledgers were staged by explicit path. The cached
macOS tree needed no work and the complete suite passed 22/22 in 2.40 seconds.
Commit `d3b5bd410e9ae3aefcb439b105e67ff95e1ca534` was pushed to
`origin/codex/arm64-apple`. An unqualified `gh repo view` chose the upstream
remote and therefore found no current branch PR; the corrected downstream-
qualified query proved `chrissotraidis/ctrpad#1` already open as a draft. No
duplicate PR was created.

Every exact build ran with both named devices and Simulator GUI off, nice 15
and one job. Reconfiguring macOS embedded `d3b5bd410e9a` in 2.00 seconds. The
revision-dependent relink took 65.03 seconds, repeated the same 32 warnings and
produced executable `1a15c334...8c44`. All 22 tests passed in 2.54 seconds. An
independent exact pixel invocation from a disposable directory retained
logical `851169f2644a1675`, blend and fallback `f6dc5a2e558bc7b5`, staged
presentation `a7798c5a6ddee965`, and the desktop two-pass route. The directory
was then deleted.

The exact iOS Simulator configure/build took 1.78 / 77.44 seconds with the same
warning set. Its unsigned thin-ARM64 executable embeds the exact revision and
hashes to `565ff441...8b8`. A disposable ad-hoc-signed copy changed that copy's
hash to `80d3b864...785` and passed strict/deep verification; the unsigned
artifact stayed unchanged. Only the disposable device booted. The exact GLES
self-test advertised coherent fetch, compiled all four PSX shaders and both
VRAM pipelines, matched every fallback byte at `f6dc5a2e558bc7b5`, retained
logical `851169f2644a1675`, and tested the real 1032×1376 presentation at
`172d49a34571b64c`. Explicit correct-ID termination ended the UIKit shell that
remains resident after the self-test's `SDL_main` returns. The known immediate-
teardown UIKit/accessibility diagnostics followed app-owned success output.

The normal exact app launched with `perf-fetch-exact-d3b5bd410`. Computer Use
enabled keyboard capture, watched the coherent boot/logo/menu sequence, and
repeated `S K K K K I`. K gas taps and D/K turn taps moved Crash into the first
inside cliff, exercising kart lighting, canyon texture, horizon, water, fence,
HUD/map and touch overlay under motion. The persistent log identifies exact
build `d3b5bd410e9a`, the Apple Software Renderer, enabled fetch, ready shaders/
pipelines, UIKit/touch loops and combined `0x4020` retail-poll masks.

Correct-ID termination after at least 285.035 seconds flushed 2,124 frame rows
at `1c74e036...04b`, a 28-byte header-only GPU CSV at `af0f3466...91f6`, and a
130-line / 14,753-byte app log at `f517e967...fbf`. The targeted renderer,
asset, cache, texture, shader, pipeline, application-error and crash scan is
empty. The CSV contains 976 normal Crash Cove frames averaging 144.598 ms
(6.92 reciprocal FPS); its last 300 average 134.157 ms (7.45 FPS), and every
120.28 average semitransparent split records fetch. This exact scene mix differs
from the diagnostic matched state and is therefore runtime acceptance, not a
manufactured second performance comparison.

The first exact analysis filter looked for the dirty run's absent 126-draw /
79-semitransparent state and reached awk's division-by-zero exit after printing
the actual distribution. The replacement analyzed the captured groups and
left the CSV untouched. During cleanup, an explicitly best-effort shutdown
line had an extra terminal `6` in the disposable UDID; its stderr was ignored,
the next line used the verified UDID, and the final list proved both named
devices off. Neither mistake changed a device or product.

The update preserved retail BIN inode `111450682`, 605,698,800-byte size and
canonical `f780bf23...07c0`, plus save inode `111309627`, 6,016-byte size and
canonical `6a01b0f5...19a3`. Computer Use released keyboard capture and quit
the GUI. Six explicitly named temporary signed-app directories were deleted;
the repo, ordinary builds, app data and evidence remain. The implementation
checkpoint is exact-accepted while the Simulator performance and physical-
device gates stay open.

The exact-acceptance goal reading was 247,009 seconds (2 days, 20 hours,
36 minutes, 49 seconds), 1,412 seconds after the dirty evidence boundary. The
goal remains active.

## 2026-08-01 — Batched consecutive coherent-fetch logical splits

The next profile-driven prototype keeps PS1 parsing and logical split traces
unchanged, then scans at `DrawAllSplits` submission. Adjacent fetch splits join
only with contiguous vertices and equal blend/texture/primitive/STP/mask/debug,
`DRAWENV` and `DISPENV` state. Logical counters retain the pre-batch values;
`gpu_framebuffer_fetch_merged_splits` records removed calls. Review added the
Khronos coherent primitive-order rationale beside the predicate.

The pixel fixture now renders two same-mode overlapping quads in addition to
the four blend equations and mixed-STP bilinear case. Desktop fallback requires
12 calls and enabled iOS fetch requires 5; every byte and a twice-blended STP
pixel must match. Logical hash remains `851169f2644a1675`, the new full blend
hash is `0c0d08324ae06c35`, and desktop/iOS presentation hashes remain
`a7798c5a6ddee965` / `172d49a34571b64c`.

Compilation respected the resource rule: GUI off, both devices off, nice 15,
one job. The reviewed desktop rebuild took 69.74 seconds with the established
32 warnings, emitted ARM64 `7b670e2f...1a53`, and passed 22/22 tests in 3.07
seconds. The earlier iOS build took 66.67 seconds; unsigned and disposable
signed hashes are `e4c29176...3e9` and `5c236fb5...c87a`, with strict signing
verification and the 12-to-5 enabled oracle.

Only `CTRPad Import Negatives` booted for the retail profile. Computer Use
captured keyboard and navigated `S K K K K I`, then exercised gas and both turn
directions through grid, coast and canyon/palm views. All bounded assets and
the touch overlay remained coherent. Correct-ID termination after at least
588.165 seconds produced frame CSV `eedb1181...90a` (4,333 lines,
1,456,292 bytes) and log `f0d6035c...955` (224 lines, 24,534 bytes), with zero
targeted faults.

The structurally identical 119-split / 78-semitransparent state has 428 batched
frames versus 151 exact unbatched frames. Calls fall 122 to 66, total 171.223
to 164.187 ms, work 147.173 to 144.513 ms, renderer triangles 136.333 to
133.964 ms and split submission 126.900 to 124.429 ms. The 4.29% reciprocal
throughput gain is accepted for publication, not as the product gate.

The historical record retains five harmless diagnostics: an active CSV was
tailed before flushing; a final command expected a GPU trace even though the
launch requested only frame perf; a size search printed historical copies;
Computer Use required a fresh state before close; and the later redundant
shutdown returned already-shutdown code 405. The corrected explicit checks
proved both devices/GUI off, primary retail/save hashes unchanged and no task
data lost. The first staged-diff check then rejected two Markdown hard-break
spaces; they were removed and the check was repeated before commit.

The dirty-evidence goal reading was 248,826 seconds (2 days, 21 hours,
7 minutes, 6 seconds), 1,817 seconds after the preceding exact-fetch boundary.
Exact post-commit rebuild/replay, broad churn, 30-FPS and physical-device gates
remain open.

## 2026-08-01 — Exact batching publication and retail replay

Commit `13f260cb8a0d` published the batching implementation and dirty evidence.
Exact macOS reconfigure/build took 1.60 / 70.34 seconds at nice 15 and one job,
emitted ARM64 `b36267d0...02b`, repeated the known 32 warnings and passed all
22 tests in 2.62 seconds. The fallback ordered-overlap fixture remained 12/12
draws with logical/blend/presentation hashes `851169f2644a1675`,
`0c0d08324ae06c35` and `a7798c5a6ddee965`.

Exact iOS reconfigure/build took 1.13 / 68.54 seconds. Its unsigned thin-ARM64
hash is `1941ddad...4186`; the isolated strict-verified ad-hoc copy and installed
executable hash to `f9cfc625...9502`. The live GLES oracle used coherent fetch,
matched every forced-fallback byte, reduced 12 draws to 5 and retained actual-
surface presentation `172d49a34571b64c`.

The compound disposable-device boot command returned a session while boot was
settling. A follow-up found the identical install active but issued a second
identical install, briefly overlapping them. No erase/uninstall occurred; work
waited for both. CoreSimulator remapped the data-container UUID from B038 to
2928 while retaining all imports, reports, logs, preferences, retail files and
memory card. Primary inode/hash checks later proved preservation.

The exact retail launch began at copyright. A batched six-key route advanced
slowly and reached name entry; its final screenshot emit alone failed on an
undefined non-persistent helper. Fresh inspection proved the screen. Triangle
and down did not cancel, so reading `SubmitName.c` identified two Start presses
as the intended path. After P/P and a trophy/menu transition, the verified
route selected Time Trial, Crash, Crash Cove, no ghost and the live grid.
Screenshots covered every intermediate screen plus coherent race/HUD/overlay;
the race timer advanced to 0:33.50 and exact retail-poll lines consumed gas and
direction masks.

Correct-ID termination after 799.006 seconds flushed frame CSV
`50fffaed...798` (5,292 lines, 1,786,746 bytes), GPU header
`af0f3466...91f6`, and log `f7a541fb...9529` (117 lines, 12,731 bytes), with
zero targeted faults. The exact 119/78 split group has 145 frames and 66 calls
versus 151 old exact frames and 122 calls. Total falls 171.223 to 165.879 ms,
but draw/split buckets are within noise; acceptance rests on the deterministic
byte oracle, primitive-order contract and structural 45.90% call reduction.

Computer Use's pre-quit accessibility diff still showed capture wording, but
Command-Q closed the GUI and shut down the disposable device. Direct process
and device checks proved both named devices off. Retail BIN inode `111450682`
and save inode `111309627` retained canonical hashes. The task-owned exact
signed-copy directory was deleted after its hash/signature evidence was
recorded; ordinary builds, installed app, app data and repository remain.

The first multi-file patch intended to record that cleanup had malformed
section context and failed verification without applying. A subsequent
read-only context command left the numeric inode inside shell backticks; zsh
reported `command not found`, then `rg` still returned the intended context.
The corrected literal patch applied only documentation.

The exact-acceptance goal reading was 250,402 seconds (2 days, 21 hours,
33 minutes, 22 seconds), 1,576 seconds after dirty evidence. The batching
checkpoint is exact-accepted. Simulator 30-FPS, broad churn, full touch race,
physical-device and final-package gates remain open.

## 2026-08-01 — User-visible one-simulator launch and keyboard route

The repository started clean at published evidence revision `6de7f17cd4cd`,
with both named devices shut down. Only disposable `CTRPad Import Negatives`
was booted. Boot readiness returned before the first compound command reached
its final launch step; an explicit state check showed exactly that one device
booted, and a separate `simctl launch --terminate-running-process` succeeded as
PID 19987. Protected `CTRPad Import Validation` stayed shut down throughout.

Computer Use first captured the copyright screen with the complete touch-first
overlay. Touch Cross advanced to the CTR title and main menu. A direct Down key
moved the highlight from Adventure to Time Trial, proving the new keyboard
route in the running iPad app; C then accepted Time Trial, Crash, Crash Cove and
No Ghost. Fresh screenshots verified character portraits/kart, track list and
thumbnail, loading preview, and the live Crash Cove Time Trial grid with HUD,
minimap, kart, track textures, transparency and overlay all present.

The 60-line, 7,084-byte live app log was still advancing after more than five
minutes. It records Down as scancode 81/mask `0x0040` and C as scancode 6/mask
`0x4000`, followed by matching retail-poll consumption. It also exposes the
remaining blocker directly: heavy menu/race scenes report roughly 5–10 FPS.
This bounded visual pass found no missing-asset screen, but does not close the
broader graphical-coherence, performance, stability or physical-device gates.
The Simulator and race were deliberately left open for user inspection.

The chronology retains two harmless recoveries and one read-only warning. A
screenshot emit reused a helper that was not available in the next Computer
Use call; the input had already landed, and a fresh-state capture verified the
title screen. The keyboard-mapping `rg` included nonexistent `src/` and still
returned the intended `platform/native_input.c` mappings. Neither changed data.

The handoff goal reading was 251,875 seconds (2 days, 21 hours, 57 minutes,
55 seconds), 1,473 seconds after exact batching acceptance. The overall goal
remains active.

## 2026-08-01 — Unified fetch-state prototypes rejected on matched live cost

The next profile-driven attempt tried to merge state transitions that the
accepted `13f260cb8a0d` renderer intentionally kept separate. Renderer-only
bytes were populated after logical trace capture so `GrVertex` stayed 20 bytes
and trace hashes saw their historical zeros. Blend, textured semitransparency,
sampled STP and draw-mask state became flat per-primitive shader input.

The first version also encoded texture format and selected 4/8/16-bit sampling
inside one coherent-fetch fragment shader. Its exact iOS byte oracle passed and
reduced the mixed-state fixture to two draws, but live Apple Software Renderer
cost made the approach untenable. In the matched 119/78 state, 26 calls still
cost 248.567 ms total and 204.207 ms in triangle submission. A transient
Computer Use `noWindowsAvailable` followed landed input; a subsequent CLI state
diagnostic mistakenly included `--terminate-running-process`. The resulting
short profile was discarded. Correct-ID termination finalized the restarted
1,704-line CSV at `23a10888...07` and 58-line log at `2b1b707c...70c`.

The replacement kept separate 4-, 8-, and 16-bit state-fetch shaders and
batched only within one format. Desktop and iOS builds repeated the established
32 warnings; 22/22 tests and every byte/hash oracle passed. Unsigned/signed iOS
hashes were `448eb314...c3f` / `aeccf0e4...7f3`. The enabled actual-surface
marker reported `fallback-draws=12 active-draws=1 mixed-state-draws=4`, logical
`851169f2644a1675`, blend `0c0d08324ae06c35` and presentation
`172d49a34571b64c`.

Only the disposable device booted. Screen-by-screen Computer Use inspection
reached coherent trophy/menu, character, Crash Cove selection, ghost prompt,
loading preview and the live grid with kart, character, track, HUD, minimap,
transparency and touch overlay present. The specialized profile finalized at
3,216 lines / `994a1f79...c2f`; the 69-line retained log is
`ce135e14...810` and has no targeted fault. Primary retail and save inodes,
sizes and hashes remained exact.

Matched data rejected the optimization. The published 66-call state averages
165.879 / 136.514 ms total/triangles. Dynamic state uses 26 calls but 248.567 /
204.207 ms; specialized state uses 52 calls but 236.441 / 190.911 ms. The final
variant therefore regresses total time 42.54% and triangle time 39.85% despite
21.21% fewer calls. Fewer calls alone did not satisfy the product gate.

All candidate source was restored to the published revision through
`apply_patch`. Three reverse-patch wrapper/format attempts applied nothing
before the context-only form succeeded. The restored one-job low-priority
macOS build repeated 32 warnings; 22/22 tests passed in 3.18 seconds and the
independent pixel test passed in 1.27 seconds with the established hashes.
Both devices and the GUI were confirmed off. The two isolated signed copies
were deleted after hash verification; evidence, builds, installed app and
simulator data remain.

The record also retains early literal-tab/macro patch misses, harmless absent-
path searches, prototype-A's accidental restart, prototype-B's recovered
window query, an obsolete retail path, the corrected xdotool-style `super+q`,
a verification loop that shadowed `PATH`, a policy-rejected `rm -rf` followed
by explicit `rm -r`, and a fault scan that first matched the retail title in
log paths. Complete commands, artifacts and preservation boundaries are in
`docs/parity/2026-08-01-ios-unified-fetch-state-rejection.md`.

The rejection/restoration goal reading was 254,820 seconds (2 days, 22 hours,
47 minutes), 2,945 seconds after the interactive handoff. The goal remains
active; Simulator cadence, wider scene churn, touch-race ergonomics, exact
post-publication replay and every physical-device/final-package gate stay open.

## 2026-08-01 — Exact publication replay after renderer-source restoration

The rejection report and its synchronized index/roadmap/decision/history files
were staged by explicit path. `git diff --cached --check` passed. Commit
`125966b21f19ea6bccc3a37b5c12b147cc69ad78` was pushed to the downstream
`codex/arm64-apple` branch; local, origin and existing draft PR #1 heads then
matched exactly. The commit changes documentation only.

Both devices and Simulator GUI were directly verified off before compilation.
The exact nice-15 / one-job macOS reconfigure/build took 65.02 seconds, repeated
the same 32 warnings and produced ARM64 `7c313610...1d0a` embedding
`125966b21f19`. All 22 tests passed in 2.57 seconds. A separate pixel invocation
passed in 1.28 seconds with desktop 12/12 draws and hashes `851169f2644a1675`,
`0c0d08324ae06c35` and `a7798c5a6ddee965`.

The exact iOS Simulator build took 71.72 seconds with the same warning set. Its
unsigned thin-ARM64 executable is `aa9b6483...23b4`; an isolated strict/deep-
verified signed copy is `4cca94f1...6fad`. Only the disposable device booted.
The install remapped the data container to E37C without replacing primary BIN
inode `111450682` or save inode `111309627`; both canonical hashes remained
unchanged. The actual-surface GLES marker returned to the accepted
`fallback-draws=12 active-draws=5` route and retained every established hash.
Correct-ID termination ended the self-test shell.

The normal exact app captured `perf-exact-125966b21`. Computer Use moved
screen-by-screen through the binary crate, Crash/trophy, main menu, character,
track, no-ghost and Crash Cove grid. The first grid capture coincided with the
overlay's hidden phase while its accessibility controls stayed live; a fresh
capture at timer 0:04.13 showed every control. Repeated C/Right taps moved the
timer to 0:18.36, changed the live frame, and emitted exact retail-poll masks
`0x4000`, `0x0020` and combined `0x4020`.

Correct-ID termination flushed CSV `eb5bc3c...1f3` (792,410 bytes, 2,375
complete data records plus header and one non-newline 31-field partial row), GPU
header `af0f3466...91f6`, and log `a1d98d65...107b` (75 lines, 8,702 bytes).
The corrected targeted scan returned zero. The 693 complete normal-race frames
average 171.335 ms / 5.84 reciprocal FPS; final 299 complete rows average
165.816 ms / 6.03 FPS. The 119/78 logical group still uses 66 calls and 56
merges. This accepts identity/correctness, not Simulator performance.

The isolated signed directory was removed after hashing. Installed exact app,
simulator data, builds, evidence and remote source remain. The exact-replay goal
reading was 255,884 seconds (2 days, 23 hours, 4 minutes, 44 seconds), 1,064
seconds after rejection/restoration. The exact checkpoint is accepted; cadence,
wider churn, touch-race ergonomics and all physical-device/final-package gates
remain open.

## 2026-08-01 — Direct RGB5551 decode rejection and publication-history boundary

Source inspection showed that `NativeRenderer_BindMainRenderTarget` already
renders the retail scene at `activeDispEnv.disp` dimensions, so lowering that
target would trade fidelity for speed and was not implemented. The remaining
obvious per-fragment redundancy was the 256x256 RG8-to-RGBA lookup after PS1
4/8/16-bit texture sampling. Nearest uses one dependent lookup; bilinear uses
four. The bounded prototype replaced only that lookup with `highp` integer
decode and intentionally emitted the existing lookup's `channel5 << 3` and
`STP << 7` values. Vertex ABI, logical trace, batching, state, feedback,
resolution and presentation stayed unchanged.

Both named devices were shut down before compilation. Computer Use's first
`super+q` had no active Simulator target; follow-up app-state requests timed
out after device shutdown. Direct process inspection identified PID 32966 and
normal `SIGTERM` closed the GUI. The nice-15 one-job macOS build completed in
71.99 seconds with the same 32 warnings. All 22 tests passed in 3.61 seconds;
the standalone 1.24-second pixel test retained logical `851169f2644a1675`,
blend `0c0d08324ae06c35`, presentation `a7798c5a6ddee965`, and fallback/active
12/12.

The iOS build completed and linked a fresh thin ARM64 app at 14:27:03 CDT, but
tool-output compaction discarded its final console chunk. The exact elapsed
time and warning count are therefore unknown and are not inferred. Direct
inspection found unsigned hash `ddc83538...773d`. An isolated copy was
ad-hoc-signed, passed strict/deep verification, and hashed `65d64ad3...7c86`.

Only disposable `26F3...09C06` booted. The first two compound boot/install
commands ended after boot evidence; the isolated install command completed in
2.71 seconds. It was an update install and remapped data to container 402596C0.
Retail inode 111450682 / 605,698,800 bytes / `f780bf23...7c0` and save inode
111309627 / 6,016 bytes / `6a01b0f5...9a3` survived unchanged. The actual GLES
oracle reported fallback/active 12/5 with established logical, blend/oracle and
`172d49a34571b64c` presentation hashes.

The initial normal launch used `--profile-renderer=perf-direct-decode-dirty-07bbc`.
It was visually useful but not a profiler option. Computer Use advanced by
accessible touch Cross and keyboard Down/C through copyright, main menu,
character, Crash Cove, No Ghost, fly-in and grid. It recovered one transient
`noWindowsAvailable` by targeting the full Simulator path. A fly-in capture
showed transition-clipped labels; a settled capture proved the full HUD, kart,
track, waterfall, sky, banner, minimap and overlay coherent. The first rotating
log later finalized at `a4914173...13be`; targeted faults are zero.

Reading `platform/native_perf.c` identified the accepted `--perf-dir PATH`
interface. The app restarted once on the same device with an unused writable
path. The log explicitly announced both CSV files. The verified screen-by-
screen route returned to a stationary Crash Cove grid and accumulated more
than 500 complete level-3 rows. Correct bundle-ID termination left no app
process and finalized frame CSV `7b574948...40b` (644,630 bytes, 1,932 lines),
GPU header `af0f3466...91f6`, and app log `68eec948...789f` (53 lines, 6,870
bytes). The CSV has 1,931 complete 59-field rows and one excluded 54-field tail;
both logs have zero targeted faults.

The matched selector requires level 3, 66 draw calls, 119 logical splits, 78
fetch splits and 56 merged splits. The accepted `perf-exact-125966b21` gives
200 rows at 187.002 ms total, 156.406 ms work, 144.237 ms triangles, 8.990 ms
present and 5.348 reciprocal FPS. The direct decoder gives 248 rows at 198.412,
164.397, 153.395, 8.455 ms and 5.040 FPS. The 0.535-ms presentation decrease
does not offset the 9.158-ms triangle increase; total regresses 6.10%.

The experiment was rejected. `apply_patch` restored the lookup uniform/helper,
four bilinear lookup calls and nearest lookup. `git diff --check` passed and
source returned exactly to published `07bbc599bccc`. The ignored build and
installed app remain candidate evidence and must not be mistaken for a clean
release artifact.

At the user's explicit request to push all accepted work and merge it into
GitHub `main` before further optimization, documentation became the immediate
gate. `docs/history/THREE-DAY-CHECKPOINT.md` now connects the complete M0–M11
campaign, exact accepted/rejected paths, resource discipline, evidence model,
current limitations, rebuild/sign/install instructions, and future gate. The
focused rejection report retains hashes, profiler correction, Computer Use
recovery, visual boundary, preservation proof and matched decision.

The first read-only documentation audit used `path` as the loop variable.
Because zsh treats lowercase `path` as its special array paired with `PATH`, the
loop hid `rg` before the final whitespace command and produced `command not
found`. That subshell exited without changing the parent environment. The
corrected loop used `doc_path`; both new documents had balanced code fences,
every referenced local Markdown file existed, no trailing whitespace was
found, and `git diff --check` remained clean.

The GitHub connector returned 404 when asked for private
`chrissotraidis/ctrpad` PR #1, so the publication workflow used its documented
authenticated `gh` fallback. The first unqualified `gh repo view` / `gh pr
view 1` resolved the configured upstream repository and printed
`CTR-tools/ctr-native`'s unrelated closed PR #1. Those calls were read-only.
The corrected commands passed `--repo chrissotraidis/ctrpad` explicitly and
proved the intended private draft PR open, mergeable, `mergeStateStatus=CLEAN`,
head `07bbc599bccc`, branch `codex/arm64-apple`, and base `main`. All subsequent
GitHub reads and mutations are repository-qualified.

The documentation-open reading was 258,100 seconds (2 days, 23 hours,
41 minutes, 40 seconds), 2,216 seconds (36 minutes, 56 seconds) after the
published exact-replay boundary. It is cumulative goal time, not a build
benchmark or person-hour estimate. The overall goal remains active.

## 2026-08-01 — Exact GitHub main merge and next-branch boundary

The final staged scope was eight documentation files, 761 additions and two
replacements. `git diff --cached --check` passed. Commit
`f5140b7eb40945ed706502ade83dd7f2ed9048cd` added the three-day checkpoint and
direct-decoder rejection report while updating README, Roadmap, Decisions,
parity index, progress log, and this journal. The worktree was clean after
commit.

`package-source.sh` validated that exact commit in 18.44 seconds. It archived
3,244 members, excluded retail media, runtime state, packages, provisioning
profiles and key material, and produced
`CTRPad-source-f5140b7eb409.tar.gz` at `dc9d5ad2...3406`. The SHA sidecar file's
own hash was `f4da65fb...e249`. Both are ignored artifacts.

`git push origin codex/arm64-apple` advanced the remote from `07bbc599b` to
`f5140b7eb`. `git ls-remote` matched local HEAD exactly. An immediate
`gh pr view` response still cached the previous PR head and unknown merge state,
so no merge was attempted. Explicit branch and PR API polling converged on
`f5140b7eb409`, `mergeable=true`, `mergeable_state=clean`, draft/open.

The premerge audit fetched both refs and found original `main`
`95417c723518`, intended head `f5140b7eb409`, 6,774 commits, 2,684 changed
files, 2,620 source/non-documentation paths, zero retail/signing-sensitive
paths, and a clean worktree. This large count is expected because the original
repository had only three viability files and the PR preserves imported
upstream source/history plus the Apple port.

PR #1 was explicitly marked ready, then merged with `--merge` and
`--match-head-commit f5140b7eb409...`; the branch was not deleted. GitHub
returned merge commit `0758e7a804390ebd8a7cc74ba4cdcaf864270717` at
2026-08-01T19:58:15Z. A fresh `git fetch`, GitHub branch API, and
`origin/main` agreed on that object. `git merge-base --is-ancestor` proved the
reviewed head included. Direct tree queries proved `package-ios.sh`,
`package-source.sh`, `docs/INSTALL-IOS.md`, the renderer, checkpoint, and
rejection report present on remote main. GitHub reports `main` as the private
repository's default branch.

New branch `codex/simulator-performance-next` was created only after those
checks, directly at the main merge commit, and pushed with upstream tracking.
Its clean `package-source.sh` run took 22.06 seconds, again archived 3,244
members with prohibited material excluded, and produced
`CTRPad-source-0758e7a80439.tar.gz` at `873de12d...120e`; its sidecar contains
that exact digest. This proves the merged identity's source-publication path,
not device installation.

The publication-close reading was 258,793 seconds (2 days, 23 hours,
53 minutes, 13 seconds), 693 seconds (11 minutes, 33 seconds) after the
rejection/history boundary. The overall goal remains active; optimization now
continues only on the new branch.

## 2026-08-01 — Holistic release re-baseline

The user explicitly asked to step back from local rendering symptoms and
identify what the project is actually trying to solve, why it still feels
broken, which technical problems remain and what the simplest completion path
is. Renderer experimentation stopped immediately. Source had already been
restored from the rejected `texelFetch` probe; `git status --short` was empty at
`32e82d8da1bd76e0330ce8d78f4d4ef9af6cf96a`. Both named CTRPad iPad Simulators
were shutdown and no CTRPad or Simulator GUI process was running.

The audit compared the goal contract with `docs/ROADMAP.md`, the three-day
checkpoint, focused parity reports and the installation guide. It separated
five layers: retail/gameplay correctness, Apple platform integration, bounded
graphical correctness, Simulator throughput and physical release acceptance.
The first three have strong focused evidence. Apple Software Renderer remains
roughly five times over the retail frame budget, but that observation cannot
establish physical-iPad performance. The final layer has no direct evidence:
`security find-identity -v -p codesigning` again returned zero valid identities,
the documented provisioning-profile locations contained no profiles, and
`xcrun devicectl list devices` returned `No devices found.`

This exposed a planning contradiction. The historical renderer checkpoint
kept physical work closed until Simulator cadence was usable, while
`docs/INSTALL-IOS.md` correctly states that Simulator evidence cannot replace
physical cadence, touch feel, full-race or signing acceptance. The roadmap now
assigns coherent bounded rendering, diagnostics and input/lifecycle correctness
to one Simulator, but assigns sustained cadence, thermals, signed update,
Files/audio/persistence and human drift-boost ergonomics to the target iPad.
No existing correctness criterion was waived.

`docs/history/RELEASE-REBASELINE.md` records the evidence map, genuine defects
already fixed, minimum release contract, non-gating optional coverage,
dependency-ordered next actions and physical-performance contingency.
`docs/DECISIONS.md` retains the ownership change as a durable decision, and
`docs/ROADMAP.md` now identifies M8-M11 physical-device/release acceptance as
the current milestone. The old renderer checkpoint remains historical and is
explicitly superseded instead of silently rewritten.

The re-baseline documentation reading was 259,866 seconds (3 days, 11 minutes,
6 seconds), 1,073 seconds (17 minutes, 53 seconds) after the publication-close
boundary. The overall goal remains active. The next exact action is to commit
this documentation so the accepted source can be rebuilt with a clean embedded
identity before replacing the stale exploratory Simulator installation.

## 2026-08-01 — Clean release re-baseline execution

The five intended documentation files were audited for balanced fences,
existing local Markdown references, trailing whitespace and `git diff --check`,
then explicitly staged. Commit `d5772375fabc361c58fbee6d35c43d5fcbdf4cd0`
(`Rebaseline release acceptance on physical iPad`) added 241 lines and removed
nine across Roadmap, Decisions, both history ledgers and the new re-baseline.
It pushed exactly to `origin/codex/simulator-performance-next`.

The GitHub connector again returned 404 for the private repository. The
authenticated `gh` fallback was repository-qualified and created draft PR #2.
The first post-create verification incorrectly used unsupported `gh pr view
--head`; it failed after the PR existed. `gh pr view 2 --repo
chrissotraidis/ctrpad` then verified OPEN/draft, head `d5772375fabc`, base
`main`, CLEAN and MERGEABLE. The failure was retained in the focused report.

Both CTRPad devices and Simulator GUI were closed before fresh configuration.
macOS ARM64 configured with clean SDL/source suffix `gd5772375f`, compiled at
nice 15 / one job with the established 32 warnings and reported exact version
`d5772375fabc`. All 22 CTests passed in 3.08 seconds. The verbose renderer test
passed in 1.33 seconds with logical `851169f2644a1675`, blend
`0c0d08324ae06c35` and desktop presentation `a7798c5a6ddee965`.

The clean iPad Simulator build repeated the 32 warnings. Its raw thin ARM64
executable is `1699c36d...7091`; an isolated ad-hoc copy under
`/tmp/ctrpad-d5772375f.O2ZNZX` strictly verified at `c6d40aaf...187f`. Only the
disposable `CTRPad Import Negatives` device booted. Before update installation,
retail inode/size/hash were `111450682` / 605,698,800 /
`f780bf23...7c0`; save identity was `111309627` / 6,016 /
`6a01b0f5...19a3`. The 18.16-second update remapped the data-container UUID but
preserved every object value, and the installed executable embedded
`d5772375fabc`.

The actual 1032x1376 GLES surface reported Apple Software Renderer, framebuffer
fetch enabled and passed logical/blend/oracle hashes plus presentation
`172d49a34571b64c` with 12 fallback / 5 active draws. The immediate self-test
exit printed one unbalanced UIKit appearance-transition warning and the
Simulator's duplicate accessibility-loader class warning after the success
line. The process had exited, but the console PTY stayed attached; an empty
poll showed no further output and Control-C closed only the attachment. Normal
product launch was separate.

Computer Use observed copyright, animated title, Adventure/main mode menus,
Crash portraits/kart, Crash Cove list/preview/minimap, No Ghost, track fly-in,
grid and live race with coherent track/kart/exhaust/HUD/minimap/overlay.
Accessible touch Cross entered the menu. Keyboard `Z`, Down, `C` and `P`
navigated to Time Trial and paused/resumed; race `C`/`D`/`E` and final touch Gas
were consumed by the logged retail poll. Home/foreground logged ordered audio
suspend/reactivate transitions, retained the paused race, rotated to landscape
and resumed. The last 850x708 screenshot is `c70f0c96...50f2` in task-owned
temporary storage.

Exact-ID termination succeeded and launchd contained no app record. The current
82-line / 9,369-byte log is `467de09b...555`; its four archives are
`68eec948...789f`, `a4914173...13be`, `25a66dd9...e2f` and
`a1d98d65...107b`. Exact `[ERROR]`, `[FATAL]`, `[CTR AssetRef]` and visibility-
fault scans returned no row. Retail/save identities remained unchanged.
Computer Use's first Command-Q status remained stale and displayed an unrelated
shutdown-device window on refresh; the second app list still cached
`isRunning=true`, but direct exact GUI-process inspection was empty. Both
CTRPad devices were confirmed shutdown.

With every Simulator closed, the clean iPhoneOS build completed at nice 15 /
one job with the same 32 warnings. The thin ARM64 device executable embeds
`d5772375fabc` at `6f1aaefa...095b`. `package-ios.sh` produced the retail-free
unsigned IPA `CTRPad-0.1.0-1-d5772375f-unsigned.ipa` at
`41e00a5d...13ed`; ZIP validation and seven-member inventory passed.
`package-source.sh` accepted the clean source and produced a 3,245-member exact
archive `CTRPad-source-d5772375fabc.tar.gz` at `117dd291...352d`, excluding
retail/runtime/package/profile/key material.

The clean-smoke close reading was 261,192 seconds (3 days, 33 minutes, 12
seconds), 1,326 seconds (22 minutes, 6 seconds) after the re-baseline
documentation boundary. The entire command, hash, visual, preservation,
warning and limitation boundary is in
`docs/parity/2026-08-01-release-rebaseline-clean-smoke.md`. The overall goal
remains active; this machine still has zero valid Apple signing identities,
provisioning profiles and connected devices, so the signed physical campaign
is not inferred from the successful unsigned handoff.

## 2026-08-01 — Renderer self-test UIKit teardown audit

The automatic goal continuation began from clean documentation head
`cd41e62c5d8820d957f770ee28df6c4e1833b884`, while repository-qualified GitHub
API reported remote `main` merge `9b4a175f5fcfa4a8b3335fc177f5dea29cbf5fcb`.
Both CTRPad devices were shutdown; exact Simulator GUI/CTRPad process queries
were empty; signing inventory still returned zero valid identities and
`devicectl` still returned `No devices found.`

The one locally actionable open observation was the immediate renderer pixel
self-test's UIKit appearance warning. The earlier lifecycle report already
explicitly retained it, so this was a source-ownership audit rather than a new
regression claim. `main.c` calls `NativeRenderer_RunPixelSelfTest` synchronously
from `SDL_main`. That function calls `Platform_Init`, completes all pixel and
presentation work, then calls `Platform_Shutdown` before returning. Shutdown
destroys the SDL UIKit window and quits SDL inside the same app-delegate/run-
loop callback that installed the root controller. UIKit therefore receives no
run-loop return between creation and destruction on this test-only route.

The normal iOS runtime differs materially: it starts the display link and
returns from `SDL_main` with the platform alive. UIKit completes appearance
before the already accepted Home/foreground, rotation and bounded termination
events. Clean `d5772375fabc` again passed that ordinary route and five-log scan;
the warning appeared only after the complete self-test success line.

No source candidate was created. Manually invoking UIKit appearance methods
was rejected because no supported public property establishes the private root
transition's pending balance. A sleep or nested run-loop spin would make a
deterministic oracle reentrant and timing-dependent. Skipping shutdown would
leak and change the test contract. A correct asynchronous continuation would
require splitting the monolithic renderer test into iOS-specific phases, which
is disproportionate without a physical production reproduction. The accepted
production lifecycle remains unchanged.

The teardown-assessment reading was 261,906 seconds (3 days, 45 minutes, 6
seconds), 714 seconds (11 minutes, 54 seconds) after the clean-smoke close
boundary. Exact reasoning and reopening criteria are recorded in
`docs/parity/2026-08-01-ios-uikit-view-lifecycle.md` and `docs/DECISIONS.md`.
The overall goal remains open at the user-owned signed physical-iPad campaign.

## 2026-08-01 — Corrected the stale Simulator installation before judging touch

The user-visible follow-up deliberately reopened the single `CTRPad Import
Validation` Simulator after the physical-device re-baseline. The first visible
session rendered copyright, logos, main menus, Adventure cinematics and the
Crash character/kart screen, but Computer Use interactions appeared
inconsistent at roughly 5–7 FPS. That observation was not accepted as a
product defect because the session had not yet correlated UIKit input with the
native log.

The subsequent exact audit found that the installed executable was stale. Its
SHA-256 was `ed53ba9f26eba0a8e501f1e02aaabead1016e3b729751ce97d34c0b28879c11c`
and its embedded input-self-test text predated the accepted
`until-retail-poll` latch and timestamped touch/retail-consumer logging. This
explains why the earlier visible app produced no `[CTR Input]` rows despite
looking substantially current. No source edit was made from that ambiguous
session.

The clean re-baseline build tree still contained the exact raw Simulator
executable `1699c36d136c7bf5ce173ea5701e6313f9d40abc038b77fc27d83a3a08ca7091`
with the required instrumentation. Its staged build-tree bundle signature was
intentionally rejected by strict verification because resources had changed
after the prior signature. An isolated copy under
`/tmp/ctrpad-current-source.KyGMSK/CTRPad.app` was re-signed ad hoc, strictly
verified and reproduced the accepted installed-executable hash
`c6d40aaf3c0cfc989e47a79b74ff86f9e52b95a74505c58abaf77d323319187f`.

Only `CTRPad Import Validation` booted. CoreSimulator boot/container/install
operations were unusually slow, so no second device or competing operation
was started. The update install completed successfully after approximately
148 seconds. The installed bundle moved from container `595B67A4-...` to
`4AE8084E-...`; its executable exactly matched `c6d40aaf...187f` and embedded
the touch-edge, retail-poll and `until-retail-poll` strings.

The update remapped the data container from `932EF994-...` to
`D6D9B51A-...` without replacing the retained files. Before and after, the
retail image remained inode `111131200`, size 605,698,800 and SHA-256
`f780bf2331476aabfc00772fa758b12dd95ebfbc907968132cbd3cdd4e2c07c0`.
The save remained inode `111222179`, size 6,016 and SHA-256
`6a01b0f5562ed7a279d8f8e51e3b1874ac39a6120f55db4fe3873288950619a3`.

The exact current app launched as PID 62455. One 100-millisecond accessible
Cross activation produced timestamped touch down `0x4000` at elapsed
`+99.540s`, retail-poll consumption of the same mask in that timestamp, and
touch up at `+99.686s`. A later accessible Circle activation produced down
`0x2000` at `+188.494s`, retail consumption at `+188.497s`, and up at
`+188.600s`. Cross therefore reached the retail consumer even while the
nearest logged Simulator cadence was 5.56 FPS. The source latch behaved as
designed.

Computer Use's synthetic `C` key did not appear in the native log even after
Simulator keyboard capture was explicitly enabled. This is retained only as
an automation limitation; it is not keyboard-product evidence in either
direction. Capture was disabled again before handoff. The existing focused
`ctr_native_input` CTest passed 1/1 in 0.17 seconds. Application source remained
unchanged.

The correction reading was 265,868 seconds (3 days, 1 hour, 51 minutes, 8
seconds), 3,630 seconds (1 hour, 30 seconds) after the 262,238-second physical-
device boundary. The Simulator is now running the exact current build; real
Apple signing and physical-iPad acceptance remain open.

## 2026-08-01 — Made exact Simulator installation reproducible

The stale-install correction exposed a process defect rather than a game-code
defect: the repository had rigorous device packaging but no equivalent
Simulator installer. Manual `simctl install` could leave product identity
implicit, and signing the build-tree bundle directly would mutate evidence
while masking its stale resource signature. The next bounded task was therefore
a repository-owned exact-install contract.

At 17:25:31–17:25:42 CDT, `docs/INSTALL-IOS.md`, `package-source.sh` and new
`tools/install-ios-simulator.sh` were staged locally. The Bash helper requires
one booted device, validates the app/bundle/thin ARM64 Simulator load command,
rejects bundled retail media, copies through `ditto`, ad-hoc signs only the
temporary copy, verifies it strictly, installs by exact UDID, resolves the app
container and compares the installed/staged executable hashes. Its optional
slow mode also fingerprints the known retail image and slot-zero save around
the update. The corresponding-source packager now requires the helper.

`bash -n` and help passed. Unknown-option and missing-app probes exited 1 as
required. An explicit request for shutdown device `26F3DEE8-...-009C9C06`
also exited 1 because sole booted device `1D19A61F-...-52E2` did not match. A
first version of the three-command test harness used zsh's reserved `status`
parameter and failed before evaluating its captured result; renaming that
harness variable to `rc` corrected the harness without changing product code.
`shellcheck` was unavailable.

At 17:26 CDT, the real helper ran with `--verify-persistence --launch` on the
sole booted validation Simulator. It copied raw executable `1699c36d...091`,
produced strictly valid isolated executable `c6d40aaf...187f`, installed that
same hash in bundle container `2BF7ACA4-...-185B`, and launched PID 66389. The
retail image retained inode 111131200, 605,698,800 bytes and
`f780bf23...7c0`; the save retained inode 111222179, 6,016 bytes and
`6a01b0f5...19a3`. The source bundle remained unchanged and strictly invalid
for the previously documented stale-resource-signature reason. Temporary
staging cleanup left no matching directory.

The new runtime session opened at 17:26:46 CDT, reported the accepted
`d5772375fabc` identity, initialized the GLES/touch/UIKit path and advanced at
roughly 44–55 FPS after initial startup. At 17:27:06, a 2064x2752 screenshot
(`11455c47...991ba`) showed the rendered Aku Aku crate/title sequence, current
Controls/Change Disc buttons and the complete touch overlay. The current log
plus four rotations had zero targeted error/fatal/asset-reference/visibility
matches. A supplementary `simctl spawn ... ps -ax` returned POSIX error 2
because the guest command was unavailable; launch PID, advancing log and
pixels remained direct process evidence.

The 17:27:33 reading was 266,659 seconds (3 days, 2 hours, 4 minutes, 19
seconds). The exact implementation, negative cases, hashes, paths and remaining
physical boundary are in
`docs/parity/2026-08-01-exact-simulator-install.md`.

At 17:32:23 CDT, the complete helper/documentation checkpoint was committed as
`67b4c6276640dc59fb4495cf26ce4e86e64ea5b1`. The exact clean commit then
produced a 3,249-member corresponding-source archive at `116781ee...42c5`;
member inspection found the helper and focused evidence report, with
retail/runtime/package/profile/key material excluded. The packager's internal
checksum verification passed. A redundant outer `shasum -c` initially ran from
the repository root and could not resolve the sidecar's `dist/`-relative
basename; rerunning from `dist/` returned `OK`. The 17:33:01 goal reading was
266,988 seconds (3 days, 2 hours, 9 minutes, 48 seconds).

## 2026-08-01 — Made the external physical-iPad campaign executable

After PR #6 merged exact Simulator installation into `main`, the completion
audit returned to the release re-baseline's only remaining owner: a user-signed
physical iPad. Current inventory again found zero valid signing identities, no
provisioning profiles and no CoreDevice devices. The repo described seven human
requirements and direct commands but had no tool binding a signed IPA,
profile-authorized UDID, update install, launch and local evidence directory.

At 17:38 CDT, local `devicectl 518.33` help established that on-disk versioned
JSON is its only supported automation output. It also established exact app
install, foreground launch, installed-app lookup and `appDataContainer` copy
commands. The retail image belongs under CTRPad Documents; logs and saves
belong under Application Support, allowing collection without copying 605 MB
of retail media.

The first JSON-schema help probe used `/tmp/ctrpad-devices.XXXXXX.json` as a
BSD `mktemp` template. Because the X run was not the terminal suffix, literal-X
paths resulted. A direct exact-file cleanup command was rejected by execution
policy; both 471/77-byte temporary files were then moved to macOS Trash with
`/usr/bin/trash`, so cleanup remained recoverable. No repo, app or Simulator
state was involved.

New `tools/ios-device-campaign.sh` separates `preflight`, `prepare` and
`collect`. Offline preflight validates the sidecar, ZIP, one-app payload,
ARM64/IOS Mach-O, strict signature, profile expiration/platform/App ID/team/
authorized leaf certificate/optional UDID and retail/runtime exclusion.
Prepare retains that extracted signed app through a no-uninstall update
install, exact-bundle lookup and
foreground launch, writing JSON/logs at every CoreDevice boundary. It records
the signed-package hash rather than claiming iPadOS exposes the installed
binary. Collect copies only Application Support, hashes saves and scans rotating
logs. In-checkout raw evidence must pass `git check-ignore`.

`docs/templates/IOS-DEVICE-ACCEPTANCE.md` now requires a timestamped complete-
race, three-boost drift, simultaneous touch, Files/import, audio/lifecycle,
cadence/thermal and update/save record. It explicitly prohibits committing raw
JSON, UDIDs, profiles, saves or retail data. Installation Information and
README usage were updated, and the source packager now requires both the tool
and template.

At 17:42–17:44 CDT, syntax/help passed. Missing command, zero timeout, missing
prepare UDID, tracked evidence path and nonexistent tracked evidence parent all
failed before mutation. A test-orchestration call initially put a string in a
numeric output-limit tuple and was rerun with corrected arguments. The clean
unsigned `41e00a5d...13ed` IPA passed its sidecar and ZIP test, then exited 1 at
`signed app has no embedded.mobileprovision`; no fake authorization was added.
A deliberate nonexistent UDID exited 1 at device details with CoreDevice error
1000 and preserved JSON version 3. The downstream install/copy commands did not
run. `shellcheck` remained unavailable; `bash -n` and `git diff --check` passed.

The 17:45:00 reading was 267,693 seconds (3 days, 2 hours, 21 minutes, 33
seconds). The exact contract, paths, negative output and physical execution
checklist are in
`docs/parity/2026-08-01-ios-physical-campaign-handoff.md`. The goal remains
open: no signed physical pass is inferred from making its procedure
reproducible.

After the documentation boundary, the unchanged macOS ARM64 product passed
22/22 CTests in 5.39 seconds. A state snapshot retained exactly one booted
Simulator with installed executable `c6d40aaf...187f`; physical-device probes
had not altered it. Collection was also extended to extract FPS and campaign-
event rows beside the targeted-fault and save-hash summaries, without treating
those rows as self-sufficient hardware acceptance.

The final offline claim audit added leaf-certificate linkage: `codesign`
extracts certificate zero from the app signature, each profile
`DeveloperCertificates` data entry is decoded, and at least one SHA-256 must
match. A synthetic plist data-array round trip produced one decoded entry and
`cmp=0`, accepting the extraction mechanism but not an absent Apple pair.
The first codesign extraction probe used a space-separated long-option value;
codesign misparsed the prefix as another code path and exited 1. Repeating with
`--extract-certificates=PREFIX` extracted Xcode's three-certificate chain and
exited 0. The helper was corrected before commit.
The signed team entitlement also uses a literal dotted plist key. Its reader
was switched to PlistBuddy rather than allowing `plutil` to interpret dots as a
key path; a synthetic `TESTTEAM` value read back exactly.

The complete linkage mechanism then extracted Xcode leaf certificate
`d84db96a...1ed57`, imported that DER value into a synthetic one-entry
`DeveloperCertificates` array, decoded it through the helper's loop and
reported `profile_count=1`, `match=1`. This tests exact certificate bytes and
comparison flow; it remains explicitly short of a CTRPad Apple profile.

The final claim-audit reading at 17:53:34 CDT was 268,214 seconds (3 days,
2 hours, 30 minutes, 14 seconds). No signed or physical criterion changed
state during the offline mechanism tests.

At 17:55–17:56 CDT, the publication audit reran script syntax/help,
`package-source.sh` syntax, `git diff --check` and the complete macOS ARM64
suite. All 22 tests passed again in 4.87 seconds. Inventory still showed one
booted Simulator, zero valid signing identities and no CoreDevice device. The
first installed-app hash lookup used the obsolete
`com.chrissotraidis.ctrpad` identifier and returned POSIX error 2; resolving
Simulator inventory showed the actual bundle remains
`io.github.chrissotraidis.ctrpad`. Repeating with that exact identifier found
the unchanged installed executable at `c6d40aaf...187f`. The lookup mistake
changed no application, Simulator or repository state.

The 17:55:51 goal reading was 268,363 seconds (3 days, 2 hours, 32 minutes,
43 seconds). This remained a local regression/publication check, not physical
iPad evidence.

The last scope review strengthened the campaign linkage before staging:
`collect` now validates UDID syntax and refuses any evidence root without a
successful prepare manifest whose exact device and bundle match the request.
This prevents a later log/save collection from being attributed to a different
signed-install campaign. The guard runs before CoreDevice access or collection
directory creation.

At 17:59 CDT, collection against the unsigned-preflight evidence root rejected
the missing prepare manifest at exit 1 and created no collection directory. A
second negative used an ignored, explicitly synthetic matching prepare
manifest and fake UDID; it passed only the new linkage guard, then CoreDevice
again returned error 1000 and wrote its failure JSON/log before any app lookup
or copy. Syntax/help/package syntax and diff hygiene passed after the change.
The 17:59:07 reading was 268,549 seconds (3 days, 2 hours, 35 minutes, 49
seconds). Neither synthetic manifest nor raw negative evidence is tracked.

At 18:00 CDT, the complete handoff became clean commit
`4349bae8dd938447b9f67fff04b34dd4b574f09b`. Packaging that exact commit
produced `CTRPad-source-4349bae8dd93.tar.gz` with 3,253 members and SHA-256
`b9dfcc85c35b9cb36d82f9eed942e7a0ab2449d2b36cb5f8de1170ff77c9cba4`.
The built-in and independent `dist/`-relative sidecar checks passed. Direct
member inspection found the campaign tool, acceptance template and focused
report; retail/runtime and credential/package scans each returned zero. The
18:00:26 reading was 268,628 seconds (3 days, 2 hours, 37 minutes, 8 seconds).

At 18:01–18:02 CDT, commits `4349bae8d` and `e7af0bc32` were pushed to
`origin/codex/simulator-performance-next`. The preferred GitHub connector again
returned HTTP 404 for this private repository; authenticated `gh` fallback
opened draft PR #7. GitHub reported the exact two commits, 12 intended files,
head `e7af0bc323034e51ba8e4e0426abd0c7c28f0e79`, base `main`, `MERGEABLE` /
`CLEAN`, and no configured checks. The 18:02:05 active-time reading was 268,724
seconds (3 days, 2 hours, 38 minutes, 44 seconds). Merge remained the next
explicit operation.

PR #7 was marked ready. The first head-protected merge attempt supplied an
incorrect expansion after the displayed nine-character prefix
(`0458baff2db...` rather than the actual `0458baff2ea...`); GitHub refused it
with `Head branch was modified`, and no merge occurred. The exact full head was
then queried, compared with local `HEAD`, and passed back unchanged. PR #7
merged at 18:02:58 CDT as
`a19c1a5ef89b12c2ca77eb38eaaa9a71f2569f34`. Fetching `origin/main` confirmed
parents `e2dfe9709...` and `0458baff2...` plus all three new handoff files. The
working branch fast-forwarded to the merge. The post-merge goal reading was
268,804 seconds (3 days, 2 hours, 40 minutes, 4 seconds); the physical-device
acceptance boundary remained unchanged.

The four-document publication record then opened as PR #8, was audited as one
commit/four files and `CLEAN`/`MERGEABLE`, and merged at 18:04:39 CDT as
`7087cc6e0a0a56da80373206ee3e954a8cdbb4b1`. Local and remote working-branch
heads were fast-forwarded to that exact `main` merge. The sole Simulator still
held executable `c6d40aaf...187f`.

Packaging exact final-main commit `7087cc6e0...` ran asynchronously. The first
verification command arrived before its output files became visible and
reported the expected path missing; listing immediately afterward showed both
archive and sidecar complete. The unchanged repeat passed the sidecar and
reported 3,253 members, SHA-256 `a748e256e017c15842102c82055b0597f69cccd481e03484c974a48c3a4e2b01`,
all three handoff files and zero retail/runtime or credential/package matches.
The 18:06:51 reading was 269,013 seconds (3 days, 2 hours, 43 minutes, 33
seconds). This closes publication of the locally executable handoff, not the
external signed-iPad campaign.

## 2026-08-01 — Corrected Apple trust and real-profile signing preflight

The next completion audit returned to the credentialed path itself. At 18:11
CDT an isolated self-signed CMS profile reproduced a claim gap: `security cms
-D` exited 0 and decoded the payload, while `security verify-cert` exited 1
with `CSSMERR_TP_NOT_TRUSTED`. OpenSSL trusted verification likewise rejected
the self-signed certificate. No keychain or trust setting was changed. The
fixture hash was `09519ad6...be8`; a one-byte-tampered copy was
`84b3c02f...5e63`.

Source review found a separate real-profile compatibility bug in
`package-ios.sh`: it required `ApplicationIdentifierPrefix.0` itself to end in
a dot and concatenated it directly with the bundle ID. Apple's signed
application identifier is `<prefix>.<bundle-id>` and the App ID prefix can
differ from Team ID. The packager now normalizes the prefix, cross-checks it
against the profile application ID and inserts the delimiter itself.

New `tools/verify-ios-signing-trust.sh` verifies CMS integrity with stock
`/usr/bin/openssl`, validates certificate chains with macOS Security, and
compares the final root DER hash against the Apple Root CA set in the system
root keychain. Profile mode additionally requires an Apple provisioning-
profile signer subject. App mode uses code-signing policy and strict/deep code
verification. Both signed packaging and physical preflight use the helper;
the packager now also binds the final leaf certificate to
`DeveloperCertificates` and reads back its application/team/keychain
entitlements.

The first candidate exposed two Bash 3.2 errors: top-level help continued into
the output-dir requirement, and an empty certificate-chain array expanded as
unbound under `set -u`. Both were corrected before integration. An initial
positive probe against all of Xcode.app spent almost three minutes in deep
signature traversal and was terminated as a task-owned process without an
accepted result. The smaller Apple-signed Calculator app completed in roughly
two seconds with a three-certificate chain, leaf `d84db96a...1ed57` and pinned
root `b0b1730e...1f024`. The exact ad-hoc CTRPad Simulator app failed for no
certificate chain. The untrusted CMS failed chain trust and the tampered CMS
failed cryptographic verification.

Unknown mode and tracked evidence paths also failed before mutation. A first
timestamp convenience command used bare `stat`, unavailable on this PATH;
repeating with `/usr/bin/stat` returned the exact retained timestamps. No
source or runtime state depended on that harness error. `shellcheck` remained
unavailable.

At 18:23 CDT, the unchanged iPhoneOS bundle produced two byte-identical
seven-member unsigned IPAs at `3354bb3e...49ed`; both sidecars passed and no
profile, code signature, retail or runtime member appeared. Device preflight
then rejected that IPA for missing `embedded.mobileprovision` before any device
command. The exact bare-prefix fixture produced
`SYNTH12345.io.github.chrissotraidis.ctrpad` as intended. After integration,
the complete macOS ARM64 suite passed 22/22 in 5.56 seconds; syntax/help and
diff hygiene passed; one Simulator remained booted; signing identities and
physical devices remained zero.

The 18:26:33 active-time reading was 270,194 seconds (3 days, 3 hours, 3
minutes, 14 seconds). A real Apple profile and signed CTRPad positive remain
unexecuted, as do installation and every physical acceptance item. Exact
evidence is in `docs/parity/2026-08-01-ios-apple-trust-preflight.md`.

At 18:31 CDT the pre-commit repeat passed all 22/22 macOS ARM64 tests in 2.68
seconds. The final verifier again accepted Apple-signed Calculator.app with
leaf `d84db96a...1ed57` and pinned root `b0b1730e...1f024`, and again rejected
the isolated self-signed profile as not trusted under basic policy. The
18:31:32 active-time reading was 270,498 seconds (3 days, 3 hours, 8 minutes,
18 seconds). This repeat validates the checked-in candidate; it does not add a
real Apple development profile, signed CTRPad build or physical-device result.

At 18:33:14 CDT, the exact 14-file checkpoint became commit
`5af382409d8da9d0fa8db5314663bb07d5d58cb7` (`Harden iOS signing trust
preflight`). Its first clean corresponding-source archive contained 3,255
members, passed its sidecar and an independent SHA-256 check at
`2c3a2d6f5ff9083fabe7019b77d2c14897c297c9598f49f06ee55a0a9ced6dce`,
included the verifier, focused report and three-day timeline, and returned zero
for the retail/runtime/profile/key/certificate/package member scan.

The commit was pushed to `origin/codex/simulator-performance-next`. The
preferred private-repository GitHub connector returned HTTP 404 without
creating a pull request. Authenticated `gh` fallback created draft PR #10 at
18:35:07 CDT. The audit found exactly one commit, the intended 14 files, 720
additions, 29 deletions, exact head `5af382409...`, `MERGEABLE` / `CLEAN` and no
configured checks. It was marked ready and the merge used that complete
protected head SHA.

PR #10 merged at 18:35:27 CDT as
`8f3b2be37c9908fc02404f5bf62ee8c984bccae6`. Fetching `origin/main` confirmed
the merge parents and all three load-bearing new history/trust files. The local
working branch fast-forwarded to the exact merge. Packaging final `main`
produced 3,255-member `CTRPad-source-8f3b2be37c99.tar.gz`; sidecar and
independent SHA-256 verification passed at
`34f600e64a4885154348ca011c290e76a04b228932a8f6a91aba3bab38236326`,
and the same forbidden-member scan returned zero. At 18:36:44, active goal time
was 270,811 seconds (3 days, 3 hours, 13 minutes, 31 seconds). The locally
actionable trust/history correction is published; external signing and physical
iPad acceptance remain open.

## 2026-08-01 — Closed suffix-only signed-entitlement authorization

The next completion audit compared the physical campaign against Apple's
install-time entitlement rules. It found a concrete false-positive at
`tools/ios-device-campaign.sh`: the signature check removed everything through
the first dot and compared only the remaining bundle-ID suffix. A valid
signature and trusted profile could therefore pass offline with different App
ID prefixes. The campaign also did not bind the signed keychain group,
`get-task-allow`, or unexpected service entitlements back to the profile.
Apple TN2415 explicitly calls out prefix and keychain mismatches as install or
launch failures.

New `tools/verify-ios-entitlement-binding.sh` validates a trusted decoded
profile against trusted signed entitlements. It requires one canonical profile
prefix and team, valid exact/final-component-wildcard App ID authorization,
exact signed prefix plus bundle ID, the matching team, one default signed
keychain group authorized by the profile, matching optional
`get-task-allow`, and a three-or-four-key CTRPad allowlist. It writes no success
manifest until all checks pass and refuses tracked output. The packager applies
it before signing and after entitlement readback; device preflight records the
result. Corresponding-source packaging requires the verifier and its test.

The new temporary-plist self-test passed an exact profile and a wildcard App ID
profile. Seven isolated mutations failed: wrong signed prefix, extra push
entitlement, second keychain group, unauthorized keychain group, mismatched
debugger entitlement, non-final wildcard and profile team mismatch. CMake now
registers this as test 23 on macOS. After reconfiguration, 23/23 passed in 20.94
seconds, including 15.99 seconds for the new test. Bash syntax and diff hygiene
passed.

Two unsigned packages remained byte-identical with seven members and passing
sidecars at `7ebb1f4e...52a36`. At 18:48:30 CDT, exactly one Simulator remained
booted, valid signing identities remained zero and CoreDevice still reported
no devices. Active goal time was 271,519 seconds (3 days, 3 hours, 25 minutes,
19 seconds). This closes another offline false positive without inferring the
still-unrun Apple-profile or physical-iPad branch. Focused evidence is in
`docs/parity/2026-08-01-ios-entitlement-authorization.md`.

The final candidate additionally required actual Boolean plist types for
`get-task-allow` and limited keychain wildcards to their final component. The
focused test passed in 4.51 seconds and the complete suite repeated 23/23 in
8.13 seconds. At 18:51:23 CDT, active goal time was 271,689 seconds (3 days, 3
hours, 28 minutes, 9 seconds). No real-profile or device claim changed.

At 18:52:18 CDT the exact 16-file checkpoint became commit `c89bdfc235e0`.
Its first clean source archive contained 3,258 members, passed checksum,
required-file, exclusion and extracted-source self-test checks, and had SHA-256
`db3be6dd...16892`. The unchanged head was pushed. The preferred GitHub
connector again returned a private-repository 404; authenticated CLI fallback
opened draft PR #12 at 18:54:11. Its audit found one commit, the intended 16
files, 653 additions, 34 deletions, `MERGEABLE` / `CLEAN` and no configured
checks.

PR #12 merged the exact protected head at 18:54:33 CDT as `d9e3c59440ca`.
Fetching `origin/main` confirmed both new tools and the focused report before
the local branch fast-forwarded. Final-main packaging again produced 3,258
members; sidecar, independent SHA-256 at `760dcee6...68d0`, forbidden-member
scan and extracted-source test all passed. At 18:55:55, active goal time was
271,962 seconds (3 days, 3 hours, 32 minutes, 42 seconds). Apple credentials
and a physical device remained absent.

## 2026-08-01 — Replaced arbitrary-text CoreDevice evidence

After the entitlement history merged through PR #13 at 18:57:15 CDT as
`890ba3f4be57`, the next audit followed the physical campaign into its supported
`devicectl --json-output` files. The installed-app proof still used `grep -Fq`
for the bundle ID across the entire JSON document. A real failed nonexistent-
device app query retained the requested bundle text in `info.arguments`; the
old grep therefore returned success even though the JSON contained CoreDevice
error 1000, `outcome=failed` and no app result. A synthetic successful envelope
with an empty `result.apps` array reproduced the same class through
`matchingBundleIdentifier`.

New `tools/verify-devicectl-json.sh` now checks the exact command type, success
outcome, absence of an error object, tool/JSON versions and operation-specific
result. Install requires one exact bundle/CTRPad app URL; installed-app requires
one exact bundle plus nonempty version/build/app URL, with prepare pinning both
versions; launch requires a positive PID and exact `CTRPad.app/CTRPad` URL.
Every generic details/list/copy call also validates its command envelope. No
manifest is written until validation passes, and tracked output is refused.

The first self-test failed because this macOS `plutil -lint` rejects JSON at
the opening brace. Replacing that parser step with `plutil -p` accepted valid
JSON while retaining malformed-input failure. The completed fixture suite
passed four positive and nine isolated negative cases, including the two
bundle-text false positives, wrong command/bundle/version, duplicate app,
wrong install bundle, zero PID and wrong executable. A real empty device list
at `devicectl 518.33` / JSON version 3 passed the envelope at SHA-256
`71bd029c...6c6d8c`; the real failed app query was rejected with no success
manifest. An unsigned IPA still failed for no embedded profile before any
device call.

The complete macOS suite passed 24/24 in 46.57 seconds. At 19:10:02 CDT exactly
one Simulator was booted, identities remained zero and CoreDevice still found
no device. The iOS Simulator product was then rebuilt at nice 15 / one job and
guarded-update-installed. The staged/installed executable matched at
`88b5e79f...400979`; the 605,698,800-byte retail image and 6,016-byte save kept
their exact inode/size/SHA-256 tuples; launch returned PID 4571. Computer Use
observed current copyright, Naughty Dog and complete main-menu pixels with all
touch controls; the Gas touch reached the retail poll. The current log had zero
targeted faults and later settled near 7.8 FPS on Apple Software Renderer.

At 19:16:58 CDT active goal time was 273,210 seconds: 3 days, 3 hours,
53 minutes and 30 seconds. The earlier user-requested 3-day/50-minute boundary
remains separately recorded at exactly 262,238 seconds. The structured evidence
and visible Simulator route are accepted; signed physical execution and every
hardware-only acceptance item remain open. Full detail is in
`docs/parity/2026-08-01-ios-devicectl-structured-evidence.md`.

At 19:26:17 CDT the exact 16-file checkpoint became commit
`55932c2c1510ffae66ad5ec57e2dd5082541f2bf`. Corresponding source contained
3,261 members and passed its sidecar at SHA-256
`da7c7d22c7e3ac227ea13c1bedc8a50549ac79585c5fc924ce74afc0e8bcc3f7`.
The first independent member-inspection command used `path` as a zsh loop
variable. Because `path` is zsh's special command-search array, later `tar` and
`grep` lookups in that shell failed. The unchanged archive was rerun with
`member_path`; all required files were present, the forbidden retail/runtime/
credential scan found zero members, and its extracted verifier passed four
positive/nine negative fixtures.

The commit pushed to `codex/simulator-performance-next`. The preferred GitHub
connector again returned private-repository HTTP 404 without creating a PR;
authenticated CLI fallback created draft PR #14. Its audit found one commit,
the exact 16 intended files, 865 additions, 25 deletions, head
`55932c2c1510...`, `MERGEABLE` / `CLEAN` and no configured checks. It was
marked ready and merged only with that protected head.

PR #14 merged at 19:28:59 CDT as
`94f4d40eda4be460f605ef7828de2d812cb7ca5f`. Fetching `origin/main` confirmed
parents `890ba3f4b... 55932c2c1...` and all three new verifier/test/report
files. The local branch fast-forwarded to the merge. At 19:29:27 CDT active
goal time was 273,968 seconds: 3 days, 4 hours, 6 minutes and 8 seconds. Exactly
one Simulator remained booted, identities remained zero and CoreDevice still
found no physical device; publication is closed, signed hardware acceptance is
not.

## 2026-08-01 — Required the CoreDevice JSON schema version

The next completion audit compared the published verifier's wording to its
behavior. Although every claimed envelope was called versioned, the code made
`info.jsonVersion` optional and recorded `not-reported`. At 19:34 CDT a copy of
the real accepted device-list JSON had only that key removed. The published
verifier still exited 0 and wrote a verified manifest at input SHA-256
`14a54a58...7af8`, proving the contradiction rather than merely inferring it.

`tools/verify-devicectl-json.sh` now extracts `info.jsonVersion` as required,
then preserves the existing integer and positive-value checks. The fixture
suite adds the missing-key mutation, which fails with
`devicectl JSON is missing JSON schema version at info.jsonVersion` and writes
no manifest. Four positives and ten negatives pass. Bash syntax and diff
hygiene pass, and the fully reconfigured macOS suite passes 24/24 in 16.61
seconds.

At 19:38:00 CDT exactly one Simulator remained booted and untouched. Active
goal time was 274,482 seconds: 3 days, 4 hours, 14 minutes and 42 seconds. The
physical inventory read at 19:34 still had zero identities, no profiles and no
CoreDevice device. This closes an offline version-contract false positive; it
does not change the signed hardware boundary.

At 19:40:20 CDT the exact 13-file checkpoint became commit
`a139cce1c26a2df3f48cedc1999d6906c2092f2e`. Its 3,261-member corresponding-
source archive passed sidecar, forbidden-member scan and extracted 4/10 fixture
suite at SHA-256
`940c878dc180fa6cfdfc3eb1994fb5b5f074ca7d0d97ffdc8d582280145863e0`.

The branch pushed unchanged. The preferred private-repository connector again
returned HTTP 404 without creating a PR; authenticated CLI fallback opened
draft PR #16. Its audit found one commit, the exact 13 intended files, 107
additions, 23 deletions, head `a139cce1c26a...`, `MERGEABLE` / `CLEAN` and no
configured checks. GitHub merged only that protected head at 19:41:57 CDT as
`7e84669019117efc40d985b5f2490a79f47a58fa`.

Fetching `origin/main` and fast-forwarding the local branch confirmed the merge
and source. At 19:42:13 CDT active goal time was 274,735 seconds: 3 days,
4 hours, 18 minutes and 55 seconds. Exactly one Simulator remained booted;
signing identities and physical devices remained zero. The mandatory-version
checkpoint is published, while the signed hardware acceptance gate remains
open.

## 2026-08-01 — Closed exact IPA-to-source identity ambiguity

The mandatory-version publication history became commit `c3589f129` at
19:43:09 CDT and merged through PR #17 at 19:43:47 as
`931a810655720bd89a3c7a72bc631610113ebfa2`. The final-main source archive
contained 3,261 members, passed its sidecar and extracted four-positive/
ten-negative CoreDevice fixture suite, returned zero forbidden members and
hashed to `c48923eb0cf154a60635c89bc2d593d871ec299e8f425420a6172f6a75281c29`.

The next completion audit inspected the existing iPhoneOS bundle rather than
assuming it corresponded to `HEAD`. Its `Info.plist` carried version `0.1.0`,
build `1` and no project source field. Binary strings retained
`890ba3f4be57-dirty`; repository `HEAD` was `931a81065572...`. Running the old
packager against that exact bundle succeeded and wrote
`dist/CTRPad-source-linkage-gap-931a8106.ipa` plus a passing sidecar. Thus the
release workflow could pair stale dirty code with current source while passing
every then-existing package check.

Source inspection found two causes. `package-ios.sh` never compared app
metadata to the checkout, and CMake's dirty test used `git diff --quiet`, which
omitted staged and untracked changes. The first implementation added bundle
source/build keys, a shared verifier and packager/campaign integration. A dirty
configure produced source `931a81065572` and build
`931a81065572-dirty`; verification failed without a manifest. Packaging the
dirty checkout also failed before staging an app. The initial fixture suite
passed two positives and five negatives.

Review then rejected the initial 12-character source field as insufficient for
an “exact source commit” claim. CMake now resolves the full 40-character Git
identity, derives only the display/build prefix, and records porcelain status
with all ordinary source states. The Apple plist stores all 40 characters.
The verifier requires that full lowercase value plus its clean 12-character
build prefix; an expected source must also be exactly 40 characters. A sixth
negative rejects truncated metadata. Outside a Git checkout, `--build
--source-commit` passes the full identity into CMake; physical preflight and
prepare require it and collection refuses prepare evidence without it.

The implementation was first committed at short head `9905f321c`. Clean
device packaging at that intermediate head proved the core mechanism and
produced two byte-identical IPAs. A final review found that the extracted-
source `--build` path also had to pass the explicit commit into CMake. The
commit was amended; the only accepted durable implementation head is
`84456cd95a587a1b23a54e37a103e176052c7b46`, with exact committer timestamp
20:07:03 CDT. The earlier hash is not a published candidate.

### Exact implementation checks

The clean iPhoneOS configure/build ran at nice priority with one job. The
bundle verifier reported:

```text
SOURCE_COMMIT=84456cd95a587a1b23a54e37a103e176052c7b46
BUILD_IDENTITY=84456cd95a58
EXPECTED_SOURCE_COMMIT_STATUS=verified
BUNDLE_ID=io.github.chrissotraidis.ctrpad
VERSION=0.1.0
BUILD=1
INFO_PLIST_SHA256=31b70d0d8ecaed71d6edb002c5a9199f614cb1d7ec6bc99b193dbb0cbcc12fd5
```

One harness command initially supplied `build-ios-device-arm64/CTRPad.app` to
`--info-plist`; the verifier returned `Info.plist not found`. The same
candidate was then passed with `/Info.plist` and produced the manifest above.

Two independently packaged seven-member unsigned IPAs and their sidecars
passed at identical SHA-256
`25a8f7511e5f792027f011619b9902255e90048c45e0938fa32f2888f111c191`.
The all-zero expected commit failed at the 40-character comparison and wrote
no identity manifest. The correct commit wrote its identity manifest and then
unsigned campaign preflight failed at missing `embedded.mobileprovision`,
before any device command. The pre-change stale IPA failed earlier for missing
`CTRNativeSourceCommit` and wrote no manifest.

The fully reconfigured/built macOS ARM64 product passed 25/25 tests in 19.53
seconds. The added test passed two positives and six negatives in 4.62 seconds.
Bash syntax and diff hygiene passed; `shellcheck` was unavailable.

Source packaging exact ref `84456cd95a58...` wrote
`CTRPad-source-84456cd95a58.tar.gz`, 3,263 members, SHA-256
`5ddf285a00611842c4cecce492d992cf904fa3d60641ce582d12f1952c89fbd9`.
Sidecar and built-in retail/runtime/package/profile/key exclusions passed; an
extracted copy repeated the 2/6 identity suite. An independent expression
initially counted one apparent forbidden member because `.str` matched the
start of `InfoPlist.strings`; anchoring the extension to the member-path end
returned zero on the unchanged archive.

### One-Simulator visible recheck

Exactly one Simulator remained booted. The low-priority, one-job ARM64
Simulator rebuild was guarded-update-installed at 20:14 CDT. Source executable
SHA-256 was `ec752629...ffdf2ef5`; the isolated signed staged and installed
executables matched at `4b1a6c6f...0eaefa3`. Across the update, the retail
image retained inode 111131200, size 605,698,800 and SHA-256
`f780bf23...2c07c0`; the slot-zero save retained inode 111222179, size 6,016
and SHA-256 `6a01b0f5...0619a3`.

The installed plist reported the full implementation commit and clean prefix.
Computer Use observed the retail copyright screen, animated Crash/trophy/title
assets and complete Adventure/Time Trial/Arcade/VS/Battle/High Score menu with
the accessible touch overlay. Cross down reached retail poll then released.
The current log began at 20:14:09 CDT, reported build `84456cd95a58`, retained
zero error/fatal/asset-reference/visibility rows and settled around 7.7 FPS in
the animated menu under Apple Software Renderer.

A first physical-device count used line count on the JSON rendering `[]` and
incorrectly printed one. Structural verification of the unchanged CoreDevice
file reported `devicectl.list.devices`, success, tool 518.33, JSON schema 3 and
an empty device array. The accepted count is zero. `security find-identity`
also returned zero valid identities.

At 20:17:24 CDT, the goal API reported 276,843 seconds: 3 days, 4 hours,
54 minutes and 3 seconds. This accepts exact local application/source pairing,
not signing or physical execution. Full detail is in
`docs/parity/2026-08-01-ios-source-identity-binding.md`.

The corresponding-source consumer route was then run in full. A new temporary
directory received an untouched extraction of
`CTRPad-source-84456cd95a58.tar.gz`; with no `.git`, it invoked
`package-ios.sh --build --source-commit
84456cd95a587a1b23a54e37a103e176052c7b46`. `CMAKE_BUILD_PARALLEL_LEVEL=1`,
nice 15 and the implementation commit timestamp constrained resources and
archive time. CMake reported the explicit source identity, all 246 iPhoneOS
targets compiled/linked, and the packager verified the clean full metadata.
The output sidecar passed at SHA-256
`314d2ef050fc1771a46b7605a00de942f6d92fdd96ecee9c4f96ff121240b0e0`.
Decoded IPA metadata held the full source and build `84456cd95a58`.

The extracted IPA is not asserted byte-equal to the in-place IPA because the
RelWithDebInfo binary records different absolute source/build directories.
This test accepts rebuildability and identity propagation, not path-independent
binary reproducibility. At 20:29:27 CDT, active goal time was 277,567 seconds:
3 days, 5 hours, 6 minutes and 7 seconds.

At 20:33:38 CDT, a final pre-publication observation parsed the booted-device
JSON as the same single iPad Simulator. The already installed app was
foregrounded as PID 25424 and captured five seconds later. The framebuffer
showed the fully textured Crash/trophy mode menu and complete touch overlay;
the ignored PNG SHA-256 was
`ad7a9098a015d58e20bb3e5c06d89b57939befa918572d5a9f61efe8f49c41b0`.
No second Simulator was booted, and the capture is not committed because it
contains the user's retail-derived graphics.

The final pre-publication CTest repeat completed at 20:35 CDT: 25/25 passed in
26.40 seconds. `ctr_ios_build_identity` completed in 4.85 seconds with two
positive and six negative cases. The implementation under test was unchanged;
the working-tree differences at that point were this documentation record.

The ten-file documentation checkpoint was committed at 20:36:40 CDT as
`f4c1300f9bb749ab901689e9b091eefa06e907f1`. Clean source packaging produced
`CTRPad-source-f4c1300f9bb7.tar.gz` with 3,264 members. Its built-in exclusions
passed; an independent end-anchored scan returned zero forbidden members; and
the extracted tree repeated the 2/6 identity suite successfully.

The first independent checksum verification invoked `shasum -c` from the
repository root. Because the sidecar records only the archive basename and the
archive resides under `dist/`, that invocation failed to open the file. It did
not invalidate or validate its bytes. The repeat from `dist/` at 20:38:17
passed for the unchanged archive at SHA-256
`27bd43cc0deaea5fcb6248c095d0eca0e727c126d5197236ab1f7ac178363ed0`.
At 20:38:30 CDT, the goal API reported 278,117 seconds: 3 days, 5 hours,
15 minutes and 17 seconds.

The branch was pushed without force, then PR #18 publication attempted the
connected GitHub app as required. It returned the same private-repository 404
seen in earlier checkpoints, so authenticated `gh` created the draft. The
server-side audit reported exactly three commits, 18 changed files, 931
additions, 40 deletions, clean mergeability, no configured checks and head
`4bc222fcb44c614349c4487431490f1ae97cdb27`. After marking the draft ready, a
protected-head merge created `d29a560e96f0731cc0528cab123b3bee4ce902c9` on
`main` at 20:40:38 CDT. The campaign branch was fast-forwarded to that merge
and pushed; booted-device JSON still contained the same sole Simulator.

Final-main corresponding-source packaging then wrote
`CTRPad-source-d29a560e96f0.tar.gz` with 3,264 members. Its sidecar passed;
the independent end-anchored forbidden scan returned zero; and a fresh
extraction passed the two-positive/six-negative identity fixture. SHA-256 was
`9ad1c1680cbf325f1567e9ae838fd72c5edaf9d6e75f877e5fd8746ce4380577`.
At 20:41:47 CDT, active goal time was 278,313 seconds: 3 days, 5 hours,
18 minutes and 33 seconds. This published exact local identity evidence, not
the still-missing signed physical-iPad result.

The publication record became commit `be7b52e28` at 20:42:40 CDT and merged
through PR #19 at 20:43:08 as final-main `88e453999`. Local branch, remote
branch and `main` aligned. The 3,264-member final-main source archive passed
its sidecar, exclusions and extracted 2/6 identity fixture at SHA-256
`b56a98328f9e171d2143cadefe18d5a872c2f4e91e3a99281239dd9f2c874713`.

## 2026-08-01 — Added sustained accessible touch controls

The next Simulator review used the existing sole `CTRPad Import Validation`
iPad. Real stick drags and ordinary accessibility activation navigated main
menu -> Time Trial -> Crash -> Crash Cove -> No Ghost. The route exposed a
control-evidence gap: button accessibility activation emitted the intended
100-ms edge pair, but desktop Computer Use cannot keep an iOS button depressed
or synthesize genuine multi-touch. It therefore could not sustain Gas while
independently steering and holding drift, even though a finger on a physical
screen could.

`CTRPadInputButton` now has explicit `Hold` and `Release` accessibility actions
and visible `Held`/`Released` state. The steering element adds full-direction,
45% slight-left/right and Center actions. All actions use the same production
`Platform_InputTouchButton` and `Platform_InputTouchLeftStick` routes as
physical touches. Full deflection still reaches the 68% D-pad outer ring for
menus; the slight actions deliberately remain below it for menu-neutral analog
steering. Physical stick contact cancels accessibility steering. A shared
`resetControlState` clears every local action latch, visual state, analog/D-pad
state and the engine touch state before settings, disc reselection and control
rebuild. UIKit's current public header declares custom-action targets weak, so
the owned actions do not retain their button or stick.

### Dirty iteration retained as diagnostic evidence

The first build invocation returned control while its one-job nested Ninja
process was still running. Repeating the request briefly left two low-priority
builders. PIDs `39048/39051/39061` and `39257/39263/39269` were explicitly
terminated; the process table was verified clear; no overlapped output was
accepted. Subsequent work used only one nice-15, `-j1` build. The Simulator
itself was never duplicated.

The first dirty session opened at 21:04:25 CDT as
`88e453999a35-dirty`. Exact log rows show:

- Down `0x0040` held at 21:06:27 and released at 21:07:00, moving the
  Adventure highlight to Time Trial;
- Gas `0x4000` held at 21:07:37 and released at 21:08:07;
- short Gas presses through character/track/ghost selection;
- Gas held from 21:10:35 while the Crash Cove timer advanced to 0:17.36 and
  the kart visibly moved;
- Left `0x0080` held/released at 21:11:18/21:11:53 while Gas remained held;
  and
- L drift `0x0400` held/released at 21:12:43/21:13:16 while Gas remained held.

Full-direction stepping was too coarse for controlled race automation and
also emitted menu D-pad state. The 21:15:33 dirty relaunch proved `Hold slight
right` offset the knob and retained Adventure without moving the selection;
Center neutralized it. The final dirty session opened at 21:18:01 after adding
the shared reset. Gas was held, Controls was opened and dismissed, and the
fresh accessibility tree returned Gas `Released` and steering `Centered`.
Retail and save identity remained unchanged across every dirty update. These
sessions established the design; their dirty build identity prevents them
from serving as release evidence.

### Exact clean acceptance

Implementation commit `a3523c7a858379a030934932fb335e6ed502457e`
(`Add sustained accessible touch controls`) was created at 21:24:14 CDT.
After a clean preset reconfigure, the one-job iPhoneSimulator build linked at
21:26:20 with exact plist source/build `a3523c7a8583...` / `a3523c7a8583`.
The established 32 source warnings appeared; there were no errors.

The guarded update installer stopped only the app process, left the sole
Simulator booted, applied an isolated ad-hoc Simulator signature and required
the staged and installed executable to match:

```text
SOURCE_EXECUTABLE_SHA256=089100e5090fc7ede2089cdb736d79c462bfc7da7ee2b97f0cac9fd46289c2c4
SIGNED_STAGED_EXECUTABLE_SHA256=443d08a93b144f832ce2337df134cea7d74d7ca6394ee75810f9de926130dabc
INSTALLED_EXECUTABLE_SHA256=443d08a93b144f832ce2337df134cea7d74d7ca6394ee75810f9de926130dabc
PERSISTENCE_VERIFIED=retail-and-slot-zero
LAUNCH_RESULT=io.github.chrissotraidis.ctrpad: 44201
```

The 605,698,800-byte retail image retained tuple
`111131200|605698800|f780bf23...2c07c0`; the 6,016-byte slot-zero save retained
`111222179|6016|6a01b0f5...0619a3`. The accepted log opened at 21:27:07,
identified clean build `a3523c7a8583`, initialized GLES/Touch/UIKit and had
zero `[ERROR]`, `[FATAL]`, `CTR AssetRef` or visibility signatures.

Computer Use visibly observed the retail copyright screen, animated Crash and
trophy title sequence, full textured mode menu and complete control overlay.
On the exact build, Gas, slight-right steering and L drift simultaneously
reported `Held`. The log recorded Gas at 21:29:31 and L drift at 21:30:23
reaching the retail poll. Opening Controls and returning changed every button
to `Released` and the stick to `Centered`. No three-lap completion is claimed:
this exact run accepted independent sustained state and lifecycle reset, not
desktop automation as a substitute for physical finger multi-touch.

The clean iPhoneOS ARM64 product linked at 21:33:30 with the same full source
identity, established warnings and no errors. The clean macOS ARM64 product
then built and passed all 25/25 tests in 27.84 seconds. The app remained
terminated during the long compiles so Apple Software Renderer did not consume
CPU; the one Simulator remained booted.

At 21:35:52, packaging wrote seven-member ARM64 unsigned
`CTRPad-0.1.0-1-a3523c7a8583-unsigned.ipa` at SHA-256
`781f8c03079370818f0d038c320799d1d64ec2343443f797a575382da757ebf0`.
At 21:36:19, corresponding-source packaging wrote a 3,264-member archive at
`62562ce95d1bdf311320d707f86dc55caede8e75b23bc4bf626473c5a4878ae0`.
Both sidecars passed and the source forbidden-member scan returned zero.

The first independent member-list command changed into `dist/` for checksum
verification and then incorrectly supplied `dist/...` to `unzip` and `tar`.
Those commands failed to find the unchanged artifacts and were not acceptance
evidence. The corrected root-directory repeat found all seven IPA members,
3,264 source members and zero forbidden members. An extracted IPA reported
Mach-O ARM64, full source `a3523c7...`, build `a3523c7a8583` and expected
unsigned `codesign` status 1.

At 21:37:01 CDT, active goal time was 281,632 seconds: 3 days, 6 hours,
13 minutes and 52 seconds. Exact details and the timestamp table are in
`docs/parity/2026-08-01-ios-sustained-accessible-controls.md`. This closes the
Simulator sustained-action gap. Apple credentials, a physical iPad, signed
install/launch, genuine multi-touch race/boost ergonomics, hardware cadence,
audio, thermal and persistence acceptance remain open; the goal stays active.

At 21:43:31 CDT, the seven-document acceptance/history set became commit
`76ef539313a97893130818ad360496f1fb102d44`. Its clean corresponding-source
archive contained 3,265 members, included the sustained-control report and
complete timeline, passed its sidecar and returned zero forbidden members at
SHA-256
`8f4cb489746b00ee31ac20c7f75d77ce0450101cac4ac498cd024cb5e0a857a4`.
At 21:44:16, active goal time was 282,054 seconds: 3 days, 6 hours,
20 minutes and 54 seconds. GitHub publication remained next.

The three commits pushed to `codex/simulator-performance-next`. The preferred
GitHub connector returned the same private-repository 404 without creating a
PR; authenticated CLI fallback opened draft PR #20. Its server audit reported
exact head `718fb73b088ce16a98ab4e90385d23fdb3d58cce`, three commits, eight
intended files, 611 additions, eight deletions, `MERGEABLE` / `CLEAN` and no
configured checks. It was marked ready and merged only with that protected
head at 21:45:48 CDT as `be9c81605133d2329041f36ac7ca77b6f9347ead`.

Fetching `origin/main`, verifying the accepted head as its ancestor and
fast-forwarding/pushing the working branch aligned all three refs. Final-main
source packaging contained 3,265 members, both required history files, zero
forbidden members and a passing sidecar at SHA-256
`e08098bb3acf725dc7393709bbf010bead3aed1b57855f9d337367f85481a463`.
At 21:46:42, active goal time was 282,209 seconds: 3 days, 6 hours,
23 minutes and 29 seconds. Local publication is closed; the signed physical-
iPad acceptance boundary remains open.

## 2026-08-01 to 2026-08-02 — Bounded accessible race controls

**Starting source:** published `main` merge `3f7b16caa95e33a68ccfbd0e9fa92854252a6b2f`

**Accepted implementation:** `c56162f68a74ebc3f381a6d77d5a00b064a6c8b2`

### Observation and rejected race attempt

The sole `CTRPad Import Validation` iPad Simulator opened the clean sustained-
control build at 21:49:38 CDT. Time Trial -> Crash -> Crash Cove -> No Ghost
continued to work through production touch input and every inspected retail
frame retained coherent sky, rock, grass, sand, water, bridge, ship, kart, HUD,
minimap and control-overlay assets.

The extended route exposed an automation-specific duration failure. A held
Gas or steering action continued while Computer Use waited 15-25 seconds for
the next accessibility tree and screenshot. The kart therefore crossed far
more track than the intended observation interval and reached walls, wrong-way
or ocean states. Those runs were rejected. They were not relabeled as input,
physics or graphics passes and did not complete a lap.

### Implementation

The touch layer now exposes:

- one- and three-second self-releasing actions on every retail button;
- 450-ms full and 45% left/right steering nudges;
- action-generation guards so an older scheduled release cannot cancel a
  later Hold, Release, reset, nudge or physical steering owner; and
- the existing normal 100-ms activation, explicit Hold/Release, full/slight
  steering and Center actions unchanged.

All actions still enter `buttonDown:`/`buttonUp:` or the normal analog/D-pad
publisher. No parallel game-state injection was added. Source citations and
the generation design are in
[2026-08-02-ios-bounded-accessible-race-controls.md](../parity/2026-08-02-ios-bounded-accessible-race-controls.md).

Dirty sessions opened at 22:39:49, 22:59:33 and 23:15:05 as
`3f7b16caa95e-dirty`. Their identities intentionally prevent release use.
Exact log rows prove a one-second Gas press at 23:00:46-23:00:47 and a three-
second press at 23:16:02-23:16:05, with both down edges consumed by the retail
poll.

### Broad visible route, not a completed lap

The cleanly restarted bounded attempt from 23:34-23:58 used one-/three-second
Gas intervals with centered, slight and full steering corrections. Computer
Use visibly observed the opening tunnel, first cliff/shoreline, central rock,
inland beach, shipwreck, bridge and final climb. Textures remained coherent.
The attempt ended against the climb wall with the HUD still `LAP 1/3`.

The result accepts that bounded touch actions move a live retail race through
substantial course geometry and self-neutralize. It explicitly rejects a full-
lap, three-lap, drift-chain or ergonomic claim.

### Clean exact validation

Implementation commit `c56162f68` was created at 00:00:58 CDT. Serialized,
nice-15, one-job builds produced:

```text
iPhoneSimulator link  00:05:34  source executable ef5b2de3...ece6d
iPhoneOS link          00:12:04  executable        d6c81c8b...3290
macOS ARM64 link       00:16:28
```

All reported the exact full source commit and clean short build. Each emitted
the established 32 warnings and no errors. The guarded Simulator update used
only the already booted device, installed signed executable
`28500b12...2ea2`, and retained:

```text
retail=111131200|605698800|f780bf23...2c07c0
save=111222179|6016|6a01b0f5...0619a3
```

The clean session opened at 00:07:02 as build `c56162f68a74`, initialized
GLES framebuffer fetch and UIKit touch/display, and showed the retail boot and
coherent Crash Cove demo. Its targeted error/fatal/asset/visibility/shader/
abort/assert scan returned zero. The ignored 00:19:52 visual checkpoint was
101,446 bytes at SHA-256 `17cf0ada...403a` and is not committed because it
contains retail-derived graphics.

### Test-orchestration correction

The first full CTest call continued after the command wrapper returned partial
output. A later process-status command accidentally appended another full
CTest command, so two test suites briefly overlapped. The duplicate reached
23/25 and was interrupted immediately; its partial output was rejected. No
compiler or Simulator was duplicated.

After the process table was empty, a final serialized repeat passed 25/25 with
zero failures in 74.85 seconds. This final run, not either overlapping/partial
route, is the acceptance evidence.

At 23:33:34 CDT the goal API reported 288,621 seconds: 3 days, 8 hours,
10 minutes and 21 seconds. At 00:20:53 it reported 291,459 seconds: 3 days,
8 hours, 57 minutes and 39 seconds. The goal remains active because signed
physical installation, a complete touch race, repeated drift boosts and all
hardware-only performance/lifecycle/persistence checks remain open.

At 00:28:34 CDT, the pre-commit history audit reconciled the parity report,
timeline, journal, progress log, roadmap, decision record and parity index.
The goal API then reported 291,910 seconds: 3 days, 9 hours, 5 minutes and
10 seconds. Publication remained next.
