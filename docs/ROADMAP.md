# CTRPad Roadmap

**Mission:** deliver a native ARM64 Crash Team Racing application for macOS,
iOS, and iPadOS that a user can sideload, supply with their own NTSC-U retail
disc image, and play with controller or touch input at retail physics and
timing.

**Roadmap status:** active

**Current milestone:** M10 — touch-control iteration, while M8/M9 await their
physical-iPad signing, controller, Files, performance, and lifecycle gates

**Last updated:** 2026-07-31

This is the working source of truth for the port. Milestone status changes only
after its acceptance evidence has been recorded. A successful compile is
evidence, not completion.

The human-readable running status and elapsed-time ledger is
`docs/history/PROGRESS-LOG.md`. The complete chronological implementation
record is maintained in `docs/history/ENGINEERING-JOURNAL.md`; it records
commands, evidence, failures, decisions, validation, and remaining limitations
so the project can be reconstructed historically rather than only understood
from its final state.

## Non-negotiable constraints

- Apple targets are ARM64 and therefore require a correct 64-bit memory model.
  The existing 32-bit configuration gate represents real pointer and serialized
  layout contracts, not an arbitrary build restriction
  (`docs/ctr-native-viability.md:56-69,152-193`).
- The macOS ARM64 build is the diagnostic and parity gate before iOS lifecycle,
  sandbox, signing, and touch work (`docs/ctr-native-viability.md:443-446`).
- Physics and timing drift is a port defect. The fixed-point MIPS/GTE layer is
  already intended to preserve console integer semantics across architectures
  (`docs/ctr-native-viability.md:243-258`).
- Game assets are never committed. Distribution requires the user to supply a
  retail NTSC-U raw MODE2/2352 image.
- Distribution is sideload-only. The application, build instructions, and
  installation information will be published under GPL-3.0; App Store
  submission is out of scope (`docs/ctr-native-viability.md:378-385`).
- Nonobvious technical decisions, failures, and parity results are documented
  under `docs/` as they happen.

## Actual repository inventory

Inventory performed on 2026-07-29 before implementation:

- Git `main` is at `95417c7` and matches `origin/main`. It has three tracked
  files: `.gitignore`, `docs/ctr-native-viability.md`, and `ref/README.md`.
- There is no game source, `CMakeLists.txt`, `game/`, `platform/`, `include/`,
  or vendored SDL tree in this checkout.
- `docs/` contains only the viability report. No prior roadmap, architecture
  decision, build log, or parity result exists.
- Contrary to `ref/README.md:5-14`, the documented `ctr-native`,
  `CTR-ModSDK`, `CTR-in-C`, and `CTR-PC-Port` clones are absent on disk.
- `ref/CTR/` is present and untracked. It contains `CTR.ccd`, `CTR.img`, and
  `CTR.sub`, not the `assets/ctr-u.bin` path expected by ctr-native.
- `CTR.ccd` describes one MODE 2 data track (`ref/CTR/CTR.ccd:6-19,78-79`).
  `CTR.img` is 740,179,104 bytes, exactly 314,702 sectors of 2352 bytes.
  `CTR.sub` is 30,211,392 bytes, exactly 314,702 records of 96 bytes. This
  establishes CloneCD raw-sector structure; NTSC-U identity and loader
  validation remain to be proven.
- `.gitignore:1-8` ignores the four absent reference clone names and
  `.DS_Store`, but does not ignore `ref/CTR/`, disc images, or extracted
  `.BIG`, `.HWL`, `.XA`, and `.STR` assets. Asset ignore protection is
  currently inadequate.
- The development host is Apple Silicon (`arm64`) on macOS 26.5 with Xcode
  26.6, Apple Clang 21.0.0, the macOS/iPhoneOS/iPhoneSimulator 26.5 SDKs,
  CMake 3.27.1, and Ninja 1.13.2.

## Dependency map

```text
M0 repository foundation
  -> M1 reproducible 32-bit baseline and parity harness
  -> M2 64-bit memory-model decisions
  -> M3 mechanical pointer-width and runtime-layout conversion
  -> M4 serialized asset relocation conversion
  -> M5 scratchpad and remaining pinned-layout conversion
  -> M6 correct macOS ARM64 build
  -> M7 shared GLES renderer
  -> M8 iOS/iPadOS application with controller input
  -> M9 sandbox storage and retail-disc import
  -> M10 touch controls
  -> M11 parity, signed packaging, documentation, and publication
```

M7 can be researched against a 32-bit build while M2–M5 are underway, but it
does not bypass M6 as the acceptance gate. M10 does not start until the game
runs on an iPad with a physical controller.

## Milestones

### M0 — Repository and evidence foundation

**Status:** completed 2026-07-29

Result:

- Retail and reference paths are protected by `.gitignore:1-35`; the upstream
  `assets/` exclusion remains at `.gitignore:40`.
- Clean reference clones are recorded in `ref/README.md` at `2df55dc5a` for
  upstream and `34648097d` for the Android GLES branch.
- Commit `268ff6977` joins the CTRPad documentation ancestry with the unchanged
  upstream source ancestry. `git diff upstream/master` reports no difference
  in `CMakeLists.txt`, `CMakePresets.json`, `main.c`, `game/`, `include/`,
  `platform/`, or `externals/`.
- The implementation branch is `codex/arm64-apple`; `upstream` points to
  `https://github.com/CTR-tools/ctr-native.git`.
- `git ls-files` finds no retail-media extension, and representative
  `git check-ignore -v` probes pass for the raw image and extracted formats.

Work:

1. Extend `.gitignore` to cover `ref/CTR/`, all raw/cooked disc image formats,
   and the extracted retail asset extensions and paths.
2. Clone `CTR-tools/ctr-native` read-only into `ref/ctr-native`, initially at
   the viability baseline `2df55dc5a`, then record the current upstream head.
3. Clone `Simon358/ctr-native-android` and its
   `feature/add-android-support` branch into `ref/` before renderer work.
4. Correct `ref/README.md` to reflect what is actually cloned, including exact
   origins, branches, and commits.
5. Add `CTR-tools/ctr-native` as the upstream Git remote. Create the downstream
   `codex/arm64-apple` work branch and incorporate upstream history into this
   repository while preserving the existing CTRPad documentation commit. The
   reference clone remains read-only; implementation happens in the downstream
   working tree.
6. Add a decision log and a results area under `docs/`.

Acceptance:

- `git check-ignore -v` proves that `.bin`, `.img`, `.iso`, `.cue`, `.ccd`,
  `.sub`, `.BIG`, `.HWL`, `.XA`, and `.STR` retail material cannot be added by
  an ordinary `git add`.
- `git status` does not enumerate any retail files.
- Reference origins and exact commits in `ref/README.md` match `git remote` and
  `git rev-parse` output.
- The downstream work branch contains both the CTRPad documentation history
  and unmodified upstream source history.
- No reference clone or retail asset is tracked.

### M1 — Reproducible upstream baseline and parity gate

**Status:** completed 2026-07-31

Result so far:

- The pinned amd64-container/i686 build completes, passes its version CTest,
  and produces an ELF 32-bit Intel 80386 executable. Exact evidence and
  dependency versions are in `docs/builds/2026-07-29-baselines.md`.
- The unchanged Apple ARM64 configure reproduces the explicit 32-bit gate at
  `CMakeLists.txt:8`.
- Existing replay/checkpoint tooling is formally rejected as a sufficient
  parity oracle in `docs/parity/TOOLING-ASSESSMENT.md`.
- Canonical state-digest schema 1 is integrated into replay format version 2.
  Its CTest proves host-address independence and detects a one-unit vehicle
  position mutation as `drivers` (`docs/parity/STATE-DIGEST.md`).
- A second media-free CTest proves identical replay-frame matching, named
  `drivers` divergence, and distinct exit statuses for harness failure (1) and
  parity failure (2). Playback has an exact-frame real-state mutation option
  for the pending golden-run proof.
- The pinned container now exposes a loopback-only noVNC recording session and
  an automated two-process replay/mutation verifier. The exact coverage and
  evidence procedure is `docs/parity/NTSC-U-GOLDEN-RUN.md`.
- The original local image was identified as PAL Europe `SCES_021.05` and is
  rejected before indexed game data loads. A replacement MODE2/2352 image now
  identifies as NTSC-U `SCUS_944.26`, passes the runtime gate, and visibly
  reaches the retail boot sequence with live keyboard-to-pad input.
- The finalized report at
  `build-linux-i686-baseline/debug/reports/20260729/ctr-225420` contains
  24,232 frames, 81 checkpoints, and observed pass evidence for all eight
  required gameplay/save behaviors.
- Strict unchanged playback matched through frame 22,391, then exposed an
  omitted host-only Enter shortcut at frame 22,392. Timing, RNG, drivers,
  world, pads, and VBlank packets still matched; only the save-transition
  allocation batch was one frame late. The diagnosis and exact evidence are
  in `docs/parity/2026-07-29-golden-run-result.md`.
