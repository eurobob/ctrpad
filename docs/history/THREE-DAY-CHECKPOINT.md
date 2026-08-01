# CTRPad three-day engineering checkpoint

## Purpose and exact boundary

This document is the readable historical map of the CTRPad port from the
initial viability-only repository through the first nearly-three-day native
Apple implementation campaign. It does not replace the append-only
`PROGRESS-LOG.md`, the command-level `ENGINEERING-JOURNAL.md`, or the exact
reports under `docs/parity/`; it connects them so a future maintainer can
understand why the repository looks the way it does and reproduce the accepted
state without reconstructing the conversation.

At this checkpoint the goal timer reads **258,100 seconds: 2 days, 23 hours,
41 minutes, 40 seconds cumulative**. That clock includes user pauses, waiting,
compilation, emulation, Simulator boot, visual inspection, profiling,
documentation, failed experiments, and publication. It is neither wall-clock
continuous CPU time nor a person-hour estimate.

The source boundary entering this document is
`07bbc599bccc26680105e63dcb51f90268e31bfa` on
`codex/arm64-apple`. The direct RGB5551 decoder tried immediately afterward was
pixel-exact but 6.10% slower on a matched Simulator scene and was restored
before publication. The intended publication therefore contains accepted
source plus the full rejection history, not the slower prototype.

## Honest product status

CTRPad now genuinely does the following:

- builds and runs as a native ARM64 macOS application;
- builds as thin ARM64 iOS/iPadOS Simulator and device bundles;
- preserves the accepted 24,232-frame i686-versus-ARM64 gameplay-state trace,
  fixed-point physics/timing transport, audio mixer oracle, save behavior, and
  renderer pixel semantics;
- presents through SDL/UIKit and GLES 3 on an iPad Simulator;
- imports a user-owned NTSC-U raw MODE2/2352 retail BIN through Files, rejects
  invalid/wrong-region/truncated media, and never packages retail data;
- stores the imported image in Documents and rotating logs/memory cards in
  Application Support, with atomic save replacement and update-install
  persistence;
- accepts native safe-area-aware multi-touch controls, control layout editing,
  touch-only menu/race input, a hardware keyboard, desktop keyboard aliases,
  and SDL controller hotplug;
- produces a reproducible retail-free unsigned IPA, validates an optional
  Apple identity/profile/keychain for signed packaging, and produces a
  deterministic GPL corresponding-source archive with Installation
  Information;
- renders the bounded Crash Cove route coherently with menus, portraits, kart,
  track, transparency, HUD, minimap, and overlay together; and
- retains structured, rotating application logs and per-stage renderer
  profiling sufficient to diagnose the remaining Simulator bottleneck.

It is **not finished**:

- Crash Cove remains roughly 5–6 FPS under the Apple Software Renderer instead
  of the retail 30-FPS budget;
- broad all-level/effect/cache-churn, repeated lifecycle/rotation, a complete
  race, and long-duration Simulator stability are not yet accepted;
- human multi-touch steering/acceleration/drift/three-boost ergonomics have not
  been accepted on physical glass;
- no user-owned Apple development identity/profile and target iPad have yet
  completed the final signed install/run/persistence/performance campaign;
- physical iPad audio, controller, Files provider, update, timing, thermals,
  and lifecycle evidence remain open; and
- final paired IPA/source publication waits on those gates.

“Working, but not yet release-complete” is therefore the accurate summary.

## How the work was done

The campaign used a dependency-ordered evidence loop throughout:

1. inspect the current repository and the viability report;
2. protect all retail/reference material from Git;
3. establish an executable 32-bit baseline and deterministic mutation-sensitive
   oracle before changing pointer width;
4. convert one memory/layout boundary at a time and compare independent
   32-/64-bit processes rather than trusting screenshots;
5. make macOS ARM64 the live diagnostic gate before iOS;
6. add shared GLES, UIKit lifecycle, sandbox/import/save, and input layers in
   that order;
