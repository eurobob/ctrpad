# CTRPad build history

## Purpose

This is the canonical map of the CTRPad Apple-port campaign from goal creation
on 2026-07-29 through the 2026-08-02 three-day-and-eleven-hour checkpoint. It
answers four questions without requiring the original Codex conversation:

1. what was attempted and in what order;
2. what succeeded, failed, or was explicitly rejected;
3. what evidence makes each accepted result reproducible; and
4. what remains before the requested signed physical-iPad release is complete.

This index summarizes the whole process. It does not replace the append-only
records linked below; those retain commands, timestamps, hashes, debugger
findings, corrected mistakes, rejected experiments and exact commit history.

## Exact time boundary

All wall times use `America/Chicago` (`CDT`, UTC-05:00).

| Boundary | Time or duration | Meaning |
| --- | --- | --- |
| Viability research commit | 2026-07-29 13:32:02 | The user-provided feasibility analysis existed before goal execution. |
| Active goal created | 2026-07-29 13:46:41 | Start of Codex goal accounting and implementation work. |
| Fresh-clone proof | 2026-08-02 02:40:02 | Active goal time reached 299,789 seconds: 3 days, 11 hours, 16 minutes, 29 seconds. |
| Low-load history checkpoint | 2026-08-02 03:10:09 | Active goal time reached 301,588 seconds: 3 days, 11 hours, 46 minutes, 28 seconds. |
| Final documentation handoff | 2026-08-02 03:21:12 | The paused goal clock read 301,777 seconds: 3 days, 11 hours, 49 minutes, 37 seconds. PR #28 had already merged the implementation and complete history to `main`; only isolated Markdown/Git publication work followed. |
| Wall-clock span to that checkpoint | 3 days, 13 hours, 23 minutes, 28 seconds | Includes paused time and periods where goal work was not active. |
| Paused/non-active difference | 1 hour, 37 minutes | Wall span minus active goal time; not claimed as implementation time. |

Goal time includes research, editing, compilation, emulation, Simulator boot,
visual checks, profiling, documentation, GitHub publication, waiting and failed
experiments. It is not uninterrupted CPU time or a person-hour estimate.

## Which historical file answers which question

| Record | Role |
| --- | --- |
| [`THREE-DAY-TIMELINE.md`](THREE-DAY-TIMELINE.md) | Timestamped executive chronology plus every durable campaign commit through the current implementation checkpoint. Start here for “what happened when?” |
| [`ENGINEERING-JOURNAL.md`](ENGINEERING-JOURNAL.md) | The command-level reconstruction: root causes, debugger evidence, hashes, temporary artifacts, mistakes, corrections and rejected claims. Start here to reproduce how a result was obtained. |
| [`PROGRESS-LOG.md`](PROGRESS-LOG.md) | Shorter append-only outcome/time/open-boundary ledger used for status and pause/resume handoff. |
| [`THREE-DAY-CHECKPOINT.md`](THREE-DAY-CHECKPOINT.md) | Readable snapshot of the first 2 days, 23 hours and 53 minutes, when the initial Apple port merged. It is a historical boundary, not the current final status. |
| [`RELEASE-REBASELINE.md`](RELEASE-REBASELINE.md) | Release contract after the initial main merge: what Simulator evidence can prove and what must be proven on the physical iPad. |
| [`../ROADMAP.md`](../ROADMAP.md) | Dependency-ordered milestones, current acceptance evidence, risks and remaining work. |
| [`../DECISIONS.md`](../DECISIONS.md) | Durable architecture and evidence decisions, including approaches deliberately rejected. |
| [`../parity/README.md`](../parity/README.md) | Index of focused validation reports, golden traces, UI checks, packaging, signing and handoff evidence. |

At the 03:10 checkpoint these records contain more than 22,000 lines of
maintainer-facing history. The detail is intentionally retained in the repo so
future work does not depend on chat memory.

## Complete phase reconstruction

### 1. 2026-07-29 13:46-17:53 — Evidence foundation before porting

- Read `docs/ctr-native-viability.md` in full and inventoried the actual
  repository/host rather than assuming the research tree matched the checkout.
- Imported CTR Native beta-7.1 history without rewriting it, protected
  `ref/`, retail-media, build, save and signing paths from Git, and wrote the
  dependency-ordered roadmap.
- Reproduced the Linux i686 upstream baseline and the intentional 32-bit Apple
  configuration failure.
- Rejected raw checkpoint bytes as a cross-process oracle because they contain
  host addresses.
- Added canonical game-visible state digests, complete coverage enforcement,
  exact input/timing transport and intentional-mutation detection.
