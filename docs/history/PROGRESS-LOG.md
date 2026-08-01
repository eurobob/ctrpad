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

### 2026-07-31 — User-requested recoverable pause

**Pause request:** the user asked to pause at a natural stopping point.

**Verifier state**

- pause command: `docker pause exciting_gagarin`;
- pause observed: 2026-07-31 01:52:57 CDT;
- container state: `running=true`, `paused=true`, `exit=0`,
  `oomKilled=false`;
- container start: 2026-07-31 00:38:33 CDT;
- **derived active wall interval before pause:** approximately 1 hour,
  14 minutes, 23 seconds;
- last completed durable progress marker: frame 10,000 of 24,232;
- latest 2,000-frame rate: 1.50 FPS;
- divergence or runtime failure before pause: none observed.

The pause freezes the existing process and memory instead of discarding the
first playback's progress. The outer verifier session was still running when
paused. Resume starts with:

```sh
docker unpause exciting_gagarin
```

After unpause, verify that the original outer verifier session still owns the
container. If that host session did not survive the task pause, preserve and
inspect playback 1's normal completion before deciding whether to restart the
full orchestrated gate; do not silently label a manually continued subset as
the scripted three-operation acceptance.

**Natural code checkpoint completed before stopping**

- Added `--probe-str-scrapbook N`, which validates the normal retail asset
  source, opens `TEST.STR` through the production disc-image path, decodes
  frames without GPU/window startup, and prints deterministic RGB555 hashes.
- Source checkpoint: `1753edbc5`
  (`test: add retail scrapbook STR decode probe`).
- ARM64 Release built successfully and passed 16/16 existing CTests.
- Optimized Linux i686 built successfully and passed 16/16 existing CTests.
- Missing, zero, nonnumeric, and duplicate probe values all exited 1 with an
  actionable error.
- The first ten real retail frames were 512 by 208 on both architectures.
- Per-frame hashes matched exactly; a strict filtered-output diff exited 0.
- Shared ten-frame sequence hash: `60dcf4c65986a034`.
- First-frame RGB555 hash: `cc257394fc1aa6bf`.
- Retail files remained ignored and untracked.

This checkpoint proves that the CPU parser/decoder is consuming actual
user-owned NTSC-U scrapbook sectors deterministically across LP64 and ILP32.
It does **not** yet prove sanitizer cleanliness, on-screen VRAM upload,
presentation timing, complete movie playback, or STR audio/video
synchronization. Those boundaries remain open when work resumes.

### 2026-07-31 — Second recoverable pause after automatic goal continuation

**Pause request:** the user said they would pause the goal and asked the work
to stop at a natural boundary.

The still-preserved verifier had been automatically resumed when the active
goal continued. Read-only inspection before stopping established:

- playback 1 was still the active stage; playback 2 had not begun;
- the immutable process remained
  `ctr_native-cutscene-fix-producer-eee2a8df5b96`;
- no divergence, process exit, or OOM had been observed;
- the last durable replay marker remained frame 10,000 of 24,232;
- `playback-1.log` was 49,349 bytes and last changed at
  2026-07-31 02:41:55 CDT;
- the runtime-flushed log was 2,424 bytes and last changed at
  2026-07-31 01:44:39 CDT; and
- playback 2 and mutation therefore remain unaccepted.

At 2026-07-31 02:44:31 CDT, `docker pause exciting_gagarin` succeeded.
Immediate verification reported:

```text
running=true
paused=true
oom=false
status=paused
CPU=0.00%
memory=229.1 MiB
```

The current process, memory, alternate-loader configuration, bind-mounted
input/evidence, and retail-image mount remain intact. This is a recoverable
execution checkpoint, not a test result. Resume with:

```sh
docker unpause exciting_gagarin
```

The Codex goal API reported **1 day, 12 hours, 15 minutes, 35 seconds** of
cumulative goal time immediately before this pause. That product timer is
recorded separately from benchmark wall time and from CPU time. The goal
remains active here so the user can pause it through the product control; it
has not been marked complete or blocked.

**Repository boundary at pause:** implementation and evidence are backed up on
draft branch `codex/arm64-apple`; only viability documentation is merged into
`main`. No merge was performed.

### 2026-07-31 — Resumed verifier and proved STR pixels reach the screen

The user explicitly resumed the goal. The preserved verifier was checked
before unpause: same immutable producer, same playback-1 process, no OOM,
clean/synchronized branch, and unchanged bind-mounted evidence. It resumed at
2026-07-31 02:48:43 CDT.

At 03:01:23 CDT, the flushed runtime log emitted the frame-12,000 marker.
Playback 1 therefore continues rather than being stuck; no divergence or OOM
has appeared. Its reported 0.43 FPS includes both intentional Docker pauses and
must not be treated as active throughput. Playback 1, alternate-layout
playback 2, and mutation remain in progress/unaccepted.

The lost host terminal exposed a recoverability gap even though Docker
preserved the expensive process. Commit `69c4c9948` adds a finalize-only mode
that requires a machine-captured container exit 0 and independently rechecks
both normal completions, distinct address/raw-checkpoint layouts, mutation
selection, divergence, and `drivers` as the first difference before hashing
evidence. The current legacy container has two active `docker wait` observers;
one writes its eventual status directly to the report.

Commit `c44ea7810` adds an actual macOS presentation probe. Exact clean ARM64
results:

- headless ten-frame hash remained `60dcf4c65986a034`;
- presented ten-frame hash was `e85a9203c966c801`;
- decoded per-frame hashes matched the existing ARM64/i686 proof;
- three runs produced byte-identical frame-9 BMP SHA-256
  `e7366bc9ff8ea4054d7c45e16f0a0eb8539c3bfb0cb8b1bba1c1bc5b3f1888e3`;
- frame 9 visibly showed a correctly oriented and colored Naughty Dog
  scrapbook title;
- six malformed CLI cases exited 1; and
- exact clean ARM64 passed 16/16 CTests.

The screenshot remains in `/private/tmp` and is not published because it is
derived from the user's retail movie. Sanitizer compilation was resumed, then
intentionally interrupted after 5/131 objects so the exact clean Release
result could be produced without competing compilers. Sanitizer acceptance is
still pending and will resume incrementally.

Detailed evidence:
`docs/parity/2026-07-31-scrapbook-str-presentation.md`.

### 2026-07-31 — STR sanitizer correction and exact clean acceptance

**Outcome**

- The first unsupported Apple leak-detector run and the later Apple-framework
  HID-enumeration fault are recorded as rejected observations.
- UBSan found a real renderer issue: four OpenGL VBO offsets were expressed
  through null-member access.
- Commit `75b09db17d1cabb91f2fece68a43edbf04662992` uses defined `offsetof`
  values and compile-time-pins the 20-byte packed vertex layout.
- The exact clean Release build identifies as `75b09db17d1c`, is native ARM64,
  passes 16/16 CTests, and produces the accepted ten-frame sequence and BMP.
- The exact clean ASan/UBSan build identifies as the same commit, passes 16/16
  CTests plus both ten-frame probes, and produces a byte-identical BMP without
  a sanitizer report. The presentation-only run sets
  `SDL_JOYSTICK_HIDAPI=0` to isolate the renderer from the documented Apple
  framework fault; the normal sanitized input CTest remains enabled and
  passes.
- Retail-derived BMPs remain in `/private/tmp`; no retail file is tracked.

**Time ledger**

- sanitizer tree created: 02:39:24 CDT;
- final exact-clean sanitizer screenshot: 03:33:43 CDT;
- observed sanitizer campaign wall interval: 54 minutes, 19 seconds,
  including rejected runs, diagnosis, correction, Release validation, and
  clean rebuild;
- source-fix commit: 03:26:12 CDT;
- exact-clean Release binary: 03:27:32 CDT;
- exact-clean sanitizer binary: 03:33:27 CDT;
- exact-clean fix-to-final-evidence interval: 7 minutes, 31 seconds; and
- Codex goal elapsed at the 03:34 checkpoint: 1 day, 13 hours, 5 minutes,
  15 seconds, reported by the goal API and not treated as labor time.

**Long verifier status at the same checkpoint**

The preserved direct-loader i686 process is still running, unpaused, and not
OOM-killed. Its flushed runtime log has now passed the frame-14,000 marker and
observed the expected inactive/active transitions at frames 13,765/14,291.
Playback 1 has not yet finished; alternate-layout playback 2 and deliberate
mutation have not begun. This is progress evidence, not acceptance.

**Git publication boundary**

The source correction was pushed to `codex/arm64-apple` at `75b09db17`; local
and remote branch tips matched immediately after the push. Draft PR #1 remains
open and unmerged. `main` still contains only the viability boundary.

### 2026-07-31 — Complete 4,424-frame scrapbook coverage

**Outcome**

- Exact clean functional source `75b09db17d1c` decoded all 4,424 retail
  scrapbook frames in both Release and ASan/UBSan.
- Every frame was 512 by 208; frame and source indices were contiguous from 0
  through 4,423.
- Both builds produced decoded sequence `4b193011f608bc90` and identical
  per-frame record manifest
  `6f70283316cf03bc786e3b96847cf04ace34a12dea33a5e3d6a630f7cb63c7e5`.
- Both builds then uploaded, presented, and read back all 4,424 frames. They
  produced sequence `e7e81ffeedaa7b2c`, identical per-frame record manifest
  `96d3254f21c70e004e2319886ef72080c5ca3d413f920c4e04a591238e1f480e`,
  and byte-identical final BMP.
- No sanitizer report appeared. The sanitizer presentation run used the
  already documented renderer-only HID isolation.
