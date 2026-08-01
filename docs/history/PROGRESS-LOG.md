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

### 2026-07-31 — Live GLES and desktop-GL equivalence slice

- Re-audited the active objective, fully reread the 511-line viability report,
  inspected the clean synchronized branch, and selected M7 renderer
  equivalence as the strongest remaining locally closable gate. Physical
  signing/device and human multi-touch acceptance remain unavailable.
- Rejected direct playback of mid-session iOS report `ctr-201917` in macOS.
  The truthful `ios`/`macos` identity mismatch was bypassed only as a
  diagnostic; the identity-incompatible checkpoint restored raw state and the
  process failed with a macOS crash report. Existing replay documentation says
  this bypass cannot make checkpoints portable, so it was not used as parity
  evidence.
- Made a local-only exact 2,000-frame version-4 prefix of accepted seed report
  `ctr-215303`, hash `6a26357c...f49`. The strict comparator proved all eight
  components equal to the source prefix. Fresh current binaries consumed only
  its pad/VSync automation through `--record-from-replay` and generated native
  platform state independently.
- Exact implementation `e6ba535a9c73` macOS report `ctr-204509` and iOS GLES
  report `ctr-204914` each finalized 2,000 frames and seven checkpoints. Their
  full comparison found zero mismatches in timing, RNG, drivers, world,
  allocation, root, pads and VSync.
- Exact producer playbacks traced frames 1,802 and 1,813. Both renderers emitted
  identical packed vertices and draw splits. Aggregate hashes were
  `63f8c781f85e4358` and `15c8c5410da67002`; both playbacks reached the normal
  2,000-frame finish without canonical divergence.
- The historical 25 false 16-bit batches at frame 1,802 were the already-fixed
  LP64 mosaic classification defect. Current macOS and GLES both correctly
  emitted zero 16-bit feedback batches; the old rejected trace was not
  repurposed as desired coverage.
- A local-only 2064-by-2752 iOS framebuffer, hash `c7ec9117...24e`, visibly
  showed coherent Arcade Crash Cove preview textures, kart colors, exhaust
  transparency, legible UI and the full touch overlay. It remained outside
  Git. Simulator throughput fell to roughly 3–11 FPS under Apple's software
  renderer and is not claimed as device performance.
- Clone BIN/save inodes `111313696`/`111309627`, sizes and hashes remained
  unchanged. The clone was shut down, source-validation PID `93637` remained
  running, the diagnostic crash reporter was dismissed, and 21/21 current
  macOS CTests passed.
- M7 now has a representative 2,000-frame live-GLES equivalence slice, but
  remains in progress: live Cocoa GLES/ANGLE, the full 24,232-frame GLES run,
  explicit remaining visual edge cases and physical-iPad cadence are open.
  The overall goal remains active.

The preceding published reading was 194,894 goal seconds. The pre-publication
reading was 196,487 seconds: 2 days, 6 hours, 34 minutes, 47 seconds cumulative,
adding 1,593 seconds (26 minutes, 33 seconds). The timer is cumulative goal
time, including pauses, not a benchmark or labor estimate.

### 2026-07-31 — Full exact-current iOS GLES golden comparison

- Preserved the already-running exact iOS/UIKit/GLES producer instead of
  interrupting it for the later keyboard request. Report `ctr-211128`
  finalized all 24,232 frames and 81 checkpoints after 1:07:38 under Apple's
  Simulator software renderer. Final replay/state hashes are
  `056866ff...c60d` and `94eca26b...05e`.
- Strict comparison against accepted macOS oracle `ctr-215303` first found all
  eight required components equal on all 24,232 frames. Exact iOS checkpoint
  80 then validated all 81 records, restored frame 24,000, and replayed to the
  normal frame-24,232 close.
- The first checkpoint launch used stale bundle ID
  `com.chrissotraidis.CTRPad` and SpringBoard rejected it as not found. The
  installed `Info.plist`/`simctl listapps` identity
  `io.github.chrissotraidis.ctrpad` was then used successfully. The rejected
  request started no app and changed no report.
- No exact-current full macOS report existed, so exact build `e6ba535a9c73`
  generated desktop-GL report `ctr-222546` from the same pad/VSync seed. It
  finalized all 24,232 frames and 81 checkpoints with exit 0 after 13:31,
  later measuring 29.90-29.91 FPS.
- The decisive same-build comparison found zero mismatches in timing, RNG,
  drivers, world, allocation, root, pads and VSync on all 24,232 frames.
  Exact checkpoint-80 playbacks also emitted identical late frame-24,001 draw
  commands, aggregate hash `d1765e952537c48b`, and both finished normally.
- Direct iOS checkpoint inspection reports `maxLap=0`, `maxCheckpoint=77` and
  `lapAdvanced=no`. This is full renderer-state/transport evidence, not a
  replacement for the separately accepted lap oracle or an open completed
  human race.
- The user's explicit keyboard request was audited against the published tip.
  Basic controls were already implemented and documented: arrows/WASD,
  C/K Gas, X/J brake, V/L item, Q/E drift, P/Enter Start and Tab/Space Select.
  The direct input test passed all aliases, quick taps and the held `K+D+E`
  chord; the complete macOS matrix passed 21/21. No duplicate source path was
  added.
- The clone retail BIN/save kept inodes `111313696`/`111309627`, sizes and
  hashes `f780bf23...07c0`/`6a01b0f5...619a`. Local reports, state files and
  traces stayed outside Git. The clone was shut down; protected source PID
  `93637` remained running.
- The full renderer choice state/cadence criterion is accepted. M7 remains in
  progress for live Cocoa GLES/ANGLE, explicit uncovered pixel edge cases and
  physical-iPad cadence. Signing/device and natural multi-touch race gates
  remain open; the overall goal remains active.

The preceding published reading was 196,487 goal seconds. The
pre-documentation reading was 202,463 seconds: 2 days, 8 hours, 14 minutes,
23 seconds cumulative, adding 5,976 seconds (1 hour, 39 minutes, 36 seconds).
The timer is cumulative goal time, including pauses, not a benchmark or labor
estimate. Exact iOS/macOS producer wall times are listed separately above.

### 2026-07-31 — Renderer pixel-semantics oracle and GLES readback fix

- Selected the remaining locally closable M7 pixel/mask/feedback boundary
  after confirming physical signing and device cadence still require external
  hardware/credentials. CTR's production mask packet consumes E6 bit 0 only,
  so the acceptance claim is intentionally bounded to that behavior.
- Added media-free `--self-test-renderer-pixels`, running after storage setup
  and before asset selection. The 32-by-16 oracle drives production draw
  packets and checks 4/8/16-bit textures, 4/8-bit CLUTs, zero/STP transparency,
  opaque/average passes, exact alpha, output mask, framebuffer feedback and
  exact RGB5551 packing/readback. Apple desktop GL now carries this as CTest
  15, bringing the full suite to 22 tests.
- Rejected the first mask fixture: red input 248 passed through normal
  modulation/quantization as 241 / `0x801e`, so it could not truthfully expect
  248 / `0x801f`. Input 255 isolates the mask behavior and passes.
- The first live UIKit/GLES run passed every RGBA check and matched desktop
  full-frame hash `851169f2644a1675`, but all packed VRAM words were zero.
  Apple's GLES rejected `GL_RG` readback from RG8 with
  `GL_INVALID_OPERATION`; the old path then exposed a stale CPU mirror.
- Fixed GLES GPU-to-CPU VRAM synchronization through guaranteed
  RGBA/UNSIGNED_BYTE readback, checked errors and explicit R/G-to-word repack.
  Dirty ownership is now released only on success. The corrected live GLES
  result matches desktop RGBA hash and exact VRAM words.
- Published exact implementation `818bc0e161d3` before the final matrix. A
  desktop verification chain initially named nonexistent `ctr`; the corrected
  `ctr_native` command passed. A separate precommit leak-detection sanitizer
  option was rejected as unsupported by Apple arm64 and replaced by supported
  ASan/UBSan.
- Exact clean validation passed desktop GL oracle plus 22/22 CTest, live iPad
  Simulator UIKit/GLES oracle, iPhoneOS ARM64 compile/link, macOS GLES compile/
  link and ASan/UBSan 22/22. Desktop and GLES hashes are identically
  `851169f2644a1675`; Cocoa still cannot execute GLES without ANGLE/EGL.
- The disposable clone retained canonical BIN/save inodes, sizes and hashes.
  An unnecessary archival hash traversal was stopped; a post-shutdown
  container query was rejected read-only. The clone was shut down, protected
  validation Simulator left booted and untouched, and no assets entered Git.
- M7 pixel semantics are accepted. Live macOS GLES and physical-iPad cadence/
  energy remain open, as do the broader signing, device-input, completed-race
  and device Files/save gates. The overall goal remains active.

The preceding published reading was 202,463 goal seconds. The
documentation-close reading was 205,801 seconds: 2 days, 9 hours, 10 minutes,
1 second cumulative, adding 3,338 seconds (55 minutes, 38 seconds). The timer
is cumulative goal time, including pauses, not a benchmark or labor estimate.

### 2026-08-01 — UIKit view lifecycle and visible transition correction

- Reproduced two unbalanced UIKit appearance-transition warnings during an
  ordinary pre-correction production launch and three after the immediate
  renderer self-test returned. Both ordinary warnings mapped to SDL's two
  nil/restore root-controller sequences, retained historically for iOS 7 and
  below despite CTRPad's iOS 15 minimum.
- Rejected the first warning-only correction. Removing the nil assignments
  reduced the short-test warning count to one and ordinary count to zero, but
  reassignment of the same controller did not attach its replacement view.
  GLES ran behind a completely black screenshot (`97de7803...a5b`), so the
  change was fully reverted and never committed.
- Implemented an iOS 15+ hierarchy-preserving path: retain the installed root
  controller, attach its replacement view directly only when it has no
  superview, and install the controller normally only when it is not root.
  Dirty-source validation restored coherent pixels and zero ordinary warnings.
- Published implementation `6b268157888f` (`fix: balance UIKit view
  transitions`) before the final matrix, then reconfigured every build so its
  embedded build ID was exact.
- Exact Simulator renderer self-test passed unchanged GLES semantic hash
  `851169f2644a1675`. It still emits one warning only after immediate teardown;
  that residual remains explicit.
- Exact production startup emitted zero appearance warnings, initialized UIKit
  FBO/RBO 1 and all PSX/VRAM pipelines, then visibly rendered the animated
  title/demo and complete touch overlay. A slow initial interval was confirmed
  as CPU-active validation of canonical 605.7 MB `ctr-u.bin`, not a hang.
- Used the actual Simulator Home control and actual SpringBoard CTRPad icon.
  Logs ordered will/did background with audio suspended, then will/did
  foreground with audio active. The retained process remained visibly
  coherent and animated.
- Used the actual Simulator Rotate control after refreshing accessibility
  state. Rendering survived, geometry changed, and the native overlay reflowed.
  The attached production console remained free of appearance warnings through
  startup, Home/resume, rotation and bounded termination.
- Exact clean validation passed desktop GL CTest 22/22, iPhoneOS ARM64
  compile/link, macOS GLES compile/link, and ASan/UBSan CTest 22/22. Input CTest
  6 also reconfirmed the already published basic keyboard aliases; no duplicate
  keyboard source path was added.
- Canonical BIN/save inodes, sizes and SHA-256 stayed unchanged. The disposable
  clone was shut down without deletion; protected validation Simulator stayed
  booted and untouched. Retail media and screenshots stayed outside Git.
- The ordinary Simulator appearance-warning, Home/resume and rotation boundary
  is accepted. Physical signing/device execution, natural termination,
  low-memory/background-save behavior, device input/race/audio/video, cadence
  and energy remain open; M8 and the overall goal stay active.

The preceding published reading was 205,801 goal seconds. The
documentation-close reading was 208,828 seconds: 2 days, 10 hours, 0 minutes,
28 seconds cumulative, adding 3,027 seconds (50 minutes, 27 seconds). The timer
is cumulative goal time, including pauses, not a benchmark or labor estimate.

