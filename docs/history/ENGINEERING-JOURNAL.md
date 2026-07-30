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
