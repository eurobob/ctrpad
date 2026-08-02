# iOS device observability checkpoint — 2026-08-02

## Outcome

CTRPad now records enough ordinary runtime evidence to decide whether the
Simulator slowdown reproduces on a physical iPad before changing the renderer.
Every 120 displayed iOS frames produce mean, median, nearest-rank p95, p99 and
maximum wall-frame time plus derived FPS. iOS also records startup and change
events for the system thermal state, Low Power Mode, battery state and battery
percentage. The physical-device collector retains both streams beside the
existing session, fault and save evidence.

This is observability, not a performance fix and not physical-device
acceptance. It changes no game state, VBlank policy, rendering commands, touch
mapping, retail data or save format.

## Why this was the next bounded step

The 2026-08-02 Simulator sample localized the visible slow motion to ordered
software-GL triangle work under extreme host contention. The remaining decision
is binary: does target hardware reproduce unacceptable cadence? The previous
device campaign saved average-FPS rows, but not stall distribution, thermal
state or Low Power Mode. Without those values, a slow device run would not say
whether the cause was persistent rendering cost, one transition stall or
thermal/power policy.

The new work closes that evidence gap before any speculative GLES or Metal
rewrite:

- `platform/native_platform.c:44-53` retains 120 inter-frame counter deltas on
  iOS; desktop keeps its existing 2,000-frame report window.
- `platform/native_platform.c:96-153` computes an even/odd median and
  nearest-rank p95/p99 from a sorted copy, so collecting evidence does not
  mutate the live window.
- `platform/native_platform.c:329-381` preserves the existing
  `[CTR Native] FPS:` line and adds one parseable `[CTR FrameStats]` row. The
  interval is between successive `Platform_EndFrame` observations and therefore
  represents wall presentation cadence, including a real stall.
- `platform/native_platform.c:297-301` discards a partial statistics window on
  foreground resume, matching the existing VBlank rebase instead of blending
  background time into foreground cadence.
- `platform/apple/native_ios_telemetry.m:47-57` emits privacy-safe state only;
  it contains no device identifier.
- `platform/apple/native_ios_telemetry.m:60-104` registers main-queue thermal,
  power and battery observers and logs the initial state.
- `platform/apple/native_ios_telemetry.m:107-130` removes those observers and
  restores the caller's prior battery-monitoring setting.
- `main.c:465-489`, `main.c:333-339` and `main.c:571-579` bind telemetry to the
  runtime's successful start, normal stop and disc-reselection cleanup paths.
- `tools/ios-device-campaign.sh:513-545` retains dedicated
  `frame-stats-rows.txt` and `device-state-rows.txt` files and records their row
  counts in the collection manifest.

## Metric definitions

For one window of `N` successive end-frame timestamps:

- `mean_ms` is total wall time divided by `N`;
- `median_ms` is the middle sample, or the mean of the two middle samples;
- `p95_ms` and `p99_ms` use the nearest-rank definition;
- `max_ms` is the largest single inter-frame interval; and
- `fps` is `1000 / mean_ms`.

The first window can legitimately include loading or a transition. Reports must
name the scene and should distinguish transition windows from sustained race
windows. Thermal `nominal` is a system state, not proof that the enclosure felt
cool; battery percentage is context, not an energy-consumption measurement.

## Timestamped implementation and validation

All times below are CDT on 2026-08-02 unless marked UTC.

| Time | Action and exact result |
| --- | --- |
| 01:46 | Re-read the goal, created `codex/device-telemetry` from remote `main` `83a618139`, and proved prior pushed head `008bfe4d1` is its second-parent ancestor. GitHub's compare API reported nothing missing from `main`; PR #25's status field remained stale/open after the earlier 502, so no duplicate merge was attempted. |
| 01:47-01:50 | Audited `Platform_CalcFPS`, UIKit lifecycle hooks, device campaign collection and the acceptance template. Chose gameplay-neutral telemetry instead of another renderer change. |
| 01:50-01:52 | Implemented frame distribution, lifecycle reset, deterministic self-test, iOS state observers and collector row retention. `clang-format` was unavailable on `PATH`; no file was rewritten by that failed command. `git diff --check` and `bash -n tools/ios-device-campaign.sh` passed. |
| 01:51-01:54 | macOS ARM64 compiled with the established 32 warnings and zero errors. Serialized CTest passed 26/26 in 48.58 seconds; the new frame-statistics test passed. |
| 01:55 | The existing device build directory correctly rejected reconfiguration because its cache was pinned to clean source `daba106ae...`. This was retained as an expected source-identity guard result, not called a compile failure. |
| 01:56-02:01 | A fresh temporary iPhoneOS build compiled 247 steps at one low-priority job. `main.c`, import, telemetry and touch all compiled; the app linked with 32 established warnings and zero errors. The result is a thin ARM64 Mach-O, platform iOS, minimum iOS 15.0, SDK 26.5. |
| 02:02-02:04 | The existing sole-Simulator build reconfigured for the dirty development tree and rebuilt eight steps with 32 established warnings and zero errors. No second Simulator was opened. |
| 02:05:03 UTC | Guarded update-install launched build `83a61813967a-dirty`; staged and installed executable SHA-256 both resolved to `0314710e...2dd36`. This is dirty development evidence, not a release identity. |
| 02:05:07 UTC | `[CTR Device]` reported `reason=startup thermal=nominal low_power=off battery_state=unknown battery_percent=-1`. The unknown battery is expected in Simulator. |
| 02:05:25 UTC | The first 120-frame window reported 8.73 FPS, 114.612 ms mean, 16.833 ms median, 19.765 ms p95, 47.647 ms p99 and an 11,732.305 ms maximum. The maximum exposes a startup/transition stall that average FPS alone hides. |
| 02:05:27 UTC | The next 120-frame window reported 55.68 FPS, 17.960 ms mean, 17.008 ms median, 26.954 ms p95, 30.689 ms p99 and 33.432 ms maximum. This proves transition and steady windows remain distinguishable. |
| 02:05-02:06 | Terminated CTRPad by exact bundle ID. The physical-campaign fault regex found zero rows. A broader diagnostic grep found five normal shader-compilation INFO lines; they were inspected and rejected as faults. Exactly one Simulator remained booted and no CTRPad process remained. |