### 2026-08-01 — Safe in-game iOS disc re-selection

- Re-read the full native viability record, then selected M9's explicit asset-
  reselection gap because it was the next software-only gate; physical signing,
  device cadence and genuine multi-touch still require hardware/credentials.
- Rejected hot-swapping the global disc/asset state under a live game. Added an
  accessible **CHANGE DISC** control that confirms save/unsaved-progress
  consequences, posts a request to the next UIKit display callback, shuts down
  the renderer/audio/touch/disc in order, and only then opens Files.
- Reused the existing coordinated same-volume validate-before-replace importer.
  Initial setup still starts in-process; runtime replacement requires a cold
  relaunch and disables the picker after success. Cancel and every error now
  state that the current disc remains installed.
- Dirty-source Simulator proof covered confirmation cancel, coherent animated
  resume, safe stop, the known first-presentation File Provider blank sheet,
  picker cancellation, full NTSC-U replacement and rendered cold relaunch.
  iOS Simulator/device and macOS compiled; desktop CTest passed 22/22.
- Reviewed the five-file diff and published implementation
  `300499d7cd00` (`feat: add safe iOS disc reselection`). Draft PR #1 was
  verified at the same full SHA before exact acceptance.
- Reconfigured all five final directories. Exact ARM64 Simulator, iPhoneOS,
  desktop GL, macOS GLES and ASan/UBSan products embed `300499d7cd00`.
  Desktop and sanitizer matrices each pass 22/22. The exact hashes are in
  `docs/parity/2026-08-01-ios-disc-reselection.md`.
- Exact Computer Use validation drove the real Simulator and Files UI. Cancel
  preserved animated gameplay. A 118-byte invalid fixture was rejected with
  the new preservation assurance and left disc/save inode, size and hash
  unchanged. The complete 605,698,800-byte NTSC-U fixture then installed at
  inode `111450682` with hash `f780bf23...07c0`; save inode `111309627`, size
  6,016 and hash `6a01b0f5...619a` remained unchanged.
- Exact cold relaunch visibly consumed the replacement and rendered coherent
  textured CTR output plus the complete touch overlay. A 610,956-byte local-
  only screenshot hashed to `a33b033b...ca70`. No new staging residue or UIKit
  appearance warning appeared. Retail media and screenshots stayed outside
  Git; the protected validation Simulator remained untouched.
- Basic keyboard controls remain available through the already published
  shared player-one path: arrows/WASD, C/K gas, X/J brake, V/L item, Q/E drift,
  P/Enter Start and Tab/Space Select. Exact CTest 6 passed in both ordinary and
  sanitizer suites; no redundant input subsystem was added.
- M9 asset re-selection is accepted on Simulator. Physical Files/signing/save,
  inaccessible-provider callbacks, hardware input/race/audio/video, cadence
  and energy remain open. The overall goal remains active.

The preceding published reading was 208,828 goal seconds. The documentation-
open reading was 211,878 seconds: 2 days, 10 hours, 51 minutes, 18 seconds
cumulative, adding 3,050 seconds (50 minutes, 50 seconds). The documentation-
close reading was 212,262 seconds: 2 days, 10 hours, 57 minutes, 42 seconds,
adding 3,434 seconds (57 minutes, 14 seconds) from the preceding published
checkpoint and 384 seconds (6 minutes, 24 seconds) during the closing audit.
Goal time includes pauses and resumes and is not a benchmark or labor estimate.

### 2026-08-01 — Control settings and reopened level-geometry gate (in progress)

- Added and pushed implementation `828d095809fc` (`feat: add customizable iOS
  controls`). The in-game **CONTROLS** sheet mirrors steering/action clusters,
  applies three size/opacity choices immediately, persists them locally,
  restores true defaults by removing preference keys, scrolls on short
  landscape displays and maintains 44-point-or-larger setting actions.
- The user's requested basic keyboard controls already existed in the shared
  SDL-to-PS1 path. Rather than add a conflicting bridge, the sheet now exposes
  the practical WASD/IJKL/Q/E/P/Tab legend. Existing exact Simulator keyboard
  delivery and ordinary/sanitizer input CTests remain the evidence boundary.
- Dirty Simulator UI covered right/large/high, all eleven controls, short-sheet
  scrolling, accessible Reset/Done and return to left/standard/standard. Disc
  inode/hash `111450682`/`f780bf23...07c0` and save
  `111309627`/`6a01b0f5...619a` remained unchanged. A stale/wrong-window view
  initially suggested trailing controls were clipped; geometry logging proved
  every frame in-bounds and a correctly focused state showed all controls.
- Rejected one LLDB-stopped geometry attempt, one nonexistent-header build
  failure, the protected-device screenshot brought forward by Simulator focus,
  and an F9/P/F10 attempt on a launch that was not armed for reports. Temporary
  geometry logging was removed before commit.
- Dirty Simulator/device/macOS builds passed. Ordinary and ASan/UBSan desktop
  suites each passed 22/22. The implementation was pushed before exact
  acceptance. The initial five-target exact command was interrupted when the
  user reported system slowness and two open simulators.
- Shut down the disposable clone and left exactly one booted device, protected
  `CTRPad Import Validation`. Heavy builds were stopped. A later nice-15,
  single-job compile was also stopped when free VM pages were roughly 64 MB.
  No build/compiler process remains, and no additional simulator will be
  booted without first preserving the one-device limit.
- The user then correctly challenged visible missing game assets. Read-only
  inspection separated the expected checkerboard Loading transition from a
  running incomplete scene. The protected 2,623-line log contains exactly 268
  AssetRef errors: 134 `LOAD_TenStages` plus 134 `MainInit` level-visibility
  cache exhaustions, all at capacity eight, and no second file/model/texture
  failure class.
- Root cause: low-memory pack resets invalidated LEV allocations but not their
  LP64 host-sized visibility/BSP sidecars. After eight distinct destination
  addresses, `Level_GetVisMem` returned null; `RenderAllLevelGeometry` then
  skipped the whole course while HUD/kart instances could remain.
- Replaced a first call-site-only draft with range-aware invalidation at the
  allocator lifetime boundary. Native ClearLowMem, PopState and PopToState now
  release only sidecars whose level lies in the discarded range, preserving a
  live Adventure-hub pack. The inactive hub `visMem2` pointer is cleared.
  Media-free coverage fills the eight slots and requires targeted, range and
  full recycling through CTest's exact success marker.
- M7 visual acceptance is reopened. The source correction is implemented but
  remains unbuilt/unaccepted under the current resource constraint. Required
  next gates are compile, focused/full tests, a disposable single-Simulator
  run through more than eight distinct level allocations, zero new exhaustion
  lines, exact build publication and visual comparison. M10 and the overall
  goal remain active.
- A proposed lightweight compile of only
  `platform/native_asset_ref.c.o` was rejected cleanly before any compiler ran:
  this project unity-builds the platform sources through `main.c.o`, so Ninja
  reported that the standalone object target does not exist. The first
  meaningful compile therefore remains the larger, deferred unity target.
- The reviewed correction and deterministic cache test were committed as
  provisional checkpoint `eeaf2c72c` (`fix: recycle level visibility
  sidecars`) and pushed to `codex/arm64-apple`. This is a recoverability and
  review boundary, not a compile, test or runtime acceptance claim.

The preceding published reading was 212,262 goal seconds. This in-progress
documentation reading was 217,521 seconds: 2 days, 12 hours, 25 minutes,
21 seconds cumulative, adding 5,259 seconds (1 hour, 27 minutes, 39 seconds).
The timer includes pauses/resumes, diagnostics, user-directed resource
correction and documentation; it is not a person-hour estimate.

The provisional-source prepublication reading was 218,058 seconds: 2 days,
12 hours, 34 minutes, 18 seconds cumulative. That adds 537 seconds (8 minutes,
57 seconds) from the interim reading and 5,796 seconds (1 hour, 36 minutes,
36 seconds) from the preceding published boundary.

- Documentation checkpoint `2951c459e` was then pushed. Draft PR #1 was
  verified open/draft at that exact head and its stale broad visual claim was
  replaced with the 268-error geometry defect and pending correction gates.
- Two publication routes were rejected without repository mutation: the first
  `gh pr create` implicitly targeted `upstream` rather than the fork, and the
  first REST body update sent an empty stdin document and returned HTTP 400.
  Explicit `--repo chrissotraidis/ctrpad` resolved the existing PR; a safely
  quoted REST field updated its description successfully.
- GitHub Actions is enabled but the private repository has no self-hosted
  runner. Official GitHub documentation identifies standard `macos-15` as an
  ARM64 M1 runner with 7 GB RAM for private repositories, consuming the
  account allowance and potentially billed minutes. The current CLI token
  cannot read billing allowance without expanding its `user` scope, so no
  workflow was created or triggered and no potential cost was assumed.
- Final resource observation remained unsuitable for a local unity compile:
  3,943 free 16-KiB VM pages (about 65 MB), 11.79 GB swap used, exactly one
  protected Simulator booted and no compiler process.

The publication-close reading was 218,564 seconds: 2 days, 12 hours,
42 minutes, 44 seconds cumulative. That adds 506 seconds (8 minutes,
26 seconds) from the prepublication reading and 6,302 seconds (1 hour,
45 minutes, 2 seconds) from the preceding published boundary.

- A later headroom audit began clean at pushed head `79f4b1bb2`, with one
  protected Simulator and no compiler. It measured 9,151 free 16-KiB pages
  (about 150 MB) and 11.69 GB swap used, still below the build boundary.
- Read-only Docker inspection found four unrelated healthy `buzz-prod`
  services, which were left untouched, plus goal-owned
  `ctrpad-i686-debug`. The latter ran no verifier: only `sleep infinity`, Xvfb
  and a stale diagnostic shell polling Xvfb. Source was mounted read-only and
  writable `/out` was a host bind mount, so `docker stop ctrpad-i686-debug`
  was recoverable and preserved all evidence.
- After that in-scope stop, the four unrelated containers remained running.
  Free pages settled near 5,864 (about 96 MB) and swap use near 11.66 GB,
  still unsafe for the large unity translation. No build was started and no
  unrelated app, service or protected Simulator state was changed.
- Final `docker ps -a` reported the stopped CTRPad container as exit 137: its
  unresponsive idle/Xvfb process group required stop-timeout escalation. With
  no verifier and host-mounted writable output, this lost no test evidence and
  remains restartable. The final readback was about 87 MB free and 11.60 GB
  swap used.

The post-cleanup timer reading was 218,822 seconds: 2 days, 12 hours,
47 minutes, 2 seconds cumulative, adding 258 seconds (4 minutes, 18 seconds)
from the publication-close reading.

The final blocker reading was 218,932 seconds: 2 days, 12 hours, 48 minutes,
52 seconds cumulative, 110 seconds (1 minute, 50 seconds) later.

### 2026-08-01 — Level-geometry regression accepted after safe resumed build

- The user explicitly resumed the active goal and confirmed the Simulator was
  unblocked. Both named Simulators were shut down for compilation. Every build
  ran at nice priority 15, one target at a time and one Ninja job at a time.
- Reconfigured exact head `4a4b148dd8d1`, rebuilt ARM64 desktop, ARM64
  ASan/UBSan, iOS Simulator and iPhoneOS products, and verified all four embed
  version `0.1.0-beta.7.1` plus that exact build ID. Their full hashes are in
  `docs/parity/2026-08-01-level-visibility-cache.md`.
- The focused visibility-sidecar test passed ordinary and sanitizer builds with
  exact marker `cache-recycle=targeted+range+all`. Both complete desktop suites
  passed 22/22; ASan/UBSan reported no finding.
- Recorded the iOS packaging boundary honestly: both ARM64 bundles compile and
  link, contain no retail media and install in Simulator, but their linker
  ad-hoc signature fails deep/strict resource sealing. Physical team signing
  and sideloadable-device acceptance remain open.