- The final frame is black movie tail and is not substituted for the
  previously inspected coherent frame-9 visual.

**Measured process wall time**

- Release full decode: 61.35 seconds;
- sanitizer full decode: 68.69 seconds;
- Release full presentation: 64.85 seconds;
- sanitizer full presentation: 78.75 seconds; and
- four serial low-priority commands combined: 273.64 seconds.

The commands ran at nice level 15 while the i686 gate continued. At the 03:44
checkpoint, Docker still reported running, unpaused, and no OOM; its latest
durable marker remained frame 14,000. Codex goal elapsed was
1 day, 13 hours, 15 minutes, 21 seconds. Real-menu 15-fps cadence, interleaved
STR XA synchronization, and menu entry/skip/teardown remain unaccepted.

### 2026-07-31 — Real-menu Scrapbook playback and teardown

**Outcome**

- Added `tools/prepare-scrapbook-test-save.mjs`, which refuses in-place edits
  and existing outputs, enables only Scrapbook bit 36 in a separate native
  save, regenerates the retail CRC, and validates its own output.
- The accepted source save remained byte-identical at SHA-256
  `6a01b0f5562ed7a279d8f8e51e3b1874ac39a6120f55db4fe3873288950619a3`.
  The ignored disposable output is checksum-valid at
  `468c43b5b58b4ebe2eb6a207b166d02c36011b27e50b76e207d6009fca5521a4`.
- Added diagnostic-only real-state-machine telemetry for start, exit reason,
  uploaded-frame count, and XA lifetime.
- Source commit `63b0a0773a00afb12e3fce9152ece4affbcfda68` was pushed to
  `codex/arm64-apple` before the exact evidence run.
- The exact app is a strict-verifiable signed ARM64 bundle, embeds build ID
  `63b0a0773a00`, and passes 16/16 CTests.
- Normal startup loaded the test card and visibly exposed the production
  seven-row menu. Ordinary mapped input selected `SCRAPBOOK`, entered the real
  movie player, showed multiple distinct coherent retail frames, accepted a
  Start skip, ran the title transition, and returned to the intact menu.
- The frozen log proves XA was active on channel 1 at start and after 674
  uploaded frames, that the exit reason was `input-skip`, and that teardown
  changed XA from active to inactive.

The real menu route, selected four-vblank/15-fps scheduler, XA lifetime, input
skip, and teardown are accepted. A screenshot cannot establish perceptual A/V
synchronization, so independently measured or listened interleaved sync and a
full natural-end menu run remain open. Retail screenshots, the test save, and
the runtime log remain ignored/untracked; the detailed evidence record stores
only descriptions and hashes.

**Time ledger**

- helper file written: 03:55:38 CDT;
- disposable checksum-valid output written: 03:56:35 CDT;
- source commit: 04:02:11 CDT;
- exact committed app binary: 04:02:57 CDT;
- inspected evidence captures: 04:03:38 through 04:05:12 CDT;
- evidence run closed: 04:06:45 CDT;
- measured helper-to-close interval: 11 minutes, 7 seconds;
- measured commit-to-close interval: 4 minutes, 34 seconds; and
- Codex goal elapsed at the 04:06 checkpoint: 1 day, 13 hours, 37 minutes,
  56 seconds. This product timer is not labor or benchmark time.

**Concurrent verifier status**

The preserved i686 direct-loader playback remains running, unpaused, and not
OOM-killed. Its flushed log now contains nine 2,000-frame FPS markers, making
frame 18,000 the latest durable progress boundary. The redirected playback log
was 82,117 bytes and changed at 04:05:48 CDT. The machine-captured exit-status
file remains empty, so playback 1, alternate-layout playback 2, and mutation
are still unaccepted.

### 2026-07-31 — Natural-end Scrapbook cadence and authored tail

**Outcome**

- Exact source `63b0a0773a00` completed all 4,424 production Scrapbook
  uploads without input skip and returned to the intact seven-row menu.
- The observed start-to-return interval was about 296 seconds against the
  294.93-second 15-FPS content model, and the runtime reported 14.95 FPS over
  its steady 2,000-frame window.
- That run's frozen log SHA-256 is
  `384f0f6b5b0d6a1df56b5114e1794fc112e1c462c6dfabf21e684c2e5bd15a5f`.
- Observation-only commit `900f5656b41d` repeated the complete natural run.
  Video ended at exactly 17,696 elapsed VBlanks (`4424 * 4`); XA exhausted at
  upload count 4,371 and 17,480 elapsed VBlanks.
- The 53 later uploaded frames are not unexplained drift: the already frozen
  all-frame manifests show 23 changing fade-out frames followed by 30
  identical black frames.
- The second live log and temporary captures were removed by normal app close
  before a post-close hash. The exact pre-close telemetry is documented
  without inventing an artifact hash.

Natural stream end, configured cadence, teardown, menu return, and measured
XA alignment to the authored silent tail are accepted. Human-perceived
listening quality/synchronization remains open.

### 2026-07-31 — Practical desktop keyboard test controls

**Outcome**

- Commit `2c10b00b34df4f0eb61aa8b72cbe99588a930ed6` adds `WASD`,
  `IJKL`, `Q/E`, `P`, and Tab aliases while preserving arrows, `Z/X/C/V`,
  Shift/Ctrl, brackets, Space, and Return.
- The aliases use the existing keyboard-to-active-low-PS1 mapper; replay
  schema, pad packets, and gameplay physics are unchanged.
- The media-free input test checks all 12 aliases, a held
  accelerate+right+R1 combination (`K+D+E`), and a latched `K+D` tap.
- The exact app identifies as `2c10b00b34df`, is thin ARM64, has executable
  SHA-256
  `6e3171d1619fbc34ee1985679604a018107ac62039e6de5d8489b1729224f9e9`,
  passes 16/16 CTests, and passes strict signature and plist validation.
- Normal-startup visual verification used `K` to advance into the real menu,
  `S` to move Adventure to Time Trial, and `W` to move back. The app closed
  normally.
- The source commit was pushed to `codex/arm64-apple` before this documentation
  update. Draft PR #1 remains open and unmerged.

**Time and concurrent work**

The source commit was created at 04:39:42 CDT, the exact executable was
written at 04:40:48 CDT, and visible verification completed by 04:43:44 CDT.
The goal API reported cumulative elapsed 1 day, 14 hours, 8 minutes,
23 seconds during the runtime checkpoint and 1 day, 14 hours, 17 minutes,
32 seconds immediately before this documentation freeze.

The long i686 verifier remains running, unpaused, and not OOM-killed.
Playback 1 has durable FPS markers through frame 22,000 and a driver-inactive
transition at frame 21,331. The process has not emitted the 24,232-frame
completion line, its machine-captured exit-status file is still empty, and
playback 2/mutation remain unaccepted.

### 2026-07-31 — Controller ownership and virtual hotplug coverage

**Outcome at the 05:18 CDT checkpoint**

- Read-only input review found that controller mappings were queried and
  preserved but never claimed on successful SDL open or released on close.
  A duplicate add could open one device twice, and removal could leave a
  ghost handle.
- Commit `2f9bf4eaedd1` claims the resolved joystick instance ID only after
  open succeeds and clears the slot to `-1` on close.
- The media-free input test attaches an SDL virtual standardized gamepad and
  verifies buttons, four analog bytes, exact rumble magnitudes, duplicate-add
  suppression, removal, and same-slot reconnect through the production path.
- The exact signed ARM64 app and combined ASan/UBSan build both pass all
  16 tests. The targeted sanitizer input test passed in 1.27 seconds and the
  complete sanitizer suite in 2.24 seconds with no finding.
- The exact app embeds `2f9bf4eaedd1`, is a valid signed thin-ARM64 bundle,
  and has executable SHA-256
  `0f23ce4c8c8770caddda4c32d28e84ad6fd0c614c20514dea343cb31ca0967c1`.
- The source checkpoint was pushed to `origin/codex/arm64-apple`; draft PR #1
  remains open and unmerged.

The first disposable i686 compile exposed two new signed/unsigned warnings in
test-only comparisons against SDL's unsigned joystick ID. They were corrected
in local follow-up commit `764205d4c`; final cross-width rebuilding is still
underway and is not yet claimed here. CMake configured the disposable i686
tree in 590.4 seconds from a read-only source mount and confirmed SDL's
virtual-joystick backend plus a 32-bit target.

The preserved parity verifier independently completed unchanged direct-loader
playback 1 at all 24,232 frames and entered alternate-loader playback 2.
Playback 2 crossed its first durable 2,000-frame marker at 2.26 FPS, with
driver 0 active at frame 1,711. Docker still reported running, unpaused, and
not OOM-killed. Playback 2, its captured container exit, and the deliberate
mutation remain unaccepted.

The goal API reported cumulative elapsed 1 day, 14 hours, 48 minutes,
56 seconds at 05:18:22 CDT. This product timer is recorded for transparency;
it is not a labor estimate or benchmark.

**Final cross-width result at 05:36 CDT**

- Clean synchronized tip `359e8d5a0` rebuilt and passed 16/16 on the signed
  ARM64 app, combined ASan/UBSan ARM64, and optimized Linux i686.
- Final hashes are signed app
  `c97953dddfe682962732aea7a2d2e8ebde6f8083a2add1eb7ef47388924ae446`,
  sanitizer
  `ec4d49d5ca1180dc2711bfd7353d58d84e9e142a724e97947940d14dad12adfd`,
  and i686
  `d21c04bd129399a28a0e983fd26d187f5c98adcc1506983caf18a029992bfd10`.
