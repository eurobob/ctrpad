# CTRPad three-day build timeline

This document is the chronological index for the CTRPad Apple-port campaign
that began on 2026-07-29 and reached the physical-iPad acceptance boundary on
2026-08-01. It is designed to answer three historical questions:

1. what changed, and when;
2. why each phase happened; and
3. where to find the exact commands, failures, evidence and rejected attempts.

The detailed technical narrative remains in
[`ENGINEERING-JOURNAL.md`](ENGINEERING-JOURNAL.md). The shorter status ledger is
[`PROGRESS-LOG.md`](PROGRESS-LOG.md), while focused validation artifacts live in
[`../parity/`](../parity/). This file makes those records navigable by time and
adds a complete durable commit ledger.

## Timestamp and completeness policy

- All wall-clock timestamps are Central Daylight Time (`-05:00`, `CDT`).
- Commit times are exact Git committer timestamps.
- GitHub merge times are exact server timestamps converted from UTC to CDT.
- Goal elapsed time is Codex active goal time, not uninterrupted wall time.
- Runtime observations are called exact only where the journal, log or report
  retained a timestamp. Work between durable checkpoints is described as an
  interval rather than assigned an invented minute.
- The commit ledger contains every commit reachable from the working branch
  between the first viability document and the last pre-timeline assessment:
  151 commits. Commands, temporary paths, hashes, rejected experiments and
  individual test results are intentionally not duplicated 151 times here;
  they remain in the linked engineering journal and parity reports.

## Time accounting

| Boundary | Exact time | Meaning |
| --- | --- | --- |
| Viability research committed | 2026-07-29 13:32:02 | The research input existed before the long-running goal began. |
| Goal created | 2026-07-29 13:46:41 | Start of Codex goal accounting. |
| Physical-device boundary recorded | 2026-08-01 15:56:54 | Goal was marked blocked after all locally actionable work was published. |
| Wall-clock span | 3 days, 2 hours, 10 minutes, 13 seconds | Includes pauses and time when the goal was not actively consuming work time. |
| Active goal time | 3 days, 50 minutes, 38 seconds | Exact `timeUsedSeconds=262238` reading at the boundary. |
| Paused/non-active difference | 1 hour, 19 minutes, 35 seconds | Wall span minus active goal time; not claimed as implementation time. |
| Post-resume correction reading | 2026-08-01 17:14:21 | Active goal time reached 3 days, 1 hour, 51 minutes, 8 seconds after correcting the stale Simulator installation. |
| Exact-installer acceptance reading | 2026-08-01 17:27:33 | Active goal time reached 3 days, 2 hours, 4 minutes, 19 seconds after making the correction reproducible. |
| Exact-installer package reading | 2026-08-01 17:33:01 | Active goal time reached 3 days, 2 hours, 9 minutes, 48 seconds after committing and packaging the helper. |
| Physical-campaign handoff reading | 2026-08-01 17:45:00 | Active goal time reached 3 days, 2 hours, 21 minutes, 33 seconds after making the external device campaign reproducible. |
| Physical-campaign claim-audit reading | 2026-08-01 17:53:34 | Active goal time reached 3 days, 2 hours, 30 minutes, 14 seconds after correcting certificate linkage and revalidating claim boundaries. |
| Physical-campaign publication audit | 2026-08-01 17:55:51 | Active goal time reached 3 days, 2 hours, 32 minutes, 43 seconds after the final local regression and exact Simulator-identity recheck. |
| Collection-linkage audit | 2026-08-01 17:59:07 | Active goal time reached 3 days, 2 hours, 35 minutes, 49 seconds after binding collection to the exact successful prepare manifest and rerunning negative guards. |

## Executive chronology

### 2026-07-29 — Evidence foundation and retail parity plan