- Booted only disposable `CTRPad Import Negatives`; protected `CTRPad Import
  Validation` remained shut down and untouched. Installed the exact app and
  used fresh Computer Use state to observe complete legal/title/menu,
  **Race Today**, forest, building, ground, foliage, sky, kart, character and
  touch-overlay rendering. This directly closes the observed missing whole-
  course-geometry class without claiming every asset is pixel-perfect.
- The cold-launch active log has 42 lines and 28 periodic FPS samples. It
  contains zero `[CTR AssetRef]`, visibility-cache exhaustion, `ERROR` or
  `FATAL` lines. The former 268-line signature did not recur.
- Install/container migration and runtime preserved active BIN inode
  `111450682`, 605,698,800-byte size and modification time, plus save inode
  `111309627`, 6,016-byte size, modification time and SHA-256
  `6a01b0f5...619a`. An unnecessary all-fixture multi-gigabyte hash traversal
  was stopped rather than add load; retained identities and the established
  canonical BIN hash are the preservation evidence.
- Captured and visually reviewed both device pixels and the Simulator window.
  The 655-by-903 window image hashes to `5637af80...26a87` and clearly shows
  the textured forest scene; it remains local-only so retail-derived pixels do
  not enter the GPL source repository.
- Bounded termination closed the attached console, then the disposable device
  was shut down. Final state is zero booted Simulators. M7's visual regression
  gate is restored; physical iPad signing, performance, real multi-input play,
  completed-race and remaining lifecycle gates stay open. The goal remains
  active.

The resumed-runtime reading was 220,500 seconds: 2 days, 13 hours, 15 minutes,
0 seconds cumulative. The documentation-close reading was 221,029 seconds:
2 days, 13 hours, 23 minutes, 49 seconds cumulative. That adds 2,097 seconds
(34 minutes, 57 seconds) from the final blocker reading, including 529 seconds
(8 minutes, 49 seconds) for the closing evidence audit. Goal time includes
pauses/resumes and is not a build benchmark or person-hour estimate.

### 2026-08-01 — Current unsigned IPA reproduced; physical signing inventory unchanged

- Audited the distinction between the raw device-build linker signature and
  the repository's actual package path. `package-ios.sh` deliberately removes
  stale signature/profile state before validating and packaging; its signed
  branch applies a supplied profile, minimal entitlements, DER signing and
  strict verification.
- Fresh read-only inventory found zero valid Apple signing identities, no
  provisioning profiles in either standard Xcode location and no connected
  physical devices. Both Simulators remained shut down. No credential/device
  result was invented and no account/keychain/device state was changed.
- Packaged the exact accepted iPhoneOS executable embedding `4a4b148dd8d1`
  twice with the same normalized timestamp. Both unsigned IPAs hash to
  `fb684064...d7d6`; `cmp` passed byte-for-byte and `unzip -t` passed.
- The archive has exactly seven standard app/legal/install members. Extraction
  proves thin ARM64, iOS platform, minimum 15.0, exact version/build/SDL
  identity, and no retail-like file, runtime data directory, profile or
  `_CodeSignature`. The outputs remain local-only under `/tmp`.
- Current-source unsigned package reproducibility is accepted. A compatible
  Apple identity, matching provisioning profile and connected iPad remain the
  external dependency for signed physical installation; the goal stays active.

The package-result reading was 221,477 seconds: 2 days, 13 hours, 31 minutes,
17 seconds cumulative. The documentation-close reading was 221,583 seconds:
2 days, 13 hours, 33 minutes, 3 seconds cumulative. This adds 554 seconds
(9 minutes, 14 seconds) from the preceding 221,029-second documentation
boundary, including 106 seconds (1 minute, 46 seconds) for the package closeout.
Goal time includes pauses/resumes and is not a build benchmark or person-hour
estimate.

### 2026-08-01 — Current iPad portrait clipping corrected and accepted in Simulator

- Reopened the broad rotation/layout claim after the geometry-acceptance frame
  showed a portrait iPad shell containing a clipped `1376x1032` landscape
  controller, a large black lower region and no visible right action cluster.
  The restored forest/building/ground textures remained valid evidence for the
  preceding level fix; they were not mistaken for a complete layout result.
- Traced the mismatch to SDL's unconditional landscape-only runtime orientation
  hint. SDL intersects that hint with the target plist, so it preserved the
  intended iPhone landscape policy but incorrectly reduced iPad's declared
  four-orientation mask to landscape too.
- Changed the iOS hint to advertise portrait, upside-down portrait and both
  landscapes. The target plist remains authoritative: iPhone still intersects
  to both landscapes and iPad retains all four. No device-specific bridge,
  forced scene request, manual transform or duplicate layout path was added.
- Built candidate Simulator and iPhoneOS ARM64 products sequentially at nice
  priority 15 and one Ninja job with both Simulators shut down. Booted only
  disposable `CTRPad Import Negatives`; cold portrait now logged `1032x1376`
  and portrait/landscape/portrait showed a full renderer plus all controls.
- Committed and pushed the source correction as `2c78c040b`, then explicitly
  reconfigured and rebuilt both products. Exact Simulator and iPhoneOS hashes
  are `87188026...9c` and `3dc6e3c7...c1`; both are ARM64 and embed clean build
  identity `2c78c040bf2a` and version `0.1.0-beta.7.1`.
- Repeated the exact run on only the disposable Simulator. Cold portrait,
  landscape and returned portrait frames were visually reviewed from fresh
  Computer Use state; the return accessibility tree contained all 11 control
  identifiers. Exact screenshot hashes, dimensions and console evidence are in
  `docs/parity/2026-08-01-ios-orientation-hint.md`; retail-derived frames remain
  outside Git.
- Exact install/run/termination preserved the active BIN and canonical save
  identities. The file log had no AssetRef/cache/error/fatal/unbalanced line.
  One Foundation `NSMapGet(...): map table argument is NULL` diagnostic appeared
  after the first rotation in both candidate and exact runs, did not recur on
  return, and did not interrupt rendering, controls, FPS output or termination;
  it remains explicitly open as a Simulator-side diagnostic.
- Terminated the app and shut down the disposable device. Final state is zero
  booted Simulators; protected `CTRPad Import Validation` was never booted or
  modified. Simulator portrait clipping is accepted as corrected. Physical
  iPad orientation/window resizing, Stage Manager, performance and human
  multi-touch remain open, so M10 and the overall goal stay active.

The orientation documentation-open reading was 222,662 seconds: 2 days,
13 hours, 51 minutes, 2 seconds cumulative. The documentation-close reading
was 223,161 seconds: 2 days, 13 hours, 59 minutes, 21 seconds cumulative. That
adds 1,578 seconds (26 minutes, 18 seconds) from the preceding 221,583-second
published boundary, including 499 seconds (8 minutes, 19 seconds) for the
closing evidence audit. Goal time includes pauses/resumes and is not a build
benchmark or person-hour estimate.

### 2026-08-01 — User-media and GPL/release hygiene re-audited read-only

- Confirmed the ignored `ref/CTR/` folder contains the current 605,698,800-byte
  BIN/95-byte CUE pair and the older CCD/IMG/SUB set, with no media indexed by
  Git. The current pair is the already accepted 257,525-sector raw
  MODE2/2352 NTSC-U fixture; the old 314,702-sector CloneCD set remains the
  documented PAL negative fixture.
- Did not redundantly hash all 1.3 GB. The canonical BIN's exact identity has
  already passed import/runtime/save preservation; this pass checked current
  path, size, CUE contract, ignore coverage and prior evidence instead.
- Found no tracked retail-media, save, IPA, profile, certificate or private-key
  extension. Confirmed the GPL license, third-party notices, Installation
  Information, source build instructions and package exclusion logic remain
  tracked.
- `bash -n package-ios.sh` passed. CMake enumerated the four Apple configure
  presets, and the existing macOS directory enumerated all 22 tests with
  `ctest --show-only`. No compile or test execution is implied by those static
  checks.
- Corrected README's stale claim that live wrong-region UI was still open; the
  accepted rejection remains intact and inaccessible-provider/physical-device
  behavior remains open. Removed one duplicated M10 orientation phrase.
- Measured only 4,687 free 16-KiB VM pages, about 73 MB. Kept both Simulators
  shut down and started no compiler or broad hash traversal. A process audit
  found an idle Simulator app with a shut-down OpenRCT2 window; Computer Use
  selected its normal Quit menu item. Final state is zero Simulator processes
  and zero booted devices. A clean rebuild is deferred for headroom, not
  reported as failed or accepted.

The audit-open reading was 223,681 seconds: 2 days, 14 hours, 8 minutes,
1 second cumulative. The documentation-close reading was 223,944 seconds:
2 days, 14 hours, 12 minutes, 24 seconds cumulative. That adds 783 seconds
(13 minutes, 3 seconds) from the preceding 223,161-second published boundary,
including 263 seconds (4 minutes, 23 seconds) for the closing evidence audit.
Goal time includes pauses/resumes and is not a build benchmark or person-hour
estimate. The goal remains active.

### 2026-08-01 — Deterministic corresponding-source artifact accepted

- Rechecked the planned clean-build gate and found only about 58 MB free with
  more than 10.5 GB swap used. Kept zero Simulators open and started no
  compiler; advanced M11's release-source artifact instead.
- Added `package-source.sh`. It archives one exact clean Git commit, requires
  the build system, complete game/platform/include/tools and vendored SDL
  source, licenses, Installation Information, modification history and both
  packagers, then rejects retail/runtime/package/profile/certificate/key-like
  members and generated trees.
- Added README and iOS installation steps requiring the source archive and IPA
  to share one build identity. Precommit shell/help checks passed; a dirty-tree
  run failed before output as intended.
- Published implementation checkpoint `95dcb67f177b`. Its first exact archive
  completed, but the second execution was interrupted after moving its tarball
  and before filling the checksum. Rejected the incomplete pair and corrected
  publication order instead of counting it.
- Published correction `4091b602ab2a`, which prepares, hashes and verifies the
  archive/sidecar pair in private staging before destination publication and
  verifies it again afterward.
- Two separate nice-15 exact runs at the correction head produced byte-
  identical 17,482,944-byte, 3,233-member archives at SHA-256
  `d1c4b4fb...64df1`. `cmp`, both sidecars, gzip, tar, required-member and
  prohibited-member checks passed. An invalid ref failed before output.
- Fresh extraction contained no `.git`, preserved both executable packagers,
  passed both shell syntax checks, enumerated all four Apple CMake presets and
  contained no prohibited extension. Local artifacts remain outside Git.
- Final state remained zero Simulator processes and zero booted devices. Low
  headroom still defers a clean extracted-source compile; source packaging is
  accepted without overstating clean build, signing or device release.
- Extended the future-ingress policy in `21fb81296bd0` to Apple `.p8`/`.pfx`
  keys, alternate profile suffixes and certificate encodings; no accepted
  archive had contained one. Committed the first acceptance record as
  `71b68f118b18`, then repeated that fully documented head twice.
- The final pair is byte-identical: 17,488,503 bytes, 3,234 members, SHA-256
  `1681c458...53f3`. Both checksums, gzip/tar structure, expanded archive scan
  and fresh extraction smoke checks passed. Zero Simulators remained open.

The source-package documentation-open reading was 224,651 seconds: 2 days,
14 hours, 24 minutes, 11 seconds cumulative. The documentation-close reading
was 225,098 seconds: 2 days, 14 hours, 31 minutes, 38 seconds cumulative. That
adds 1,154 seconds (19 minutes, 14 seconds) from the preceding 223,944-second
published boundary, including 447 seconds (7 minutes, 27 seconds) for the
closing and final-current-head audit. Goal time includes pauses/resumes and is
not a build benchmark or person-hour estimate. The goal remains active.

### 2026-08-01 — Isolated signing-keychain mechanics accepted, Apple gate retained

- Repeated external inventory: zero valid code-signing identities, no standard-
  location profiles and no connected device. Headroom was still only about
  135 MB with 10.50 GB swap used, so no Simulator/compiler was started.