- Native name-entry scancodes now travel through the fixed-size pad snapshot.
  A state-preserving migration attempt stopped at frame 367 because version 2
  does not record VBlanks emitted between tracked frames during loading.
  `--record-from-replay` now uses only the validated old pad snapshots and
  memcard seed to automate a fresh recording with its own timing/checkpoint.
  This was the open boundary at that checkpoint; later clean reports and the
  completed process/mutation verifier below closed it.
- The first full current-source optimized-i686 version-4 regeneration is a
  rejected parity candidate. It matched all eight components through frame
  6,779, then diverged in `drivers` and `root` at frame 6,780 while timing,
  input, VSync, RNG, world, and allocation remained exact. Raw driver dumps,
  instruction-level LLDB/GDB captures, and checkpoint asset bytes traced the
  first mismatch to `VehLap_UpdateProgress`: two bots carried the `0xff`
  checkpoint sentinel, so the native ports indexed beyond the real restart
  array and consumed relocated pointer words. ILP32 stored process-specific
  host pointers there while LP64 stored guest references. The resulting
  progress value therefore depended on pointer representation and ASLR.
- Native `VehLap_UpdateProgress` now ignores an index outside the level's
  restart-node count before resolving or reading the array. The ASM-verified
  PS1 instruction path is unchanged. A fourteenth media-free test covers the
  first and last valid indices and rejects one-past-end, `0xff`, empty, and
  null cases on ARM64, ARM64 ASan/UBSan, and i686.
- The corrected replacement reports both finalized all 24,232 frames, but
  their full comparison is also rejected. Timing, pads, and VSync match for
  every frame; RNG, world, allocation, and root first differ at frame 16,561,
  followed by drivers at frame 17,213. Exact recurrence analysis proves that
  i686 makes five additional particle-RNG calls at the first boundary.
  Accepted-producer memory inspection proves the delta is five correctly
  initialized i686 potion-shatter particles that ARM64 misinitializes and
  immediately loses. `RB_Explosion.c` had cast nine packed 0x24-byte retail
  emitter records to a pointer-bearing host struct whose LP64 size is 0x30.
  The table is now typed, `Particle_Init` accepts a const emitter, and a
  fifteenth media-free test checks all fields on both widths plus the exact
  original 81-word layout on i686. ARM64 Release, ARM64 ASan/UBSan, and i686
  each passed 15/15 tests. A clean ARM64 replay then proved that correction
  removes every earlier RNG and driver mismatch and both frame-16,561 short
  world/allocation ranges.
- The clean ARM64 replay retained only a later mismatch against the stale
  i686 report: world/root frames 22,158-22,656 and allocation frames
  22,158-22,630. Decoding checkpoint 22,200 proved the delta is exactly ten
  missing ARM64 cutscene particles from overlay-233 group 46. Seven group
  headers repeated the same ILP32-only union-member encoding pattern. They now
  use `FuncInit`; a sixteenth test validates the semantic fields and config
  pointers on both widths and exact original record bytes on i686. ARM64
  Release, ARM64 ASan/UBSan, and optimized i686 pass 16/16. Full clean-pair
  regeneration is still required before the gate can pass.
- Clean pushed ARM64 producer `eee2a8df5b96` subsequently finalized all
  24,232 frames and 81 checkpoints. Its full comparison against the immutable
  earlier i686 report now matches timing, RNG, drivers, world, allocation,
  root, pads, and VSync for every frame, proving both old mismatch ranges are
  gone while the byte-preserving i686 semantics remain unchanged. The clean
  same-commit i686 producer passes 16/16 tests and finalized report
  `ctr-025812` with all 24,232 frames and 81 checkpoints. The two clean
  reports match all eight components on every frame; the expanded transport
  audit and game-generated save also match. The independent verifier then
  completed two unchanged 24,232-frame i686 playbacks under distinct host/raw
  checkpoint layouts and rejected the automatic frame-1,711 driver mutation
  with exit 2 and `drivers` first. Finalize-only artifact verification exited
  0. The full M1 gate is accepted. Evidence is in
  `docs/parity/2026-07-31-full-cross-width-acceptance.md`; rejected diagnostic
  routes remain in `docs/parity/2026-07-30-full-cross-width-result.md`.
- `tools/extend-replay-input.mjs` can now promote a validated version-2 or
  version-3 replay to version 4 while preserving its historical in-frame
  packet timing after frame zero. It requires an independent complete
  version-4 replay to supply frame zero's otherwise-unrecorded boundary.
  Clean ARM64 report `ctr-223221`
  replayed the resulting 24,232-frame seed to normal exit and reached
  `lapIndex=1` at frame 21,300. Its expanded VBlank sequence, elapsed times,
  and PSX pad transport match the validated promoted seed on every frame.
  This closes the current-format structural lap-coverage task without treating
  copied input digests as parity evidence.

Work:

1. Reproduce upstream's supported 32-bit Linux build in a pinned container or
   CI environment and run it with the retail image.
2. Read the checkpoint, replay scheduler, savestate, and CTest paths in full.
3. Determine the strongest deterministic comparison available: per-frame pad
   snapshots, checkpoints, serialized state hashes, render-independent game
   state, or a combination.
4. Create a repeatable golden run covering startup, menus, a race, powerslide
   boosts, items, lap completion, and save/load.
5. Record exact commands, compiler versions, inputs, hashes, outputs, known
   nondeterminism, and failures in `docs/parity/`.
6. Attempt an unchanged Apple ARM64 configure to preserve the expected
   64-bit-gate and `-msse` failure evidence.

Acceptance:

- The supported 32-bit build and golden run are reproducible from documented
  commands.
- The parity gate fails when a known game-state byte is perturbed.
- The gate distinguishes gameplay state from allowed host-only differences
  such as window handles, file paths, clocks, and native pointer values.
- If existing tooling cannot provide a trustworthy gate, its exact limitation
  and the replacement harness are documented before 64-bit structural work.

### M2 — 64-bit memory-model design and layout census

**Status:** census, prototype, and ADR accepted; downstream parity acceptance
still depends on M1

Result:

- The DWARF-backed census assigns all 939 pointer-bearing field contexts to
  serialized-file, scratchpad-guest, resident-map, or runtime-host ownership.
- `ADR-0001-guest-references.md` selects checked eight-bit-tag/24-bit-offset
  guest references and rejects low host-address allocation as a correctness
  dependency.
- The ARM64-host prototype traverses the real frame-1800 Roo's Tubes MPAK
  above `UINT32_MAX` and rejects four corrupt-reference cases.

Work:

1. Recount pointer narrowing, integer-pointer fields, pointer-bearing
   size-pinned structs, scratchpad `Ptr32` slots, and in-place asset relocation
   sites against the incorporated upstream commit.
2. Classify every affected field as:
   - serialized/file-layout guest value;
   - scratchpad/retail-layout guest value;
   - resident executable-map guest value;
   - runtime-only host pointer;
   - opaque retail address label never dereferenced.
3. Prototype the two load-bearing approaches against one real asset:
   centralized guest offsets/handles with boundary translation, and a bounded
   guest arena. Do not depend on a low-address host allocation that Apple
   platforms cannot guarantee.
4. Write an architecture decision record with measured tradeoffs and
   migration rules.

Acceptance:

- Every pointer-bearing pinned field has an owner and migration decision.
- A real relocated asset can be loaded and traversed without truncating a host
  pointer.
- Corrupt/out-of-range guest references fail deterministically with a useful
  diagnostic.
- The chosen representation preserves on-disc and retail scratchpad byte
  layouts.

### M3 — Mechanical pointer-width and runtime-layout conversion

**Status:** mechanical compile and i686 regression gates accepted; runtime
acceptance still depends on M1, M4, and M5

Result so far:

- Pointer-to-integer and integer-to-pointer warning coordinates are both zero,
  down from 162 and 121. All 203 unique narrowing source lines are removed.
- Forced-LP64 layout failures are down from 673 to zero. All assertions now
  have explicit ownership-aware contracts. Serialized model and level
  references retain their four-byte retail layout behind checked accessors;
  resident executable/data maps retain ILP32 retail anchors and have separate
  measured LP64 runtime offsets.
- The current Level/Nav consumer batch passes all seven media-free i686 tests,
  including its synthetic serialized-graph and LP64 visibility-sidecar
  checks. The zero-error resident-map batch also passes all seven tests with
  the historical warning baseline unchanged. Detailed measured and rejected
  batches are recorded in
  `docs/architecture/64-bit-conversion-log.md`.

Work:

- Replace pointer-narrowing round trips with `uintptr_t`, native pointers, or
  the selected guest-reference type according to the census.
- Convert runtime-only integer pointer fields and parameters.
- Relax or split layout assertions only where a structure is proven
  runtime-only; retain guest-layout assertions.
- Make SSE flags architecture-conditional.
- Add `-fno-strict-aliasing` and `-fwrapv` for compilers that support them.
- Lift the global 32-bit gate only when targeted assertions and diagnostics
  replace it.

Acceptance:

- Apple Clang ARM64 compiles the converted mechanical surface without
  pointer-to-int or int-to-pointer truncation warnings.
- A checked/sanitized build reports no new pointer overflow, alignment, or
  bounds failures in the golden run.
- The 32-bit golden baseline remains byte-identical in game-visible state.

Expected commit sequence: build portability flags; guest-reference primitives;
runtime-only field conversions by subsystem; parameter/cast cleanup; assertion
split; gate replacement.