| Time | Durable event | Result |
| --- | --- | --- |
| 13:32 | Viability research committed (`95417c723`) | Established that the native source was a feasible foundation and identified the Apple, ARM64, touch, retail-media and GPL boundaries. |
| 13:46 | Goal accounting began | Objective required a native ARM64 macOS/iOS/iPadOS application, retail NTSC-U import, retail timing/physics, saves, touch controls and corresponding source. |
| 13:52–13:54 | Roadmap, asset safeguards and beta-7.1 source baseline (`c5496cbfe` through `6b3238104`) | Created the dependency-ordered plan and separated proprietary user media from publishable source. |
| 14:26–15:28 | Reproducible i686 baseline, rejected raw-checkpoint oracle, retail-region validation (`1502955cb` through `d029ebc89`) | Made the old-width producer reproducible and rejected evidence that embedded host addresses. |
| 15:28–17:53 | Canonical digest and NTSC-U golden-run preparation (`d029ebc89` through `868a4e308`) | Established typed gameplay-state comparison and complete golden coverage before changing pointer width. |

### 2026-07-30 — ARM64 correctness and macOS native parity

| Time | Durable event | Result |
| --- | --- | --- |
| 00:54 | Engineering journal introduced (`a40a7584c`) | Began the command-, failure- and evidence-level historical record. |
| 11:39–14:24 | ARM64 runtime port, LP64 allocation correction and level mosaic reference fix (`2f341999b` through `5f6d3ca2d`) | Reached a runnable 64-bit product without hiding cross-width differences. |
| 14:47–18:12 | Live parity checkpoints, AI bounds correction and archived evidence (`f1c63bf5a` through `270fd7e2b`) | Converted crashes and state mismatches into bounded source fixes and replay proof. |
| 18:59–19:01 | Native macOS ARM64 application bundle (`630e556c1`, `9beead6b8`) | Produced the first clean Apple application workflow. |
| 20:44–22:09 | Full parity rejection followed by potion and cutscene emitter layout corrections (`57cfd6533` through `a31bf8534`) | Preserved rejected evidence, fixed two real pointer-width layout defects, then accepted the corrected proof. |
| 22:17–23:39 | Replay reproducibility, cadence, save persistence and audio (`0e524c0c0` through `88ae012d5`) | Added the evidence needed to compare timing and persistent behavior, not merely screenshots. |

### 2026-07-31 — iOS productization, import, saves and touch play

| Time | Durable event | Result |
| --- | --- | --- |
| 00:04–00:38 | Quick keyboard taps and full cross-width trace (`24aff7d88` through `e3a7e79fa`) | Preserved short input edges and closed the remaining desktop parity trace. |
| 01:39–04:20 | Running time ledger and Scrapbook video/audio investigation (`15f39c58c` through `900f5656b`) | Added explicit elapsed-time history and tested real STR/XA presentation rather than assuming decoder coverage. |
| 04:39–06:29 | Practical keyboard controls, controller ownership and audio oracle (`2c10b00b3` through `acf0a6aaa`) | Made the desktop build testable by keyboard and proved live race movement plus mixer equivalence. |
| 06:53–09:47 | Shared GLES3, UIKit presentation and lifecycle cooperation (`4695d9cb3` through `5d6a3baac`) | Replaced desktop-only rendering assumptions with a shared GLES path and a UIKit-owned display lifecycle. |
| 10:06–12:17 | iOS sandbox split and Files-based retail import (`02a6623f8` through `5fc4237fe`) | Kept bundled/source assets read-only, writable state private and user retail media imported through Files. |
| 12:48–13:21 | Atomic memory-card writes (`4b078065f`, `716ea5c71`) | Established old-or-new save replacement and a clean iOS save campaign. |
| 13:59–16:25 | Native touch overlay, reliable menu gestures and touch-only racing (`c496c27f0` through `46108c6a6`) | Added touch as a peer input source, entered Crash Cove without keyboard control and revalidated keyboard compatibility. |
| 16:43–17:35 | Adaptive iPad layout and reproducible sideload packaging (`db45004f9` through `2f3f24c6e`) | Supported portrait/landscape UIKit scenes and created credential-free unsigned packaging plus signing preflight. |
| 17:55–18:54 | Import failure, interruption and installed-asset recovery (`3c190e5a4` through `db78d5a25`) | Exercised wrong-region/content failures, killed a real import and recovered reserved stages on subsequent launch. |
| 19:38–20:38 | Current touch trace and keyboard ownership (`2143abdb7` through `23029b2c7`) | Reconfirmed Gas/steering in a race and shared hardware keyboard with the primary local player. |
| 21:07–23:39 | Cross-target GLES equivalence, full golden run and pixel semantics (`95d840603` through `8bf0f4128`) | Compared macOS GL with iOS GLES through a full typed run and added exact renderer-pixel oracles. |