- Added optional `--keychain` support to `package-ios.sh` and Installation
  Information. It scopes valid-identity discovery and `codesign` to one already
  unlocked keychain without changing the user's default or search list.
- Candidate and exact missing-input tests rejected keychain-only invocation.
  A synthetic self-signed identity imported into the isolated keychain but
  remained untrusted; the real packager rejected it before output as intended.
- Recorded the OpenSSL 3 default-PKCS#12 incompatibility and successful legacy-
  container import. Did not authorize synthetic user trust or weaken the valid-
  identity gate.
- Synthetic CMS decoding accepted the intended iOS App ID/team/device/expiry.
  A separately bounded ad-hoc DER diagnostic passed deep/strict verification
  and preserved all four entitlement values while explicitly reporting no
  TeamIdentifier.
- Committed/pushed implementation `37a5e16760ba`. Two exact unsigned packages
  remained byte-identical, seven-member and retail/profile/signature-free at
  SHA-256 `582b8491...b929`.
- Deleted the temporary keychain. Final user search list retained only the
  login keychain; valid identities and trust settings remained empty. Computer
  Use also found no running security/authorization app. No local synthetic key,
  profile, IPA or signed-app artifact entered Git.
- Accepted isolated-keychain mechanics and fail-closed behavior only. A valid
  Apple identity/profile, real signed IPA and connected iPad remain open.

The isolated-keychain documentation-open reading was 225,723 seconds: 2 days,
14 hours, 42 minutes, 3 seconds cumulative. The documentation-close reading
was 225,928 seconds: 2 days, 14 hours, 45 minutes, 28 seconds cumulative. That
adds 830 seconds (13 minutes, 50 seconds) from the preceding 225,098-second
published boundary, including 205 seconds (3 minutes, 25 seconds) for the
closing evidence and cleanup audit. Goal time includes pauses/resumes and is
not a build benchmark or person-hour estimate. The goal remains active.

### 2026-08-01 — Exact current-head Apple matrix and unsigned IPA refreshed

- Resumed from exact clean GitHub head
  `bbb17478c76d7f8868ebfd3a6bf1b6d4cc90bfa5`; local, upstream and draft-PR
  heads matched before the matrix.
- Kept zero Simulator application processes and zero booted devices for the
  entire checkpoint. Configures, builds, tests and packages ran sequentially
  at nice 15 with one build/test job. Sampled `memory_pressure` stayed between
  42% and 53% system-wide free and always reported zero throttled pages.
- Reconfigured/rebuilt the exact ordinary macOS ARM64 product. The thin binary
  hashes to `279a7965...a4aea2`, embeds `bbb17478c76d`, and passed 22/22 CTests
  in 3.52 seconds (3.69 seconds outer wall).
- Rejected the nonexistent `macos-arm64-sanitizers` preset before compilation,
  then reconfigured the established explicit ASan/UBSan directory instead.
  Its thin binary hashes to `5172e794...a28d`, embeds the same identity, and
  passed 22/22 in 6.53 seconds with no sanitizer finding.
- Reconfigured/rebuilt both iOS SDK products. Simulator SHA-256 is
  `47984fe3...1333`; iPhoneOS SHA-256 is `7229ffd7...2490`. Both are thin
  ARM64, iOS 15+, UIKit/GLES, and embed the current build/SDL markers.
- Confirmed the applications intentionally contain the executable, plist and
  three distribution documents, not retail media. The user's BIN is imported
  to Documents at runtime; absence from the bundle is not evidence for the
  already-corrected visibility-cache regression.
- Retained the known signing boundary: strict verification rejects the
  Simulator's linker-ad-hoc resource state, and the device app is unsigned.
  Neither was relabeled as a real Apple-signed bundle.
- Packaged the exact device app twice with one fixed source epoch. The
  1,449,260-byte seven-member IPAs compare byte-for-byte and hash to
  `05ff4601...c97dab`; both sidecars and ZIP validation passed, with no retail
  media, profile, signature or save/container data. Temporary packages were
  removed after recording their reproducible identity.
- This accepts a bounded current-head in-place refresh. Fresh extracted-source
  build, macOS app-bundle refresh, Apple signing, real device install and all
  physical-iPad behavior remain open.

The matrix evidence-open reading was 226,759 seconds: 2 days, 14 hours,
59 minutes, 19 seconds cumulative. The documentation-close reading was
227,262 seconds: 2 days, 15 hours, 7 minutes, 42 seconds cumulative. That adds
1,334 seconds (22 minutes, 14 seconds) from the preceding 225,928-second
published boundary, including 503 seconds (8 minutes, 23 seconds) for the
closing audit and first documentation commit. Goal time includes
pauses/resumes and is not a build benchmark or person-hour estimate. The goal
remains active.

### 2026-08-01 — Simulator stability gate reopened; logging and accessible input corrected

- The user explicitly made a stable, visibly correct Simulator run with robust
  logging a prerequisite for physical-iPad work. This supersedes any inference
  that a successful compile, launch or earlier bounded visual pass was enough.
- Rebuilt exact clean baseline `43245107c279` with one job at nice 15 while all
  Simulators were shut down. Its thin ARM64 executable hashed to
  `d37cdf88...c493`; a strictly verified ad-hoc-signed install copy hashed to
  `5f22d274...b176a` after the expected signature transform.
- Booted only disposable device `CTRPad Import Negatives`; the protected
  validation device stayed shut down. Update installation preserved the
  complete 605,698,800-byte imported BIN. The observed title/menu, character,
  Crash Cove list/preview/fly-in/grid/live-lap and pause frames were coherent,
  and the 88-line log had no AssetRef/cache/error/fatal/unbalanced marker.
- Did not accept that run. The software-rendered game commonly sustained only
  about 4-8 FPS, several short keyboard/accessibility actions needed repeats,
  and the 3,704-byte production log overwrote its predecessor while providing
  no timestamps, severity, session identity or input correlation.
- Changed the logger to prefix persistent entries with UTC, elapsed session
  time and severity; flush every entry; report exact build/target/path/session
  metadata; and rotate the last four sessions as `.1` through `.4`. Added
  bounded mapped touch and keyboard event lines. Console presentation remains
  human-readable and system-console diagnostics remain external.
- Corrected UIKit accessibility activation for game buttons by issuing the
  same press/release state transitions used by touch. Utility controls were
  deliberately left unchanged. The fix did not add a hidden text-input route
  or remap retail controls.
- The macOS one-job build completed in 107.14 seconds with the established 32
  warnings; all 22 CTests passed. Two isolated logger runs retained current and
  prior logs at SHA-256 `ae03ee0...058d` and `bb937fcf...05ac`; both renderer
  pixel tests passed hash `851169f2644a1675`.
- A dirty-source iOS diagnostic build completed in 36.47 seconds. It exercised
  the correction, but CMake still embedded the configured clean
  `43245107c279` identity, so neither its executable nor runtime is exact
  post-commit evidence. This limitation is recorded rather than hidden.
- In the sole-device diagnostic, one accessible Start skipped presentation,
  one accessible Cross opened Adventure, later Cross entered the character and
  name screens, and Cross inserted `A` into the retail name field. The
  10,396-byte log correlated exact down/up masks and retained the exact baseline
  as `.1`; it had no AssetRef/cache/error/fatal/unbalanced marker. Home/resume
  preserved the PID and name-entry state while logging the four lifecycle
  transitions.
- Shut down both CTRPad devices, the app and Simulator after the run. A clean
  post-commit Simulator build still must cover broader level/scene churn,
  keyboard reliability, rotation/Home/resume, graphical and log scans, and a
  usable stability-test frame rate before physical-device work resumes.

The checkpoint evidence-open reading was 232,171 seconds: 2 days, 16 hours,
29 minutes, 31 seconds cumulative. An in-progress documentation reading was
232,558 seconds: 2 days, 16 hours, 35 minutes, 58 seconds cumulative. Final
checkpoint close and publication times will be recorded after exact clean-build
acceptance. Goal time includes pauses/resumes and is not a build benchmark or
person-hour estimate. The goal remains active.

### 2026-08-01 — First exact stability replay rejected; retail-consumer latch implemented

- Committed and pushed logging/accessibility checkpoint `7bcc51a790a1`, then
  freshly configured both Apple graphs with every Simulator shut down. The
  exact macOS binary SHA was `be060628...47795`; its one-job build took 56.43
  seconds and passed 22/22 CTests in 2.97 seconds. The exact iOS Simulator
  binary SHA was `3b646623...d1d8`; its one-job build took 67.04 seconds. A
  strictly verified ad-hoc test copy transformed to `13cca464...a4d5`.
- Booted only `CTRPad Import Negatives`. Its migrated container retained the
  complete 605,698,800-byte BIN. Exact sessions rotated current plus three
  predecessors as intended; the 9,370-byte Adventure log hashed to
  `8ec554ad...d8687` and the 5,685-byte follow-up to `aa12c685...9353`.
- Visually observed coherent presentation/main menu, Adventure New/Load,
  stored profile, character/name screens, hub, kart, Aku Aku, scenery, HUD,
  minimap, two orientations and same-PID Home/resume. Both exact logs had zero
  AssetRef/cache/error/fatal/unbalanced markers.
- Rejected the run because logged single `S` and `K` actions did not reliably
  reach the retail menu, while touch did. Runtime remained roughly 4-8 FPS and
  slow transitions could let a subsequent action land on the next screen.
- Identified the boundary mismatch: the two-snapshot latch expired on native
  VSync, but retail input is consumed later by `GAMEPAD_ProcessHold`. Quick
  keyboard and touch pulses now remain latched until that retail consumer
  samples the packet, which then logs and acknowledges the exact masks.
- Updated the input self-test to read quick C/Right, K/D, touch chord and
  Circle taps repeatedly before acknowledgement and require immediate neutral
  afterward. Its isolated CTest passed with marker
  `tap-latch=c+right until-retail-poll`.
- Terminated the app, shut down the sole device and quit Simulator before the
  correction build. Exact post-fix compile, full tests and live replay remain
  required; the Simulator and physical-device gates stay open.

### 2026-08-01 — Fresh extracted corresponding source built and tested

- Packaged documented head `34ea4415cda8` into a fresh temporary directory.
  Its archive was valid and retail-free, but inspecting its no-`.git` CMake
  path showed the app would embed `unknown-dirty`. Rejected the expensive
  compile before relying on a wrong source identity.
- The first independent sidecar check also ran from the wrong working
  directory and could not resolve its basename-relative archive. Repeating
  beside the sidecar passed; this was a command-location error, not corrupt
  output.
- Added a guarded source identity path in `4673e1f9fcab`: exact source-root Git
  remains authoritative and dirty-aware; generated `CTRPad-source-<12-hex>`
  roots recover the packager identity without `.git`; renamed roots can supply
  a validated 12–40-hex commit. Parent Git repositories are ignored.
- A dirty-checkout candidate retained `34ea4415cda8-dirty`. A mismatched
  12-hex override and non-hex override each exited 1 before generating a build
  graph. Updated README and Installation Information with the extracted-root
  procedure.
- Committed and pushed the implementation before the long exact test. Two
  exact source archives from `4673e1f9fcab` compare byte-for-byte: 17,505,454
  bytes, 3,236 members, SHA-256 `5cdbbf9d...1a2cd`. Both sidecars and archive
  validation passed; retail/runtime/package/profile/key material was absent.
- Fresh extraction had no `.git`, passed both shell syntax checks and listed
  all four Apple presets. Its configure emitted exact archive-root identity
  `4673e1f9fcab` and completed in 73.09 seconds.
- The single-job nice-15 build compiled all 242 targets in 209.27 seconds,
  repeated 32 established warnings and linked a thin ARM64 binary at SHA-256
  `a645cdec...ade2`. It reported version
  `0.1.0-beta.7.1 (4673e1f9fcab)` and passed 22/22 CTests in 3.06 seconds
  (3.29 seconds outer wall).