### M4 — Serialized asset relocation

**Status:** atomic relocation boundary, malformed-input tests, and serialized
model and Level/Nav consumer migration pass the media-free i686 gate; the
retail ARM64 boot now crosses 2,000 frames, while full asset/play/replay
acceptance remains pending and still depends on M3

Work:

- Replace `LOAD_RunPtrMap` host-pointer writes into 32-bit asset slots with the
  selected guest-reference representation.
- Convert every relocated model, level, string-table, and command-list
  consumer through checked translation boundaries.
- Integrate the checkpoint pointer-slot registry where useful instead of
  creating a second unsynchronized relocation inventory.
- Add malformed-map, overflow, duplicate-patch, and out-of-range tests.

Acceptance:

- The full retail asset set validates and loads on 32-bit and ARM64 hosts.
- No loaded file image contains a truncated native pointer.
- Golden races spanning multiple tracks, models, menus, and credits match the
  32-bit baseline.
- Asset translation failures name the asset, offset, and violated range.

### M5 — Scratchpad and remaining pinned layouts

**Status:** implementation in progress; checkpoint v3 captures above 4 GiB,
and exact-binary playback now restores every rolling checkpoint in the
2,200-frame intro-to-race segment across different and overlapping ARM64 ASLR
layouts. Broader renderer/gameplay coverage remains open; final acceptance
depends on M3 and M4

Work:

- Convert DrawLevel, RenderBucket, Torch, and other `Ptr32` scratchpad slots
  without changing retail offsets.
- Finish resident executable-map and MEMPACK pointer-field conversion.
- Convert checkpoint/savestate formats that currently serialize host addresses.
- Version changed developer-tool formats and retain explicit compatibility
  handling.

Acceptance:

- All scratchpad size and offset assertions still pass.
- Renderer-heavy scenes, split-screen paths, effects, and allocation churn pass
  sanitizer runs.
- Save/checkpoint/replay round trips are deterministic across process image
  bases.
- The repository-wide census has no unexplained native-pointer narrowing.

### M6 — Correct macOS ARM64 desktop build

**Status:** in progress; native configure/build/CTest, a launchable signed
development app bundle, direct Metal-backed visual inspection, corrected
24,232-frame ARM64 playback, and a guarded native restart-node boundary are
complete. Both observed cross-width emitter defects are corrected, the clean
ARM64 report matches the prior i686 trajectory across all 24,232 frames, and a
fresh current-build version-4 report structurally reaches `lapIndex=1`.
Independent i686 repeatability/mutation, broad audio listening, and complete
manual play with physical controllers remain open.

Result so far:

- `macos-arm64` configures and builds a thin ARM64 Mach-O with Apple Clang;
  all sixteen media-free tests pass. The newest tests verify all 65
  translated retail physics constants across all four engine classes and
  resolve all 46 generic reads across the 51 real VS/battle quip metadata
  records, every native render-list head, and all four red-beaker cloud draw
  records on both pointer widths. The twelfth test exercises audio snapshot
  capture/restore through a deliberately four-byte-aligned byte stream; the
  thirteenth distinguishes an address-shaped scalar word from a relocated
  pointer that later reverted, enumerates pool allocations through the
  free-list complement, covers the camera collision pointer, and proves that
  overlapping recorded/live address ranges cannot relocate one typed slot
  twice. The fourteenth rejects invalid restart-node indices, including the
  AI `0xff` sentinel that exposed pointer-representation-dependent progress in
  the full cross-width trace.
- `macos-arm64-app` builds a distinct thin-ARM64 `CTRPad.app`, targets the
  macOS 11.0 deployment floor, binds bundle identifier
  `io.github.chrissotraidis.ctrpad`, and applies a strict-verifiable ad-hoc
  development signature after linking. The launcher keeps the user-supplied
  raw image external and creates only an ignored symlink beside the bundle;
  the bundle itself contains no retail byte.
- The first retail launch exposed and sanitizer-localized three LP64 runtime
  defects: a HOWL pool stride, a four-byte MPK reference read as a host
  pointer, and unaligned/undersized render-bucket storage. Their complete
  failure/correction sequence is recorded in
  `docs/history/ENGINEERING-JOURNAL.md`.
- After correction, both the ASan/UBSan build and the normal build completed
  2,000 retail frames without a reported fault. The normal run reported
  31.25 FPS and was deliberately stopped.
- The forced-LP64 audit remains at zero errors, assertions, and narrowing
  coordinates. The finalized isolated i686 regression build passes 13/13 as
  an ELF32 Intel 80386 executable (Build ID
  `155f5086a4f576fc2b65367915dfa4b59d3832c2`); the immutable historical
  baseline executable remains untouched.
- Direct macOS application control now sees the bundled process and its real
  window. A 15,803-frame diagnostic launch rendered the Naughty Dog splash,
  textured 3D tracks, karts, item crates, UI text, and the main menu through
  the Apple M2 / OpenGL 4.1 Metal path. F10 finalized the recording and
  Command-Q closed the app cleanly. Rapid synthetic one-tap gameplay keys were
  not sampled reliably in that initial run, so that observation remains
  visual/lifecycle evidence rather than input acceptance. A later direct
  trace proved SDL delivered key-down and key-up between two retail polls.
  The host layer now retains mapped press edges for exactly one PSX-shaped
  snapshot. A single live `C` press reached the retail bus as packet
  `00 41 ff bf 80 80 80 80` and advanced the textured menu into Adventure.
  Exact source commit `24aff7d88`, 16/16 Release, ASan/UBSan, and i686 test
  results, and rejected observations are recorded in
  `docs/parity/2026-07-31-macos-arm64-keyboard-tap.md`.
- Checkpoint v3 now stores host addresses at native width. After correcting
  nested gamepad, HOWL, resident-data, process-stack, and transient
  render-bucket ownership, a frame-zero checkpoint captured at one ARM64 ASLR
  layout restored and replayed 178/178 frames at another layout with zero
  stale-pointer findings and exit status 0.
- A 1,900-frame replay-seeded ARM64 attempt was visibly stuck and is rejected.
  Its timing/allocation/root state differed at frame zero because replay
  version 2 omits VBlanks emitted between tracked frames during asynchronous
  loading.
- The visible frame-307 stall was traced to retail small/medium process-stack
  strides that could not hold widened LP64 objects. Host strides now fit every
  supported object while preserving retail item counts.
- Normalizing the maximum `0x9ec0` widened-pool overhead keeps the physical
  backing and remaining gameplay allocation pressure retail-equivalent. This
  clears the former frame-1709 particle-pool allocation loop.
- A one-player LEV legitimately leaves inactive visibility slots null. The
  LP64 sidecar now preserves those optional slots and validates only populated
  references plus active-player requirements, clearing the first-race-frame
  null dereference.
- The retail `MetaPhys` table's byte offsets are translated relative to the
  native `Driver` constant block instead of being applied to the widened
  struct base. This restores gravity/speed/handling/collision values and
  clears the subsequent race collision divide trap without changing any
  physics formula.
- The broad ARM64 coverage report
  `/tmp/ctrpad-arm64-current-normal-HiaVBo/debug/reports/20260730/ctr-062951`
  completed 2,200 frames, activated the race at frame 1,711, captured eight
  rolling checkpoints, and exited 0. Relative to pre-sanitizer `ctr-060443`,
  RNG, drivers, world, and allocation match all 2,200 frames; timing/root
  differ from frame 1,756 after corrected race-render dispatch begins.
- A fresh combined ASan/UBSan campaign retained four rejected reports and
  corrected, in order: an unaligned audio snapshot cast, native render-list
  offsets used as retail table indices, four scratch `Ptr32` values used as
  host cursors, and a clipped OT pointer reconstructed from a packed word.
  Final sanitizer report
  `/tmp/ctrpad-arm64-asan-clipot-XvhS0n/debug/reports/20260730/ctr-062801`
  finalized all 2,200 frames with eight checkpoints and exit 0. The complete
  failure/correction chain is in `docs/history/ENGINEERING-JOURNAL.md`.
- Replay format version 3 fingerprints the exact executable bytes in addition
  to the commit/build label. A different dirty binary with the same
  `a40a7584c576-dirty` label is now rejected before checkpoint restore, while
  an exact copied binary restored frame-zero state at a different ASLR/mempack
  layout and completed 2,200/2,200 frames with race activation at frame 1,711.
- The old whole-region stale-pointer scan was rejected after four adjacent
  16-bit race-flag scalars at SDATA offset `0x538` happened to form an aligned
  address inside the recorded mempack. Restore now keeps a bounded ledger of
  pointer relocations actually applied and fails only if one of those slots
  reverts to its old-process value. Ledger overflow is itself a hard failure.
- Rolling-checkpoint playback can select any zero-based record with
  `--replay-start-checkpoint`. Exact-binary playback of checkpoints 0 through
  7 (frames 0 through 2,100) reaches frame 2,200 for every record. The review
  exposed four restore boundaries hidden by frame-zero coverage: live thread
  and level-instance pools are the complement of their free lists, LP64
  visibility/model caches must be rebuilt per process, and
  `CameraDC.ptrQuadBlock` is a relocatable runtime pointer.