- The exact i686 compile eliminated both new ID-comparison warnings and
  retained only four established warnings. Its ELF32 Intel 80386 executable
  embeds the clean tip and GNU Build ID
  `e71bd4d9b99bf1efc5214a6d4c250ca22621ef96`.
- A read-only-output CTest attempt exited 8 before tests because it could not
  write `LastTest.log`; it is rejected as an invocation error. The corrected
  disposable-output invocation passed all 16 in 3.51 seconds.

The preserved alternate-loader verifier independently crossed its second
durable FPS marker at frame 4,000 and observed the expected driver transitions
at frames 3,017/3,070 and 3,920/3,973. It remains running, unpaused, and not
OOM-killed; its machine-captured status file remains empty, so completion and
mutation are still unaccepted. Goal elapsed at 05:36:44 CDT was 1 day,
15 hours, 7 minutes, 35 seconds.

### 2026-07-31 — Live keyboard Time Trial movement

- Exact signed app `359e8d5a0f07` launched through normal retail startup and
  visibly rendered SCEA/Naughty Dog presentation, the seven-row menu, Time
  Trial setup, and coherent Crash Cove race geometry, kart, HUD, and minimap.
- An initial single-tap route fell into the attract sequence and is rejected;
  the corrected bounded `S`/`L` route entered Time Trial and selected Crash.
- Sparse automation taps did not maintain held acceleration and left the kart
  at the line; that attempt is also rejected.
- Dense ordinary `K` input moved the kart from the start, and dense `K+D`
  input changed its heading/location. A later sequential `K+D+E` stream moved
  it farther, but cannot prove that all three keys overlapped in one poll.
- The final local-only screenshot is 139,137 bytes with SHA-256
  `e6e97000036efe26b162cba3022d3a6e3842656c2c63a488b322b3dc532f0644`.
  It is not tracked because it contains retail-derived pixels.
- The app closed normally and the OS reported it no longer running.

Basic live keyboard menu navigation, acceleration, and steering are accepted.
The deterministic self-test remains the simultaneous `K+D+E` proof; a
human-held powerslide, clean lap, and complete race remain open. Goal elapsed
was 1 day, 15 hours, 17 minutes, 19 seconds at 05:46:33 CDT. The concurrent
alternate-loader verifier remained healthy and had crossed driver transitions
at frames 4,636/4,689, but not yet its 6,000-frame marker.

### 2026-07-31 — Locked a cross-width SPU mixer and Room-reverb oracle

- Added `--self-test-audio-mixer` and a seventeenth CTest with no retail-media
  dependency.
- The test uploads a synthetic 16-byte PS1 ADPCM block through the production
  SPU API, keys the real streaming decoder, and renders a left-panned looping
  voice. It then renders a centered one-shot voice through the Room reverb and
  requires wet output after the voice becomes inactive.
- The first exploratory matrix printed the values without accepting them as
  constants. Ordinary ARM64, ASan/UBSan ARM64, and optimized i686 agreed, so
  the final gate now requires dry digest `132e19d77167fb3d`, wet digest
  `4bdedc91d1293ad8`, and exactly 6,905 wet-tail frames.
- Exact clean commit `87f8e7a052c2` passes 17/17 on all three targets. The
  signed app passes strict signature/plist checks and is thin ARM64. ASan and
  UBSan report no finding. The optimized i686 binary is ELF32 Intel 80386.
- Exact executable hashes are ARM64 app
  `85c03b1a557ad379a869ded940b1ed37879c918d96401a7c6f860d99e7611d93`,
  sanitizer
  `5382e363bd6c9872674d3a65077c47e7f23dbaf4e96a1f01e82f6b4ff1601dc6`,
  and i686
  `abc019636ca9e8f5081584b7fcc12b36b918299148df7118bc2bd0e07394ef48`.
- Implementation commit `87f8e7a05` and documentation commit `9ed75956c`
  were pushed; local HEAD, `origin/codex/arm64-apple`, and the repository-pinned
  draft PR head all matched `9ed75956cf7078fa759bbb8189ac7f5c20fc5e94`.
- This accepts deterministic SPU voice decode, panning, one-shot stop, and the
  Room wet tail. Subjective listening, representative retail mixes, other
  presets, broad XA transitions, long soak, and iOS audio remain open.

The audit ran approximately 05:50–06:24 CDT. Goal elapsed advanced from
141,675 to 143,697 seconds and stood at 1 day, 15 hours, 54 minutes,
57 seconds at the checkpoint. The preserved alternate-layout verifier crossed
its frame-6,000 and frame-8,000 markers and remained running, unpaused, and
not OOM-killed. Its status file was still empty; completion and mutation were
not accepted.

### 2026-07-31 — Landed the first shared GLES 3 renderer slice

- Audited the populated `ref/ctr-native-android` branch and ported only the
  load-bearing ES profile, GLSL ES 300, SDL proc-loader and desktop-feature
  guard concepts. Existing packed RG8 VRAM was already suitable and was not
  replaced.
- Commits `4695d9cb3` and `78ef952db` add selectable desktop/GLES production
  dialects, move profile selection before SDL window creation, validate the
  ES 3 entry-point contract, and make pre-context shutdown idempotent.
- The first exact macOS GLES launch exposed a post-failure exit 139. The final
  exact build reports the missing Cocoa ANGLE/EGL runtime and exits cleanly
  with status 1. CTest 14 now exercises two shutdowns before renderer init.
- Exact clean `78ef952dbecc` passed 18/18 ordinary ARM64 tests in 0.70 seconds,
  18/18 GLES-configured ARM64 tests in 0.70 seconds, 18/18 combined
  ASan/UBSan tests in 5.00 seconds with no finding, and 18/18 optimized i686
  tests in 2.98 seconds.
- A thin ARM64 iOS Simulator executable linked UIKit and OpenGLES and embeds
  GLSL ES 300 plus the SDL resolver. Its empty bundle metadata and unbound
  ad-hoc linker signature keep install/launch/device acceptance open.
- The exact desktop renderer still reported Apple M2 / OpenGL 4.1, compiled
  all PSX shaders and VRAM pipelines, and opened CoreAudio. A local visual
  check showed coherent presentation textures; no retail-derived screenshot
  was tracked.
- Signed desktop, macOS-GLES, iOS Simulator and sanitizer hashes are recorded
  in `docs/parity/2026-07-31-shared-gles3-dialect-bringup.md`, along with the
  rejected leak-detector, Docker-platform, asset-path and bundle-symlink
  attempts.

The checkpoint ran approximately 06:25–07:08 CDT. Goal elapsed advanced from
144,159 to 146,335 active seconds and stood at 1 day, 16 hours, 38 minutes,
55 seconds. The preserved alternate-loader verifier remained running,
unpaused and not OOM-killed; it crossed frame 10,000, but its machine status
file remained empty, so completion and mutation were not accepted.

### 2026-07-31 — First live iPad Simulator GLES presentation

- Commit `98ae2c6d86fe` converts the earlier iOS compile/link probe into
  CMake-generated ARM64 Simulator and device app bundles. It adds
  credential-free iPhone/iPad metadata, iOS 15.0 presets, landscape
  declarations and SDL's UIKit-owned entry point.
- The first manually patched temporary bundle is rejected as reproducibility
  evidence. Its diagnostics nevertheless exposed that `SDL_MAIN_HANDLED`
  prevented SDL's iOS main initialization, which the committed entry change
  corrects.
- After startup, the complete retail draw path and `glDrawArrays` ran against
  a black screen. A point-to-pixel sizing change was necessary but did not fix
  presentation and is rejected as the black-frame root cause.
- SDL/UIKit owned framebuffer 1 and renderbuffer 1 while the desktop-derived
  renderer rebound framebuffer 0. Querying SDL's UIKit window properties,
  retaining that presentation FBO and rebinding its RBO before swap produced
  the first live GLES pixels.
- The first visible run was clipped because Simulator hardware remained in
  portrait despite the landscape app declarations. Rotating the simulated
  device to Landscape Right yielded a full 1376-by-1032 landscape surface.
- Every producer was rebuilt from exact clean source `98ae2c6d86fe`. The
  exact Simulator run reported UIKit framebuffer/renderbuffer 1, Apple
  Software Renderer, OpenGL ES 3.0, GLSL ES 3.00, four ready PSX shaders,
  ready VRAM pipelines and 44.1-kHz stereo CoreAudio. Visual inspection showed
  coherent Crash/trophy/logo/menu geometry, colors, text and textures.
- The local-only title-menu JPEG is 199,375 bytes, 932 by 768, and has SHA-256
  `77916c2f69de6ef39432006a9aa3f8ca778aa20f261ff764576bd29be30a8a12`.
  It was not tracked because it contains retail-derived pixels. The temporary
  test package and retail copy also remained outside Git.
- Exact clean 18/18 results are macOS desktop GL in 3.58 seconds, macOS GLES
  configuration in 1.60 seconds, and ASan/UBSan in 10.82 seconds with no
  finding. The optimized Linux i686 build also passed 18/18 in 4.23 seconds;
  it is ELF32 Intel 80386 with GNU Build ID
  `dfac03fc776068dfd25ee53f0284975b1d914217` and SHA-256
  `4bcc7844e9cd107ea0ddc67e734397a0df420d6b635843454975289742c21611`.
  The macOS GLES binary still exits cleanly with status 1 because this host
  lacks Cocoa ANGLE/EGL. Other exact hashes are in the parity report.