- Recorded rather than hid vendored SDL's no-Git diagnostic
  `SDL-3.4.10-HEAD-HASH-NOTFOUND`; CTRPad's app identity is exact and the
  hashed archive contains all SDL source.
- A second no-`.git` extraction renamed to `renamed-source` accepted the full
  explicit commit, emitted its dedicated identity diagnostic and generated
  exact compile ID `4673e1f9fcab`. A redundant second unity build was skipped.
- Kept zero open Simulators and zero booted devices. Closing memory pressure
  reported 48% system-wide free and zero throttled pages. Deleted about 279 MiB
  of task-owned temporary archives, extracted trees and build output by exact
  path; repository and retail data were untouched.
- This accepts fresh extracted-source build/test on the development host, not
  an independently provisioned clean Mac, refreshed app/iOS products, Apple
  signing or physical-iPad behavior.

The extracted-source evidence-open reading was 228,335 seconds: 2 days,
15 hours, 25 minutes, 35 seconds cumulative. The documentation-close reading
was 228,550 seconds: 2 days, 15 hours, 29 minutes, 10 seconds cumulative. That
adds 1,288 seconds (21 minutes, 28 seconds) from the preceding 227,262-second
published boundary, including 215 seconds (3 minutes, 35 seconds) for the
closing audit and first acceptance-documentation commit. Goal time includes
pauses/resumes and is not a build benchmark or person-hour estimate. The goal
remains active.

### 2026-08-01 — Exact keyboard consumption and two-track graphics accepted; full Simulator gate stays open

- Published retail-consumer input checkpoint `ba80d153ae55` and backed it up
  to `origin/codex/arm64-apple` before the live acceptance run.
- Compiled with every Simulator shut down, nice 15 and one job. Exact macOS
  and iOS Simulator thin ARM64 executables hash to `55070327...c2ed2b` and
  `6f5f7152...eb923`; builds took 67.25 and 69.93 seconds. The macOS suite
  passed 22/22 in 2.41 seconds.
- Booted only the disposable import-negative device. Update installation
  retained the complete 605,698,800-byte retail image and 6,016-byte save.
- Used only the published keyboard map to enter Time Trial, select Crash and
  Crash Cove, skip the fly-in, drive, pause/resume, open Change Level, select
  Roo's Tubes and reach its course. All 20 key-down entries have 20 matching
  retail-poll consumption entries; single quick actions no longer vanished
  between native VSync and `GAMEPAD_ProcessHold`.
- Inspected coherent title/menu, character portraits, Crash Cove and Roo's
  Tubes preview/fly-in/grid/course/HUD/minimap/banner pixels. Rotation and an
  explicit Home/resume retained PID `65296`, the level state and graphics.
- Preserved a fully flushed 128-line / 13,642-byte current log at SHA-256
  `fa832418...bbcc1` plus four exact prior sessions. All five scan clean of the
  known asset/cache/app-error/fatal/unbalanced/assert/signal/crash markers.
  The unified log contains five framework-level CFBundle/CoreAudio Simulator
  limitations and no new CTRPad crash report.
- Kept the full gate open. Sixty-three FPS samples range 4.61-22.80 and average
  7.37, with the late run near 4.61-5.30 FPS. Two tracks do not prove every
  level/effect. Performance profiling, an observed correct-ID shutdown and
  broader graphical churn are required before physical-device work.
- Corrected a pushed cleanup interpretation: the command targeted
  `com.chrissotraidis.ctrpad`, but the built plist identifier is
  `io.github.chrissotraidis.ctrpad`. `found nothing to terminate` was a wrong-
  target command result, not evidence of a crash or unexplained app exit. The
  subsequent bounded RunningBoard query was unnecessary. The one disposable
  device was shut down; final state was both devices off with no game or
  Simulator GUI process consuming the host.

The exact post-fix documentation-open reading was 236,181 seconds: 2 days,
17 hours, 36 minutes, 21 seconds cumulative. The pre-publication verification
reading was 236,875 seconds: 2 days, 17 hours, 47 minutes, 55 seconds, adding
694 seconds (11 minutes, 34 seconds) for the closing documentation/audit and
4,317 seconds (1 hour, 11 minutes, 57 seconds) from the previous 232,558-second
in-progress ledger reading. Two exact 3.5 MB temporary signed-app copies were
deleted; the final log and visual evidence remain. Evidence commit
`b1663fe5b62b454e4098fa84da42028ca6fb0ce2` was then pushed and confirmed as
the draft PR head. The post-push publication reading was 236,994 seconds: 2
days, 17 hours, 49 minutes, 54 seconds, adding 119 seconds (1 minute, 59
seconds) after the pre-publication audit. Goal time includes pauses/resumes and
is not a build benchmark or person-hour estimate. The goal remains active.

### 2026-08-01 — Staged presentation removes one Simulator bottleneck; gate stays open

- Extended the opt-in frame profiler with packed-VRAM restore/present timing
  and per-frame renderer draw/split/vertex/semitransparent counters. The
  unoptimized diagnostic localized Crash Cove's 230.729-ms average frame to
  137.739 ms of split submission plus 47.201 ms of final presentation.
- Replaced host-resolution integer VRAM unpacking with logical-resolution RGBA
  resolve followed by nearest framebuffer blit. Retained the old direct path as
  a 2× presentation oracle; every captured byte matched at present hash
  `a7798c5a6ddee965`, while the established logical hash remained
  `851169f2644a1675`.
- Built sequentially with every Simulator closed, nice 15 and one job. The
  macOS build took 68.06 seconds; the focused pixel test passed and the full
  suite passed 22/22 in 5.66 seconds. The iOS Simulator build took 74.03
  seconds, linked thin ARM64, required `_glBlitFramebuffer`, repeated only the
  32 established warnings and passed strict signing verification in a
  disposable copy.
- Booted only `CTRPad Import Negatives`; the protected device stayed off. Used
  the published keyboard route to reach Crash Cove and inspected coherent
  logo/menu, character, track, ghost, fly-in and grid/race frames including
  kart, lights, banner, HUD, minimap, track, cliffs, sky, water and touch
  controls. No retail-derived capture was committed.
- Preserved 3,000 optimized timing frames at SHA-256 `e1036716...e034` and a
  99-line / 11,145-byte app log at `9a0b455e...603a`. Correct-ID explicit
  termination succeeded. Five retained app logs had no known
  asset/cache/application-fault marker; unified logging held only the same five
  CFBundle/CoreAudio Simulator limitations and no new CTRPad diagnostic.
- Crash Cove presentation fell from 47.201 to 7.737 ms (83.6%), work from
  198.501 to 150.674 ms (24.1%), and total from 230.729 to 181.024 ms (21.5%).
  This is a real improvement but still only about 5.52 FPS average and roughly
  5.4× the 30-FPS frame budget. Split submission remains 131.012 ms across an
  average 116.64 splits and 72.64 semitransparent splits. The Simulator and
  physical-device gates remain open pending safe batching/state reduction,
  exact post-commit replay and broader scene churn.
- Shut down both named devices and quit Simulator before documentation. The
  evidence-analysis goal reading was 239,829 seconds: 2 days, 18 hours, 37
  minutes, 9 seconds cumulative, 47 minutes 15 seconds after the preceding
  published boundary. Goal time includes pauses/resumes and is not a build
  benchmark or person-hour estimate. The goal remains active.

Full evidence is in
`docs/parity/2026-08-01-simulator-renderer-profile.md`.

### 2026-08-01 — Live exact viewer retained; one-pass semitransparency prototype under review

- Rebuilt and tested published checkpoint `ff26c0815a04` with both Simulators
  closed, then installed a strictly verified disposable signed copy as an
  update on the sole disposable device. The protected validation device stayed
  shut down. The running log identifies the exact iOS build and Apple Software
  Renderer; the imported retail image, extracted assets and saves stayed in
  the existing data container.
- Left the exact viewer open at the user's request. Computer Use inspection
  showed a coherent Crash Cove lap with kart, terrain, HUD, minimap and the
  touch overlay. The live log records both keyboard and touch masks reaching
  the retail-poll consumer. At the 1,553-second snapshot it contained 130
  lines / 13,371 bytes at SHA-256 `c7e02716...6816`, with no targeted
  asset/texture/cache/application-fault marker. A later view showed `9:59.99`
  while FPS lines continued; `UI_DrawRaceClock` intentionally caps ordinary
  non-seven-lap displays at that value, so this was not labeled a freeze.
  It was closed naturally after about 37 minutes, 27 seconds; the final exact
  log contained 166 lines / 16,467 bytes at SHA-256 `d07493d5...0f6`, still
  without a targeted renderer, asset, cache or application fault.
- Did not reinterpret visual coherence as Simulator acceptance. Sustained
  race readings settled around 6 FPS, confirming that the software renderer is
  still far outside the 30-FPS stability gate even after the presentation
  optimization.
- Audited the next dirty prototype against the measured 72.64 average
  semitransparent splits. It uses coherent
  `GL_EXT_shader_framebuffer_fetch` on GLES to apply PS1 STP-aware average,
  add, subtract and quarter-source equations in one ordered draw while
  retaining the established two-pass path everywhere else. A new perf counter
  identifies the splits that actually take the optimized path.
- Strengthened the renderer pixel oracle to cover all four equations, an exact
  mixed-STP/non-STP bilinear boundary and the GLES/portable-fallback
  comparison. Static review corrected generated-shader dependency order and
  preserved the capability result before
  renderer shutdown so the eventual iOS marker cannot falsely report fallback.
- Khronos revision 8 confirms that the coherent extension observes previous
  overlapping samples in API primitive order, permits ES3 `inout` fragment
  outputs and remains orthogonal to fixed-function blending. The current Apple
  device and Simulator SDK headers expose the extension. These are design
  prerequisites, not runtime proof.
- The prototype remains deliberately uncommitted and uncompiled while the
  exact game is open for user inspection. It must pass desktop fallback pixels,
  all 22 CTests, iOS GLES compilation, an enabled-path pixel comparison, a
  one-Simulator Crash Cove profile and broader graphical/log churn before it
  can be accepted or pushed.

The in-progress goal reading was 242,344 seconds: 2 days, 19 hours, 19 minutes,
4 seconds cumulative, 2,515 seconds (41 minutes, 55 seconds) after the prior
239,829-second evidence reading. Goal time includes user inspection and
pauses/resumes; it is not a build benchmark or person-hour estimate. The goal
remains active.

### 2026-08-01 — Coherent one-pass PS1 semitransparency passes its dirty gate

- Closed the exact viewer before compiling. Every build used nice 15, one job,
  two shut-down named devices and a closed Simulator GUI. The desktop fallback
  pixel oracle covers all four blend equations, mixed bilinear STP/non-STP,
  masks, feedback and staged presentation; all 22 native CTests passed.
- Rejected three incomplete self-test states. iOS created a 1032×1376 surface
  despite the requested 64×32 test window, so dynamic checked host buffers
  replaced the invalid fixed assumption. Representative pixels then hid a
  complete-buffer mismatch, so forced two-pass and fetch draws now run in the
  same GLES context. That exposed a real `(21,7)` bilinear edge divergence;
  the one-pass discard threshold was corrected to match both fallback passes.
- Final hashes remained `851169f2644a1675` for the logical renderer,
  `f6dc5a2e558bc7b5` for both blend paths, and `a7798c5a6ddee965`
  for desktop staged presentation. The iOS oracle reported
  `blend-oracle=match`, `framebuffer-fetch=enabled` and actual
  `present=resolve+blit@1032x1376`.
- The final macOS incremental build took 75.79 seconds and the 22-test suite
  took 1.90 seconds. The iOS incremental build took 65.07 seconds. Both
  repeated only the established 32 warnings. The unsigned and disposable
  signed thin-ARM64 executable hashes were `76473dce...a54` and
  `ff1a87b4...a08`; strict/deep signing verification passed.