- Rejected the user's first structurally valid disc because it was PAL; the
  later NTSC-U `SCUS_944.26` image became the accepted private input. No retail
  bytes entered Git or release packages.

Primary records: roadmap M0-M2, the journal's 2026-07-29 entries, and the
NTSC-U/state-digest reports under `docs/parity/`.

### 2. 2026-07-29 17:53 to 2026-07-30 14:47 — LP64 conversion by measured boundary

- Audited every pointer-sized field and separated retail/guest 32-bit layout
  from native host ownership instead of mechanically widening serialized data.
- Added guest offsets/translation and host sidecars for checkpoints, levels,
  models, scratch state, runtime maps, renderer lists and overlay-owned data.
- Used sanitizers and real retail playback to find misaligned audio state,
  clipped pointers, free-list invariants, non-idempotent relocation, AI bounds,
  texture references, physics constants and emitter-layout faults.
- Rejected stale/dirty checkpoint evidence and fingerprinted replay inputs to
  exact executable identity.
- Extended replay format coverage across VSync/loader timing, localized RNG
  drift to a widened cutscene record and made canonical allocation state
  independent of host storage geometry.
- Completed a current-source 24,232-frame ARM64 recording and the reproducible
  i686 comparison path.

Primary records: journal LP64/replay sections, `docs/architecture/`, and the
2026-07-30 parity reports.

### 3. 2026-07-30 14:47 to 2026-07-31 06:29 — Full parity and macOS product behavior

- Traced the first cross-width divergence to an invalid relocated AI restart
  index, then closed potion/cutscene emitter layout defects.
- Proved the independent 32-/64-bit processes match eight accepted state
  components for all 24,232 frames and that an intentional mutation diverges
  at the expected component/frame.
- Built the native macOS ARM64 app bundle and verified cadence, save/relaunch,
  XA/audio output, mixer/reverb and renderer behavior.
- Added practical keyboard aliases, preserved quick taps through retail polls,
  and implemented deterministic controller slot ownership/hotplug/rumble.
- Exercised live Time Trial acceleration/steering and full Scrapbook STR
  decode/presentation rather than relying only on headless replay state.

Primary records: full cross-width acceptance, macOS cadence/save/audio,
keyboard/controller and Scrapbook reports.

### 4. 2026-07-31 06:29-10:28 — Shared GLES and native UIKit lifecycle

- Added one shared GLES 3 renderer dialect rather than forking gameplay or
  maintaining an unrelated mobile renderer.
- Presented through SDL/UIKit on a real iPad Simulator surface and corrected
  scene/view/display-link lifecycle ownership.
- Made foreground/background rebase timing without injecting inactive time
  into retail VBlank progression.
- Matched full 24,232-frame macOS GL and iOS GLES traces and locked texture,
  CLUT, transparency, blend, mask, feedback, packed-VRAM and presentation pixel
  semantics.

Primary records: shared-GLES bring-up, iOS lifecycle, full GLES golden and
pixel-semantics reports.

### 5. 2026-07-31 10:28-16:58 — Sandbox, Files import, saves and touch-first play

- Split bundled read-only assets, Documents retail media and Application
  Support logs/saves.
- Implemented security-scoped Files selection, staged sector/region validation,
  atomic replacement and interrupted-import recovery. Invalid replacement input
  preserves the known-good installed image.
- Made memory-card writes durable and atomic; injected failures preserve the
  old save.
- Added a safe-area-aware multi-touch overlay with analog steering, D-pad menu
  ring, Gas/Brake/Item/View, two drift/boost controls, Start/Select, control
  settings and safe disc reselection.
- Added iOS hardware-keyboard input and adaptive portrait/landscape layout.
- Verified touch-only menu/race movement, pause/rotation/resume, persistent
  settings and exact update-install save/media invariants in one Simulator.

Primary records: iOS storage/import/save/touch/keyboard/layout reports.

### 6. 2026-07-31 16:58 to 2026-08-01 14:56 — Packaging, recovery and measured renderer work

- Added deterministic retail-free IPA packaging, optional explicit signing
  keychain/profile/identity handling, embedded GPL/notices/Installation
  Information and exact corresponding-source archives.
- Hardened wrong-region, truncated and interrupted Files-import recovery on
  real native picker routes.
- Reproduced stale/incorrect Simulator installs and added exact installed
  executable identity checks.
- Diagnosed missing geometry/assets through visible capture, logs, pixel
  oracles and matched render traces.
- Accepted coherent framebuffer-fetch presentation and state-aware batching;
  rejected a unified fetch-state batch after live regression.
- Rejected direct RGB5551 decode even though pixels matched because the matched
  Simulator scene was 6.10% slower.