- The exact Simulator and device binaries are thin ARM64 with iOS 15.0 floors
  and SHA-256 values `9e09fb41...b1709771` and
  `09576e97...f1d3f43`. The Simulator package was ad-hoc signed locally only;
  the physical-device product is unsigned and untested on hardware.
- Simulator keyboard capture was visibly active while `C` was sent during the
  exact clean intro, but the intro could have ended naturally before the next
  observation. An earlier pre-commit run advanced immediately from the title
  menu into Adventure with `C`; repeated exact-run menu taps were inconsistent,
  so iPad keyboard delivery/navigation is not accepted. Basic keyboard support
  itself remains accepted through commit `2c10b00b3`, its deterministic
  PSX-packet test and the live macOS movement evidence.
- Two SDL/UIKit appearance-transition warnings keep background/resume,
  rotation and view lifecycle open. Display-driven pacing, controller input,
  physical-device signing/execution, sandbox import/saves and touch controls
  are also still open; neither M7 nor M8 is marked accepted.

This slice began at the prior 146,335-second goal checkpoint. The first
documentation reading was 150,428 seconds. At 08:23:51 CDT, the pre-commit
reading was 150,887 seconds, or 1 day, 17 hours, 54 minutes, 47 seconds
cumulative: an interval of 4,552 seconds (1 hour, 15 minutes, 52 seconds).
The timer is product-task elapsed time, not a labor estimate or benchmark.

The protected alternate-loader i686 verifier was not restarted, paused,
rebuilt or terminated. Docker still reported running, unpaused and not
OOM-killed, and its machine status file remained empty. Completion, alternate
layout and deliberate mutation remain unaccepted.

### 2026-07-31 — Cooperative UIKit lifecycle and display-loop checkpoint

- Started from clean source `cbdd58435173`; the user's NTSC-U BIN remained in
  ignored `ref/CTR/` and was copied only into disposable local test packages.
- Source inspection established that SDL mobile lifecycle notifications are
  delivered synchronously to event watches rather than queued for the retail
  poll loop. The app now installs an idempotent lifecycle reducer at that
  boundary.
- Split one native retail loop iteration into `CTR_MainStep`. Desktop retains
  its loop; iOS schedules one step with SDL's UIKit animation callback and
  returns from standard `main`, allowing UIKit to regain control between
  retail frames.
- Backgrounding now pauses audio, clears stale queued PCM, clears transient
  keyboard/name-entry transport and publishes released PSX pad packets.
  Foregrounding rebases only the host VBlank deadline, preserves the
  game-visible count, and resumes audio. SDL quit/window-close paths are now
  cooperative.
- A first pacing correction removed the project's explicit 200-us iOS spin.
  A subsequent source audit found a second final sub-millisecond spin inside
  `SDL_DelayPrecise`, so iOS now uses the fully yielding `SDL_DelayNS` path.
- CTest 19 covers paired, duplicate and direct background/foreground events,
  low memory, termination, audio pairing, cooperative quit and VBlank rebase.
  Exact desktop GL, GLES-configured and ASan/UBSan ARM64 runs passed 19/19;
  sanitizers reported no finding. The exact optimized i686 producer also
  passed 19/19 in 10.34 seconds and emitted an ELF32 Intel 80386 binary with
  GNU Build ID `d032b695e7957142bc16a754a8af8a2946dab2b5` and SHA-256
  `5f1f8b06ceacbd4d4bd80e2c0e62f056faaddac0e1d81a48c67f6c9d80abdc65`.
- An exact clean, locally ad-hoc-signed iPad Simulator package identified as
  `afb5463cc511` completed two full Home/background/foreground cycles. Both
  restored CoreAudio and coherent animated retail textures without a black or
  stale frame. The process was later stopped with `simctl`, so natural
  termination is not claimed.
- Exact ARM64 product hashes, lifecycle-log hash and package details are in
  `docs/parity/2026-07-31-ios-lifecycle-display-loop.md`. No package,
  screenshot, disc byte, memory card or raw media output entered Git.
- Simulator cadence remains rejected. Five short windows ranged from 7.04 to
  9.56 FPS. A 324-complete-frame diagnostic averaged 119.983 ms per frame and
  attributed 105.157 ms to renderer triangle submission on Apple's software
  GLES renderer. The partial 325th row and absent shutdown summary were
  excluded.
- LLDB attach and two `sample` attempts stalled without usable reports; they
  were terminated and support no claim. Two recurring unbalanced UIKit
  appearance-transition warnings also remain open.
- The source checkpoint was committed as `afb5463cc511` and pushed to
  `origin/codex/arm64-apple`; the draft pull request remains the publication
  boundary and is not merged.
- A follow-up keyboard audit confirmed that the requested basic test controls
  were already present in commit `2c10b00b34df`: `WASD`, `IJKL`, `Q/E`, `P`
  and Tab are additive aliases for D-pad, face buttons, shoulders, Start and
  Select. The current input self-test requires all 12 aliases, one-snapshot
  quick taps and a simultaneous `K+D+E` chord, while the prior exact signed
  macOS run visibly proved menu navigation, acceleration and steering. The
  current `afb5463cc511` ARM64 app repeated the focused CTest successfully:
  1/1 in 1.44 seconds, with the complete alias/tap/gamepad marker.

The previous documented reading was 150,887 goal seconds. Exact lifecycle
validation was recorded at 154,273 seconds: 1 day, 18 hours, 51 minutes,
13 seconds cumulative and an interval of 3,386 seconds (56 minutes,
26 seconds). This is cumulative task time, not a performance or labor metric.
The final pre-publication keyboard, documentation and i686 audit reading was
155,756 seconds (1 day, 19 hours, 15 minutes, 56 seconds), adding 1,483 seconds
(24 minutes, 43 seconds) and making the full checkpoint interval 4,869 seconds
(1 hour, 21 minutes, 9 seconds).
The protected historical i686 verifier remained running, unpaused and not
OOM-killed with an empty machine status file; its independent completion and
mutation gates remain unaccepted.

## Current open path to the requested product

1. Close remaining macOS M6 runtime evidence, including broader audio
   listening, renderer, controller, and full-race manual-play coverage.
2. Continue live GLES 3 runtime, frame-capture, parity, and cadence validation
   from the landed shared renderer dialect.
3. Continue the landed iOS/iPadOS lifecycle/display loop with rotation,
   physical-hardware cadence, sandbox audio/saves, and controller acceptance.
4. Complete physical-device Files/import coverage and persistent sandbox
   storage for game-driven iOS saves.
5. Build and iterate genuinely playable, simultaneous analog touch controls.
6. Run device parity and lifecycle acceptance, produce a signed sideloadable
   iPad build, and publish GPL-3.0-complete source/install information without
   retail bytes or credentials.

This list remains deliberately broader than the current macOS gate. Passing
the current replay does not redefine the final objective as complete.

### 2026-07-31 — iOS sandbox storage and Documents-only retail startup

- Audited every ordinary asset, log, memory-card, performance, replay,
  savestate and screenshot path before editing. The existing process-wide
  `chdir` made relative writers follow the asset base, which would point at the
  read-only installed bundle on iOS.
- Commit `02a6623f80a0` adds one storage owner. iOS now prefers
  `Documents/CTRPad/assets`, places private state under
  `Library/Application Support/chrissotraidis/CTRPad`, and uses the bundle only
  as an immutable asset fallback. Desktop retains its portable beside-assets
  behavior.
- The generated iOS metadata now enables Files sharing and opening documents
  in place. This creates a Files-visible import boundary; it does not yet add
  the fresh-install document picker or validation/error UI required for M9.
- CTest 16 is a retail-free sandbox/portable path contract. Exact pushed
  commit `02a6623f80a0` passed 20/20 on the signed macOS ARM64 app and the
  macOS GLES-selected build. Combined ASan/UBSan also passed 20/20 with
  fail-fast options and no finding.
- A first dirty Simulator run proved private directory/log creation and
  bundle fallback. An attempted incremental replacement with a clean package
  retained the old bundle asset, so it was rejected as Documents-only
  evidence instead of being reported as success.
- The corrected fresh 3.4 MB package contained no asset directory. With the
  ignored 605,698,800-byte retail BIN only in Documents, it selected the
  Documents base, initialized GLES 3, all PSX/VRAM pipelines and CoreAudio,
  and rendered coherent textured Naughty Dog crate pixels.
- The exact committed repeat installed a locally signed 3.4 MB package with
  no bundled media. Simulator migrated the Documents file through the app
  update without changing its size or inode, build `02a6623f80a0` launched
  from that file, and its private Application Support log captured three FPS
  windows. This is import-file update persistence, not yet a game-save
  persistence or physical-device claim.
- Exact hashes are macOS app `f6a3bf25...fdeaa282`, macOS GLES
  `b4676a7a...997f5b0`, ASan/UBSan `37aeba39...580d262`, Simulator pre-sign
  `806c1ec7...866e10f`, and unsigned device `d4d083b8...137eb18`. Complete
  values and the local exact package/log/screenshot hashes are in
  `docs/parity/2026-07-31-ios-sandbox-storage.md`.
- The recurring UIKit appearance-transition warnings and rotated raw
  Simulator capture remain open. Simulator cadence remains roughly 7–9 FPS on
  Apple Software Renderer and is not accepted as device performance.
- Implementation commit `02a6623f8` was pushed at 10:06 CDT; local HEAD and
  `origin/codex/arm64-apple` matched immediately afterward. The user's retail
  files remained ignored, and no package, retail byte, save, log or screenshot
  was staged.