- Booted only the disposable device, installed the signed dirty app as an
  update and enabled Simulator keyboard capture. `S K K K K I` reached live
  Crash Cove. Repeated K and paired D/K taps moved Crash from the grid to the
  coastal fence. Menus, character model/portraits, track preview, ghost prompt,
  fly-in, kart, lights, HUD, banner, cliffs, horizon, animated water, fences
  and touch overlay remained coherent. Retail input consumption was logged.
- Explicitly terminated the correct bundle, flushed 5,172 timing rows and a
  173-line / 18,813-byte app log, shut down the sole disposable device and
  confirmed the protected device stayed off. The CSV hashes to
  `f8fbef54...ee5`; the log hashes to `096615ca...c76`; targeted fault count is
  zero. Computer Use lost its app window after device shutdown, so a normal
  SIGTERM closed only the already-verified Simulator GUI PID.
- In matched Crash Cove geometry (123 splits, 79 semitransparent splits,
  5,405.94 split vertices), fetch removes exactly 79 draws: 205 to 126. Total
  time falls 187.614 to 172.324 ms (8.15%), split submission 135.412 to
  127.764 ms (5.65%), and reciprocal throughput rises 5.33 to 5.80 FPS. This
  remains about 5.17× the 30-FPS budget, so Simulator and physical-device gates
  stay open pending exact post-commit replay and more submission reduction.

Full dirty evidence and rejected-attempt chronology are in
`docs/parity/2026-08-01-ios-framebuffer-fetch.md`.

The evidence-analysis goal reading was 245,597 seconds: 2 days, 20 hours,
13 minutes, 17 seconds cumulative, 3,253 seconds (54 minutes, 13 seconds) after
the prior 242,344-second boundary. Goal time includes builds, rejected attempts,
user inspection and pauses/resumes; it is not a build benchmark or person-hour
estimate. The goal remains active.

### 2026-08-01 — Pushed and exact-replayed coherent framebuffer fetch

- Committed the reviewed implementation and complete dirty chronology as
  `d3b5bd410` and pushed `codex/arm64-apple`. Confirmed the existing draft PR
  remains open at `chrissotraidis/ctrpad#1`; an initial unqualified `gh` query
  resolved the upstream remote, so the final query named the downstream repo
  explicitly rather than creating a duplicate PR.
- With both devices and the GUI off, exact macOS configure/build took 2.00 /
  65.03 seconds and repeated only 32 established warnings. Executable
  `1a15c334...8c44` embeds `d3b5bd410e9a`; the pixel hashes remained logical
  `851169f2644a1675`, blend/fallback `f6dc5a2e558bc7b5`, presentation
  `a7798c5a6ddee965`; all 22 tests passed in 2.54 seconds.
- Exact iOS configure/build took 1.78 / 77.44 seconds with the same warnings.
  The thin-ARM64 unsigned executable hashes to `565ff441...8b8`; an isolated
  signed copy hashes to `80d3b864...785` and passed strict/deep verification.
  Its enabled GLES oracle matched every fallback byte on the actual 1032×1376
  surface. Correct-ID termination ended the resident UIKit self-test shell.
- Booted only the disposable device and exact-replayed `S K K K K I`, followed
  by K gas and D/K turn taps. Crash Cove remained coherent from logo/menu
  through motion against the inside cliff; exact retail-poll input was logged.
  The 285-second run flushed 2,124 frame rows and a 130-line log. CSV SHA is
  `1c74e036...04b`, log SHA is `f517e967...fbf`, targeted faults are zero.
- The exact CSV has 976 Crash Cove frames at 6.92 reciprocal FPS overall and
  7.45 over the final 300. Every semitransparent split in that window records a
  framebuffer fetch. This accepts the exact implementation route but still
  rejects the 30-FPS Simulator product gate.
- Verified the retail BIN retained inode `111450682`, 605,698,800 bytes and
  canonical SHA `f780bf23...07c0`; the save retained inode `111309627`, 6,016
  bytes and canonical SHA `6a01b0f5...19a3`. Quit the GUI, confirmed both
  devices off and deleted six explicit disposable signed-app directories. No
  product, app data, build result or repository file was removed.
- Transparently retained two harmless command failures in the detailed record:
  an analysis filter requested an absent exact 126/79 state and divided by
  zero, then the corrected query used actual groups; the first best-effort
  cleanup shutdown line had one extra UDID digit, then the verified command and
  final status succeeded.

The exact-acceptance goal reading was 247,009 seconds: 2 days, 20 hours,
36 minutes, 49 seconds cumulative, 1,412 seconds (23 minutes, 32 seconds) after
the dirty evidence boundary. Goal time includes publication, builds, Simulator
boot/oracles/live replay, cleanup and analysis; it is not a build benchmark or
person-hour estimate. The goal remains active.

### 2026-08-01 — Same-state coherent-fetch batching passes its dirty gate

- Preserved logical PS1 split generation and render traces, but joined adjacent
  contiguous framebuffer-fetch splits only when their complete host state is
  identical. Added a separate merged-split counter so logical work and removed
  host calls remain distinguishable.
- Confirmed against Khronos revision 8 that coherent fetch observes previous
  overlapping samples in API primitive order. Expanded the byte oracle with two
  identical overlapping quads: portable fallback must issue 12 draws, enabled
  GLES must issue 5, and the STP pixel must show the blend applied twice.
- With both devices and Simulator off, the final low-priority one-job desktop
  rebuild took 69.74 seconds, repeated 32 established warnings and produced
  ARM64 executable `7b670e2f...1a53`. All 22 tests passed in 3.07 seconds;
  logical/presentation hashes stayed fixed and the new blend hash is
  `0c0d08324ae06c35`.
- The dirty iOS ARM64 executable hashes to `e4c29176...3e9`; its isolated
  strict-verified ad-hoc copy hashes to `5c236fb5...c87a`. The actual-surface
  oracle reported fallback/active 12/5 calls, byte match, enabled fetch and the
  established `172d49a34571b64c` presentation hash.
- Booted only the disposable device, used `S K K K K I` plus gas/turn inputs,
  and inspected coherent menu, character, track, grid, coast and canyon/palm
  scenes. The advancing log proved the slow view was not stuck.
- Correct-ID termination flushed 4,332 frames plus header at
  `eedb1181...90a` and a 224-line log at `f0d6035c...955`; targeted faults are
  zero. Both devices and GUI are off. Retail BIN/save inode, size and canonical
  hashes remain unchanged.
- In 428 frames matched to 151 exact unbatched frames, the same 119 logical and
  78 semitransparent splits fall from 122 to 66 calls (-45.90%). Total time
  falls 171.223 to 164.187 ms (-4.11%) and reciprocal throughput rises 5.84 to
  6.09 FPS. This is still about 4.93 times budget; exact post-commit replay,
  wider churn and all device gates remain open.
- Retained harmless routes: an active-file partial tail, the expected absent
  GPU trace after a non-trace launch, a noisy size-based preservation search,
  Computer Use's fresh-state retry, and a redundant shutdown that found the
  device already off.

Full chronology and hashes are in
`docs/parity/2026-08-01-ios-framebuffer-fetch-batching.md`.

The dirty-evidence goal reading was 248,826 seconds: 2 days, 21 hours,
7 minutes, 6 seconds cumulative, 1,817 seconds after the prior boundary. Goal
time includes user viewing, profiling, review, compilation and documentation;
it is not a build benchmark or person-hour estimate. The goal remains active.

### 2026-08-01 — Published and exact-replayed framebuffer-fetch batching

- Committed the reviewed implementation/history as `13f260cb8a0d` and pushed
  `codex/arm64-apple`. Local, remote-tracking and draft-PR heads converged on
  the same revision; no duplicate PR was opened.
- With both devices/GUI off, exact macOS configure/build took 1.60 / 70.34
  seconds, repeated 32 established warnings and produced ARM64
  `b36267d0...02b`. All 22 tests passed in 2.62 seconds with the exact 12-draw
  fallback oracle and established logical/presentation hashes.
- Exact iOS configure/build took 1.13 / 68.54 seconds with the same warnings.
  Unsigned/signed hashes are `1941ddad...4186` / `f9cfc625...9502`; strict
  signing passed. The actual 1032x1376 GLES oracle matched every fallback byte,
  reduced 12 draws to 5 and retained `172d49a34571b64c` presentation.
- The first boot/install command returned a session during CoreSimulator boot
  settling; a diagnostic follow-up briefly overlapped two identical installs.
  Work paused until both exited. The data-container UUID remapped, but all
  historical data, retail/save inodes and canonical hashes remained intact.
- A route launched too early from copyright reached retail name entry. Its
  screenshot helper then referenced a non-persistent variable. Fresh screen
  inspection proved the route result; source review identified P/P as the
  retail cancel path. The verified menu route then selected Time Trial, Crash,
  Crash Cove, no ghost and the race grid screen by screen.
- The 799.006-second exact run flushed 5,291 frames plus header at
  `50fffaed...798`, a header-only GPU file at `af0f3466...91f6`, and a 117-line
  app log at `f7a541fb...9529`; targeted faults are zero. Exact input masks were
  consumed, bounded menu/name/character/track/race assets stayed coherent, and
  the timer advanced to 0:33.50.
- The exact 119/78 logical state again falls from 122 to 66 calls (-45.90%)
  across 145 matched frames. Total averages 165.879 ms versus 171.223 ms, but
  draw/split bucket noise is flat; the structural call reduction and byte
  oracle, not a claimed exact-stage speedup, accept the route. Final 300 frames
  average 5.98 FPS.
- Command-Q closed the GUI and shut down the disposable device even though the
  preceding accessibility diff still displayed capture text. Direct checks
  proved both named devices and the GUI off. The explicit temporary signed-app
  copy was deleted after its hash/signature evidence was recorded; builds,
  installed app, data and source remain.
- A malformed multi-file cleanup-doc patch applied nothing. The following
  read-only context query accidentally put a numeric inode in shell backticks,
  produced `command not found`, then still returned the requested context. The
  corrected literal patch changed only these history files.

The exact-acceptance reading was 250,402 seconds: 2 days, 21 hours, 33 minutes,
22 seconds cumulative, 1,576 seconds after the dirty boundary. Full details are
in `docs/parity/2026-08-01-ios-framebuffer-fetch-batching.md`. The checkpoint is
exact-accepted; the overall goal remains active.

### 2026-08-01 — Interactive one-simulator Crash Cove handoff

- Confirmed a clean worktree at pushed revision `6de7f17cd4cd`; both named iPad
  simulators initially were shut down. Booted only disposable `CTRPad Import
  Negatives`; protected `CTRPad Import Validation` remains shut down.
- Launched installed app `io.github.chrissotraidis.ctrpad` as PID 19987 and left
  Simulator open for user inspection. Computer Use verified copyright, title,
  main menu, character select, Crash Cove track select, no-ghost prompt,
  loading preview and the live Time Trial starting grid.
- Verified keyboard control in the actual retail path: Down selected Time Trial
  and C accepted the subsequent choices. The app log recorded and retail poll
  consumed scancodes 81 and 6 with masks `0x0040` and `0x4000` respectively.
- The visible route has character, kart, track geometry/textures, HUD, minimap,
  transparency and touch overlay present. No missing-asset screen was observed
  on this bounded route, but the live log still reports roughly 5–10 FPS in the
  heavier scenes; the Simulator stability/performance gate therefore remains
  open and no physical-device work is authorized yet.
- Preserved harmless diagnostics: boot readiness returned before the first
  compound command reached launch, so state was rechecked and launch was run
  separately; one screenshot emit referenced a helper unavailable in the next
  Computer Use call and fresh state recovered it; a read-only mapping search
  included absent `src/`, producing only an `rg` path warning.

The handoff goal reading was 251,875 seconds: 2 days, 21 hours, 57 minutes,
55 seconds cumulative, 1,473 seconds after exact batching acceptance. Goal time
includes this interactive launch and inspection; it is not a build benchmark
or person-hour estimate. The overall goal remains active.

### 2026-08-01 — Rejected unified fetch-state batching after live profiling

