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

## Current open path to the requested product

1. Finish independent-process/mutation acceptance for the existing full
   cross-width trace.
2. Close remaining macOS M6 runtime evidence, including broader audio
   listening, renderer, controller, and full-race manual-play coverage.
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