- A separate exact optimized i686 producer was started from a read-only source
  mount. At this intermediate record it was still compiling and therefore was
  not yet counted as a pass. The protected historical alternate-loader
  verifier was left running, unpaused and unrestarted.
- The exact i686 producer subsequently passed 20/20 in 3.85 seconds. It is an
  ELF32 Intel 80386 PIE, embeds `02a6623f80a0`, has GNU Build ID
  `e3fab55f8a436052e856dd313a72e08ef78f84b5`, SHA-256
  `96158b047af41542bbe1797e1c16d4de5ecc0c51b358ba02ca06a5780b5bdd33`,
  and repeated only four established warnings.
- Goal elapsed advanced from the prior 155,756-second checkpoint to 158,249
  seconds at 10:26 CDT: 1 day, 19 hours, 57 minutes, 29 seconds cumulative,
  adding 2,493 seconds (41 minutes, 33 seconds). The protected verifier still
  had an empty status file after ten 2,000-frame playback-2 windows; no
  completion or mutation result was inferred.

### 2026-07-31 — Fresh-install iOS Files import

- Commit `7872f7e61ad6` splits iOS media selection from runtime startup and adds
  an ARC UIKit onboarding/document-picker bridge. A missing asset returns from
  `SDL_main` without blocking UIKit; successful validation enters the ordinary
  game startup path in the same process.
- Files selections use coordinated security-scoped reading and a unique
  same-volume staging directory. The production disc/asset loaders distinguish
  unreadable raw media, wrong region and missing required content. Only a fully
  valid stage moves/replaces `Documents/CTRPad/assets/ctr-u.bin`; all failures
  preserve an existing import and remove staging residue.
- A fresh iPad Pro 13-inch (M5) iOS 26.5 Simulator showed the native no-media
  screen and real Files picker. Cancel returned to the ready state. A 118-byte
  local fixture received the raw MODE2/2352 error, installed nothing and left
  no staging directory. A no-media cold relaunch returned to the chooser.
- The user's ignored 605,698,800-byte NTSC-U source was selected through Files.
  Its source/destination SHA-256 matched
  `f780bf2331476aabfc00772fa758b12dd95ebfbc907968132cbd3cdd4e2c07c0`.
  PID `93200` persisted into the rendered game and a later cold launch used the
  installed Documents image directly.
- The implementation was committed and pushed before exact validation. An
  exact locally signed Simulator app embedding `7872f7e61ad6` repeated the
  media-free picker/import path. PID `99595` remained continuous into the game,
  the installed image retained its size/hash, and no stage leaked.
- The exact local-only screenshot visibly contains coherent checkered-flag
  geometry/textures. It is 2064 by 2752 pixels with SHA-256
  `e83b12b1551a9f5e62915f6ed4e401433da0070fe11c1e556cfc609040be36d9`;
  its portrait raw capture is not promoted to physical rotation acceptance.
- Exact hashes are macOS ARM64 app `f488dc74...53b7e`, ASan/UBSan
  `c28e83e9...36cb`, signed Simulator executable `44e66cfc...209a`, and
  unsigned/unrun device executable `6350c11c...eff`. The isolated optimized
  i686 producer passed 20/20 in 4.04 seconds, is ELF32 Intel 80386 with GNU
  Build ID `063a0ff1...a6c7c`, and has SHA-256 `361d313e...bab12`. macOS and
  sanitizer each passed 20/20; codesign/plist/architecture/platform checks
  passed as applicable.
- Recurring Simulator scene/assets/full-screen, appearance-transition and
  WebCore/WebKit warnings remain documented. Simulator software-renderer
  cadence remains about 5–9 FPS and is not a hardware performance claim.
- No retail byte, fixture, app package, screenshot, log, memory card or staging
  output entered Git. Physical Files/signing, live wrong-region/incomplete/
  inaccessible/interrupted import, iOS saves, device play and touch remain
  open. Complete evidence is in
  `docs/parity/2026-07-31-ios-files-import.md`.

### 2026-07-31 — Historical alternate-layout verifier completed

- The protected verifier completed naturally at 12:05:03 CDT. Its `--rm`
  container `ec58fcd7069c` disappeared only after completion; no pause, stop,
  restart, rebuild or termination was issued in this slice. Its previously
  empty machine-owned status file became the two bytes `0\n`.
- Direct and copied-loader i686 processes each completed all 24,232 unchanged
  replay frames. Their host samples differed (`sdata=0x403cd040` versus
  `0x3efaf040`) and their restored raw checkpoint checksums differed
  (`0xd4c950a8` versus `0x46478f61`), proving the intended address/layout
  separation while canonical playback remained equal.
- The automatically selected first-active-driver mutation at replay frame
  1,711 changed `posCurr.x` by one, exited through parity status 2, and named
  `drivers mask=0x00000004` as the first canonical difference. Pads, VBlank,
  timing, RNG, world and allocation still matched at that frame.
- A clean detached `7872f7e61ad6` worktree ran finalize-only verification with
  coverage disabled because the manual coverage form is separately accepted.
  It launched no game process, rechecked both completions, source/binary
  identity, raw/address separation and mutation semantics, then exited 0 with
  `Replay process-determinism and mutation verification passed.`
- Producer SHA-256 is `d2e6f060...276c8e`; alternate loader
  `eccfafa9...4278d`; playback logs `26a80193...9249` and
  `a579d856...f728`; mutation log `affa27f8...c8c`; final evidence-manifest
  SHA-256 `19fce285...fdcdf`.
- This closes M1's independent-process/mutation gate. It does not close the
  broader M6 physical-controller, complete-race, human-audio, renderer,
  savestate or natural-quit boundaries.
- The prior documented timer was 158,249 seconds. The final documentation
  reading at 12:16 CDT was 164,821 seconds: 1 day, 21 hours, 47 minutes,
  1 second cumulative, adding 6,572 seconds (1 hour, 49 minutes, 32 seconds).
  This is the product's cumulative goal timer, not benchmark or labor time.

### 2026-07-31 — Atomic saves and clean iOS game-driven persistence run

- Commit `4b078065ff03b71e02ce7b8a5b03351a777e2830` replaces direct
  final-save truncation with a hidden same-directory temporary write, durable
  platform flush, close, and atomic replacement. All failure routes remove
  temporary residue and preserve an existing final save.
- New CTest 17 proves initial write, replacement, readback, no temporary leak,
  injected open failure, preservation of the previous payload, and successful
  retry. Clean macOS ARM64 and ASan/UBSan pass 21/21; exact i686 writer flags
  compile cleanly; iOS Simulator and device ARM64 products link.
- The implementation was committed and pushed immediately. Local HEAD and
  `origin/codex/arm64-apple` matched at `4b078065ff03`; no retail media, app,
  save, report, screenshot, or crash artifact entered Git.
- The exact signed Simulator executable has SHA-256
  `848c18d5692634413b47b553f5fb2e9887dba12ce8bf8e1474b7ca87684568ac`
  and visibly renders coherent crate, title/menu and attract-mode textures
  from the retained Documents import.
- The requested practical keyboard controls remain published at
  `2c10b00b34df`: `WASD`, `IJKL`, `Q/E`, `P`, and Tab. macOS automation and
  live menu/race movement are accepted. Simulator hardware-keyboard options
  were enabled, but temporary production-boundary logging showed Computer Use
  key injection delivered zero SDL events; physical iPad keyboard acceptance
  therefore remains open.
- A checkpoint-74 shortcut was rejected because it used
  `--replay-bypass-header` across different executable identities. It crashed
  before any save at `VehBirth_SetStartlinePosition +172` through a stale low
  address. This matches the documented non-portable checkpoint boundary and
  is not counted as an atomic-save defect or current-source normal launch.
- The replacement clean run started exact PID `36490` at 13:14:59 CDT with
  `--record-from-replay`, consuming only the accepted 24,232-frame controller
  stream from frame zero and creating fresh iOS-process checkpoints and a fresh
  isolated memory card. At the intermediate entry it remained alive through
  frame 900/checkpoint 3. The real save, suspend/resume, cold read, hashes, and
  terminal duration remain in progress rather than inferred.
- The previous published timer was 164,821 seconds. The continuation audit read
  168,298 seconds: 1 day, 22 hours, 44 minutes, 58 seconds cumulative, adding
  3,477 seconds (57 minutes, 57 seconds). A final timer is recorded when the
  live run and publication checkpoint finish.

Full implementation, build, visual, rejected-route, and live-run evidence is
in `docs/parity/2026-07-31-ios-memory-card-atomicity.md` and the corresponding
engineering-journal entry.

### 2026-07-31 — Completed iOS save proof and added native touch controls

- The clean exact-`4b078065ff03` Simulator recording reached its real
  game-created save at 14:22:16 CDT. The 6,016-byte
  `BASCUS-94426-SLOTS` has CRC remainder zero and SHA-256
  `6a01b0f5562ed7a279d8f8e51e3b1874ac39a6120f55db4fe3873288950619a3`;
  no temporary writer file remained.
- Background/foreground preserved the save's inode, size, time and hash while
  lifecycle logs showed audio suspension and reactivation. The resumed screen
  visibly retained coherent kart, terrain, particle, HUD and minimap content.
- The report finalized naturally at 14:27:21 CDT after 24,232 frames and 81
  checkpoints. Its replay, states, metadata and log hashes are recorded in the
  parity report. The idle UIKit process was terminated only after report close
  and artifact inspection.
- The accepted report-root save was transparently copied byte-for-byte into
  the production default root to isolate a cold-reader test. A later app
  update preserved it and touch-only Adventure -> Load navigation displayed
  profile `A`. This accepts the production reader and update retention, not
  automatic report-to-default migration or a natural default-root save.
