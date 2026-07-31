# CTRPad Running Progress Log

This is the human-readable, append-only status and elapsed-time ledger for the
CTRPad port. It answers four questions at each checkpoint:

1. What was completed?
2. What is still running or unproven?
3. How long did the work or validation take?
4. Where is the recoverable GitHub checkpoint?

The detailed reconstruction record remains
[ENGINEERING-JOURNAL.md](ENGINEERING-JOURNAL.md). That journal contains exact
commands, rejected approaches, debugger observations, hashes, and corrections.
[ROADMAP.md](../ROADMAP.md) is the current milestone source of truth. This file
is the shorter historical index and time ledger; it does not replace either
artifact.

## Time-accounting rules

- Dates and wall-clock times use `America/Chicago` unless an entry explicitly
  carries another offset.
- **Goal elapsed** is the Codex product timer as displayed or reported by the
  user. It measures elapsed task lifetime, not uninterrupted human typing or
  CPU time.
- **Process elapsed** is measured from the operating system or container. It
  can overlap agent work and other background validation.
- **CPU time** may greatly exceed wall time when a renderer uses several
  workers. It is never presented as person-hours.
- A duration is labeled `measured`, `derived`, or `user-reported`. Unknown
  durations remain unknown rather than being reconstructed from memory.
- Long-running work receives intermediate entries. An in-progress entry is not
  acceptance evidence.
- `pushed` means recoverable on a remote branch. It does not mean `merged`.

## Git publication boundary

At the 2026-07-31 01:36 CDT checkpoint:

- merged `main`: `95417c723518407d6bfe3c81a37606294963efe2`
  (`Add CTR Native iPadOS viability research`);
- pushed implementation branch: `codex/arm64-apple`;
- pushed implementation head before this documentation checkpoint:
  `e3a7e79fa4638497094deb912c1bc9c234e7714f`;