- Prototyped renderer-only per-primitive blend/STP/mask state after logical
  trace capture. The first route used one dynamically branched 4/8/16-bit
  shader; the second retained three texture-format-specialized shaders.
- Both candidates preserved the 20-byte vertex ABI, exact logical hash
  `851169f2644a1675`, blend hash `0c0d08324ae06c35`, iOS presentation hash
  `172d49a34571b64c` and desktop fallback. Enabled iOS reduced the ordered
  overlap fixture from 12 to 1 draw; the mixed-state fixture used 2 draws in
  the dynamic route and 4 in the specialized route.
- Only the disposable simulator booted. Computer Use verified coherent
  trophy/menu, character, track, no-ghost, loading and Crash Cove grid scenes;
  no bounded-route asset loss appeared. Correct-ID termination finalized the
  specialized 3,216-line CSV at `994a1f79...c2f` and 69-line log at
  `ce135e14...810`; the targeted fault scan is empty.
- The exact 119-split / 78-semitransparent published state averages 66 calls,
  165.879 ms total and 136.514 ms triangle time. Dynamic state batching cut
  calls to 26 but regressed to 248.567 / 204.207 ms. Format specialization
  recovered only to 52 calls and 236.441 / 190.911 ms: 42.54% slower total
  than published despite 21.21% fewer calls.
- Rejected both candidates, restored all four source files through
  `apply_patch`, rebuilt at nice 15 / one job with the same 32 warnings, passed
  22/22 tests in 3.18 seconds and independently passed the desktop pixel oracle
  in 1.27 seconds. No renderer diff remains.
- Retail BIN/save inodes, sizes and canonical hashes are unchanged. Both
  devices and Simulator GUI are off. Two explicitly verified task-owned signed
  copies were removed; installed app, app data, builds and profile/log evidence
  remain.
- Retained the full patch-context failures, transient Computer Use windows,
  accidental prototype-A relaunch, obsolete retail-path query, corrected
  `super+q`, restore-patch attempts, shell `PATH` shadow, policy-rejected
  `rm -rf` and fault-scan false positive in the detailed report.

Full evidence is in
`docs/parity/2026-08-01-ios-unified-fetch-state-rejection.md`.

The rejection/restoration reading was 254,820 seconds: 2 days, 22 hours,
47 minutes cumulative, 2,945 seconds after the interactive handoff. Goal time
includes user inspection, implementation, builds, signing, oracles, live
profiling, restoration, cleanup, testing and documentation; it is not a build
benchmark or person-hour estimate. The overall goal remains active.

### 2026-08-01 — Published rejection record and exact-replayed accepted renderer

- Committed the six-file documentation-only record as `125966b21f19` and pushed
  `codex/arm64-apple`. Local, remote-tracking and existing draft-PR heads match;
  no rejected source or duplicate PR was published.
- With both devices/GUI off, exact nice-15 one-job macOS/iOS builds took 65.02 /
  71.72 seconds and repeated only 32 established warnings. The macOS ARM64
  executable is `7c313610...1d0a`; the iOS unsigned/signed hashes are
  `aa9b6483...23b4` / `4cca94f1...6fad`; all embed `125966b21f19` and signing
  passed strict/deep verification.
- All 22 macOS tests passed in 2.57 seconds. Desktop and actual-surface iOS
  oracles retained the established logical/blend/presentation hashes; iOS
  reported the accepted 12-to-5 ordered-overlap path, proving the rejected
  12-to-1 shader was no longer installed.
- Only the disposable simulator booted. Container remap preserved retail/save
  inodes, sizes and hashes. Computer Use verified coherent intro/trophy/menu,
  character, track, no-ghost and grid screens. A fresh capture confirmed the
  temporarily hidden overlay returned; the timer advanced to 0:18.36 and
  C/Right inputs reached retail poll as `0x4000`, `0x0020` and `0x4020`.
- Correct-ID termination flushed 2,375 complete frame records plus one partial
  final row at `eb5bc3c...1f3`, header-only GPU data at `af0f3466...91f6`, and
  a 75-line log at `a1d98d65...107b`; targeted faults are zero. Normal race is
  5.84 reciprocal FPS and the final 299 complete frames are 6.03 FPS, so the
  Simulator performance and device gates stay open.
- Removed only the isolated signed copy after recording its hash. Installed
  exact app, data, builds, evidence, commit and remote branch remain.

The exact-replay reading was 255,884 seconds: 2 days, 23 hours, 4 minutes,
44 seconds cumulative, 1,064 seconds after the rejection boundary. Full
evidence is in
`docs/parity/2026-08-01-ios-unified-fetch-state-rejection.md`. The checkpoint
is exact-accepted; the overall goal remains active.

### 2026-08-01 — Rejected direct RGB5551 decode and wrote the three-day history

- Inspected the accepted renderer and rejected resolution reduction because
  the main target already uses the retail logical display size. Prototyped the
  narrower one-file removal of the dependent RGB5551 lookup texture.
- Closed both devices/GUI before compilation. A stale Computer Use window
  caused one failed Command-Q and two state timeouts; direct PID inspection and
  normal SIGTERM closed the exact GUI. All builds retained nice 15 / one job.
- The dirty macOS build took 71.99 seconds, repeated the 32 known warnings,
  passed 22/22 tests in 3.61 seconds, and independently passed the pixel oracle
  in 1.24 seconds. The fresh iOS thin-ARM64 executable is `ddc83538...773d`;
  its isolated strict-verified ad-hoc copy is `65d64ad3...7c86`.
- The disposable-device update preserved the 605,698,800-byte retail BIN and
  6,016-byte save at their canonical inodes/hashes. The actual iOS oracle kept
  12 fallback / 5 active draws and logical `851169f2644a1675`, blend
  `0c0d08324ae06c35`, and presentation `172d49a34571b64c`.
- Computer Use verified copyright/menu, Crash/kart/portraits, Crash Cove
  preview/map, No Ghost, fly-in and a complete grid/HUD/minimap/overlay. An
  animated fly-in label initially looked clipped; the settled frame proved it
  was a transition, not missing data.
- The first normal launch used an invalid `--profile-renderer` shorthand and
  correctly produced no CSV. Source inspection found `--perf-dir`; one
  controlled restart on the same device created the intended profile. This
  mistake is retained rather than laundering the first run into evidence.
- Correct-ID termination finalized 1,931 complete 59-field rows plus one
  excluded 54-field partial row at `7b574948...40b`, a header-only GPU file,
  and clean rotating logs. In the identical 66-call / 119-split / 78-fetch /
  56-merge state, 248 candidate frames average 198.412 ms versus 187.002 ms for
  200 accepted frames: a 6.10% regression.
- Rejected the candidate and restored the lookup-texture source with
  `apply_patch`. The worktree returned exactly to accepted
  `07bbc599bccc`; ignored candidate build/install evidence is not a release
  artifact.
- Added `docs/history/THREE-DAY-CHECKPOINT.md`, an end-to-end milestone,
  evidence, limitation, rebuild, failure and elapsed-time map, plus the focused
  direct-decoder rejection report and cross-links from README, Roadmap,
  Decisions, parity index, progress log, and engineering journal.
- The first local-link audit reused zsh's special `path` variable in a
  subshell, which hid `rg` from the final whitespace command. The shell exited
  without persistent environment change. The corrected `doc_path` audit found
  balanced fences, no missing referenced Markdown file and no trailing space.
- The connected GitHub app returned 404 for the private PR. The permitted CLI
  fallback's first unqualified read-only query resolved upstream
  `CTR-tools/ctr-native` and its unrelated closed PR #1. No mutation occurred.
  Repeating with explicit `--repo chrissotraidis/ctrpad` proved the intended
  private draft PR #1 open, clean and mergeable from `codex/arm64-apple` to
  `main`; every publication command is repository-qualified.

The rejection/history reading was 258,100 seconds: 2 days, 23 hours,
41 minutes, 40 seconds cumulative, 2,216 seconds (36 minutes, 56 seconds) after
the exact-replay boundary. The overall goal remains active: Simulator cadence,
broader churn, physical-iPad execution, human touch ergonomics, user-owned
signing, and final package/source pairing are still open.

### 2026-08-01 — Merged the documented Apple port into GitHub main

- Committed the eight-file, 761-addition/two-replacement historical checkpoint
  as `f5140b7eb409`. No source, build product, retail file, save, IPA, profile,
  certificate or keychain material was in the commit.
- The exact commit passed the clean GPL source packager in 18.44 seconds:
  3,244 members, prohibited material excluded, archive `dc9d5ad2...3406`.
- Pushed `codex/arm64-apple`; local and remote branch IDs matched. GitHub's
  first PR read was briefly stale at the old head, so merging paused. The API
  then converged on `f5140b7eb409`, `mergeable=true`, `state=clean`.
- Audited the intended large import relative to the original viability-only
  `main`: 6,774 commits, 2,684 changed files, 2,620 source/non-document files,
  and zero sensitive paths. Marked private PR #1 ready and merged with a normal
  history-preserving merge commit.
- Verified `origin/main`, the GitHub branch API and repository default branch
  at `0758e7a804390ebd8a7cc74ba4cdcaf864270717`. The reviewed head is its
  ancestor and all rebuild/install/history files exist in the merged tree.
- Created and pushed `codex/simulator-performance-next` directly from that
  merge before resuming optimization. The merged commit's own source package
  passed in 22.06 seconds with 3,244 members and archive
  `873de12d...120e`; prohibited material remained excluded.

The publication-close reading was 258,793 seconds: 2 days, 23 hours,
53 minutes, 13 seconds cumulative, 693 seconds (11 minutes, 33 seconds) after
the rejection/history boundary. The overall goal remains active.

### 2026-08-01 — Re-baselined the project around physical release evidence

- Stopped speculative Simulator renderer tuning and mapped every major layer
  to the actual signed-iPad definition of done.
- Recorded that retail state/parity, shared Apple builds, focused GLES pixels,
  import/save, lifecycle, touch/keyboard/controller composition and bounded
  coherent graphics are implemented, while signed physical-device acceptance
  remains unproved.
- Corrected the acceptance model: Simulator remains responsible for build,
  pixel, bounded visual, input, lifecycle and logging correctness; sustained
  cadence, thermals and multi-touch feel now belong to the target iPad.
- Defined the minimum physical release campaign and the contingency that a
  larger renderer change is considered only if physical profiling reproduces
  the Simulator bottleneck.
- Re-audited the machine: both CTRPad Simulators were shut down, the worktree
  began clean, and there were zero valid code-signing identities,
  provisioning profiles or connected physical devices.

The re-baseline documentation reading was 259,866 seconds: 3 days, 11 minutes,
6 seconds cumulative, 1,073 seconds (17 minutes, 53 seconds) after the
publication-close boundary. The overall goal remains active.

### 2026-08-01 — Executed the clean re-baseline smoke and physical handoff build

- Committed and pushed the five-file release re-baseline as `d5772375fabc` and
  opened mergeable draft PR #2 against `main`.
- With both Simulators and the GUI closed, rebuilt macOS ARM64, passed 22/22
  tests, and repeated the established logical/blend/presentation pixel hashes.
- Rebuilt the clean iPad Simulator app, passed the actual 1032x1376 GLES pixel
  oracle, update-installed it on exactly one disposable device and preserved
  the retail BIN/save inodes and hashes.
- Computer Use navigated touch/keyboard through Time Trial to a live Crash Cove
  race, paused, backgrounded/foregrounded, rotated to landscape, resumed and
  consumed touch Gas with coherent track/kart/HUD/minimap/overlay graphics.
- Exact-ID termination left no app process. The 82-line current log and four
  archives have zero targeted asset, visibility, error or fatal markers; both
  devices and Simulator GUI ended closed.
- Rebuilt the clean thin ARM64 iPhoneOS product and produced retail-free
  unsigned IPA `41e00a5d...13ed` plus matching 3,245-member GPL source archive
  `117dd291...352d`.