- Commit `c496c27f04c8` adds a safe-area-aware native UIKit overlay with an
  analog stick, Cross/Square/Circle/Triangle, L1/R1, Start and Select. Touch is
  composed as a player-one peer: active-low buttons combine with controller
  and keyboard, while active touch steering replaces only the left axes.
- The first live prototype rendered coherent Sony/crate/title/menu textures
  and selected Adventure with Gas, but short stick drags did not move menus.
  That result was rejected; menu code consumes D-pad, so the stick gained a
  0.68-radius outer ring that emits direction edges while retaining analog
  steering.
- LLDB proved a second failure precisely: the touch path produced pad bytes
  `00 73 bf ff 80 80 80 80` for Down, but a neutral host update replaced that
  one-snapshot edge before `GAMEPAD_ProcessHold`. Commit `c783eda740c4` keeps
  keyboard and touch press edges for two host snapshots. Stable Simulator
  menus then moved on the first direction tap.
- The exact `c783eda740c4` matrix passed 21/21 macOS ARM64 CTests and 21/21
  combined ASan/UBSan tests with no finding; the direct optimized i686 input
  compile passed; thin ARM64 Simulator and device products linked. Exact
  unsigned executable hashes are `27fc3a7d...bd6c3` for Simulator and
  `4211ceb7...a819` for device.
- The signed exact Simulator executable hash is `60a1373d...29f3`; strict/deep
  verification passed, but it is ad-hoc and has no TeamIdentifier. It is not a
  physical-device or distribution-signing result. Simulator cadence remained
  approximately 6–9 FPS under Apple Software Renderer.
- Corrected/rejected work remains visible in the detailed report: five bad
  Objective-C constraint selectors, the live-insufficient one-snapshot latch,
  a stale CTest regex, a cancelled parallel build whose `cc1` was actually
  CPU-active, an i686 macro/include-order collision, and an unsuitable
  clang-format gate. None was used as acceptance evidence.
- Both source commits were pushed to `origin/codex/arm64-apple` and remain in
  open draft PR #1, not merged to `main`. Retail media, saves, packages,
  screenshots, logs and credentials remained outside Git.
- The requested keyboard controls remain available: `WASD`, `IJKL`, `Q/E`,
  `P`, and Tab. The two-snapshot correction now applies to keyboard quick taps
  as well as touch; the established held-key and gamepad paths are unchanged.
- Physical-iPad development signing, real-device cadence/rotation, touch
  ergonomics/accessibility, full touch-only racing and practical drift/boost
  remain open. The goal remains active rather than being declared complete.
- A final read-only physical-gate audit found no `devicectl` device, zero valid
  code-signing identities, and no local provisioning profile. The unsigned
  device binary is built, but installing it requires a connected iPad and
  Apple development signing assets.

The previous published timer was 168,298 goal seconds. The pre-publication
documentation reading was 176,394 seconds: 2 days, 0 hours, 59 minutes,
54 seconds cumulative, adding 8,096 seconds (2 hours, 14 minutes, 56 seconds).
The timer is cumulative product-task time, including pauses, not a benchmark
or labor estimate.

### 2026-07-31 — Touch-only race movement, rotation reflow and analog trace

- Re-audited clean published branch tip `da151bfef`, exact installed touch
  executable `60a1373d...29f3`, retained 605,698,800-byte BIN and 6,016-byte
  save before live input. Local and remote branch identities matched and no
  source edit was present.
- Touch alone advanced the presentation, selected Time Trial through the
  stick's outer ring, confirmed Crash, Crash Cove and No Ghost, skipped the
  fly-in, and reached the normal lap-1 starting grid.
- Twelve Gas taps advanced time but did not establish held acceleration. Ten
  in-button drags produced slight movement. A longer bounded contact sequence
  moved Crash off the grid and beneath the CTR banner by timer `1:02:13`; the
  camera and minimap marker changed. A later alternating Gas/right-stick
  sequence reached `1:32:26` but did not show decisive turning, so live
  sustained steering is not claimed.
- Pause opened the retail Pause menu. The initial portrait-device launch was
  visibly letterboxed at 743 by 1018. Manual device rotation while paused
  produced a full 932-by-768 landscape layout with every safe-area control
  reflowed; Gas selected Resume and the race continued.
- Three public scene-geometry request variants were tried to fix automatic
  initial landscape: before overlay attachment, in `viewDidAppear`, and a
  concrete LandscapeRight preference. All compiled for Simulator/device, all
  returned without an error callback, and none changed the portrait cold
  launch. Every experimental source line was reverted; no dead workaround was
  committed.
- After clean reconfiguration, an exact `da151bfefb18` Simulator build had
  unsigned SHA-256 `476ccfca...f8a6` and signed SHA-256
  `896a13d7...457b`; strict/deep verification and installed-hash comparison
  passed. Repeated installs retained the BIN and saves byte-for-byte.
- LLDB on that exact clean app observed a visible right-edge stick contact
  enter `Platform_InputTouchLeftStick` as `(32766, 250, active=1)`, followed by
  `(0, 0, active=0)` release. This directly accepts UIKit-to-native analog
  delivery, while the deterministic oracle remains the simultaneous held-
  snapshot proof.
- Local-only screenshots record portrait Pause (`23a7f8d6...f2db`), landscape
  Pause (`41949c13...ed80`), forward movement (`d63fc9d4...b3646`) and later
  movement (`1d188f44...5a5f`). No screenshot or retail-derived artifact is
  staged.
- Automatic initial orientation, human-held Gas-plus-steering/drift, a full
  touch-only lap/race and physical-iPad feel remain open. The goal remains
  active.

The preceding published reading was 176,394 goal seconds. The pre-publication
documentation reading was 179,389 seconds: 2 days, 1 hour, 49 minutes,
49 seconds cumulative, adding 2,995 seconds (49 minutes, 55 seconds). The
timer is cumulative product-task time, including pauses, not a benchmark or
labor estimate.

### 2026-07-31 — Keyboard-controls request revalidated at current tip

- The requested basic keyboard controls were already present in published
  source commit `2c10b00b34df`: `WASD` maps the D-pad, `IJKL` maps the four
  face buttons, `Q/E` maps L1/R1, `P` maps Start and Tab maps Select. Original
  Arrow, `Z/X/C/V`, Shift, Ctrl, bracket, Return and Space bindings remain
  additive (`platform/native_input.c:308-341`).
- The map and a short race recipe are visible to testers in
  `README.md:205-233`; no hidden keyboard-only physics path exists.
- Reconfigured and rebuilt current published tip `8490b3126081` for macOS
  ARM64. The build succeeded with the 32 established warnings and produced a
  thin ARM64 Mach-O with SHA-256
  `46e70980e10de096472314fb4c641b8122d890271f9d183d6c3e6a61a94e6382`.
- The direct input self-test passed all 12 aliases, a held `K+D+E` chord, a
  quick `K+D` tap across two host snapshots, and controller/touch composition
  (`platform/native_input.c:1607-1707`). The complete macOS ARM64 suite passed
  21/21 with zero failure.
- No source edit was made for this request because duplicating or remapping an
  already accepted implementation would regress existing testers. The exact
  current-tip validation is the new work. macOS keyboard play remains
  accepted; physical-iPad hardware-keyboard delivery remains open.

The preceding published reading was 179,389 goal seconds. The pre-publication
documentation reading was 179,718 seconds: 2 days, 1 hour, 55 minutes,
18 seconds cumulative, adding 329 seconds (5 minutes, 29 seconds). The timer is
cumulative product-task time, including pauses, not a benchmark or labor
estimate.

### 2026-07-31 — iPadOS 26 adaptive scene correction and exact acceptance

- Simulator runtime diagnostics explained the portrait cold launch: on
  iPadOS 26, `UIRequiresFullScreen` is deprecated/ignored and applications
  must support all orientations and dynamic scene resizing. Apple documentation
  confirmed that scene interface-orientation updates may not visually rotate a
  window. This supersedes the earlier assumption that portrait meant the SDL
  landscape hint had failed.
- Commit `db45004f909d` adds
  `UIRequiresFullScreenIgnoredStartingWithVersion=26` and all four iPad
  orientations while retaining the older full-screen and phone landscape
  preference. The native platform comment now states the actual compatibility
  boundary; no private or ineffective scene-forcing code was added.
- An exact clean signed Simulator build embedded `db45004f909d`, passed
  strict/deep verification, cold-launched coherently in portrait, reflowed to a
  full 932-by-768 landscape view and back, and emitted no runtime configuration
  fault. Unsigned/signed executable SHA-256 values were
  `4898a9af...beb0` and `40851400...b96`.
- The exact thin device ARM64 product linked at SHA-256
  `483ae8c6...ff0`; it remains unsigned because no physical iPad, development
  identity or provisioning profile is available. The exact macOS ARM64 build
  passed 21/21 CTests in 1.66 seconds at SHA-256 `40452872...ca1`.
- The final Simulator install preserved the 605,698,800-byte retail BIN
  (`f780bf23...07c0`) and 6,016-byte save (`6a01b0f5...619a`) byte-for-byte.
  Screenshots, the retail image, save, packages and logs stayed local-only.
- The requested keyboard controls remain published and accepted on macOS:
  `WASD`, `IJKL`, `Q/E`, `P` and Tab, plus the original aliases. Physical-iPad
  keyboard delivery, human multi-touch/drift play, a full race, signing and
  device performance remain open. The active goal continues.