- Wrote the initial three-day checkpoint, full historical journal and release
  handoff rather than erasing failed experiments.

Primary records: iOS package/source reports, Simulator stability/renderer
reports, `THREE-DAY-CHECKPOINT.md`, decisions and journal.

### 7. 2026-08-01 14:56-18:07 — First main merge and physical release re-baseline

- Merged the reviewed Apple port through PR #1, then moved all subsequent work
  to GitHub-backed `codex/` branches.
- Re-baselined “done” around a signed physical-iPad campaign rather than
  treating a successful Simulator run or unsigned package as release complete.
- Rebuilt exact source across Apple targets, preserved update data and created
  device/source handoff artifacts.
- Added guarded exact Simulator installation and a three-phase physical-device
  campaign: read-only preflight, exact install/launch/collection, and human
  acceptance.
- Published the re-baseline and campaign history through PRs #2-#9.

Primary records: `RELEASE-REBASELINE.md`, clean-smoke, exact-installer and
physical-campaign handoff reports.

### 8. 2026-08-01 18:07-20:43 — Apple trust, entitlements and exact evidence identity

- Rejected self-signed/untrusted CMS profiles and pinned trust to Apple roots.
- Bound Team ID, App ID prefix, bundle identifier, signing identity, keychain,
  debugger authorization and minimal entitlement set to the trusted profile.
- Replaced arbitrary-text `devicectl` success with versioned structural JSON
  verification and adversarial fixtures.
- Bound every IPA, installed bundle and campaign manifest to the full
  40-character source commit and matching corresponding source.
- Built an extracted source archive with no `.git` directory to prove the
  package itself is sufficient.
- Published these gates through PRs #10-#19 while leaving the real
  identity/profile/device positive deliberately open.

Primary records: Apple-trust, entitlement, structured-CoreDevice and
source-identity reports.

### 9. 2026-08-01 20:43 to 2026-08-02 00:32 — Sustained and bounded touch controls

- Added explicit one-/three-second accessibility button actions, hold/release,
  full/slight steering and neutralization without bypassing production input.
- Rejected a runaway held-input race attempt and replaced it with bounded
  actions plus stale-timer generation guards.
- Exercised a broad Crash Cove route with retail input-edge logs and preserved
  saves, but ended on `LAP 1/3`; no complete-race claim was made.
- Rebuilt exact Apple targets, passed 25/25 tests at that stage and published
  the accepted control checkpoints through PRs #20-#23.

Primary records: sustained-control and bounded-race-control reports.

### 10. 2026-08-02 00:32-02:23 — Performance root cause and target-device telemetry

- Repeated the unfinished Simulator race and retained it as negative/partial
  evidence instead of calling the game complete.
- Identified Apple Software Renderer at runtime and sampled the live call stack:
  most main-thread stacks were in ordered draw submission/software triangle
  filling while the eight-core host was severely oversubscribed.
- Concluded that Simulator slow motion is a host/software-renderer result and
  does not justify weakening retail resolution, ordering or pixel semantics.
- Added 120-frame mean/median/p95/p99/max wall cadence plus FPS, thermal state,
  Low Power Mode and battery context for the physical campaign.
- Expanded the suite to 26 tests, rebuilt exact Apple products, separated an
  11.7-second transition stall from a later 55.68-FPS Simulator window and
  published telemetry/history through PRs #24-#27.

Primary records: Simulator performance handoff and iOS device observability.

### 11. 2026-08-02 02:23-03:10 — Remote rebuild, Start clarity and resource stop

- Cloned exact GitHub `main` `cb459a198798` through depth-one HTTPS without
  local-object sharing.
- Built macOS ARM64 and iPhoneOS from that clone, passed 26/26 tests and created
  verified matching retail-free unsigned IPA/source artifacts.
- Found that the working production Start input was visibly labeled only
  `PAUSE`; changed it to `START / PAUSE` with accessibility label
  `Start or pause`.
- Visibly tapped that control to advance the retail game to the textured main
  menu and preserved imported media/save identity across guarded update.
- Committed exact implementation `df1b80372` and completed exact-commit Apple
  compiles with the established warnings and zero errors.
- When host load reached `188.43 181.14 128.81`, obeyed the user's full stop.
  Final audit found no booted Simulator, no project process and no push. The
  later history pass stayed Markdown-only and did not restart the app/compiler.

Primary records:
[`../parity/2026-08-02-remote-main-fresh-clone.md`](../parity/2026-08-02-remote-main-fresh-clone.md)
and
[`../parity/2026-08-02-ios-start-control-clarity.md`](../parity/2026-08-02-ios-start-control-clarity.md).