- Retained one self-test-exit UIKit appearance-transition console warning as an
  open focused observation instead of treating the successful normal
  Home/foreground route as proof that every exit path is warning-free.

The clean-smoke close reading was 261,192 seconds: 3 days, 33 minutes, 12
seconds cumulative, 1,326 seconds (22 minutes, 6 seconds) after the re-baseline
documentation boundary. The overall goal remains active because real Apple
signing and physical-iPad acceptance are still open.

### 2026-08-01 — Closed the self-test warning assessment without a risky patch

- Re-audited the clean tree, GitHub `main`, both Simulator states, signing
  identities and connected devices; the source was clean, both devices and GUI
  were closed, and real signing/device inputs remained absent.
- Traced the retained UIKit warning through synchronous renderer-test
  `Platform_Init` → draw/readback → `Platform_Shutdown` before a UIKit run-loop
  return. The ordinary production display-loop route does not use that
  teardown sequence.
- Rejected manual UIKit transition balancing, arbitrary nested-loop delay,
  skipped cleanup and an iOS-only asynchronous test refactor because each
  changes working production or deterministic test semantics without a
  physical defect.
- Kept application source unchanged and documented the exact condition that
  would reopen the issue: the signed physical app reproducing the warning on a
  normal route.

The teardown-assessment reading was 261,906 seconds: 3 days, 45 minutes, 6
seconds cumulative, 714 seconds (11 minutes, 54 seconds) after the clean-smoke
close boundary. The overall goal remains open only at the real
signing/physical-iPad campaign.

### 2026-08-01 — Replaced the stale visible Simulator build with the exact current build

- The first user-visible follow-up rendered retail assets but the installed
  executable was stale: SHA `ed53ba9f...79c11c`, old input-self-test text and
  no current touch/retail-poll log strings.
- Rejected that session as input evidence instead of changing working source.
- Re-signed an isolated copy of the exact clean Simulator build and
  update-installed executable `c6d40aaf...187f` on the single validation
  device.
- The update preserved the retail image at 605,698,800 bytes /
  `f780bf23...7c0` and the save at 6,016 bytes / `6a01b0f5...19a3`, including
  their pre-update inodes.
- Exact current-build Cross and Circle activations each logged touch down,
  retail-poll consumption and touch up. Cross was consumed at `+99.540s` even
  near 5.56 Simulator FPS, proving the accepted latch survives low cadence.
- Computer Use's synthetic keyboard event did not enter the app log; retained
  as automation non-evidence. Keyboard capture was disabled before handoff.
- Focused `ctr_native_input` passed 1/1 in 0.17 seconds. No application source
  change was needed.

The correction reading was 265,868 seconds: 3 days, 1 hour, 51 minutes, 8
seconds cumulative, 3,630 seconds (1 hour, 30 seconds) after the
262,238-second physical-device boundary. The exact current build remains open
in the one iPad Simulator; real signing and physical-device acceptance remain
open.

### 2026-08-01 — Enforced exact Simulator update installation

- Added `tools/install-ios-simulator.sh` so future evidence requires one booted
  Simulator, a valid thin ARM64 Simulator app, an isolated strict ad-hoc
  signature and equal staged/installed executable hashes.
- Added an optional slow update-preservation proof and made the GPL source
  package require the helper.
- Passed syntax/help checks and safe rejection of an unknown option, a missing
  app and the explicitly requested shutdown negative-test Simulator. The first
  local zsh harness used its reserved `status` variable; the corrected `rc`
  harness passed. `shellcheck` was unavailable.
- Performed one real guarded update on the sole validation Simulator. Exact
  executable `c6d40aaf...187f` installed and launched as PID 66389; retail and
  save inodes, sizes and hashes were unchanged.
- Captured current Controls/Change Disc/touch-overlay and rendered retail
  pixels at 17:27:06 CDT. The new log initialized GLES, touch and UIKit and had
  zero targeted faults across its five-file rotation set.

The 17:27:33 reading was 266,659 seconds: 3 days, 2 hours, 4 minutes, 19
seconds cumulative. Reproducible Simulator product identity is now closed;
real signing and every physical-iPad criterion remain open. Exact evidence is
in `docs/parity/2026-08-01-exact-simulator-install.md`.

At 17:32:23 CDT this checkpoint became commit `67b4c6276`. Its clean 3,249-file
GPL source archive included the installer/report and excluded private/runtime
material at SHA-256 `116781ee...42c5`. The package's built-in checksum passed;
a redundant caller first ran `shasum -c` from the wrong directory, then passed
unchanged from `dist/`. The 17:33:01 reading was 266,988 seconds (3 days,
2 hours, 9 minutes, 48 seconds); GitHub publication remained next.

### 2026-08-01 — Added the physical-iPad campaign handoff

- Re-audited the completion boundary after PR #6: zero Apple signing
  identities, no profiles and no CoreDevice physical devices remain.
- Added `tools/ios-device-campaign.sh` with separate signed-IPA `preflight`,
  non-destructive update-install/launch `prepare`, and post-human-test
  Application Support `collect` phases using versioned `devicectl` JSON.
- Bound the app's extracted leaf signing certificate to one of the provisioning
  profile's `DeveloperCertificates`, beyond App ID/team/UDID matching.
- Added a redacted timestamped acceptance template covering Files import,
  complete touch race, three-boost drift, audio/lifecycle, cadence/thermals,
  log review and save/update persistence.
- Forced raw in-repo evidence into gitignored paths, avoided installed-binary
  hash claims that iPadOS cannot prove, and excluded the Documents retail tree
  from collection.
- Passed syntax/help and safe argument/path failures. The current unsigned IPA
  passed sidecar/ZIP checks then correctly failed for missing profile; a fake
  UDID produced CoreDevice error 1000 and JSON version 3 before any install.
- Recorded and corrected a literal-suffix BSD `mktemp` probe, a rejected direct
  cleanup replaced by recoverable Trash, and one malformed test-orchestration
  output-limit tuple. `shellcheck` remains unavailable.

The 17:45:00 reading was 267,693 seconds: 3 days, 2 hours, 21 minutes, 33
seconds cumulative. This improves the other-Mac/iPad handoff but does not close
Apple signing or any physical result. Exact evidence is in
`docs/parity/2026-08-01-ios-physical-campaign-handoff.md`.

The unchanged macOS ARM64 suite subsequently passed 22/22 in 5.39 seconds.
Exactly one Simulator remained booted at installed hash `c6d40aaf...187f` and
was untouched. Device collection now also extracts FPS and campaign-event rows
for later scene-qualified physical analysis.

The certificate-data loop independently round-tripped one synthetic plist
entry byte-for-byte (`cmp=0`). Real leaf-certificate/profile authorization
remains unpassed until a user-signed IPA is supplied.
The first space-separated codesign certificate-prefix probe failed; the
required `--extract-certificates=PREFIX` form extracted Xcode's three-
certificate chain and was retained before commit.
Literal dotted team-entitlement readback was likewise assigned to PlistBuddy
and reproduced a synthetic `TESTTEAM` value exactly.
The final certificate-link loop compared real Xcode leaf DER
`d84db96a...1ed57` with a synthetic one-entry profile array and returned
`profile_count=1`, `match=1`; no CTRPad Apple authorization is inferred.

The final claim-audit reading was 268,214 seconds at 17:53:34 CDT: 3 days,
2 hours, 30 minutes, 14 seconds. Signed CTRPad preflight and all physical gates
remain unexecuted.

At 17:55–17:56 CDT the final working-tree audit passed Bash/package syntax,
diff hygiene and all 22/22 macOS ARM64 tests in 4.87 seconds. One obsolete
bundle-ID lookup failed without mutation; the corrected exact identifier
confirmed the sole Simulator still installed `c6d40aaf...187f`. Signing
identity/device inventory remained zero. The 17:55:51 active-time reading was
268,363 seconds (3 days, 2 hours, 32 minutes, 43 seconds).

The pre-stage audit also made `collect` require the exact successful prepare
manifest/UDID/bundle tuple before accessing CoreDevice, preventing evidence
from being attached to the wrong signed-install campaign.

At 17:59 CDT, a root without that manifest failed before creating a collection;
an ignored synthetic matching-manifest negative advanced to and failed at the
fake device with CoreDevice error 1000 before app lookup/copy. Final syntax and
diff checks passed. The 17:59:07 reading was 268,549 seconds (3 days, 2 hours,
35 minutes, 49 seconds); neither raw negative is tracked.

At 18:00 CDT commit `4349bae8d` produced a clean 3,253-member corresponding-
source archive at SHA-256 `b9dfcc85...cba4`. Internal and independent checksum
checks passed; the tool/template/report were present and both retail/runtime
and private/package scans returned zero. The 18:00:26 reading was 268,628
seconds (3 days, 2 hours, 37 minutes, 8 seconds).

At 18:01–18:02 CDT both commits reached GitHub. The connector returned its
private-repository HTTP 404, then authenticated `gh` fallback opened draft PR
#7. Its exact 12-file/two-commit scope was `MERGEABLE`/`CLEAN` with no configured
checks. The 18:02:05 reading was 268,724 seconds (3 days, 2 hours, 38 minutes,
44 seconds); merge was still pending.

The first protected merge used a mistyped full SHA after the short prefix and
was safely refused without a merge. Re-querying and matching exact head
`0458baff2ea...` succeeded: PR #7 merged into `main` at 18:02:58 CDT as
`a19c1a5ef`. Remote-tree inspection found all three new handoff files. The
post-merge active-time reading was 268,804 seconds (3 days, 2 hours, 40 minutes,
4 seconds); signed physical acceptance remains open.

PR #8 merged the four-document history at 18:04:39 CDT as `7087cc6e0`. Local,
remote branch and `main` were aligned, with the sole Simulator still at
`c6d40aaf...187f`. Final-main source packaging completed asynchronously; an
early verification saw the not-yet-visible path, then the unchanged retry
passed at 3,253 members / `a748e256...2b01`, with the handoff files present and
zero retail/runtime or private/package matches. The 18:06:51 reading was
269,013 seconds (3 days, 2 hours, 43 minutes, 33 seconds).

### 2026-08-01 — Hardened Apple trust and real-profile compatibility

- Proved `security cms -D` decoded a self-signed CMS at exit 0 while actual
  certificate evaluation returned `CSSMERR_TP_NOT_TRUSTED`.
- Fixed the signed packager's trailing-dot App ID prefix assumption and now
  construct/cross-check Apple's `<prefix>.<bundle-id>` form.
- Added a shared CMS/code-signing verifier that validates the chain and pins
  its final DER certificate to an Apple Root CA from the system root keychain;
  profile mode also requires a provisioning-profile signing subject.
- Made signed packaging bind the app leaf certificate to the profile and read
  back application, team and keychain entitlements; device preflight retains
  both trust manifests and root hashes.
- Rejected tampered/untrusted CMS, ad-hoc CTRPad app, unknown mode and tracked
  trust evidence. Corrected initial Bash 3.2 help/empty-array defects.
- Terminated an overly broad Xcode.app deep-signature probe after almost three
  minutes, then accepted the same mechanism on small Apple-signed Calculator
  with three certificates and pinned root `b0b1730e...1f024`.
- Produced two unchanged byte-identical seven-member unsigned IPAs at
  `3354bb3e...49ed`; unsigned device preflight still fails before device access.
- Passed 22/22 regressions in 5.56 seconds plus syntax/help/diff checks;
  `shellcheck` was unavailable.

The 18:26:33 reading was 270,194 seconds: 3 days, 3 hours, 3 minutes, 14
seconds. No real Apple profile, signed CTRPad IPA or physical result is inferred
from the corrected offline gate. Exact evidence is in
`docs/parity/2026-08-01-ios-apple-trust-preflight.md`.

At 18:31 CDT, the checked-in candidate repeated 22/22 tests in 2.68 seconds,
accepted the Apple-signed Calculator chain/root proof and rejected the
self-signed profile. Active goal time was 270,498 seconds (3 days, 3 hours, 8
minutes, 18 seconds); the external signing/device boundary is unchanged.