The preceding published reading was 179,718 goal seconds. The pre-publication
documentation reading was 181,770 seconds: 2 days, 2 hours, 29 minutes,
30 seconds cumulative, adding 2,052 seconds (34 minutes, 12 seconds). The timer
is cumulative product-task time, including pauses, not a benchmark or labor
estimate.

### 2026-07-31 — Reproducible retail-free iOS package and signing-readiness

- Commits `6db6116fe67a`, `a37cdf2aa5af`, and `207121134a05` add the
  source-controlled iOS packaging path. Apple bundles now embed `LICENSE`,
  `THIRD_PARTY_NOTICES.md`, and `INSTALL-IOS.md`; `package-ios.sh` validates a
  thin ARM64/iOS app and emits a standard `Payload/CTRPad.app` IPA.
- Unsigned mode deliberately removes signatures and profiles for later
  user-side re-signing. Signed mode accepts a user-owned identity/profile,
  validates platform, expiry, App ID and optional device, constructs minimal
  app/team/keychain entitlements, requests DER entitlements, and strictly
  verifies/read-backs the signature. No private key or credential is copied.
- Exact tip `207121134a05` produced a thin device executable at SHA-256
  `62e8148d...64be9`. Two independent unsigned IPA runs were byte-identical at
  SHA-256 `78b93b01...e0c63`; extraction showed exactly seven members, exact
  legal/install resources, and no retail-like file, runtime directory,
  provisioning profile, or code signature. An injected `ctr-u.bin` was
  rejected.
- The exact macOS ARM64 regression product at SHA-256 `9e90e9ad...fb246`
  passed 21/21 CTests in 1.17 seconds. A Simulator update/launch retained the
  imported BIN and save by inode, byte size, and SHA-256.
- Four rejected/corrected routes remain in the permanent record: an
  impractically slow Xcode-generator configure, an IPA missing the top-level
  `Payload/` directory, ZIP nondeterminism from temporary mtimes, and dotted
  entitlement keys misconstructed by `plutil`. `shellcheck` was unavailable,
  so only `bash -n` and the recorded behavioral/build/archive gates are
  claimed.
- All source commits are pushed to `origin/codex/arm64-apple` and draft PR #1.
  They are not merged to `main`. A real Apple identity/profile, a connected
  iPad, signed installation, on-device import/update persistence, physical
  multi-touch feel, drift/boost, a full race, cadence, thermal and performance
  acceptance remain open. The goal remains active.

The preceding published reading was 181,770 goal seconds. The pre-publication
documentation reading was 183,956 seconds: 2 days, 3 hours, 5 minutes,
56 seconds cumulative, adding 2,186 seconds (36 minutes, 26 seconds). The timer
is cumulative product-task time, including pauses, not a benchmark or labor
estimate.

### 2026-07-31 — Live Files wrong-region and incomplete-image recovery

- A disposable clone of the preserved iPadOS 26.5 evidence device exercised
  two previously inspection-only import outcomes through the real Files picker.
  The clone was used so the accepted source BIN/save container was never
  renamed or edited.
- The archived 740,179,104-byte PAL image produced the exact detected identity
  `SCES_021.05`; a 94,080,000-byte/40,000-sector NTSC-U truncation reached the
  distinct required-content error rather than invalid-format or wrong-region.
- Both failures re-enabled the chooser, removed staging residue, created no
  destination, and preserved the accepted BIN (`f780bf23...07c0`) and memory
  card (`6a01b0f5...619a`) by inode, size, and SHA-256.
- Selecting the full NTSC-U source afterward installed the exact accepted hash
  and reached rendered game output in the original PID `77827`. A cold relaunch
  used PID `78515`, bypassed onboarding, and rendered with the touch overlay.
- The initial `simctl clone` metadata exposed stale absolute source-device
  URLs. Those paths were rejected; reinstalling the exact signed app migrated
  the clone to self-contained URLs before any evidence interaction.
- The disposable device remains shut down, not deleted. The original evidence
  Simulator was restored and its original BIN/save inodes and hashes remained
  unchanged. Retail files, fixtures, screenshots, app bundles, and containers
  remain local-only and ignored.
- Inaccessible-provider/copy-interruption, termination during copy, explicit
  active-image re-selection, physical Files behavior and signing remain open.
  The goal remains active.

The preceding published reading was 183,956 goal seconds. The pre-publication
documentation reading was 185,118 seconds: 2 days, 3 hours, 25 minutes,
18 seconds cumulative, adding 1,162 seconds (19 minutes, 22 seconds). The
timer is cumulative product-task time, including pauses, not a benchmark or
labor estimate.

### 2026-07-31 — Interrupted-import next-launch recovery

- A source audit found that every handled import failure removed its unique
  `.ctrpad-import-*` stage, but process termination could bypass those handlers
  and leave a partial 605 MB copy in Documents indefinitely.
- Commit `c745390a55eb` centralizes the reserved staging prefix and, before
  media-free onboarding, removes only direct child directories with that
  prefix and a nonempty suffix. It reports the recovered count and keeps retry
  enabled; unrelated entries and ordinary files are not cleanup targets.
- Exact clean source identity `c745390a5` linked thin ARM64 Simulator and
  device products at SHA-256 `019cf0a4...25fce` and
  `1cc45ac0...f1919`. The macOS ARM64 product at
  `ea2c719c...ef934` passed 21/21 CTests in 0.94 seconds. Both iOS products
  retain the iOS 15.0 floor and SDK 26.5.
- A unique ad-hoc Simulator copy passed strict/deep signature verification.
  The isolated negative-test clone was seeded with two stale stages plus three
  controls: a nonmatching directory, the exact prefix without a suffix, and a
  same-prefix ordinary file.
- Launch PID `81703` removed exactly the two stale stages, preserved all three
  controls, left no installed partial destination, and visibly reported
  `Recovered 2 interrupted imports` with an enabled chooser.
- The clone's accepted 605,698,800-byte BIN (`f780bf23...07c0`) and 6,016-byte
  save (`6a01b0f5...619a`) retained their inodes, sizes and hashes. Restoring
  the accepted destination and cold-launching PID `81856` rendered retail CTR
  output with the complete touch overlay.
- The clone was shut down. The original validation Simulator was restored and
  relaunched at PID `82204`; its original BIN/save inodes and hashes remained
  unchanged. All retail data, fixtures, signed app copies and screenshots
  remained outside Git.
- The source commit was pushed to `origin/codex/arm64-apple` and the correct
  repo-qualified draft PR #1 remains open against `main`. Physical iPad
  signing/import and a real Files-provider/background termination during copy
  remain open; the seeded durable recovery state is accepted without claiming
  that live event.

The preceding published reading was 185,118 goal seconds. The pre-publication
documentation reading was 186,029 seconds: 2 days, 3 hours, 40 minutes,
29 seconds cumulative, adding 911 seconds (15 minutes, 11 seconds). The timer
is cumulative product-task time, including pauses, not a benchmark or labor
estimate.

### 2026-07-31 — Real Files import `SIGKILL` and recovery

- The disposable clone exercised the actual process-death path after the prior
  seeded-state acceptance. Its accepted BIN/save were isolated under the same
  preserved inodes and hashes; the original validation Simulator was shut down
  before mutation.
- An initial blank document-provider sheet logged File Provider error `-1002`.
  Dismissal returned to the enabled cancel state, and a fresh picker presentation
  loaded the real **On My iPad → CTRPad** hierarchy. No inaccessible-file
  callback is claimed from that provider UI failure.
- A first `simctl terminate` attempt was rejected as interruption evidence: the
  app completed and installed the exact accepted 605,698,800-byte image before
  the graceful termination finished. A shell-polling attempt then missed the
  short-lived local APFS stage and was explicitly stopped.
- A local-only cleanly compiled ARM64 `kqueue` helper watched the import base and
  full staged asset. Its source/binary hashes were `5483b6a5...67d3f` and
  `22940c2f...f8970`; neither artifact entered Git or the app.
- The real Files picker selected the full NTSC-U source. The helper observed
  stage UUID `77A87CDE-A75D-4FC7-93D2-F0480E7F4075`, full asset inode
  `111335815`, accepted size/hash, then sent uncatchable `SIGKILL` to exact app
  PID `93005` before validation/install.
- After death, the full stage survived, PID `93005` and the final destination
  were absent, and the retained BIN/save were unchanged. Relaunch PID `93222`
  removed that exact stage, preserved all three cleanup controls, left no
  destination, and visibly reported one recovered import.
- Restoring accepted clone inode `111313696` and cold-launching PID `93310`
  rendered the retail Sony presentation with the complete touch overlay. The
  clone was shut down; original validation PID `93637` relaunched with original
  BIN/save inodes `111131200`/`111222179` and exact hashes intact.
- This accepts actual Simulator Files-process death and next-launch recovery.
  Partial-byte provider interruption, inaccessible URL delivery, physical-iPad
  termination, Apple signing and explicit active-image re-selection remain
  open. The goal remains active.

The preceding published reading was 186,029 goal seconds. The pre-publication
documentation reading was 187,126 seconds: 2 days, 3 hours, 58 minutes,
46 seconds cumulative, adding 1,097 seconds (18 minutes, 17 seconds). The timer
is cumulative product-task time, including pauses, not a benchmark or labor
estimate.

### 2026-07-31 — Installed-asset interrupted-stage recovery

- Contradiction review after the live `SIGKILL` acceptance found a narrower
  lifecycle hole: termination after destination installation but before stage
  removal leaves a valid BIN, so the next launch bypasses media-free onboarding
  and its cleanup.