### 2026-08-01 — Lifecycle hardening, release hygiene and Simulator re-baseline

| Time | Durable event | Result |
| --- | --- | --- |
| 00:15–01:27 | UIKit view balance and safe retail-disc reselection (`6b2681578` through `e8e8e0e1f`) | Preserved the drawable/controller hierarchy and avoided mutating live retail media. |
| 02:09–04:40 | Custom controls, visibility diagnosis and correction (`828d09580` through `faa32dc04`) | Added control customization, reproduced missing geometry, rejected unsafe cleanup and accepted the bounded visibility fix. |
| 04:51–05:07 | Portrait resizing/reflow (`2c78c040b`, `09d72ab1a`) | Corrected clipping exposed by the prior geometry work. |
| 05:19–06:35 | Release hygiene, corresponding source and signing isolation (`aad05e284` through `43245107c`) | Proved retail media and credentials stay out of artifacts, built matching source and validated extracted-source builds. |
| 07:44–09:10 | Durable Simulator logging, accessibility and retail-poll input hold (`7bcc51a79` through `867dbd264`) | Reopened Simulator acceptance honestly and corrected the boundary between a quick host event and the retail consumer. |
| 09:51–12:50 | Simulator presentation profiling, coherent framebuffer fetch and split batching (`ff26c0815` through `6de7f17cd`) | Improved the GLES presentation path while retaining exact fallback comparison. |
| 13:05–14:12 | User-visible Crash Cove handoff and rejected fetch-state prototype (`9a33ae00e` through `07bbc599b`) | Kept the game visible, measured a broader prototype, rejected it when it did not improve matched live cost and restored exact source. |
| 14:56–15:00 | Three-day checkpoint, PR #1 merge and main publication boundary (`f5140b7eb` through `32e82d8da`) | Merged the complete touch-first ARM64 Apple port into GitHub `main`. |
| 15:18 | Holistic release re-baseline (`d5772375f`) | Moved final cadence/touch/performance acceptance from Apple Software Renderer to physical iPad while keeping Simulator as the correctness/logging gate. |
| 15:44 | Clean Apple release smoke documented (`cd41e62c5`) | Recorded 22/22 tests, exact pixel hashes, macOS/iOS builds, Simulator lifecycle, menus/race, touch/keyboard, update preservation and unsigned packages. |
| 15:45:38 | PR #2 merged to `main` (`9b4a175f5`) | Published the re-baseline and clean-smoke record. |
| 15:54 | UIKit self-test teardown boundary documented (`79152b7b5`) | Traced the remaining warning to synchronous test teardown before UIKit's first run-loop return and rejected unsafe warning-suppression changes. |
| 15:55:44 | PR #3 merged to `main` (`cd4804186`) | Published the final local source assessment and historical checkpoint. |
| 15:56:54 | Goal marked blocked at 262,238 seconds | Zero Apple signing identities, no provisioning profile and no connected physical device prevented the required signed-iPad acceptance. |
| 16:00–16:33 | User-requested live recheck | Booted only `CTRPad Import Validation`, launched `io.github.chrissotraidis.ctrpad`, visibly rendered retail copyright/logo/menu/cutscene/character assets and exercised touch navigation. The Apple Software Renderer ran around 7 FPS, but a later exact audit proved this installed executable stale; its input behavior is not current-product evidence. |
| 17:08–17:14 | Exact-install and touch-correlation correction | Replaced stale executable `ed53ba9f...79c11c` with exact current `c6d40aaf...187f`, preserved retail/save hashes and inodes, and logged Cross/Circle down → retail-poll → up on the current build. Focused input CTest passed; no source change was needed. Synthetic Computer Use keyboard input remained automation non-evidence. |
| 17:25–17:27 | Reproducible exact Simulator installer | Added the one-booted-device, isolated-sign and installed-hash contract; safe negative probes passed, an exact live update preserved retail/save identity, PID 66389 relaunched, and current controls plus rendered retail pixels appeared with clean targeted logs. |
| 17:32–17:33 | Installer commit and source-package proof | Committed `67b4c6276`, produced a 3,249-member source archive containing the helper/report, and corrected a redundant checksum caller's working directory before publication. |
| 17:38–17:59 | Physical-iPad campaign handoff | Added signed-IPA preflight, non-destructive CoreDevice install/launch, prepare-bound local log/save collection and a redacted human acceptance template; corrected certificate extraction/linkage and dotted-entitlement reads; unsigned IPA, unbound evidence and nonexistent-device probes failed at the intended boundaries; 22/22 regressions and the sole exact Simulator identity passed before publication, so physical acceptance remains open. |