7. build exact committed revisions after dirty-source experiments;
8. run desktop tests, desktop pixel tests, actual iOS surface oracles, live
   retail routes, preservation hashes, and targeted log scans;
9. profile matched draw/split populations, not unmatched headline FPS; and
10. retain rejected approaches and operational mistakes in the repo instead of
    rewriting history around the successful path.

Resource discipline was tightened after the Mac became slow: at most one iPad
Simulator is booted; the protected validation device is normally kept off;
Simulator GUI is closed before compilation; Apple builds use nice level 15 and
one job; app termination uses the exact bundle ID; retail/save identity is
checked across installs; and task-owned temporary signed copies are isolated.

## Chronological milestone map

### 2026-07-29 — Evidence foundation and retail identity

The repository initially contained only the viability report, ignore file, and
stale reference README. M0 inventoried the actual tree, imported upstream
CTR Native history at beta 7.1 without altering its source baseline, added the
`upstream` remote, created `codex/arm64-apple`, and made retail/reference paths
untrackable. The user's initial CloneCD image was structurally valid but PAL;
later supplied NTSC-U media identified as `SCUS_944.26` and became the accepted
input. No retail bytes entered Git.

M1 created a reproducible Linux i686 container and canonical state digest.
Raw checkpoints were rejected as an oracle because they encode host addresses.
The accepted harness records game-visible components, input transport, timing,
and saves; requires complete coverage; fails automation on divergence; and
detects an intentional mutation at the first changed component.

Primary evidence:

- `docs/ctr-native-viability.md`
- `docs/parity/NTSC-U-GOLDEN-RUN.md`
- `docs/parity/2026-07-29-golden-run-result.md`
- `docs/parity/STATE-DIGEST.md`
- `docs/parity/TOOLING-ASSESSMENT.md`

### 2026-07-30 to early 2026-07-31 — LP64 conversion and macOS ARM64 parity

The port replaced assumptions that a host pointer fits in a 32-bit serialized
slot with explicit guest offsets/translation and host-side ownership where
needed. Mechanical pointer-width, serialized asset relocation, scratchpad,
checkpoint, renderer-offset, AI bound, mosaic, potion emitter, and cutscene
emitter defects were exposed by real playback rather than fixed wholesale.
Above-4-GiB and malformed-input probes prevented accidental pointer truncation.

Two unchanged i686 processes under different host/raw-checkpoint layouts and
the ARM64 process ultimately matched all eight accepted state components for
24,232 frames. The mutation run diverged at frame 1,711 with `drivers` first.
The current-format replay reached lap two at frame 21,300, closing the concern
that only bootstrap/menu behavior had been tested.

The macOS application then gained an ARM64 bundle/launcher and exact cadence,
save/relaunch, XA/audio-output, mixer/reverb, keyboard-tap, controller-hotplug,
and live race-movement evidence. Practical keyboard aliases were added for
testing without a controller.

Primary evidence:

- `docs/parity/2026-07-30-arm64-full-regeneration.md`
- `docs/parity/2026-07-31-full-cross-width-acceptance.md`
- `docs/parity/2026-07-30-macos-arm64-cadence.md`
- `docs/parity/2026-07-30-macos-arm64-save-relaunch.md`
- `docs/parity/2026-07-30-macos-arm64-audio-output.md`
- `docs/parity/2026-07-31-macos-arm64-audio-mixer-oracle.md`
- `docs/parity/2026-07-31-macos-arm64-keyboard-tap.md`
- `docs/parity/2026-07-31-macos-arm64-controller-hotplug.md`

### 2026-07-31 — Shared GLES and native iOS lifecycle

The desktop GL renderer gained a shared GLES 3 dialect rather than a parallel
renderer. SDL/UIKit presentation, scene/display lifecycle, safe background and
foreground behavior, and iOS-owned entry were then brought up on a real ARM64
iPad Simulator surface. A full regenerated iOS GLES trace matched the current
macOS desktop-GL trace for all 24,232 frames. Focused pixel oracles locked
4/8/16-bit textures, CLUT, zero transparency, STP, four blend modes, ordered
overlap, mask semantics, feedback, packed VRAM, and final presentation hashes.