### 12. 2026-08-02 03:05-03:21 — Complete-history publication and handoff

- Reconstructed the full campaign into this canonical 11-phase history,
  timestamped progress log, command-level engineering journal and 212-entry
  pre-publication commit ledger rather than relying on conversation memory.
- Committed the documentation as `a19337a30`, audited PR #28 as exactly two
  commits and 12 files (772 additions, 13 deletions), clean/mergeable and with
  no configured checks.
- The GitHub connector returned HTTP 404 while creating the PR; the documented
  `gh` fallback created the ready PR without changing its reviewed scope.
- PR #28 merged protected head `a19337a30` to `main` as
  `028dd651efc473197c67e0ec8477174891504b3d` at 03:18:01 CDT. GitHub state,
  merge parents, fresh remote resolution and ancestry all passed.
- New customizable-touch and icon work owned by another bot appeared in the
  shared checkout after that merge. It was deliberately excluded from this
  publication and preserved untouched; the final record was prepared in an
  isolated worktree based on `origin/main`.
- No Simulator, compiler or game process was started for this final history
  pass. The goal remained paused at 301,777 active seconds.

## What was deliberately rejected or corrected

The repository records failures because many were plausible shortcuts that
would weaken the requested product:

| Attempt or mistake | Why it was rejected/corrected |
| --- | --- |
| Raw checkpoint equality | Encoded host addresses and could not prove independent 32-/64-bit gameplay equality. |
| PAL retail image | Structurally valid but not the requested NTSC-U retail identity. |
| Wholesale pointer widening | Broke retail serialized layouts; replaced with explicit guest/host ownership. |
| Stale/dirty replay or Simulator evidence | Could attribute a result to the wrong executable; exact source/install identities became mandatory. |
| Screenshots alone as parity | Could miss game-state divergence; retained only as a supplement to traces/digests/pixel oracles. |
| Direct RGB5551 decode | Pixel-correct but 6.10% slower in the matched live scene. |
| Unified framebuffer-fetch state batch | Regressed live frame cost and was restored. |
| Simulator FPS as device truth | Apple Software Renderer and host contention are not target-iPad performance. |
| Runaway held accessibility controls | Could leave input latched and drive off course; replaced with bounded actions/generation guards. |
| `PAUSE` as the only Start label | Functionally emitted Start but concealed how to begin; corrected to `START / PAUSE`. |
| Wrong-directory checksum/profile/cache probes | Retained in the journal, immediately corrected, and never counted as passing evidence. |
| Continuing under extreme load | User ordered a stop; runtime/build activity ended before later Markdown-only publication work. |

## Durable GitHub history

The campaign used reviewable branches and PRs rather than force-pushing the
working history. PR #1 merged the initial Apple port. PRs #2-#9 re-baselined
physical release and added the campaign. PRs #10-#19 hardened Apple trust,
entitlements, structured device evidence and exact source identity. PRs #20-#23
published sustained/bounded controls. PRs #24-#27 published the performance
handoff, telemetry and their history records. PR #28 published the Start-label
correction plus the complete three-day/eleven-hour historical reconstruction;
its exact merge is `028dd651e`.

The exact commit-by-commit ledger—214 campaign commits through PR #28 merge
`028dd651e` at this checkpoint—is in
[`THREE-DAY-TIMELINE.md`](THREE-DAY-TIMELINE.md). That ledger, not a prose
summary, is the authoritative answer to which changes were durable at each
timestamp.

## Accepted product state at the 3-day/11-hour boundary

Accepted locally:

- native ARM64 macOS, iOS and iPadOS builds;
- full accepted 24,232-frame cross-width gameplay-state/timing/physics trace;
- shared GLES renderer and locked pixel semantics;
- real NTSC-U Files import with invalid/recovery guards;
- atomic persistent saves and guarded update preservation;
- controller, desktop/iOS keyboard and touch-first controls, including a clear
  Start/Pause control;
- structured rotating logs and target-device frame/thermal telemetry;
- deterministic retail-free unsigned IPA plus corresponding GPL source; and
- a fresh remote-main rebuild proving no hidden worktree input is required.

Still required for the actual goal:

1. a valid user-owned Apple development identity and matching provisioning
   profile;
2. a connected target iPad and exact signed guarded-update installation;
3. a complete touch-only three-lap race and repeated three-boost drift chain on
   physical glass;
4. target-device cadence/frame pacing, audio latency/output, thermal/power,
   lifecycle/orientation/controller and Files-provider evidence; and
5. cold-relaunch and update-install save/import persistence on that device,
   followed by the final paired signed IPA/source publication.

Until all five have exact evidence, “working bounded Apple port, not signed
physical release complete” is the honest status.