## Complete durable commit ledger

This is the exact commit-by-commit ledger for the campaign branch. A commit is a
durable checkpoint, not proof that every experiment immediately preceding it
succeeded; rejected work is explicitly preserved in the commit subject and in
the engineering journal.

```text
2026-07-29 13:32:02  95417c723  Add CTR Native iPadOS viability research
2026-07-29 13:52:47  c5496cbfe  docs: establish Apple port roadmap and asset safeguards
2026-07-29 13:54:06  268ff6977  merge: establish ctr-native beta-7.1 source baseline
2026-07-29 13:54:51  6b3238104  docs: close repository foundation milestone
2026-07-29 14:26:05  1502955cb  build: add reproducible Linux i686 baseline container
2026-07-29 14:28:40  a71db6914  docs: reject raw checkpoints as parity oracle
2026-07-29 14:43:33  f8974377b  build: use available CPUs for emulated baseline
2026-07-29 14:54:24  a76ac25a4  build: preserve dpkg query placeholders
2026-07-29 15:28:35  89d08cdee  assets: reject incompatible retail disc regions
2026-07-29 15:28:45  d029ebc89  parity: add canonical gameplay state digest
2026-07-29 15:48:27  cc9c06f2a  parity: make replay failures automation-safe
2026-07-29 16:27:57  df558504c  parity: prepare NTSC-U golden run
2026-07-29 17:53:07  868a4e308  parity: enforce complete golden coverage
2026-07-30 00:54:19  a40a7584c  docs: add chronological engineering journal
2026-07-30 11:39:07  2f341999b  port runtime to ARM64 with parity gates
2026-07-30 12:24:27  38c9bfb53  fix LP64 language allocation pressure
2026-07-30 13:56:35  53ab70e96  Fix LP64 level mosaic texture references
2026-07-30 14:24:52  5f6d3ca2d  Harden full i686 replay verification
2026-07-30 14:47:33  f1c63bf5a  docs: record live parity and publication checkpoint
2026-07-30 17:02:04  b6aff5939  fix native AI checkpoint bounds
2026-07-30 17:03:25  55d3b71c6  docs: record checkpoint publication
2026-07-30 17:29:26  b5c2eba39  docs: record corrected full ARM64 checkpoint
2026-07-30 17:59:14  b6bc353f0  docs: archive full parity evidence
2026-07-30 18:12:48  270fd7e2b  docs: record corrected cross-width boundary
2026-07-30 18:59:33  630e556c1  feat: add macOS ARM64 app bundle workflow
2026-07-30 19:01:15  9beead6b8  docs: record clean macOS bundle checkpoint
2026-07-30 20:44:01  57cfd6533  test: archive full cross-width parity rejection
2026-07-30 20:44:43  185e9f9b9  docs: record parity checkpoint publication
2026-07-30 21:12:59  77d230a0f  fix: preserve potion emitter layout across widths
2026-07-30 21:13:42  7af15bea2  docs: record potion fix publication
2026-07-30 21:51:16  eee2a8df5  fix: preserve cutscene emitter layout across widths
2026-07-30 22:09:32  a31bf8534  docs: record finalized emitter parity proof
2026-07-30 22:17:12  0e524c0c0  tool: make replay input extensions reproducible
2026-07-30 22:33:08  a269843a2  tool: preserve historical replay timing in v4 seeds
2026-07-30 22:56:50  8a905023c  docs: record current-build lap coverage
2026-07-30 23:01:04  3166ae959  tool: support immutable replay gate inputs
2026-07-30 23:17:56  7419715c4  test: measure macos replay cadence
2026-07-30 23:29:05  a6741e7d6  test: prove macos save persistence
2026-07-30 23:39:46  88ae012d5  test: validate macos audio output
2026-07-31 00:04:09  24aff7d88  fix: preserve quick keyboard taps
2026-07-31 00:12:12  dfec4f928  docs: record quick keyboard tap investigation
2026-07-31 00:26:49  4a1b36676  docs: accept full cross-width parity trace
2026-07-31 00:38:20  e3a7e79fa  test: support alternate i686 replay mapping
2026-07-31 01:39:08  15f39c58c  docs: add running progress and time ledger
2026-07-31 01:56:22  1753edbc5  test: add retail scrapbook STR decode probe
2026-07-31 01:57:42  f6aec7cd9  docs: record recoverable pause checkpoint
2026-07-31 02:45:47  ac6bd768b  docs: record second recoverable pause
2026-07-31 02:53:52  69c4c9948  test: make replay evidence finalization recoverable
2026-07-31 03:03:13  c44ea7810  test: capture scrapbook STR presentation
2026-07-31 03:07:15  a39fd9f47  docs: record scrapbook STR presentation evidence
2026-07-31 03:26:12  75b09db17  fix: use defined GL vertex attribute offsets
2026-07-31 03:37:12  05fcc610f  docs: record STR sanitizer acceptance
2026-07-31 03:46:36  28838f0af  docs: accept complete scrapbook presentation probe
2026-07-31 04:02:11  63b0a0773  Instrument real Scrapbook playback evidence
2026-07-31 04:10:39  a47217008  docs: accept focused real-menu Scrapbook route
2026-07-31 04:20:53  900f5656b  Trace Scrapbook XA exhaustion against video cadence
2026-07-31 04:39:42  2c10b00b3  feat: add practical desktop keyboard controls
2026-07-31 04:46:43  df17f4643  docs: record keyboard and natural scrapbook evidence
2026-07-31 04:54:36  2f9bf4eae  fix: own controller slots across hotplug
2026-07-31 05:17:09  764205d4c  test: keep virtual controller ids unsigned
2026-07-31 05:21:24  359e8d5a0  docs: record controller validation checkpoint
2026-07-31 05:38:18  1a9beef4b  docs: finalize controller validation evidence
2026-07-31 05:48:11  4fd643429  docs: record live keyboard race movement
2026-07-31 06:13:09  87f8e7a05  test: lock cross-width audio mixer oracle
2026-07-31 06:28:23  9ed75956c  docs: record cross-width audio mixer oracle
2026-07-31 06:29:26  acf0a6aaa  docs: record pinned GitHub validation
2026-07-31 06:53:46  4695d9cb3  feat: add shared GLES3 renderer dialect
2026-07-31 07:02:04  78ef952db  fix: exit cleanly when renderer setup fails
2026-07-31 07:11:42  db58041a3  docs: record shared GLES3 bring-up
2026-07-31 08:03:54  98ae2c6d8  feat: present GLES through UIKit
2026-07-31 08:25:37  cbdd58435  docs: record live iPad GLES bring-up
2026-07-31 09:08:48  afb5463cc  feat: cooperate with UIKit lifecycle
2026-07-31 09:46:19  c6419701b  docs: record iOS lifecycle and keyboard checkpoint
2026-07-31 09:47:21  5d6a3baac  docs: clarify publication evidence
2026-07-31 10:06:21  02a6623f8  feat: split iOS assets and writable storage
2026-07-31 10:28:37  5e77e3960  docs: record iOS sandbox storage checkpoint
2026-07-31 11:07:21  7872f7e61  feat: import iOS retail media through Files
2026-07-31 12:17:39  5fc4237fe  docs: record iOS Files import and parity completion
2026-07-31 12:48:14  4b078065f  fix: write memory cards atomically
2026-07-31 13:21:54  716ea5c71  docs: record atomic iOS save campaign
2026-07-31 13:59:29  c496c27f0  feat: add native iOS touch controls
2026-07-31 15:03:10  c783eda74  fix: make touch menu gestures reliable
2026-07-31 15:29:18  da151bfef  docs: record iOS touch and save evidence
2026-07-31 16:19:36  8490b3126  docs: record touch-only race evidence
2026-07-31 16:25:03  46108c6a6  docs: revalidate keyboard controls
2026-07-31 16:43:11  db45004f9  feat: adapt iPad layout for iPadOS 26
2026-07-31 16:58:48  6e856cbbd  docs: record iPadOS 26 adaptive layout
2026-07-31 17:15:44  6db6116fe  feat: add iOS sideload packaging
2026-07-31 17:18:08  a37cdf2aa  fix: make iOS package reproducible
2026-07-31 17:26:19  207121134  fix: construct Apple signing entitlements
2026-07-31 17:35:04  2f3f24c6e  docs: record iOS package acceptance
2026-07-31 17:55:23  3c190e5a4  docs: accept live iOS import failures
2026-07-31 17:59:54  c745390a5  fix: recover interrupted iOS imports
2026-07-31 18:09:51  dab6ef3d3  docs: record interrupted import recovery
2026-07-31 18:29:00  1f6e61e7c  docs: record live interrupted import
2026-07-31 18:30:52  8c177e832  fix: recover iOS stages before runtime
2026-07-31 18:42:51  560f6dd20  docs: record installed-asset import recovery
2026-07-31 18:54:22  db78d5a25  docs: revalidate current iPad package
2026-07-31 19:38:03  2143abdb7  docs: record current touch gameplay trace
2026-07-31 20:09:14  e6ba535a9  fix: share iOS keyboard with primary input
2026-07-31 20:38:16  23029b2c7  docs: record iOS keyboard acceptance
2026-07-31 21:07:44  95d840603  docs: accept GLES renderer equivalence slice
2026-07-31 22:47:21  c05aba78a  docs: accept full iOS GLES golden run
2026-07-31 23:24:24  818bc0e16  fix: verify GLES pixel semantics
2026-07-31 23:39:22  8bf0f4128  docs: accept renderer pixel semantics
2026-08-01 00:15:26  6b2681578  fix: balance UIKit view transitions
2026-08-01 00:33:31  b71153f1c  docs: accept UIKit view lifecycle
2026-08-01 00:56:44  300499d7c  feat: add safe iOS disc reselection
2026-08-01 01:27:58  e8e8e0e1f  docs: accept iOS disc reselection
2026-08-01 02:09:50  828d09580  feat: add customizable iOS controls
2026-08-01 03:05:20  eeaf2c72c  fix: recycle level visibility sidecars
2026-08-01 03:07:24  2951c459e  docs: record controls and visibility diagnosis
2026-08-01 03:13:00  79f4b1bb2  docs: record provisional publication boundary
2026-08-01 03:17:00  d793ecf1b  docs: record safe resource cleanup
2026-08-01 03:18:22  4a4b148dd  docs: record final build-headroom blocker
2026-08-01 04:34:57  8eeebdd48  docs: accept level visibility correction
2026-08-01 04:40:07  faa32dc04  docs: revalidate current iOS package
2026-08-01 04:51:06  2c78c040b  fix: allow iPad portrait resizing
2026-08-01 05:07:17  09d72ab1a  docs: accept iPad orientation reflow
2026-08-01 05:19:39  aad05e284  docs: audit user media and release hygiene
2026-08-01 05:25:39  95dcb67f1  feat: package corresponding source
2026-08-01 05:27:59  4091b602a  fix: stage source package checksum
2026-08-01 05:35:13  21fb81296  fix: exclude signing credentials from source
2026-08-01 05:35:41  71b68f118  docs: accept corresponding-source package
2026-08-01 05:38:29  65c3f4ab3  docs: record hardened source package
2026-08-01 05:47:05  37a5e1676  feat: support isolated signing keychains
2026-08-01 05:52:27  bbb17478c  docs: accept isolated-keychain preflight
2026-08-01 06:13:58  60e8a7aaa  docs: refresh exact Apple build matrix
2026-08-01 06:14:22  34ea4415c  docs: close Apple matrix time ledger
2026-08-01 06:22:16  4673e1f9f  fix: preserve source archive build identity
2026-08-01 06:35:26  25ca4132a  docs: accept extracted source build
2026-08-01 06:35:47  43245107c  docs: close extracted source time ledger
2026-08-01 07:44:51  7bcc51a79  Improve simulator diagnostics and accessible controls
2026-08-01 08:18:19  ba80d153a  Hold quick input until retail poll
2026-08-01 08:55:57  b1663fe5b  Document exact simulator stability checkpoint
2026-08-01 08:56:55  25976cd77  Record simulator checkpoint publication time
2026-08-01 09:10:35  867dbd264  Correct simulator cleanup diagnosis
2026-08-01 09:51:01  ff26c0815  Optimize Simulator presentation and profile splits
2026-08-01 11:28:47  d3b5bd410  Optimize PS1 semitransparency on GLES
2026-08-01 11:45:33  0b9c7125c  Document exact framebuffer fetch acceptance
2026-08-01 12:16:52  13f260cb8  Batch coherent framebuffer-fetch splits
2026-08-01 12:50:47  6de7f17cd  Document exact framebuffer-fetch batching acceptance
2026-08-01 13:05:30  9a33ae00e  Document interactive Crash Cove simulator handoff
2026-08-01 13:57:50  125966b21  Document rejected unified fetch-state batching
2026-08-01 14:12:40  07bbc599b  Record exact restored-renderer replay
2026-08-01 14:56:32  f5140b7eb  Document three-day Apple port checkpoint
2026-08-01 14:58:14  0758e7a80  Merge touch-first ARM64 Apple port (#1)
2026-08-01 15:00:47  32e82d8da  Record main publication boundary
2026-08-01 15:18:42  d5772375f  Rebaseline release acceptance on physical iPad
2026-08-01 15:44:56  cd41e62c5  Document clean Apple release smoke
2026-08-01 15:54:52  79152b7b5  Document UIKit self-test teardown boundary
```

