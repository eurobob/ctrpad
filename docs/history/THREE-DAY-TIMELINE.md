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
- The commit ledger contains every campaign commit through sustained-control
  documentation head `76ef53931`: 192 commits. Commands, temporary paths,
  hashes, rejected
  experiments and individual test results are intentionally not duplicated
  192 times here;
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
| Physical-handoff source package | 2026-08-01 18:00:26 | Active goal time reached 3 days, 2 hours, 37 minutes, 8 seconds after committing the handoff and verifying its clean 3,253-member corresponding-source archive. |
| Physical-handoff PR audit | 2026-08-01 18:02:05 | Active goal time reached 3 days, 2 hours, 38 minutes, 44 seconds after pushing both commits and auditing draft PR #7 as clean and mergeable. |
| Physical-handoff main merge | 2026-08-01 18:02:58 | PR #7 merged to `main` as `a19c1a5ef`; the post-merge goal reading reached 3 days, 2 hours, 40 minutes, 4 seconds. |
| Final publication/source boundary | 2026-08-01 18:06:51 | PR #8's history merged as `7087cc6e0`; its final-main source package passed at 3,253 members / `a748e256...2b01`, and active goal time reached 3 days, 2 hours, 43 minutes, 33 seconds. |
| Apple trust/preflight audit | 2026-08-01 18:26:33 | Active goal time reached 3 days, 3 hours, 3 minutes, 14 seconds after correcting untrusted-CMS acceptance and real-profile App ID prefix construction. |
| Apple trust pre-commit repeat | 2026-08-01 18:31:32 | Active goal time reached 3 days, 3 hours, 8 minutes, 18 seconds; 22/22 tests and the Apple-positive/self-signed-negative trust probes passed again. |
| Apple trust main publication | 2026-08-01 18:36:44 | PR #10 had merged the trust/history checkpoint into `main` as `8f3b2be37`; its 3,255-member final-main source archive passed at `34f600e6...6326a`, and active goal time reached 3 days, 3 hours, 13 minutes, 31 seconds. |
| Entitlement-authorization audit | 2026-08-01 18:48:30 | Active goal time reached 3 days, 3 hours, 25 minutes, 19 seconds after replacing suffix-only App ID validation with exact profile/signature authorization and expanding the suite to 23 tests. |
| Entitlement candidate repeat | 2026-08-01 18:51:23 | Active goal time reached 3 days, 3 hours, 28 minutes, 9 seconds after Boolean-type/final-wildcard hardening and a final 23/23 repeat. |
| Entitlement main publication | 2026-08-01 18:55:55 | PR #12 merged the exact authorization checkpoint into `main` as `d9e3c5944`; its 3,258-member final-main source archive and extracted test passed at `760dcee6...68d0`, and active goal time reached 3 days, 3 hours, 32 minutes, 42 seconds. |
| Structured CoreDevice/Simulator recheck | 2026-08-01 19:16:58 | Active goal time reached 3 days, 3 hours, 53 minutes, 30 seconds after closing arbitrary-text `devicectl` evidence and exact-installing the current one-Simulator build. |
| Structured evidence main publication | 2026-08-01 19:29:27 | PR #14 merged the 16-file checkpoint into `main` as `94f4d40ed`; its 3,261-member source archive passed after correcting the independent zsh harness, and active goal time reached 3 days, 4 hours, 6 minutes, 8 seconds. |
| Mandatory JSON-version audit | 2026-08-01 19:38:00 | Active goal time reached 3 days, 4 hours, 14 minutes, 42 seconds after reproducing and rejecting an unversioned CoreDevice envelope and passing 24/24 tests. |
| Mandatory-version main publication | 2026-08-01 19:42:13 | PR #16 merged the exact 13-file checkpoint into `main` as `7e8466901`; active goal time reached 3 days, 4 hours, 18 minutes, 55 seconds. |
| Mandatory-version history publication | 2026-08-01 19:43:47 | PR #17 merged the publication record into `main` as `931a81065`; its final-main 3,261-member source archive passed at `c48923eb...c1c29`. |
| Exact IPA/source identity audit | 2026-08-01 20:17:24 | Active goal time reached 3 days, 4 hours, 54 minutes, 3 seconds (`timeUsedSeconds=276843`) after reproducing stale-package acceptance, binding all 40 source characters, passing 25/25 tests, packaging exact source and visually rechecking the sole Simulator. |
| Extracted-source release package | 2026-08-01 20:29:27 | Active goal time reached 3 days, 5 hours, 6 minutes, 7 seconds after a fresh no-`.git` extraction built all 246 iPhoneOS targets and packaged a full-identity IPA with one low-priority job. |
| Visible pre-publication state | 2026-08-01 20:33:38 | Booted-device JSON again contained exactly one iPad Simulator; foregrounding the existing exact-source app showed the textured mode menu and complete touch overlay, captured locally at `ad7a9098...c41b0`. |
| Final pre-publication regression | 2026-08-01 20:35 | The unchanged implementation passed 25/25 again in 26.40 seconds; source-identity test 25 passed its 2/6 cases in 4.85 seconds. |
| Documentation source checkpoint | 2026-08-01 20:38:30 | Commit `f4c1300f9` recorded the exact-source/history docs; its 3,264-member archive passed exclusions, extraction and corrected-directory checksum at `27bd43cc...63ed0`; active goal time reached 3 days, 5 hours, 15 minutes, 17 seconds. |
| Exact-source main publication | 2026-08-01 20:41:47 | PR #18 merged its audited three-commit/18-file head to `main` as `d29a560e9`; the 3,264-member final-main source archive passed at `9ad1c168...80577`, and active goal time reached 3 days, 5 hours, 18 minutes, 33 seconds. |
| Exact-source history publication | 2026-08-01 20:43:08 | PR #19 merged the publication record to `main` as `88e453999`; local/remote/main aligned and its 3,264-member final source archive passed at `b56a9832...4713`. |
| Sustained-control clean acceptance reading | 2026-08-01 21:37:01 | Clean implementation `a3523c7a8` passed Simulator/device builds, 25/25 tests, exact install/persistence/visual/control checks and matching IPA/source packaging; active goal time reached 3 days, 6 hours, 13 minutes, 52 seconds. |
| Sustained-control documentation package | 2026-08-01 21:44:16 | Documentation commit `76ef53931` recorded the control/timeline checkpoint; its 3,265-member source archive passed sidecar/required-file/exclusion checks at `8f4cb489...57a4`, and active goal time reached 3 days, 6 hours, 20 minutes, 54 seconds. |

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
| 18:00 | Handoff commit and source-package proof | Committed `4349bae8d`; its 3,253-member source archive included the new campaign files, passed two checksum checks and contained no retail/runtime or credential/package matches. |
| 18:01–18:02 | Physical-handoff GitHub review | Pushed the two-commit checkpoint; the private-repo connector returned 404, authenticated CLI fallback opened draft PR #7, and its exact 12-file scope was clean/mergeable with no configured checks. |
| 18:02–18:03 | PR #7 main merge and verification | A mistyped full protected-head SHA was refused without merging; the exact queried head then merged as `a19c1a5ef`, and fetched `origin/main` contained the tool, template and focused report. |
| 18:04–18:06 | History merge and final-main source proof | PR #8 merged as `7087cc6e0`; local/remote heads aligned, the sole Simulator hash stayed exact, and an early asynchronous-package lookup was repeated unchanged after completion to accept the 3,253-member clean source archive. |
| 18:11–18:31 | Apple trust and real-profile preflight | Reproduced untrusted CMS decoding, added Apple-root-pinned profile/app chain verification, corrected App ID prefix construction and app/profile certificate binding, rejected tampered/untrusted/ad-hoc inputs, retained deterministic unsigned packages and passed 22/22 tests; a final 18:31 repeat passed the Apple-positive and self-signed-negative probes, while the real signed/device positive remains open. |
| 18:33–18:36 | Apple trust GitHub publication | Committed the exact 14-file checkpoint, verified its source archive, pushed the branch, recorded the private-repository connector 404, opened/audited PR #10 with authenticated CLI fallback, merged the exact head into `main`, then verified the 3,255-member final-main source archive and its exclusion scan. |
| 18:40–18:51 | Signed-entitlement authorization | Audited Apple's install-time rules, found suffix-only App ID comparison, added exact prefix/team/keychain/debugger/minimal-entitlement binding shared by packaging and device preflight, passed two positive/seven negative fixtures plus two 23/23 CTest runs, and retained deterministic unsigned packaging; real signing/device execution remains open. |
| 18:52–18:55 | Entitlement GitHub publication | Committed and packaged the 16-file checkpoint, used authenticated CLI fallback after the connector's private-repository 404, audited/merged exact PR #12, then verified the final-main source archive, exclusion scan and extracted-source entitlement test. |
| 18:56–18:57 | Entitlement history publication | Recorded the accepted merge/source boundary and merged it through PR #13 as `890ba3f4b`, aligning local branch, remote branch and `main`. |
| 18:59–19:17 | Structured CoreDevice evidence and current-build screen | Reproduced the installed-app arbitrary-text false positive; replaced it with versioned command/result validation; retained the initial JSON-parser failure; passed four positive/nine adversarial fixtures and 24/24 tests; then rebuilt, exact-installed and visibly opened the main menu on the sole Simulator with retail/save identity preserved. |
| 19:26–19:29 | Structured-evidence GitHub publication | Committed the 16-file checkpoint, built its 3,261-member source archive, corrected an independent zsh `path`-array harness mistake, used authenticated CLI fallback after the private-repository connector 404, audited PR #14's exact one-commit scope and merged its protected head into `main` as `94f4d40ed`. |
| 19:34–19:38 | Mandatory CoreDevice schema version | Removed `info.jsonVersion` from the accepted real list envelope, reproduced an incorrect `JSON_VERSION=not-reported` success, made the positive integer version mandatory, added the tenth adversarial fixture and passed 24/24 tests in 16.61 seconds without touching the sole Simulator. |
| 19:40–19:42 | Mandatory-version GitHub publication | Committed the 13-file checkpoint, verified its 3,261-member source archive, used authenticated CLI fallback after the connector 404, audited PR #16 as an exact clean one-commit change, and merged its protected head to `main` as `7e8466901`. |
| 19:43 | Mandatory-version history publication | Committed the exact publication boundary and merged it through PR #17 as final-main `931a81065`; its 3,261-member source archive and extracted 4/10 fixture suite passed. |
| 19:45–20:17 | Exact package/source identity binding | Reproduced a successful IPA from stale dirty binary `890ba3f4be57-dirty`, added full-commit bundle/campaign binding and comprehensive dirty detection, rejected a 12-character “exact” draft, passed 2/6 identity fixtures and 25/25 tests, produced byte-identical IPAs plus matching 3,263-member source, and visibly rechecked full textures/controls/logs on the one Simulator. |
| 20:22–20:29 | Fresh extracted-source package | Extracted exact source with no `.git`, ran the documented `--build --source-commit` path under one-job/low-priority limits, built all 246 iPhoneOS targets and verified a full-identity unsigned IPA plus sidecar. |
| 20:33 | Final visible publication check | Reconfirmed exactly one booted Simulator, foregrounded the existing app without a reinstall, and captured the fully textured mode menu plus touch overlay; the ignored screenshot is evidence only and contains no committed retail graphics. |
| 20:35 | Final regression repeat | Repeated all 25 macOS tests against the unchanged implementation; every test passed, including the exact-source identity fixture. |
| 20:36–20:38 | Documentation source checkpoint | Committed the ten-file history/report checkpoint, packaged its 3,264-member corresponding source, preserved a wrong-directory checksum failure, then passed the unchanged sidecar from `dist/` plus extracted exclusions/identity checks. |
| 20:39–20:41 | Exact-source GitHub publication | Pushed the three commits, used authenticated CLI after the private-repository connector 404, audited PR #18 as an exact clean 18-file change, merged its protected head to `main`, synchronized the branch and verified the 3,264-member final-main source archive. |
| 20:42–20:43 | Exact-source history GitHub publication | Recorded the PR #18/source boundary, merged it through PR #19 as `88e453999`, aligned local/remote/main and verified the 3,264-member final source archive. |
| 20:43–21:04 | Sustained-control diagnosis and first implementation | Re-entered Time Trial/Crash/Crash Cove/No Ghost using the sole Simulator, proved ordinary accessibility activation could not sustain Gas/steering/drift, added explicit holds, and rejected overlapped builder output after terminating both accidental one-job process groups. |
| 21:04–21:15 | Dirty sustained-control live proof | Held/released menu Down and Gas, reached a moving Crash Cove race, held Gas plus Left plus L drift, then added 45% slight steering after full deflection proved too coarse for menu-neutral validation. |
| 21:15–21:18 | Slight-steering and lifecycle-reset proof | Confirmed slight analog did not emit the 68%-threshold menu D-pad edge; added one local/global neutralization route and proved Controls returned every button and stick to neutral. |
| 21:24 | Sustained-control implementation commit | Committed clean implementation `a3523c7a8` before acceptance builds. |
| 21:26–21:32 | Exact Simulator build, install and visible acceptance | Built with one low-priority job, installed staged/installed hash `443d08a9...dabc`, preserved exact retail/save tuples, observed complete graphics, held Gas/slight-right/L drift together and proved settings neutralization with zero targeted log faults. |
| 21:33–21:35 | iPhoneOS and macOS matrix | Clean iPhoneOS compiled; clean macOS compiled and passed 25/25 tests in 27.84 seconds. |
| 21:35–21:37 | Exact IPA/source packaging | Wrote seven-member ARM64 unsigned IPA `781f8c03...ebf0` and 3,264-member source `62562ce9...8ae0`; sidecars and corrected independent scans passed after retaining one wrong-directory audit failure. |
| 21:43–21:44 | Timestamped history checkpoint | Committed the seven-document acceptance/history set as `76ef53931`; its 3,265-member source archive included the new report/timeline, passed its sidecar and returned zero forbidden members. |

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
2026-08-01 15:45:38  9b4a175f5  Merge pull request #2 from chrissotraidis/codex/simulator-performance-next
2026-08-01 15:54:52  79152b7b5  Document UIKit self-test teardown boundary
2026-08-01 15:55:44  cd4804186  Merge pull request #3 from chrissotraidis/codex/simulator-performance-next
2026-08-01 16:39:06  f6834b78e  Document complete timestamped Apple port timeline
2026-08-01 16:40:52  03b4eddad  Merge pull request #4 from chrissotraidis/codex/simulator-performance-next
2026-08-01 17:16:59  b80d7b5af  Correct stale Simulator handoff evidence
2026-08-01 17:18:21  89091839d  Merge pull request #5 from chrissotraidis/codex/simulator-performance-next
2026-08-01 17:32:23  67b4c6276  Enforce exact Simulator install identity
2026-08-01 17:33:38  f9b5d4fa0  Document installer source package proof
2026-08-01 17:35:35  e2dfe9709  Merge pull request #6 from chrissotraidis/codex/simulator-performance-next
2026-08-01 17:59:44  4349bae8d  Add physical iPad campaign handoff
2026-08-01 18:00:56  e7af0bc32  Document physical handoff package proof
2026-08-01 18:02:32  0458baff2  Record physical handoff publication
2026-08-01 18:02:58  a19c1a5ef  Merge pull request #7 from chrissotraidis/codex/simulator-performance-next
2026-08-01 18:04:01  cc332f0db  Record physical handoff main merge
2026-08-01 18:04:38  7087cc6e0  Merge pull request #8 from chrissotraidis/codex/simulator-performance-next
2026-08-01 18:07:25  47edfdfb1  Close physical handoff publication record
2026-08-01 18:07:50  7a31a8fdb  Merge pull request #9 from chrissotraidis/codex/simulator-performance-next
2026-08-01 18:33:14  5af382409  Harden iOS signing trust preflight
2026-08-01 18:35:26  8f3b2be37  Merge pull request #10 from chrissotraidis/codex/simulator-performance-next
2026-08-01 18:37:33  0e1ac22be  Record Apple trust publication
2026-08-01 18:38:15  d0d6e8058  Merge pull request #11 from chrissotraidis/codex/simulator-performance-next
2026-08-01 18:52:18  c89bdfc23  Bind iOS entitlements to provisioning profiles
2026-08-01 18:54:32  d9e3c5944  Merge pull request #12 from chrissotraidis/codex/simulator-performance-next
2026-08-01 18:56:30  1ac3708bf  Record entitlement authorization publication
2026-08-01 18:57:15  890ba3f4b  Merge pull request #13 from chrissotraidis/codex/simulator-performance-next
2026-08-01 19:26:17  55932c2c1  Verify structured CoreDevice campaign evidence
2026-08-01 19:28:59  94f4d40ed  Merge pull request #14 from chrissotraidis/codex/simulator-performance-next
2026-08-01 19:30:23  2000a8cd8  Record CoreDevice evidence publication
2026-08-01 19:31:04  5e8acb3f2  Merge pull request #15 from chrissotraidis/codex/simulator-performance-next
2026-08-01 19:40:20  a139cce1c  Require versioned CoreDevice evidence
2026-08-01 19:41:57  7e8466901  Merge pull request #16 from chrissotraidis/codex/simulator-performance-next
2026-08-01 19:43:09  c3589f129  Record versioned evidence publication
2026-08-01 19:43:47  931a81065  Merge pull request #17 from chrissotraidis/codex/simulator-performance-next
2026-08-01 20:07:03  84456cd95  Bind iOS builds to exact source commits
2026-08-01 20:36:40  f4c1300f9  Document exact iOS source binding history
2026-08-01 20:39:21  4bc222fcb  Record documentation source checkpoint
2026-08-01 20:40:38  d29a560e9  Merge pull request #18 from chrissotraidis/codex/simulator-performance-next
2026-08-01 20:42:40  be7b52e28  Record exact source publication
2026-08-01 20:43:08  88e453999  Merge pull request #19 from chrissotraidis/codex/simulator-performance-next
2026-08-01 21:24:14  a3523c7a8  Add sustained accessible touch controls
2026-08-01 21:43:31  76ef53931  Document sustained control acceptance
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
The later physical handoff and structured CoreDevice claim audit are in
[`../parity/2026-08-01-ios-physical-campaign-handoff.md`](../parity/2026-08-01-ios-physical-campaign-handoff.md)
and
[`../parity/2026-08-01-ios-devicectl-structured-evidence.md`](../parity/2026-08-01-ios-devicectl-structured-evidence.md).
The exact package/source binding audit is in
[`../parity/2026-08-01-ios-source-identity-binding.md`](../parity/2026-08-01-ios-source-identity-binding.md).
The post-publication sustained-control audit, every dirty and clean live
checkpoint, artifact hash and deliberately open device boundary are in
[`../parity/2026-08-01-ios-sustained-accessible-controls.md`](../parity/2026-08-01-ios-sustained-accessible-controls.md).

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

At 21:37:01 CDT the later sustained-control checkpoint had increased the active
goal clock to 281,632 seconds: 3 days, 6 hours, 13 minutes and 52 seconds. It
added explicit sustained accessibility actions, clean exact builds across all
three Apple products, 25/25 tests, exact one-Simulator visual/input/reset/log
evidence and a matching unsigned IPA/source pair. It still did not add Apple
credentials, a physical device, physical multi-touch or a complete on-device
race, so the definition-of-done boundary is unchanged.