- A later repeated sanitizer launch rejected the first rolling-checkpoint
  acceptance: old and live mempack ranges partially overlapped, so duplicated
  semantic walkers could relocate one already-live slot a second time. The
  typed relocation ledger now has a fixed open-addressed slot index and makes
  every relocation idempotent. Five exact checkpoint-6 sanitizer replays pass;
  one used live mempack base `0x106c3b3a0` against recorded
  `0x106ddb3a0`, exercising real range overlap.
- Final ordinary report `ctr-081618`, produced by binary SHA-256
  `7282aa566ca1237ca63f539553cc33be4ae8027de2b8c8d8c299d18bdda6c740`,
  restored checkpoints 0 through 7 with the exact producing binary and no
  header bypass; every run reached frame 2,200 and exited 0. Final sanitizer
  report `ctr-081155` and the full failure/correction chronology are recorded
  in `docs/history/ENGINEERING-JOURNAL.md`.
- Replay version 4 now owns VSync packets emitted before the next tracked
  frame and run-length-encodes repeated calls. Correcting the widened decoded
  cutscene record, filtering freed driver slots, and defining schema-2
  allocator lifecycle semantics closes the remaining cross-width prefix
  defects. Final ordinary ARM64 report `ctr-103032` and disposable i686 report
  `ctr-153116` match timing, RNG, drivers, world, allocation, and root for all
  2,200 frames, including race activation at frame 1,710. Final combined
  ASan/UBSan report `ctr-103044` also completes and matches all six components.
  Exact identities, hashes, rejected probes, and the later native
  instance-name sanitizer correction are recorded in
  `docs/parity/2026-07-30-arm64-prefix-parity.md`.
- The first 24,232-frame ARM64 version-4 regeneration was rejected after it
  entered the MEMPACK allocation-error loop at frame 22,156. Same-frame
  checkpoints proved that a derived LP64 language-pointer table had consumed
  32,268 bytes of the retail-pressure arena. Keeping that table in fixed
  native storage allowed replacement report `ctr-115352` to finalize all
  24,232 frames and 81 checkpoints. Exact hashes and the failure/correction
  chain are in
  `docs/parity/2026-07-30-arm64-full-regeneration.md`. Full optimized-i686
  regeneration and eight-component comparison remain pending.
- The raw retail `Driver` offsets in `UI_VsQuipReadDriver` are now translated
  at three named native scalar ranges with width and bounds checks. A
  media-free test walks all 51 real NTSC-U metadata records on ARM64 and
  i686. Actual multiplayer end-of-race/VS execution remains unverified.
- The level render-list walkers no longer write pointer heads through retail
  `slot * 8 + 4` / `0x28` offsets. They select named host-width fields, and a
  media-free test covers all six heads on i686 and ARM64.
- The frame-1,802 striped race surface was traced above OpenGL to DrawLevel's
  LP64 texture-word classifier. It treated an ADR-0001 guest reference as a
  host pointer, advanced to the wrong texture record, and emitted false
  16-bit framebuffer pages. Resolving and bounding the mosaic reference
  restores the exact i686 CPU render trace: one flush, 5,991 vertices, 352
  splits, and zero 16-bit splits. The corrected ARM64 capture has coherent
  Crash Cove road, dirt, grid, kart, sky, and HUD textures. Exact evidence is
  in `docs/parity/2026-07-30-arm64-full-regeneration.md`.
- Current-source optimized-i686 report `ctr-190458` is retained as a rejected
  diagnostic run. It matched all eight components through frame 6,779 before
  an AI `0xff` checkpoint triggered an out-of-bounds restart-node read.
  Presented-window captures through frame 11,400 show coherent Crash Cove
  geometry, kart, HUD, minimap, portraits, and textures under the known
  llvmpipe color skew, so the process was not a black-screen stall. The full
  byte-, field-, debugger-, failure-, and visual-evidence chain is recorded in
  `docs/history/ENGINEERING-JOURNAL.md`.
- Clean committed ARM64 producer `55d3b71c6da5` finalized corrected report
  `ctr-170507` with all 24,232 frame records, 81 checkpoints, the scripted
  powerslide, and persisted 6,016-byte save. Two independent unchanged ARM64
  playback processes passed all frames under different ASLR layouts, and the
  deliberate frame-1,711 driver-position mutation failed on the `drivers`
  component as required. Corrected optimized-i686 report `ctr-223323`
  finalized all 24,232 frames but is rejected: timing, pads, and VSync match
  throughout; RNG differs over 4,845 frames, drivers over 202, world over 519,
  allocation over 553, and root over 5,344. The first frame has exactly five
  additional i686 particle-RNG calls. Exact ranges, accepted-binary tracing,
  and rejected diagnostic routes are in
  `docs/parity/2026-07-30-full-cross-width-result.md`.
- Direct read-only inspection of the exact accepted producers at frame 16,561
  found 34 ARM64 versus 39 i686 live particles and 94 versus 89 free slots.
  The last 34 i686 records match the complete ARM64 list field-for-field; the
  five extra i686 list-head records are potion fragments with 19 frames left.
  The shared common RNG state advances once for each fragment because the
  emitter randomizes Y velocity by 400, explaining the exact five-call delta
  and the 20-frame allocation/world mismatch. The root cause was the raw
  32-bit emitter-table cast in `RB_Explosion.c`; the typed replacement and its
  cross-width byte-layout test pass 15/15 on ARM64 Release, ARM64 ASan/UBSan,
  and optimized i686. A clean committed ARM64 regeneration then removed every
  frame-16,561 RNG, driver, allocation, and world mismatch against the prior
  i686 report. The only remaining ranges begin at frame 22,158.
- Checkpoint 22,200 contains ten live i686 particles and zero ARM64 particles;
  all other pool counts match. The ten records have the color, axis, scale,
  and descending lifetime sequence emitted by overlay-233 particle group 46.
  Seven overlay-233 function records were initialized through the axis member
  of a pointer-bearing union, which preserved retail bytes on ILP32 but placed
  color/lifespan inside the widened function pointer on LP64. Those records
  now use typed function initializers. A sixteenth media-free test validates
  all seven groups, terminators, eight configs, and exact i686 retail bytes;
  ARM64 Release, ARM64 ASan/UBSan, and optimized i686 pass 16/16. Clean ARM64
  report `ctr-215303` then matched the earlier immutable i686 trajectory on
  all eight components across all 24,232 frames. Clean same-commit i686 report
  `ctr-025812` finalized 24,232 frames and 81 checkpoints with process exit 0.
  It matches `ctr-215303` on timing, RNG, drivers, world, allocation, root,
  pads, and VSync for every frame. The stricter transport audit also matches
  pad bytes, elapsed time, total VBlanks, raw blocks, and both expanded VBlank
  sequences on all 24,232 frames. This accepts the formal clean cross-width
  pair. The later alternate-layout verifier completed both unchanged
  24,232-frame playbacks and rejected the automatic frame-1,711 mutation with
  `drivers` first, completing the remaining M1 gate.
- `tools/inspect-replay-lap-coverage.mjs` validates checkpoint checksums and
  resolves player lap/checkpoint fields for checkpoint versions 2 and 3 on
  ILP32 and LP64. It confirms both the historical version-2 report and fresh
  current-build version-4 report `ctr-223221` reach `lapIndex=1` at frame
  21,300. The fresh report finalized 24,232 frames and 81 checkpoints with
  process exit 0. Full transport expansion finds zero PSX pad, elapsed-time,
  or VBlank-sequence mismatches against its promoted input. The inherited
  version-4 input and steering extensions A and B remain rejected:
  A ends at `maxLap=0`, while B was stopped at frame 28,850 after checkpoint
  progress had remained unchanged for 7,550 frames and never advanced a lap.
- `tools/analyze-replay-cadence.mjs` validates complete replay timing and can
  measure checkpoint-local wall cadence with live line-buffered markers.
  Clean report `ctr-215303` uses 32 ms and two VBlanks together on
  24,200/24,231 post-bootstrap frames. A 3,232-frame/6,465-VBlank segment
  measured 108.051815 seconds against a 108.079040-second model
  (`-0.025190%`) and exited 0. The macOS ARM64 desktop cadence task is
  accepted; iOS display/lifecycle pacing remains a later platform gate.
- Clean current ARM64 report `ctr-215303` wrote a 6,016-byte
  `BASCUS-94426-SLOTS` profile from an empty seed. The checked-in save
  inspector validates its one-block wrapper, `0xffee`/`0x1600` retail profile
  header, and zero CRC remainder. A later normal startup of the exact
  immutable producer read all 5,760 payload bytes from the default memcard
  root and returned `NATIVE_MEMCARD_OK` plus game-level `MC_RETURN_IOE`.
  macOS file persistence and checksum-valid relaunch are accepted; the
  diagnostic process itself was killed after observation, so clean normal
  quit and all iOS sandbox/lifecycle cases remain open.