## Exact local evidence

Temporary build and runtime evidence remains outside Git and contains no retail
image:

```text
iPhoneOS configure log SHA-256:
f6064d5d2b7baa6faf6a59ebc13e0330a4d302e183f3f9e16d3957be1a412113
iPhoneOS build log SHA-256:
201a017e68317c22cc77ea1645001219ad5a817836c9f2e7ebd017921a619d0f
temporary iPhoneOS executable SHA-256:
aadc4e6d1e0b60f2f6e061d3947cf165aed7623a2d555046d017e0913b2822de

Simulator configure log SHA-256:
874d78857a726369afc3a9e2fa07d9236817f22353f389036e0f00b166cfcc69
Simulator build log SHA-256:
33119ccddd961663c90319abb5bec5345afb9963e73d86715ee0ab23f20a6108
final runtime log SHA-256:
eb3f18dfed78efd3fa26566f260209d2b837bb9a1e4cc50c40a7e2502acad618
campaign fault scan: 0 rows, empty SHA-256
e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855
```

The guarded Simulator update preserved the slot-zero file exactly:

```text
inode 111222179
size 6016
mtime 1785525736
sha256 6a01b0f5562ed7a279d8f8e51e3b1874ac39a6120f55db4fe3873288950619a3
```

## Remaining decision tree

1. Publish this telemetry source and rebuild/sign that exact clean commit on a
   Mac with a valid Apple identity/profile.
2. On the target iPad, run a named race scene long enough to retain multiple
   120-frame windows and record thermal/power transitions.
3. Complete the touch-only three-lap race, repeated three-boost drift chain,
   lifecycle, audio and save/update checks.
4. If sustained physical cadence is acceptable, do not rewrite the renderer
   for Simulator-only software-GL behavior.
5. If sustained physical cadence is unacceptable, profile the same scene on
   device and optimize the measured draw path under the existing pixel/parity
   gates.

No signed installation, physical touch feel, physical cadence, thermal change,
audio result, complete race or update-persistence result is claimed here.

## Clean implementation and other-Mac handoff

Commit `2bfa532074eb18fb013e4a7d8bc571bc00367e54` froze the 16-file telemetry,
collector and documentation change. The worktree was clean and its staged
scope contained no retail media, packages, profiles, keys, screenshots, build
output or runtime data.

The exact source identity then passed:

- macOS ARM64 build: 32 established warnings, zero errors;
- serialized macOS CTest: 26/26, zero failures in 27.98 seconds;
- iPhoneOS build: 32 established warnings, zero errors;
- iPhoneOS executable: thin ARM64, platform iOS, minimum 15.0, SDK 26.5;
- bundle source identity: full `2bfa532074eb18fb013e4a7d8bc571bc00367e54`;
  and
- intentional signature state: unsigned.

Clean build evidence SHA-256:

```text
macOS configure  c102397313af56d80186eba34b9c86b6f1c92edb3769f065ed07e12a81f3ddcc
macOS build      9a00a00cf9ade3ab285f545a2eb30cb2f24f24f580cdca66596a479176c90708
macOS CTest      112467dd5fc70a31254e658f65fa3669c0db1c0538ea0d22a1e5884352c1814e
iPhoneOS config  9dd5441cb3e5966d1c806ae2aecf40a1e98df7f8757c9d5bf7522599a672c452
iPhoneOS build   af5dbc0b9fa5d055aeeb85d431ef56131c30a7a0c2fb8ae3d5eb75f813c3b3de
iPhoneOS binary  e287a2461781c5eaabafc9f80408894b74c688111ad22a5e278412ff1fc10a3e
```

Two ignored handoff artifacts were created from that commit:

```text
CTRPad-0.1.0-1-2bfa532074eb-unsigned.ipa
size 1462189
members 7
sha256 e47db84e7d84eb1ac369244f2c74953d2e8ebab8fa45213500c7fa46928a344f

CTRPad-source-2bfa532074eb.tar.gz
size 17753198
members 3270
sha256 f483424180d5128611baf61a5ec3b1cca24026e402e91ccea2a8ddb6b490a431
```

The IPA checksum, zip structure, full source/build identity, thin ARM64 iOS
load command, distribution resources and retail exclusion passed. `codesign`
failed as expected for an unsigned handoff. The source sidecar passed from its
`dist/` directory and the archive contains this report plus both telemetry
source files. Retail extensions, runtime state, profiles and keys are absent.
The two broad `dist` directory matches are committed vendored SDL paths under
`externals/SDL/src/hidapi/dist/`, not generated packages.

On another Mac, use the published branch or final `main`, verify the source
archive sidecar from the directory containing it, and follow `docs/INSTALL-IOS.md`.
The IPA can be re-signed only with the tester's matching Apple identity and
profile; no credential is included.