- Commit `8c177e8327f3` exposes the existing direct-child, reserved-prefix
  cleanup before ordinary iOS runtime startup. It does not add a second matcher
  or scan recursively. Missing-media onboarding continues to use the same
  helper and visible recovered-count message.
- A pre-commit diagnostic matrix linked both thin ARM64 iOS products with no
  new Objective-C warning and passed macOS 21/21 in 1.26 seconds. All presets
  were then explicitly reconfigured for exact clean identity
  `SDL-3.4.10-beta-7.1-138-g8c177e832`; the accepted macOS run passed 21/21 in
  1.10 seconds.
- Exact Simulator/device/macOS executable SHA-256 values are
  `e9cb3919...dee96`, `f747d24b...ab8`, and `02a83420...891b`. All are thin
  ARM64; iOS Simulator/device target their correct platforms, iOS 15.0 minimum
  and SDK 26.5. The unique ad-hoc Simulator copy passed strict/deep verification
  at signed hash `c3e6a5d7...97033`.
- The disposable clone retained a valid BIN and save while an empty
  `.ctrpad-import-installed-destination-leftover` positive fixture was added
  beside the three earlier cleanup controls. Exact app installation migrated
  the data-container UUID; the new path was resolved and every fixture/BIN/save
  inode verified before launch.
- Exact app PID `95815` removed only the positive fixture, preserved the
  nonmatching directory, exact bare prefix, same-prefix regular file, valid BIN
  and save, bypassed onboarding, and visibly rendered the game with the full
  touch overlay. The retail/save hashes remained `f780bf23...07c0` and
  `6a01b0f5...619a`.
- Requested `simctl` stdout/stderr files were not produced, so no console-count
  observation is claimed. Filesystem scope, preserved identities, exact
  installed hash and the rendered frame are the accepted evidence. Cold
  relaunch PID `95992` remained stage-free.
- The clone was terminated and shut down. Original validation PID `93637`
  remained in the foreground with source BIN/save inodes `111131200`/`111222179`
  and exact hashes unchanged. Retail data, fixture, bundle and screenshot stayed
  outside Git.
- Source commit `8c177e832` was pushed to
  `origin/codex/arm64-apple` before this documentation checkpoint. The goal
  remains active for physical iPad signing/import, provider-specific partial
  transfer behavior, hardware controls/keyboard, full touch race and device
  timing/performance acceptance.

The preceding published reading was 187,126 goal seconds. The pre-publication
documentation reading was 188,010 seconds: 2 days, 4 hours, 13 minutes,
30 seconds cumulative, adding 884 seconds (14 minutes, 44 seconds). The timer is
cumulative product-task time, including pauses, not a benchmark or labor
estimate.

### 2026-07-31 — Current-tip iPad package revalidation

- Rechecked the actual branch and external signing state before packaging.
  Local/remote tip was `560f6dd20`; the keychain exposed zero valid code-signing
  identities, the normal provisioning-profile location contained no profile
  files, and `devicectl` found no connected device.
- Explicitly reconfigured and rebuilt macOS, Simulator ARM64 and device ARM64 in
  parallel. Every product embedded clean identity
  `SDL-3.4.10-beta-7.1-139-g560f6dd20`; macOS passed 21/21 tests in 1.11
  seconds. Exact Simulator/device/macOS hashes were `a84eb77d...a4b3`,
  `667048ab...c712`, and `82c911c8...4521`.
- Two independent unsigned package runs produced byte-identical 1,433,744-byte
  IPAs at SHA-256 `ad8736cd...d3fa`. The archive held exactly seven expected
  app/legal/install members with source-normalized internal timestamps.
- Extraction passed integrity, architecture/platform/minimum-OS/identity checks
  and exact executable/resource comparisons. Retail-like files, runtime data
  directories, a profile, and a signature were absent. The expected unsigned
  `codesign` failure was retained as evidence rather than reported as a pass.
- Current-tip negative probes rejected an injected `ctr-u.bin`, identity without
  profile, device without signing inputs, and attempted output overwrite; none
  created a package.
- The exact Simulator sibling was ad-hoc signed only for local execution and
  installed on the disposable clone. PID `97960` rendered the retail copyright
  frame and full touch overlay while preserving the clone BIN/save inodes,
  sizes, and hashes. The clone was shut down afterward.
- Original validation PID `93637` remained in front with original BIN/save
  inodes `111131200`/`111222179` and hashes unchanged. IPAs, fixtures, app copy,
  containers and screenshot remained outside Git.
- The unsigned package path is current and reproducible, but the signed
  sideloadable deliverable still requires a user-owned Apple identity/profile
  plus target iPad. Physical Files/update persistence, multi-touch/full-race,
  keyboard and performance gates remain open; the goal remains active.

The preceding published reading was 188,010 goal seconds. The pre-publication
documentation reading was 188,683 seconds: 2 days, 4 hours, 24 minutes,
43 seconds cumulative, adding 673 seconds (11 minutes, 13 seconds). The timer is
cumulative product-task time, including pauses, not a benchmark or labor
estimate.

### 2026-07-31 — Current-tip live Gas and analog-steering trace

- Reused the exact audited Simulator product for implementation tip
  `560f6dd20` while the branch documentation tip was `db78d5a25`. Installed
  executable SHA-256 remained `df2d6249...41ee`; no source rebuild or product
  change was inferred from the later documentation commits.
- Launched the disposable clone with wait-for-debugger and used the overlay to
  reach Time Trial → Crash → Crash Cove → No Ghost → live lap 1/3. The game,
  textures, HUD, minimap and complete accessibility-labelled overlay remained
  visible.
- Twelve Gas UI actions moved Crash forward from the starting line. A live
  stick drag reached signed X `32763`, then released to `(0,0,inactive)`.
- LLDB inspected the native touch state at every stick callback. Cross was
  never present in `heldButtons`; attempted Option-assisted two-contact,
  click/drag overlap and dual-drag routes were therefore rejected as
  serialized automation rather than multi-touch.
- A normal LLDB attach hung for roughly 90 seconds and was stopped. An
  optimized conditional breakpoint also stopped every input poll and was
  deleted. The accepted wait-for-debugger run used an auto-continuing
  `Platform_InputTouchLeftStick` breakpoint.
- Local-only 2064-by-2752 baseline/after screenshots hashed to
  `f6a35759...5ac8` and `4e1ac6f...421`. Neither screenshot entered Git.
- After clean debugger detach, Pause opened the retail Pause menu. Clone and
  source BIN/save sizes, hashes and inodes remained unchanged; the disposable
  clone was shut down without deletion and source PID `93637` remained running.
- Single-control current-tip gameplay is strengthened. Natural simultaneous
  Gas/steer/drift, a full touch-only race, physical-iPad keyboard behavior,
  signing and device performance remain open; the goal remains active.

The preceding published reading was 188,683 goal seconds. The pre-publication
documentation reading was 191,189 seconds: 2 days, 5 hours, 6 minutes,
29 seconds cumulative, adding 2,506 seconds (41 minutes, 46 seconds). The timer
is cumulative product-task time, including pauses, not a benchmark or labor
estimate.

### 2026-07-31 — iOS hardware keyboard now controls player one

- The practical keyboard aliases were already published, but a delayed live
  iOS report proved Simulator controller enumeration moved the keyboard to
  player two while touch remained player one. Start and Down arrived in slot
  one, explaining why retail menus did not respond.
- Commit `e6ba535a9c73` shares keyboard, touch and controller input in iOS's
  primary PSX-shaped pad while retaining separate-player desktop ownership. A
  media-free test covers both policies. iOS replay metadata now says `ios`
  instead of `macos`.
- A normal LLDB attach and two dyld-stopped wait-for-debugger launches were
  rejected as debugger perturbation. Immediate report `ctr-195059` overflowed
  its frame-zero VSync runs; delayed F9/F10 recording was the accepted route.
- The source fix was committed and pushed before final acceptance. Exact clean
  Simulator/device/macOS binaries hashed to `1cef6404...c62cb`,
  `4ca08e9b...9db1`, and `cfd3d9b4...1dca`; macOS passed 21/21 CTests.
- Exact Simulator installation migrated but preserved the imported BIN and
  save under inodes `111313696`/`111309627`, accepted sizes and hashes.
- Keyboard-only `P`, `S`, and `K` input navigated presentations → Time Trial →
  Crash → Crash Cove → No Ghost → starting grid. Final report `ctr-201917`
  contains 1,869 frames, seven checkpoints, clean build ID `e6ba535a9c73`,
  `platform=ios` and fingerprint `e9d9b4240372487c`.
- Slot zero contains Start at frame 361, Down at 528, and Cross at 906/1249/
  1376/1562, each followed by neutral. Slots one through three stayed
  disconnected and neutral. Exact replay completed all 1,869 frames without a
  canonical divergence and visibly reproduced the race grid.
- Clone and source BIN/save identities remained unchanged after replay. The
  disposable clone was shut down; source PID `93637` returned to the
  foreground. Reports, screenshots, products, retail data and saves remained
  outside Git.
- Simulator keyboard navigation/release is accepted. Physical-iPad keyboard,
  signing, physical Files/update/save behavior, natural multi-touch, a full
  race and device performance remain open. The goal remains active.

The preceding published reading was 191,189 goal seconds. The pre-publication
reading was 194,894 seconds: 2 days, 6 hours, 8 minutes, 14 seconds cumulative,
adding 3,705 seconds (1 hour, 1 minute, 45 seconds). The timer is cumulative
goal time, including pauses, not a benchmark or labor estimate.