- The exact immutable ARM64 producer opened CoreAudio at 44.1 kHz stereo. An
  ordinary 702-frame SDL disk-sink probe produced distinct non-silent PCM with
  no full-scale samples and zero measured underrun/overflow deltas. A live
  audio-thread breakpoint then proved initial XA request `category=1`,
  `track=80` loaded 52 sectors at 37.8 kHz and decoded its first sector into
  4,032 source frames before interpolation/mixing. The device/open/output and
  initial XA decode boundaries are accepted; listening quality, broad
  mix/reverb/track coverage, STR synchronization, and iOS routes remain open.
- Clean commit `87f8e7a052c2` adds a media-free production-path audio oracle.
  A synthetic PS1 ADPCM block crosses SPU upload, Key On, streaming decode,
  interpolation, stereo volume, master volume, mixing, and frame rendering;
  a one-shot copy then feeds the Room reverb. Signed ARM64, ASan/UBSan ARM64,
  and optimized i686 pass 17/17 with exact dry/wet digests
  `132e19d77167fb3d` / `4bdedc91d1293ad8` and 6,905 post-voice wet-tail
  frames. Deterministic cross-width voice decode, panning, one-shot stop, and
  Room reverb are accepted. Human listening, representative retail mixes,
  other presets, broad XA transitions, long CoreAudio soak, and iOS routes
  remain open. Exact evidence is in
  `docs/parity/2026-07-31-macos-arm64-audio-mixer-oracle.md`.
- Mapped macOS keyboard press edges now survive a complete key-down/key-up
  pair between retail polls and are consumed for exactly one snapshot.
  Replay, disabled-pad, init/shutdown, and state-restore boundaries clear the
  host-only latch. The exact committed source passes 16/16 on ARM64 Release,
  ARM64 ASan/UBSan, and optimized i686; the signed app is a strict-verifiable
  thin ARM64 bundle. Commit `2c10b00b34df` adds documented `WASD`, `IJKL`,
  `Q/E`, `P`, and Tab aliases without removing the original map. The exact
  app passed 16/16 and visibly accepted `K`, `S`, and `W` through normal boot
  and main-menu navigation. A later clean-tip signed-app run entered Time
  Trial and visibly moved the Crash Cove kart with dense `K` acceleration and
  `K+D` steering input. A sequential automation stream cannot prove a live
  simultaneous drift chord, and a natural human-held lap/full race remains
  open.
- SDL controller slots now record the resolved joystick instance ID after a
  successful open and return the mapping to `-1` on close. This prevents a
  duplicate add from opening one device in two slots and prevents removal
  from leaving a second ghost handle. A virtual standardized gamepad drives
  the production add/snapshot/rumble/remove/reconnect path and proves
  active-low buttons `0xb5ff`, analog bytes `80 ff 00 80`, exact low/high
  rumble magnitudes, one-handle duplicate-add behavior, and slot reuse. Exact
  clean-tip ARM64 Release, ASan/UBSan, and optimized i686 builds pass 16/16;
  physical MFi/Bluetooth/USB hardware and full-race play remain open. Evidence
  is in `docs/parity/2026-07-31-macos-arm64-controller-hotplug.md`.
- A headless retail scrapbook probe now reads `TEST.STR` through the production
  disc-image path and hashes decoded RGB555 output without renderer startup.
  ARM64 and optimized i686 produce identical dimensions and hashes for the
  first ten 512-by-208 frames, including sequence hash
  `60dcf4c65986a034`.
- Exact clean ARM64 commit `c44ea7810391` now drives those ten frames through
  production `LoadImage`, the host VRAM texture, the direct-VRAM presentation
  shader, portable RGBA framebuffer readback, and BMP output. Three runs
  produced identical presented sequence hash `e85a9203c966c801` and
  byte-identical frame-9 BMP; visual inspection shows the coherent Naughty Dog
  scrapbook title. ASan/UBSan then exposed undefined null-member OpenGL
  attribute offsets. Commit `75b09db17d1c` replaces them with pinned
  `offsetof` values. Its exact clean Release and sanitizer builds each pass
  16/16 tests and reproduce the same ten decoded/presented hashes and
  byte-identical BMP. The renderer sanitizer run disables SDL HID enumeration
  to avoid a separately documented Apple CoreGraphics ASan fault; the
  sanitized input CTest still passes. Exact results and every rejected run are
  in `docs/parity/2026-07-31-scrapbook-str-presentation.md`. Exact clean
  Release and sanitizer runs subsequently decoded, uploaded, presented, and
  read back all 4,424 frames with identical per-frame manifests and sequence
  hashes. Complete probe coverage is accepted. Exact ARM64 commit
  `63b0a0773a00` then loaded a checksum-valid disposable save through normal
  startup, exposed and entered the production Scrapbook row, uploaded 674
  frames with XA channel 1 active and the four-vblank scheduler selected,
  accepted a Start skip, changed XA from active to inactive during teardown,
  and returned to the intact menu. Two later normal-startup runs uploaded all
  4,424 frames, ended at exactly 17,696 elapsed VBlanks, and returned to the
  menu. XA exhausted before the existing 23-frame fade plus 30-frame black
  tail. Natural end, cadence, and authored-tail alignment are accepted;
  human-perceived listening quality/synchronization remains open.
- Red-beaker rain no longer reads per-player MVP translations out of widened
  instance function/thread pointers. Named draw-record fields preserve the
  retail depth/LOD byte alias, and a four-player cross-width test covers the
  unexercised item-effect layout. Live item-effect rendering remains
  unverified.

Work:

- Validate retail-asset and human audio listening, broader reverb/XA
  transitions, broader desktop renderer,
  full-race manual keyboard play, physical MFi/Bluetooth/USB controllers,
  replays, and savestates.
- Run sanitizers and the full parity gate under Apple Clang.
- Retain the finalized `ctr-025812` cross-width and independent-process
  evidence. `CTRPAD_REQUIRE_COVERAGE=0` remains specifically the
  process-determinism/mutation proof; the separately accepted manual coverage
  form supplies scenario coverage. All pre-correction mismatched and partial
  reports remain diagnostic inputs, not acceptance artifacts.

Acceptance:

- A clean checkout configures, builds, and launches natively on Apple Silicon.
- The golden suite matches game-visible 32-bit state exactly.
- At least one complete Adventure-mode progression segment and representative
  Arcade races play without corruption, crash, or timing drift.
- Saves persist across relaunch.
- No unresolved sanitizer finding or pointer-width warning remains.

M6 is a hard gate. iOS platform work does not proceed on an incorrect macOS
build.

### M7 — Shared OpenGL ES 3 renderer

**Status:** in progress; shared dialect, live iPad Simulator GLES pixels, a
full 24,232-frame exact-current desktop-GL/GLES state/transport match, three
exact representative render traces and a targeted cross-API pixel-semantics
oracle have landed; live macOS GLES and physical-device cadence remain open

Checkpoint `78ef952dbecc` adds the explicit ES 3.0 / GLSL ES 300 production
dialect, SDL proc loading, desktop-only feature guards, pre-context-safe
cleanup, an exact dialect/lifecycle CTest, an iOS Simulator ARM64 compile/link
probe, and clean 18/18 ordinary ARM64, sanitizer ARM64, GLES-configured ARM64,
and optimized i686 gates. Desktop GL still initializes all PSX/VRAM shaders on
Apple M2. The host lacks the ANGLE/EGL runtime required by SDL's Cocoa GLES
backend, so that checkpoint claimed no live GLES pixels or M7 acceptance. Full
evidence is in `docs/parity/2026-07-31-shared-gles3-dialect-bringup.md`.

Checkpoint `98ae2c6d86fe` subsequently launches that shared GLES renderer on an
ARM64 iPad Simulator through SDL/UIKit. The exact clean run selected UIKit's
framebuffer/renderbuffer 1, compiled all PSX and VRAM pipelines, opened
CoreAudio, and rendered a coherent full-frame textured title menu. This clears
the live-context/pixel bring-up boundary on Simulator, but one title scene is
not the representative frame comparison, deterministic state or cadence
evidence required for M7 acceptance. Detailed evidence is in
`docs/parity/2026-07-31-ios-simulator-gles-bringup.md`.

Current implementation `e6ba535a9c73` closes a substantial part of that open
boundary. Exact macOS desktop-GL and iOS UIKit/GLES binaries independently
regenerated a 2,000-frame boot-origin version-4 scenario from identical pad
and VSync input. Timing, RNG, drivers, world, allocation, root, pads and VSync
matched on all 2,000 frames. Exact native playbacks emitted identical packed-
vertex/draw-split traces at frames 1,802 and 1,813, and a local-only iOS frame
showed coherent indexed textures, colors, exhaust transparency and the touch
overlay. The historical false 16-bit batches at frame 1,802 were the already-
fixed LP64 mosaic-classification defect; both current renderers correctly emit
zero. Full commands, hashes, rejected cross-platform-checkpoint route and
preservation evidence are in
`docs/parity/2026-07-31-ios-gles-desktop-gl-equivalence.md`.

The exact same implementation build subsequently regenerated the complete
24,232-frame/81-checkpoint scenario natively on macOS desktop GL and iOS
UIKit/GLES. All eight required components matched on every frame with zero
mismatches. Exact checkpoint-80 playbacks reached frame 24,232, and both
renderers emitted identical packed vertices/draw splits at late frame 24,001
(`d1765e952537c48b`). Direct checkpoint inspection truthfully reports
`lapAdvanced=no`; the separate accepted current-format lap report remains the
lap-structure oracle. Full report hashes, the one rejected stale-bundle-ID
launch, keyboard revalidation and preservation evidence are in
`docs/parity/2026-07-31-ios-gles-full-golden.md`.