## What the process actually did

The campaign did not move directly from “port” to “done.” It repeatedly used
the same evidence loop:

1. preserve or construct a reproducible producer;
2. reproduce the next failure before editing source;
3. reject evidence that depended on host addresses, stale checkpoints or a
   different executable identity;
4. make the narrowest source correction;
5. rebuild the affected products;
6. compare typed state, pixels, logs, saves, packages or live UI as appropriate;
7. retain negative/rejected results in the journal; and
8. publish a clean commit before advancing the milestone.

That process discovered real LP64 layout faults, rendering-feedback semantics,
UIKit ownership constraints, short-input loss at the retail poll boundary,
interrupted-import recovery gaps, portrait clipping and resource-lifetime
faults. It also prevented several false fixes: raw checkpoint comparison,
cross-executable state restoration, automatic orientation tricks, unsafe live
disc swapping, broad visibility cleanup and renderer prototypes that measured
no better on matched evidence.

## Final local evidence and remaining boundary

The final clean-smoke details, hashes and exact commands are in
[`../parity/2026-08-01-release-rebaseline-clean-smoke.md`](../parity/2026-08-01-release-rebaseline-clean-smoke.md).
The holistic boundary is in [`RELEASE-REBASELINE.md`](RELEASE-REBASELINE.md),
and the test-only UIKit warning assessment is in
[`../parity/2026-08-01-ios-uikit-view-lifecycle.md`](../parity/2026-08-01-ios-uikit-view-lifecycle.md).

At the 15:56:54 boundary, local evidence included:

- 22/22 ordinary macOS tests passing;
- exact logical, blend and presentation pixel checks;
- clean macOS ARM64, iOS Simulator and iPhoneOS builds;
- visible retail boot, menu, character, track and live Crash Cove assets;
- touch and keyboard retail-consumer paths;
- background, foreground, rotation, pause/resume and audio lifecycle logs;
- retail-image and memory-card preservation across an update install;
- reproducible unsigned IPA and matching corresponding-source archive; and
- no targeted asset-reference, visibility, fatal or error log signatures.

It did not include an Apple-issued signing identity, provisioning profile or
connected physical iPad. Therefore it did not prove signed installation,
physical multi-touch feel, hardware cadence, audio latency, thermals or update
persistence on a real device. Those are the remaining completion conditions;
Simulator software-renderer speed must not be substituted for them.
