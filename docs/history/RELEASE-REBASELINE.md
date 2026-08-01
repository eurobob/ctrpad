# CTRPad release re-baseline

## Purpose and boundary

This 2026-08-01 review steps back from individual renderer experiments and
maps the actual product state to the goal's definition of done. It begins from
GitHub `main` merge `0758e7a804390ebd8a7cc74ba4cdcaf864270717` and follow-up
documentation branch `codex/simulator-performance-next` at
`32e82d8da1bd76e0330ce8d78f4d4ef9af6cf96a`.

The review opened at 259,501 seconds of cumulative goal time: 3 days, 5
minutes, 1 second. That is wall-clock goal duration, including user pauses,
builds, test playback, Simulator work, documentation, profiling and GitHub
publication. It is not a person-hour or compilation estimate.

## Product conclusion

CTRPad is a working bounded Apple port, but it is not yet a release-complete
iPad product. The evidence does not support the simpler claim that "the game
does not work":

- the accepted i686 and ARM64 producers match eight gameplay-state components
  for all 24,232 frames, and the mutation run fails at the injected divergence
  (`docs/history/THREE-DAY-CHECKPOINT.md:121-139`);
- macOS ARM64, iOS Simulator ARM64 and iPhoneOS ARM64 products build from one
  codebase, while the GLES trace matches the desktop-GL trace for those 24,232
  frames (`docs/history/THREE-DAY-CHECKPOINT.md:152-169`);
- Files import, retail-image validation, atomic persistent saves, UIKit
  lifecycle, touch, keyboard and controller composition exist and have focused
  tests (`docs/history/THREE-DAY-CHECKPOINT.md:171-197`); and
- bounded two-track Simulator routes show coherent menus, characters, track
  geometry, kart, effects, HUD, minimap and touch overlay with structured
  rotating logs (`docs/history/THREE-DAY-CHECKPOINT.md:220-245`).

The release is incomplete because no user-owned Apple identity, provisioning
profile and target iPad have yet completed a signed install, real-device
performance, touch-feel, audio, Files, lifecycle, update and persistence
campaign (`docs/history/THREE-DAY-CHECKPOINT.md:53-65`).

## What made the application look broken

Three genuine defects were found behind the reports of missing assets and
stuck input:

1. level-visibility sidecars outlived their level and could be reused against
   later content;
2. short native input edges could expire across host VSync refreshes before
   the slow retail consumer polled them; and
3. the original single-session log destroyed the previous run and lacked the
   timestamps/build identity needed to diagnose an intermittent failure.

Those defects were corrected and the inspected routes were replayed. The
remaining stuck feeling is dominated by the iOS Simulator's Apple Software
Renderer. A representative accepted Crash Cove state remains about 166-187 ms
per presented frame while retail 30 FPS has a 33.3-ms budget. Pixel-exact
presentation resolve and coherent-fetch batching materially reduced specific
costs, but three later exact candidates regressed live Simulator time and were
rejected (`docs/history/THREE-DAY-CHECKPOINT.md:247-275`).

This evidence proves that the Simulator route is slow. It does not prove that
an iPad GPU is slow, and it must not be used to infer physical-device cadence.

## Corrected acceptance ownership

The earlier plan kept physical-device work closed until the Simulator route
was both correct and fast. That was a reasonable diagnostic precaution while
graphics were missing, but it became an invalid release dependency after the
bounded visual, lifecycle, input and logging route was accepted.

The acceptance owners are now:

| Requirement | Acceptance owner |
|---|---|
| ARM64 configure/build/link and media-free native tests | macOS/iOS build matrix |
| Retail physics, timing and state transport | cross-width replay/parity oracle |
| GLES texture, blend, mask, feedback and presentation semantics | desktop and actual-surface pixel oracles |
| Coherent bounded graphics, import, save, input and lifecycle diagnostics | exactly one iPad Simulator plus rotating logs |
| Sustained 30-FPS cadence, frame pacing, thermals and energy | physical target iPad |
| Multi-touch steer/gas/drift/three-boost ergonomics | a human on physical glass |
| Signed install, update, Files provider, audio, controller and persistence | user-owned signed build on the target iPad |

Simulator 30-FPS performance is no longer a physical-device prerequisite.
This does not waive Simulator correctness: the clean accepted source must still
build, replace the stale exploratory installed bundle, pass its bounded smoke
route, terminate under the exact bundle identifier, and leave actionable logs.

## Minimum release contract

The first sideloadable release must prove the goal, not every conceivable
content combination. Its required human/device campaign is:

1. sign and install a clean ARM64 iPhoneOS bundle without deleting an existing
   application container;
2. import a user-owned NTSC-U single-track MODE2/2352 BIN through Files and
   reject at least one invalid replacement without losing the accepted image;
3. navigate with touch, complete one race, and perform sustained steering plus
   gas, drift hold and a three-boost chain on physical glass;
4. verify game audio, pause, background/foreground, rotation or resize, and
   optional connected-controller coexistence;
5. create or update a memory-card save, cold relaunch, update-install the same
   bundle identifier, and load the preserved profile;
6. measure real presentation cadence and thermals while scanning the retained
   logs for asset, renderer, input, lifecycle and save faults; and
7. package the accepted signed IPA with the exact matching GPL corresponding
   source archive, Installation Information and honest known limitations.

Exhaustive all-level/effect churn, Simulator 30 FPS, a second renderer backend,
live macOS GLES, every controller model and Adventure-mode completion are
valuable coverage, but they are not substitutes for or automatic prerequisites
of this minimum release contract. Any failure encountered on the required
device route becomes release-blocking and expands the necessary work.

## Shortest dependency-ordered path

1. Freeze the accepted renderer. Do not retain another Simulator optimization
   without exact desktop/iOS pixel oracles and a matched live improvement.
2. Rebuild the clean accepted source with every Simulator shut down; pass the
   native tests and independent renderer pixel oracle.
3. Boot exactly one disposable iPad Simulator, update-install that clean app,
   replay the bounded graphical/input/lifecycle route, inspect the logs and
   shut it down.
4. Obtain the user-owned Apple identity/profile/device inputs and execute the
   physical campaign above using `package-ios.sh` and `docs/INSTALL-IOS.md`.
5. If the physical iPad meets cadence, fix only concrete device defects and
   release. If it reproduces the Simulator bottleneck, profile the physical
   GLES path before deciding whether a Metal translation/backend is warranted.

The repository currently has no valid Apple signing identity, provisioning
profile or connected physical device, so steps 1-3 and release preparation can
continue locally while step 4 remains an external boundary. The goal remains
active until the signed physical campaign and paired publication are complete.

## First clean execution of this plan

Clean commit `d5772375fabc` completed local steps 1-3 and the unsigned part of
step 4. macOS ARM64 passed 22/22 tests and the independent renderer pixel
oracle; the actual iPad Simulator surface passed the established GLES hashes;
an update install preserved the retail BIN and save at identical inodes and
hashes; and the bounded Crash Cove route passed visual, input, Home/foreground,
rotation, log and correct-ID termination checks. Both devices and the Simulator
GUI were then shut down.

The same clean identity produced a thin ARM64 iPhoneOS executable, retail-free
unsigned IPA and exact 3,245-member GPL source archive. The detailed commands,
hashes, visual boundary, console warning, log scan, preservation proof and
limitations are in
`docs/parity/2026-08-01-release-rebaseline-clean-smoke.md`. Real signing and the
physical-device campaign remain open because this machine still has no valid
Apple identity, provisioning profile or connected device.