Checkpoint `818bc0e161d3` closes the remaining locally testable pixel boundary
with a media-free production-pipeline oracle. Exact Apple M2 desktop GL and
UIKit/GLES runs agree on full-frame hash `851169f2644a1675` while directly
checking 4/8/16-bit textures, both CLUT paths, zero/STP transparency, average
blend, E6 output-mask bit 0, real framebuffer feedback, exact alpha and
RGB5551 VRAM packing/readback. The first live GLES run exposed a real Apple
GLES `GL_RG` readback failure despite correct visible RGBA pixels; the fixed
path uses guaranteed RGBA/UNSIGNED_BYTE readback and repacks R/G only after an
error-free call. Exact-commit desktop and sanitizer suites pass 22/22; iOS
Simulator live execution, iPhoneOS ARM64 link and macOS GLES compile/link also
pass. The implemented mask claim is deliberately limited to CTR's bit-0
packet path. Full rejected-fixture, failure, hash and preservation evidence is
in `docs/parity/2026-07-31-renderer-pixel-semantics.md`.

Work:

- Diff Simon358's GLES branch against the exact upstream baseline and port only
  understood changes.
- Maintain the implemented SDL GLES 3 context path, ES 3 shaders, guaranteed
  RGBA readback/repack, and platform guards for unsupported debug-only desktop
  GL features.
- Keep the existing 24-bit GPU-link bridge unchanged unless a failing test
  demonstrates a defect.
- Validate desktop GL and GLES output using frame captures and game-state
  parity.

Acceptance:

- macOS can run the shared GLES path and complete the golden suite.
- Representative frame captures have no missing primitives, incorrect CLUT
  colors, mask-bit failures, transparency regressions, or framebuffer-feedback
  artifacts.
- Renderer choice does not alter game-visible state or frame cadence.

The third criterion is accepted for the complete 24,232-frame scenario. The
second is accepted by the cross-API production pixel oracle in addition to the
coherent framebuffer and exact traced frames 1,802, 1,813 and 24,001. The
first criterion remains blocked locally by the absent Cocoa ANGLE/EGL runtime;
physical-device cadence/energy remains an explicit hardware confirmation
boundary.

### M8 — iOS/iPadOS application with controller input

**Status:** in progress; reproducible app shell, Simulator launch and
background/resume/rotation plus balanced ordinary UIKit presentation landed;
physical-device/controller/full-lifecycle acceptance depends on M6 and M7

Checkpoint `98ae2c6d86fe` adds ARM64 Simulator/device presets, credential-free
iPhone/iPad metadata, landscape declarations, SDL's iOS-owned entry point,
physical-pixel sizing and UIKit presentation-object handling. An exact clean
local-only Simulator package launches the production retail path, renders and
opens audio. The generated physical-device binary is unsigned and has not run
on hardware. The ordinary-launch appearance warning reported at that checkpoint
was subsequently corrected by `6b268157888f`; physical lifecycle acceptance
remains explicitly open.

The 2026-07-31 physical-gate audit reported `No devices found` from
`xcrun devicectl list devices`, zero valid code-signing identities from
`security find-identity -v -p codesigning`, and no local provisioning-profile
files. The unsigned ARM64 device product is ready for that gate, but hardware
installation cannot begin on this Mac until an iPad and Apple development
signing assets are available.

Checkpoint `afb5463cc511` adds a synchronous SDL/UIKit lifecycle reducer,
cooperative quit, paired audio/input suspension, host-only VBlank deadline
rebasing, and a `CADisplayLink` callback that returns to UIKit between retail
steps. An exact locally signed Simulator package completed two full Home and
foreground cycles with live textured rendering and resumed CoreAudio. The iOS
wait now yields completely instead of entering either the project spin window
or `SDL_DelayPrecise`'s final sub-millisecond spin. Simulator performance is
not accepted: a 324-frame diagnostic averaged 8.335 FPS and attributed
105.157 ms of 119.983 ms per frame to software-renderer triangle submission.
Detailed evidence is in
`docs/parity/2026-07-31-ios-lifecycle-display-loop.md`.

Checkpoint `6b268157888f` modernizes SDL's UIKit view-replacement path for
CTRPad's iOS 15 minimum. It preserves the installed root controller and
directly attaches a replacement GLES view instead of clearing and restoring
the controller through a historical iOS 7 workaround. The first attempt
removed the reset but failed to attach the replacement view, yielding a fully
black screen despite zero ordinary warnings; it was reverted. The accepted
path preserved coherent pixels and produced zero appearance-transition
warnings during exact production startup, real Simulator Home/resume, rotation
and bounded termination. Ordered audio/lifecycle markers, overlay reflow,
renderer hash `851169f2644a1675`, unchanged BIN/save hashes, desktop and
sanitizer CTest 22/22, iPhoneOS ARM64 link and macOS GLES link all pass. One
warning after the deliberately immediate renderer-self-test teardown remains
recorded; natural app-initiated termination and physical-hardware lifecycle
remain open. Full chronology, hashes and commands are in
`docs/parity/2026-08-01-ios-uikit-view-lifecycle.md`.

Work:

- Retain the completed device/simulator presets, bundle metadata and
  credential-free signing configuration; add final launch artwork only when
  it can be distributed without retail content.
- Retain the landed SDL-owned entry point, display callback, lifecycle reducer
  and cooperative quit; finish rotation, natural termination, low-memory and
  background-save behavior on physical hardware.
- Retain the fully yielding iOS wait and exact NTSC host deadline; measure
  hardware cadence/energy and decide whether the remaining synchronous retail
  step must become a fully nonblocking scheduler.
- Bring up audio, GLES, and MFi/Bluetooth controllers on a real iPad.

Acceptance:

- A development-signed build installs and launches on a real ARM64 iPad.
- With a controller, the user can navigate menus, complete races, hear XA/audio,
  view STR video, suspend/resume, and relaunch.
- Backgrounding does not corrupt saves, audio, renderer state, or timing.
- Controller gameplay matches the macOS parity run.

### M9 — Sandbox storage and retail-disc import

**Status:** in progress; sandbox ownership, Files document-picker import,
staged validation, same-process startup, Simulator relaunch, and atomic
memory-card replacement have landed; a clean frame-zero Simulator gameplay
run created and preserved a retail memory-card profile across backgrounding;
an explicit default-root seed was retained across app updates and cold-read;
live Simulator wrong-region and incomplete-image failures are now accepted;
reserved interrupted-import staging is recovered safely on every launch;
an actual Simulator Files import was killed after the full staged image became
visible and recovered on relaunch; explicit asset re-selection now stops the
active runtime, rejects invalid media without replacement, atomically installs
a valid image, preserves the save and cold-relaunches on Simulator; physical
Files/signing/save behavior and inaccessible-provider coverage remain open;
depends on M8

Checkpoint `02a6623f80a0` establishes the storage ownership boundary without
shipping retail data. On iOS, a valid raw image in
`Documents/CTRPad/assets/ctr-u.bin` takes precedence over bundle fallback
assets, while the log, memory-card root and relative diagnostics live beneath
Application Support. The iOS metadata exposes Documents through Files sharing;
desktop retains its portable beside-assets layout. A clean 3.4 MB Simulator
bundle with no asset inside it launched the retail presentation from a local
Documents-only copy, created its private log, initialized GLES/VRAM/CoreAudio,
and rendered coherent textured pixels. Media-free storage coverage is CTest
16. Detailed evidence and the deliberately open boundary are in
`docs/parity/2026-07-31-ios-sandbox-storage.md`.

Checkpoint `7872f7e61ad6` adds the fresh-install native UIKit screen and Files
picker. It obtains a copy selection, coordinates security-scoped reading,
stages it on the destination volume, distinguishes raw-image, region and
required-content failures, and installs `ctr-u.bin` only after complete
validation. The successful callback reselects the Documents asset and starts
the existing runtime in the same process. A fresh iPad Simulator accepted
cancel, invalid-format rejection, a 605,698,800-byte NTSC-U import, visible
textured startup and cold relaunch without a staging leak or bundled retail
data. Detailed evidence is in
`docs/parity/2026-07-31-ios-files-import.md`.

A later isolated Files repeat used the exact signed `a37cdf2aa5af` runtime
(later runtime changes were packaging-only) and closed two previously open
branches. The real picker selected the archived PAL image and displayed its
detected `SCES_021.05` identity, then selected a 40,000-sector truncated
NTSC-U fixture and displayed the distinct required-content failure. Each
rejection re-enabled the chooser, removed its staging root, and preserved the
accepted 605,698,800-byte image and 6,016-byte memory card by inode and SHA-256.
Selecting the full NTSC-U image then started the game in the original PID; a
cold relaunch bypassed onboarding. The disposable clone was shut down and the
original evidence Simulator was restored unchanged.