- review surface: draft PR
  [chrissotraidis/ctrpad#1](https://github.com/chrissotraidis/ctrpad/pull/1);
- PR base/head: `main` / `codex/arm64-apple`;
- merge status: open, draft, and clean;
- implementation commits merged into `main`: none.

Only the viability material is merged. Runtime source, tests, evidence, and
these progress records are backed up on the draft branch until the user
explicitly authorizes a merge.

## Running timeline

### 2026-07-29 — Evidence-first start

**Outcome**

- Read `docs/ctr-native-viability.md` in full before source changes.
- Inventoried the actual checkout and Apple Silicon host.
- Wrote `docs/ROADMAP.md` and fixed the dependency order from repository
  foundation through signed touch-first iPad delivery.
- Protected the user-owned retail media from ordinary Git staging.
- Imported the exact GPL-3.0 upstream source history without rewriting it.

**First project commit:** `c5496cbfe` at 2026-07-29 13:52:47 CDT.

**Historical detail:** engineering-journal entries “Repository inventory and
viability boundary” and “Repository and evidence foundation.”

### 2026-07-29 — Reproducible baseline, retail-input correction, and parity oracle

**Outcome**

- Built the upstream i686 baseline in a pinned Ubuntu container.
- Reproduced the intentional Apple 32-bit configure rejection.
- Rejected raw checkpoint bytes as a valid cross-process parity oracle.
- Added canonical game-visible state digests and mutation-sensitive replay
  automation.
- Diagnosed the original supplied image as PAL `SCES_021.05`, added a runtime
  region gate, and accepted the replacement NTSC-U `SCUS_944.26` raw image.
- Reached the retail boot sequence with live keyboard-to-pad input.

**Important failed route:** the PAL image was initially interpreted through
NTSC-U indexes and reached invalid VRAM data. The result was retained as an
input/build-region mismatch, not erased as an unexplained renderer crash.

**Historical detail:** engineering-journal entries from “Reproducible i686
baseline” through “Canonical parity harness.”

### 2026-07-29 to 2026-07-30 — Complete replay and 64-bit conversion

**Outcome**

- Captured the first 24,232-frame NTSC-U run with 81 rolling checkpoints.
- Found and corrected replay transport gaps rather than accepting nearly equal
  runs.
- Wrote the 64-bit layout census and guest-reference architecture decision.
- Converted runtime pointer-bearing layouts, serialized asset relocation,
  scratch storage, checkpoints, renderer links, and relevant overlay state for
  LP64.
- Preserved separate retail-shaped serialized data instead of widening on-disc
  structures.
- Reached the first native macOS ARM64 build and iterated through real startup,
  renderer, and checkpoint faults under sanitizers and cross-width replay.

**Historical detail:** `docs/architecture/`,
`docs/parity/2026-07-30-arm64-prefix-parity.md`, and the corresponding
engineering-journal entries.

### 2026-07-30 — Full ARM64 trajectory and visible renderer correction

**Outcome**

- Finalized a clean 24,232-frame ARM64 report.
- Found LP64-only memory pressure from a derived language table and moved that
  table outside the retail-pressure arena.
- Traced a striped Crash Cove surface to an LP64 mosaic-reference
  classification error and restored coherent road, dirt, grid, kart, sky, and
  HUD textures.
- Corrected AI checkpoint bounds exposed by optimized i686 playback.
- Built and strictly verified a thin, ad-hoc-signed macOS ARM64 app bundle.

**Visual boundary:** direct ARM64 observation showed coherent SCEA/Naughty Dog
boot screens, textured CTR menus, Crash Cove, and Adventure cutscenes. This
does not yet accept every effect, STR movie playback, controller path, or
prolonged manual play.

**Historical detail:**
`docs/parity/2026-07-30-arm64-full-regeneration.md` and the engineering
journal.

### 2026-07-30 to 2026-07-31 — Cross-width root-cause closure

**Outcome**

- Rejected a full i686 result with real RNG/world/allocation divergence.
- Traced the first mismatch to five potion fragments created through a
  width-sensitive emitter layout and corrected it with typed initialization.
- Traced the later mismatch to overlay-233 cutscene particle group 46 and
  corrected seven pointer-bearing union initializers.
- Rebuilt and ran 16 media-free layout/regression tests on ARM64 Release,
  ARM64 ASan/UBSan, and optimized i686.

**Historical detail:**
`docs/parity/2026-07-30-full-cross-width-result.md`.

### 2026-07-31 — First complete same-commit ARM64/i686 acceptance

**Outcome**

- The immutable optimized-i686 recording began at
  2026-07-30 21:58:11 CDT and ended at 2026-07-31 00:21:14 CDT.
- **Measured recording wall time:** 2 hours, 23 minutes, 3 seconds.
- It finalized 24,232 frames and 81 checkpoints with exit 0 and no OOM.
- All eight canonical components matched the clean ARM64 run for every frame:
  timing, RNG, drivers, world, allocation, root, pads, and VSync.
- Independent transport expansion matched pads, elapsed time, VBlank totals,
  raw blocks, and pre-/in-frame VBlank sequences for all 24,232 frames.
- Both runs produced the same checksum-valid 6,016-byte save.

**Acceptance boundary:** this accepts the full same-commit cross-width
trajectory. It does not yet accept two independent i686 layouts plus deliberate
mutation rejection.

**Historical detail:**
`docs/parity/2026-07-31-full-cross-width-acceptance.md`.

### 2026-07-31 — macOS cadence, saves, audio, and quick keyboard taps

**Outcome**

- Measured an ARM64 checkpoint segment within `-0.025190%` of the exact
  NTSC-U cadence model.
- Proved checksum-valid save persistence across a later normal launch.
- Proved a 44.1-kHz stereo CoreAudio device path, non-silent PCM, and initial
  XA sector decode.
- Diagnosed quick key-down/key-up loss between retail pad polls and added a
  one-snapshot host edge latch.
- Direct LLDB evidence showed one `C` tap produce the expected active-low pad
  packet and advance the signed app from the textured menu into Adventure.
- Release ARM64, ASan/UBSan ARM64, and optimized i686 passed 16/16 tests.

**Remaining boundary:** broad manual keyboard/controller play, STR movie sync,
full audio quality/coverage, and normal quit remain open.

**Historical detail:** the four dated reports under `docs/parity/` and the
engineering journal.

### 2026-07-31 — Active independent-process replay gate

**Status:** in progress; not accepted yet.

**Goal elapsed at user checkpoint:** `1 day, 11 hours, 48 minutes, 20 seconds`
(user-reported Codex product timer).

**Overall completion estimate at that checkpoint:** approximately 40% of the
full signed touch-first iPad objective; approximately 80% of the macOS/64-bit
foundation. These are planning estimates, not acceptance metrics.

**Active verifier**

- immutable producer source: `eee2a8df5b9605d27c7b20e943bba76174a4f6fc`;
- verifier source/pushed branch head: `e3a7e79fa4638497094deb912c1bc9c234e7714f`;
- container start: 2026-07-31 00:38:33 CDT;
- first playback mode: direct i686 loader layout;
- observed direct addresses:
  `sdata=0x403cd040`, `gGT=0x403d6bf4`,
  `mempack=0x408c8bc0`;
- recorded/restored raw checkpoint:
  `0xd4c950a8` / `0xd4c950a8`, diagnostic equality `yes`;
- canonical replay target: 24,232 frames;
- latest durable marker before this documentation edit:
  race active at frame 9,173;
- exact expected transitions observed:
  active 1,711; inactive/active 3,017/3,070; 3,920/3,973;
  4,636/4,689; 6,959/7,012; 8,107/8,160; 9,120/9,173;
- divergence, runtime failure, and OOM at the checkpoint: none observed;
- measured process elapsed at 2026-07-31 01:36:44 CDT:
  58 minutes, 11 seconds;
- accumulated multi-worker CPU time at that instant:
  5 hours, 36 minutes, 46 seconds. This is renderer CPU across concurrent
  workers, not wall time or labor time.

The redirected stdout log is block-buffered and can appear motionless between
4-KiB flushes. The runtime-owned `Crash Team Racing.log` is explicitly flushed
and was therefore selected as the authoritative progress stream. That log
proved the apparently slow 8,000-frame section was continuing and matched the
recording's frame-8,107/8,160 and frame-9,120/9,173 transitions.

After playback 1, the same immutable producer must complete all frames through
the copied i386 loader at a different address layout. The verifier then mutates
the first active-race frame and requires exit 2 with `drivers` as the first
canonical difference. No result will be promoted before all three operations
finish and their evidence is hashed.

## Current open path to the requested product

1. Finish independent-process/mutation acceptance for the existing full
   cross-width trace.
2. Close remaining macOS M6 runtime evidence, beginning with STR movie decode
   and broader renderer/input coverage.
3. Implement and validate the shared GLES 3 renderer.
4. Add the iOS/iPadOS lifecycle, sandbox, display pacing, audio, and controller
   application shell.
5. Add document-picker import and persistent sandbox storage for the user's raw
   NTSC-U image and saves.
6. Build and iterate genuinely playable, simultaneous analog touch controls.
7. Run device parity and lifecycle acceptance, produce a signed sideloadable
   iPad build, and publish GPL-3.0-complete source/install information without
   retail bytes or credentials.

This list remains deliberately broader than the current macOS gate. Passing
the current replay does not redefine the final objective as complete.