Primary evidence:

- `docs/parity/2026-07-31-shared-gles3-dialect-bringup.md`
- `docs/parity/2026-07-31-ios-simulator-gles-bringup.md`
- `docs/parity/2026-07-31-ios-lifecycle-display-loop.md`
- `docs/parity/2026-07-31-ios-gles-desktop-gl-equivalence.md`
- `docs/parity/2026-07-31-ios-gles-full-golden.md`
- `docs/parity/2026-07-31-renderer-pixel-semantics.md`

### 2026-07-31 — Sandbox, import, persistence, and input

The iOS bundle/Documents/Application Support ownership model replaced desktop
relative-path assumptions. Files import stages a security-scoped selection,
validates raw sector structure and retail identity, atomically installs only a
valid image, recovers interrupted stages, and starts the retail runtime in the
same process. An existing verified import wins over invalid replacement input.

Memory-card writes use same-directory durable temporary files and atomic
replacement. Injected write failures preserve the old save. Fresh launch,
game-created save, background/foreground, cold relaunch, update install, and
later retail Load-screen reading were exercised in Simulator.

The native touch overlay composes into player one, supports multiple held
actions plus analog steering and menu outer-ring input, respects safe areas,
and exposes accessible buttons. Hardware keyboard input was moved into the
same primary input owner. Touch-only and keyboard-only routes reached race
screens. iPadOS 26 uses adaptive scenes/orientations instead of relying on the
deprecated full-screen lock.

Primary evidence:

- `docs/parity/2026-07-31-ios-sandbox-storage.md`
- `docs/parity/2026-07-31-ios-files-import.md`
- `docs/parity/2026-07-31-ios-memory-card-atomicity.md`
- `docs/parity/2026-07-31-ios-touch-controls.md`
- `docs/parity/2026-07-31-ios-hardware-keyboard.md`

### Late 2026-07-31 to 2026-08-01 — Packaging and GPL publication path

`package-ios.sh` now produces a deterministic retail-free unsigned IPA or,
when explicitly supplied, validates a compatible Apple identity, provisioning
profile, device, entitlements, and optional isolated keychain before signing.
It never owns or exports the private key. `package-source.sh` produces the
exact deterministic GPL corresponding source with vendored SDL, build inputs,
notices, modification history, and Installation Information; it rejects dirty
trees, retail/runtime data, credentials, profiles, binaries, and IPAs. A fresh
extracted archive with no `.git` built ARM64 and passed all 22 tests while
retaining source identity.

Primary evidence:

- `docs/INSTALL-IOS.md`
- `docs/parity/2026-07-31-ios-sideload-package.md`
- `docs/parity/2026-08-01-corresponding-source-package.md`
- `docs/parity/2026-08-01-ios-isolated-keychain-signing.md`
- `docs/parity/2026-08-01-current-head-apple-matrix.md`
- `docs/parity/2026-08-01-extracted-source-build.md`

### 2026-08-01 — Simulator stability, controls, and graphical diagnosis

The apparent missing-asset/stuck reports were treated as release blockers.
Renderer and lifecycle logs became timestamped, severity-tagged, flushed, and
rotated across five sessions. Accessible controls gained correct down/up
semantics. Quick inputs are held until the retail input consumer actually
observes them instead of expiring after an arbitrary render cadence. Level
visibility sidecars are reclaimed on level unload, eliminating a real
long-session cache-lifetime problem. Disc reselection, control settings, UIKit
view transition balancing, portrait resize, and orientation messaging were
also exercised.