Checkpoint `c745390a55eb` closes the recoverable-residue half of the
interrupted-import gate. Before showing media-free onboarding, iOS now removes
only direct child directories in the reserved
`.ctrpad-import-<nonempty-suffix>` namespace and reports the recovered count
without disabling retry. A clean exact Simulator build removed two seeded
stale stages while preserving a nonmatching directory, the exact bare prefix,
a same-prefix ordinary file, the retained 605,698,800-byte image and the
6,016-byte save. After restoring the valid destination, the same build cold-
launched rendered CTR output with the touch overlay. A later real Files
selection created a full 605,698,800-byte stage with the accepted hash; an
event-driven diagnostic sent `SIGKILL` before validation/install, leaving that
stage and no destination. The exact build removed it on launch, displayed the
singular recovery message, and then cold-launched normally after the accepted
destination was restored. This accepts Simulator process-death recovery during
an import; inaccessible-provider and physical-device behavior remain open.

Checkpoint `8c177e8327f3` closes a follow-up lifecycle hole found during
contradiction review: if termination happened after the validated destination
was installed but before the now-empty stage was removed, the next launch would
bypass onboarding and never run its cleanup. iOS now exposes the same narrow
recovery operation before ordinary runtime startup when the selected asset is
already valid. An exact signed Simulator build launched with a valid BIN plus a
seeded `.ctrpad-import-installed-destination-leftover` directory, removed that
one stage, retained the nonmatching/bare-prefix/ordinary-file controls, kept the
BIN and save at their original inodes and hashes, and visibly entered the game.
A cold relaunch remained stage-free. This accepts both media-free and
valid-installed-asset recovery paths without broadening the cleanup namespace.

Checkpoint `4b078065ff03` replaces direct memory-card truncation with a hidden
same-directory temporary-file transaction. It writes and flushes all bytes,
closes the temporary file, then atomically replaces the final save; every
failure preserves the previous final file and removes temporary residue.
CTest 17 explicitly injects an open failure and proves preservation plus
successful retry. Clean macOS ARM64 and ASan/UBSan runs pass 21/21, the exact
i686 source compile passes with implicit declarations promoted to errors, and
both iOS ARM64 products link. A clean exact-app frame-zero replay-seeded
recording completed all 24,232 frames without restoring an incompatible
checkpoint. It created a 6,016-byte checksum-valid profile, left no temporary
residue, survived a Home/background/foreground cycle unchanged, finalized 81
checkpoints, and was then explicitly copied from the isolated report root to
the default private root for a bounded cold-reader test. It later appeared as
profile `A` in the exact clean app's retail Load screen after repeated bundle
updates. Complete implementation, build, visual, keyboard-delivery,
rejected-shortcut, and live-save evidence is in
`docs/parity/2026-07-31-ios-memory-card-atomicity.md`.

Checkpoint `300499d7cd00` closes explicit active-image re-selection on
Simulator. An accessible touch-overlay control confirms before stopping the
game, performs ordered runtime/disc shutdown at the UIKit display-loop
boundary, and then reuses the existing security-scoped staged importer. Cancel
returned to animated gameplay. The real Files picker rejected a 118-byte
invalid fixture while preserving the current BIN/save by inode and SHA-256,
then atomically installed the complete NTSC-U fixture at a new inode, retained
the 6,016-byte save unchanged, disabled further selection and required a cold
relaunch. The exact build visibly rendered CTR from the replacement. Desktop
GL and ASan/UBSan pass 22/22; all five final products are ARM64 and embed the
exact commit. Evidence is in
`docs/parity/2026-08-01-ios-disc-reselection.md`.

Work:

- Retain the landed split between immutable bundle resources, imported retail
  media, user-visible Documents and private Application Support state.
- Retain the landed nonblocking iOS document picker, security-scoped coordinated
  copy, same-volume staging and validate-before-replace contract.
- Retain the live Simulator wrong-region and incomplete-image outcomes;
  retain the accepted real Files/`SIGKILL`/next-launch recovery; complete
  physical-device repetition plus inaccessible-file and device background/
  termination-during-copy coverage.
- Retain the accepted Simulator game-driven save, suspend/resume, app-update
  retention, cold retail Load-screen read and explicit asset re-selection;
  repeat the required subset on physical hardware.
- Retain atomic same-directory memory-card replacement and private Application
  Support ownership for memcards, settings, logs, and crash diagnostics.

Acceptance:

- A fresh install with no asset presents an import flow, not a crash or terminal
  log. **Accepted on Simulator; physical device remains open.**
- Importing a valid user-supplied image reaches the game without extraction.
  **Accepted on Simulator through Files and same-process continuation.**
- Cooked ISO, wrong-region, truncated, and inaccessible files receive distinct
  errors. **Invalid-format, detected wrong-region, and truncated/incomplete
  outcomes are accepted through the real Simulator Files picker. Recovery of
  reserved stale stages is accepted from both isolated seeded state and an
  actual Files import killed after its full stage became visible.
  Inaccessible-provider behavior and physical hardware remain open.**
- Saves persist across launch, backgrounding, app updates, and asset
  re-selection. **Save creation, backgrounding, update retention and cold read
  are accepted on Simulator. Re-selection preserves the exact save inode,
  size and hash across invalid and valid Files outcomes plus cold relaunch.
  Physical hardware remains open.**
- No retail byte is included in the application bundle or Git history.

### M10 — Touch-first controls

**Status:** in progress; a functional safe-area-aware Simulator prototype,
peer input composition, continuous analog steering, menu D-pad edges and core
button layout have landed; physical-iPad ergonomics and drift-boost acceptance
remain open; depends on M8 and M9

Checkpoint `c496c27f04c8` adds an iOS UIKit overlay with a virtual analog
stick, Cross/Square/Circle/Triangle, L1/R1, Start and Select. Touch is composed
with player one's PS1-shaped snapshot after controller and keyboard input, so
buttons combine active-low and touch steering replaces only the left analog
pair while active. Lifecycle suspension, replay/state installation, shutdown
and restore reset host contacts.

Checkpoint `c783eda740c4` adds D-pad direction edges at the stick's outer 32%
and retains quick keyboard/touch edges for two host snapshots. LLDB proved why
the earlier one-snapshot version failed: the correct `00 73 bf ff ...` Down
packet was replaced by neutral `ff ff` before the retail `GAMEPAD_ProcessHold`
poll. The exact clean app then navigated Adventure → Time Trial → Adventure,
opened Adventure → Load, and displayed persisted profile `A` using the touch
overlay. Exact build, test, visual and rejected-route evidence is in
`docs/parity/2026-07-31-ios-touch-controls.md`.

A later touch-only live run entered Time Trial, selected Crash, Crash Cove and
No Ghost, skipped the fly-in, accelerated from the grid to beneath the CTR
banner, opened Pause, reflowed into a full 932-by-768 landscape layout after a
device rotation, resumed with Gas, and continued moving. LLDB on an exact
`da151bfefb18` rebuild observed a right-edge UIKit stick contact as signed
analog `(32766, 250, active=1)` followed by neutral release. Desktop automation
cannot sustain two independent contacts, so simultaneous human-held Gas plus
steering/drift and a complete race remain unaccepted. Three public scene-
geometry request variants failed silently to rotate a portrait cold launch and
were fully reverted. Apple runtime diagnostics and current platform
documentation subsequently established that iPadOS 26 supports resizable
windowed scenes and no longer treats a forced landscape full-screen launch as
the correct product contract. Checkpoint `db45004f909d` supports all four iPad
orientations from iPadOS 26 while retaining the older landscape/full-screen
preference. Exact portrait and landscape Simulator runs reflowed the renderer
and every safe-area control with no runtime configuration fault. Physical-
device orientation and feel remain open.

A current-implementation follow-up at branch documentation tip `db78d5a25`
repeated the full touch route to a live Crash Cove grid using the exact audited
`560f6dd20` Simulator product. Gas UI actions moved Crash forward; an
auto-continuing LLDB breakpoint observed stick X reach `32763` and then return
to neutral on release. Native state showed Cross absent from `heldButtons` at
every stick callback, so Option-assisted, click/drag overlap and concurrent-
drag automation routes were rejected as serialized rather than counted as
multi-touch. Exact trace, screenshot hashes and failed-tooling history are in
`docs/parity/2026-07-31-ios-touch-controls.md`. This strengthens live
single-control delivery but leaves physical held Gas/steer/drift unchanged as
the M10 exit gate.

Checkpoint `e6ba535a9c73` corrects hardware-keyboard ownership on iOS. The
existing practical aliases were already reaching SDL, but an enumerated
Simulator controller moved the keyboard to player two while touch remained
player one. iOS now composes keyboard, touch and controller into the primary
PS1-shaped pad; desktop retains separate-player controller behavior. An exact
1,869-frame report records Start, Down and four Cross presses only in player
one, each with next-frame neutral release, while slots two through four remain
disconnected. Keyboard-only input reached the Crash Cove Time Trial starting
grid. Exact root cause, build hashes, rejected diagnostic routes and packet
frames are in `docs/parity/2026-07-31-ios-hardware-keyboard.md`. Physical-iPad
keyboard delivery remains open.

Work:

- Retain the implemented touch peer in the platform input composition path so
  it can coexist with a connected controller.
- Retain true analog steering, outer-ring menu directions, multi-touch holds,
  and the two-host-snapshot press-edge transport.
- Iterate on layouts for steering + held drift + three boost taps, the core
  simultaneity problem identified in the viability report
  (`docs/ctr-native-viability.md:319-358`).
- Cover menu navigation, accelerate, brake/reverse, hop/drift, fire/aim,
  camera, pause, and race-start skip without obscuring critical play space.
- Add safe-area, hand-size, handedness, opacity, scale, and remapping options as
  testing justifies them.

Acceptance:

- Touch-only users can start the app, select content, race, pause, and save.
- **Startup, menu navigation, selection, cold Load-screen access, Time Trial
  entry, forward race movement, Pause and post-rotation Resume are accepted on
  Simulator. Current-tip live traces also accept Gas movement and near-full
  analog steering with neutral release as separate contacts. Hardware-keyboard
  Start/Down/Cross delivery and release are accepted through a keyboard-only
  route to the race grid; physical keyboard and a complete human multi-touch
  race/save are still open.**
- Steering remains continuously analog while accelerate and drift are held.
- A tester can intentionally execute repeated three-boost drift chains in both
  turn directions without grip changes or missed simultaneous contacts.
- Controls work across supported iPad aspect ratios, orientations, and safe
  areas and meet practical touch-target/contrast accessibility requirements.
- Connecting a controller does not require disabling touch and causes no stuck
  inputs.

### M11 — Retail parity, signing, and GPL-compliant release

**Status:** in progress; reproducible retail-free unsigned IPA packaging,
profile-aware DER signing preparation, embedded legal/install resources and
direct/AltStore-style installation information have landed; user-owned Apple
signing, physical-iPad install and final device acceptance remain open; depends
on all prior milestones

Checkpoints `6db6116fe67a`, `a37cdf2aa5af` and `207121134a05` add a guarded
device packager and its corrections. The script verifies the ARM64/iOS bundle,
requires GPL/notices/Installation Information, rejects known retail and runtime
data, packages the standard `Payload/CTRPad.app` tree, and normalizes every
archive timestamp to `SOURCE_DATE_EPOCH`. Two exact current-tip runs produced
byte-identical unsigned IPAs. The optional signed path validates the profile's
platform, expiry, App ID and optional device, constructs minimal exact
application/team/keychain entitlements, requests DER entitlements, and strictly
verifies the result. A real Apple identity/profile was unavailable, so the
physical signature/install gate remains open rather than inferred from an
ad-hoc Simulator signature. Exact evidence is in
`docs/parity/2026-07-31-ios-sideload-package.md`.

Published implementation tip `560f6dd20963` was revalidated after the later
touch/import/recovery work rather than relying only on the original packaging
checkpoint. Two fresh packages were byte-identical at SHA-256
`ad8736cd...d3fa`, contained exactly the seven expected app/legal/install
members, and embedded thin ARM64/iOS executable identity `560f6dd20`. Extraction
proved no retail-like file, runtime directory, provisioning profile, or code
signature. The exact Simulator sibling launched from an update install,
rendered the copyright screen and full touch overlay, and retained the cloned
BIN/save identities. A fresh local audit still found zero code-signing
identities, no provisioning-profile files, and no connected devices, so this
strengthens current-source package readiness without claiming the physical
signature/install gate.

Work:

- Run the complete cross-architecture parity suite and prolonged playtesting.
- Produce reproducible macOS and iOS/iPadOS release builds.
- Document Xcode, direct-device, and AltStore-style sideload installation.
- Publish complete corresponding source, build scripts, notices, modifications,
  and Installation Information; exclude all game assets and signing secrets.
- Record known limitations honestly.

Acceptance:

- A clean machine can build from the published source and instructions.
- A person can sign, install, import their own valid NTSC-U image, play with
  touch, and retain saves.
- Physics, input timing, replay/checkpoint state, and frame pacing satisfy the
  parity gate on macOS ARM64 and iPad hardware.
- The released archive contains no retail assets, credentials, or undocumented
  generated dependencies.
- License texts, third-party notices, source offer, and modification/install
  information are complete.

## Verification strategy

Every structural change is checked at four levels:

1. **Compile-time layout:** static size/offset assertions remain for retail and
   serialized representations.
2. **Runtime safety:** bounds-checked guest translation plus ASan/UBSan where
   supported.
3. **Determinism:** identical scripted inputs produce identical game-visible
   checkpoint/state hashes against the 32-bit golden run.
4. **Product behavior:** controller and touch playtesting, frame captures,
   audio/video checks, save/relaunch, and device lifecycle.

Visual similarity or successful play alone cannot clear a parity milestone.
Conversely, host pointer values, OS paths, renderer object names, and wall-clock
timestamps are explicitly excluded from game-visible deterministic state.

## Open risks

| Risk | Current evidence | Mitigation / exit criterion |
|---|---|---|
| Guest-reference design expands into a full arena rewrite | Asset relocation writes host bases into 32-bit file slots; about 75 pinned pointer-bearing structs were estimated | M2 prototype on real assets before broad edits; preserve guest layout and centralize translation |
| Existing replay/checkpoint tooling is not a sufficient parity oracle | Prior report found infrastructure but did not run or assess coverage (`docs/ctr-native-viability.md:471-474`) | Prove mutation sensitivity in M1 or build a state-hash harness |
| Replay repeatability or mutation sensitivity regresses after the accepted cross-width trace | Resolved baseline: `ctr-215303`/`ctr-025812` match all eight components and transport for 24,232 frames; two unchanged i686 playbacks completed under distinct layouts; frame-1,711 mutation exited 2 with `drivers` first | Retain the manifests and require replacement golden producers to pass the same cross-width, two-process, alternate-layout and mutation gates |
| Current-format lap coverage could not be established from inherited input | Resolved: clean current-build version-4 report `ctr-223221` reaches `lapIndex=1` at frame 21,300; expanded promoted-seed/current VBlank sequences, elapsed times, and PSX pad transport match across all 24,232 frames | Retain the accepted report and rejected extensions A/B; require any replacement seed to pass the same structural and transport checks |
| Upstream has moved since `2df55dc5a` | Viability report is commit-specific | Freeze a reproducible baseline, inspect current head, then rebase intentionally |
| Reference documentation is stale | `ref/README.md` claims four clones that are absent | Derive documentation from actual remote/commit checks |
| Retail assets can be committed accidentally | M0 ignore and tracked-file probes pass | Keep the M0 checks in release verification |
| Wrong-region retail media can corrupt indexed loads | The archived PAL image identifies as `SCES_021.05`; the replacement identifies as `SCUS_944.26` | Keep the import/runtime identity gate and exercise both accept/reject cases |
| Apple lifecycle/pacing changes perturb timing | macOS ARM64 clean report `ctr-215303` uses the retail 32-ms/two-VBlank step on 99.872065% of post-bootstrap frames; a 6,465-VBlank checkpoint segment measured within -0.025190% of the exact 59.817333-Hz model; exact-current iOS GLES and macOS desktop GL reports now match timing and VSync on all 24,232 regenerated frames | Preserve both full reports and measure physical-iPad wall cadence |
| Touch boost chains are ergonomically poor despite correct input injection | Three boosts require steering + drift hold + repeated taps | Device prototypes and repeated triple-boost usability criterion |
| Signing/publishing depends on credentials and external accounts | Not yet inventoried | Keep configuration credential-free; verify available team/device before release milestone |
| GPL provenance and commercial-IP exposure need legal judgment | Facts are documented but not legal advice (`docs/ctr-native-viability.md:363-400`) | Sideload-only, publish corresponding source, and obtain counsel review before public release |

## Decision log

- **2026-07-29 — Start from evidence, not the prior “wait” verdict.** The goal
  authorizes proceeding. Estimates and unknowns in the viability report will be
  replaced with build results.
- **2026-07-29 — Preserve upstream history in the downstream repository.**
  `ref/ctr-native` remains read-only. Implementation will occur on a dedicated
  downstream branch whose history incorporates upstream and the existing
  CTRPad documentation.
- **2026-07-29 — Treat the present CloneCD image as unvalidated retail input.**
  MODE2/2352 structure is proven; region/revision and loader compatibility are
  not.
- **2026-07-29 — Mac ARM64 retail parity is the iOS gate.** iOS complexity will
  not be used to debug unresolved 64-bit state corruption.
- **2026-07-31 — Preserve long replay work across user pauses.** The active
  independent-process verifier is frozen with `docker pause`, not killed or
  promoted early. At the second pause (02:44:31 CDT), playback 1's last durable
  marker remained frame 10,000; playback 2 and mutation were still unaccepted.
  Full timing and recovery details are in
  `docs/history/PROGRESS-LOG.md` and
  `docs/history/ENGINEERING-JOURNAL.md`.
- **2026-07-31 — Accept the naturally completed alternate-layout verifier.**
  Both unchanged i686 processes completed 24,232 frames under distinct host
  and raw-checkpoint layouts; the automatic frame-1,711 driver mutation exited
  through the required parity-failure route with `drivers` first. Finalize-only
  artifact verification exited 0. M1 is complete; broader M6 product evidence
  remains open.