The accepted exact route churned Crash Cove and Roo's Tubes, rotation, Home,
resume, keyboard and touch input without known asset/cache/app-fault markers.
Later bounded routes repeatedly showed coherent copyright/title/menu,
character, track, ghost, loading, and Crash Cove grid frames. This closes the
specific observed route, not every content/lifecycle combination.

Primary evidence:

- `docs/parity/2026-08-01-level-visibility-cache.md`
- `docs/parity/2026-08-01-ios-disc-reselection.md`
- `docs/parity/2026-08-01-ios-control-settings.md`
- `docs/parity/2026-08-01-ios-uikit-view-lifecycle.md`
- `docs/parity/2026-08-01-ios-orientation-hint.md`
- `docs/parity/2026-08-01-simulator-stability-logging.md`

### 2026-08-01 — Measured renderer optimization

Per-stage profiling localized the old Crash Cove frame to packed-VRAM
presentation and split submission. Logical-size packed-VRAM resolve plus
nearest host blit preserved pixels and reduced the presentation bucket from
47.201 to 7.737 ms. Coherent framebuffer fetch then replaced correct but
expensive two-pass semitransparency on supported GLES, with exact fallback
comparison. Same-state coherent-fetch batching reduced the representative
119-split/78-fetch state from 122 to 66 calls while preserving order and the
pixel oracle.

Three tempting follow-ons were rejected after live matched profiles:

- dynamic unified format/state shader: 26 calls, 248.567 ms;
- format-specialized per-primitive state shader: 52 calls, 236.441 ms; and
- direct RGB5551 integer texture decode: 66 calls, 198.412 ms versus the
  accepted matched 187.002 ms.

This is why the accepted source still contains the tiny RGB lookup texture and
does not chase minimum API-call count. Exactness plus lower live cost—not code
novelty—controls retention.

Primary evidence:

- `docs/parity/2026-08-01-simulator-renderer-profile.md`
- `docs/parity/2026-08-01-ios-framebuffer-fetch.md`
- `docs/parity/2026-08-01-ios-framebuffer-fetch-batching.md`
- `docs/parity/2026-08-01-ios-unified-fetch-state-rejection.md`
- `docs/parity/2026-08-01-ios-direct-rgb5551-decode-rejection.md`

## Accepted current evidence boundary

`07bbc599bccc` is the most recent published accepted-record commit before this
documentation checkpoint. It records the exact replay of source-identical
`125966b21f19`; the executables below correctly embed `125966b21f19`, while the
later `07bbc599bccc` change is documentation only:

- macOS nice-15/one-job build: 65.02 seconds, 32 established warnings;
- macOS thin ARM64 executable:
  `7c31361034ed04a6fd5d05ae8c435d6b2ba9d7319e44594d4c1d1a0231d21d0a`;
- 22/22 tests: 2.57 seconds;
- desktop pixel oracle: logical `851169f2644a1675`, blend
  `0c0d08324ae06c35`, presentation `a7798c5a6ddee965`;
- iOS nice-15/one-job build: 71.72 seconds, same warning set;
- iOS unsigned/signed executable:
  `aa9b648321d0d48db079cf335225368622ef79708bc7b03a66837f9f25cb23b4` /
  `4cca94f178f3b09dddbd5c4fc49c4556c27c53648b16fe275e7d4a92c0ba6fad`;
- actual-surface oracle: 12 fallback draws, 5 coherent-fetch draws, logical
  and blend hashes above, presentation `172d49a34571b64c`;
- exact live CSV: 2,375 complete records plus one explicitly excluded partial
  row, SHA-256 `eb5bc3c0566b5bdf7bbfca8acb107aaedc3b5151bdbe434a02cdc701cf0801f3`;
- exact live log: 75 lines / 8,702 bytes, SHA-256
  `a1d98d657307f7751dc2b7e09d5d09d214747130934d0c1aabd1feb0ec41107b`,
  zero targeted faults; and
- retail/save inode, size, and SHA-256 remained unchanged.

The current direct-decode report adds a later pixel/visual/preservation pass
but rejects its performance and restores source. A documentation-only exact
source rebuild is not necessary before merging the history; anyone producing
an IPA must build the clean merged commit so the embedded identity matches.

## Rebuild on another ARM64 Mac and test on an iPad

Use a clean clone of the merged commit. Do not copy `ref/CTR`, any BIN/IMG,
`assets/`, saves, profiles, certificates, keychains, `build-*`, or `dist/` from
this development machine.

```sh
git clone https://github.com/chrissotraidis/ctrpad.git
cd ctrpad
git checkout main

# Simulator development build
cmake --preset ios-simulator-arm64
cmake --build --preset ios-simulator-arm64 --parallel 1

# Device bundle and reproducible retail-free unsigned IPA
./package-ios.sh --build

# Matching GPL corresponding source
./package-source.sh
```

For a directly installable build, supply your own App ID, Apple signing
identity, provisioning profile, and device ID exactly as documented in
`docs/INSTALL-IOS.md`. The repository contains no signing credential and
cannot manufacture one. Preserve the same bundle ID for update installs;
deleting the app deletes its private container unless the sideload tool backs
it up. On first launch, choose your own NTSC-U single-track MODE2/2352 BIN in
Files. The retail image is imported at runtime and is never part of the IPA.

## Historical fidelity and where to look next

This checkpoint intentionally records failed commands and rejected designs at
the level needed to avoid repeating them. The complete chronological record is:

- `docs/history/PROGRESS-LOG.md` — concise running status and elapsed ledger;
- `docs/history/ENGINEERING-JOURNAL.md` — detailed commands, recoveries,
  artifacts, hashes, and reasoning;
- `docs/DECISIONS.md` — durable architectural decisions and acceptance bounds;
- `docs/ROADMAP.md` — milestone dependencies, accepted evidence, and open
  gates; and
- `docs/parity/README.md` — index of exact evidence reports.

The next optimization must start from the clean merged source on a new
`codex/` branch. It should target measured split/triangle cost without changing
retail resolution, ordering, blend/STP, mask, feedback, physics, or timing; pass
22 tests and both desktop/iOS pixel oracles; profile matched Crash Cove states;
exercise wider scene/effect/cache churn; and keep the physical-device gate
closed until the Simulator route is stable and diagnosable.

## GitHub publication outcome

The reviewed eight-file history checkpoint was committed as
`f5140b7eb40945ed706502ade83dd7f2ed9048cd`. Its clean-tree source-package
preflight took 18.44 seconds, included 3,244 members, excluded retail/runtime/
package/profile/key material, and produced archive SHA-256
`dc9d5ad2bd29faddf273d1981e95009de47351e144f452b41fa4c95b2f353406`.
The branch push was verified by comparing the local, remote-tracking, and
`ls-remote` object IDs.

GitHub then reported private PR #1 clean and mergeable at that exact head. It
was marked ready and merged with history preserved into `main` as
`0758e7a804390ebd8a7cc74ba4cdcaf864270717`. A fresh fetch, the GitHub branch
API, and `origin/main` all returned that object; the reviewed head is its
ancestor, the repository default branch is `main`, and the merged tree contains
the device/source packagers, Installation Information, renderer, focused
rejection report, and this checkpoint.

Further work moved to GitHub-backed branch
`codex/simulator-performance-next`, created directly from the merge commit.
Packaging that exact merged commit took 22.06 seconds and again included 3,244
members with prohibited material excluded. Its source archive is
`CTRPad-source-0758e7a80439.tar.gz` at SHA-256
`873de12d84d7f9dd55ff9bc5ea73fe8f7538282d93906f1915554560e690120e`.
Generated archives remain ignored local artifacts; the authoritative rebuild
input is the merged Git source.

The publication-close reading was 258,793 seconds: 2 days, 23 hours,
53 minutes, 13 seconds cumulative, 693 seconds (11 minutes, 33 seconds) after
the rejection/history boundary. The overall goal remains active.
