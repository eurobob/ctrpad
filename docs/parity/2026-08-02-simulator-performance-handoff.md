# Simulator performance diagnosis and physical-iPad handoff

## Status and decision

The 2026-08-02 resumed Simulator run proves that the exact retail-derived
presentation is coherent and that production touch edges reach the retail pad
consumer. It does **not** prove acceptable game speed or complete playability.

The observed slow motion is not an unexplained hang. Three independent signals
agree that this Mac is CPU-rasterizing a draw-heavy PS1-compatible GLES path
while the host is severely oversubscribed:

1. the runtime identifies `Apple Software Renderer` and GLES 3.0;
2. a live sample places about 70% of the main-thread stacks under
   `DrawOTag -> DrawAllSplits`, primarily inside software `glDrawArrays`; and
3. the eight-logical-CPU host reported load averages of
   `88.64 50.16 32.89` while CTRPad, WindowServer, Codex, File Provider,
   Docker/virtualization and CrossOver workloads were active.

The repository should therefore be packaged and published now for physical
iPad measurement. Another Simulator renderer rewrite is not justified before
the target device is measured. If a real iPad reproduces the bottleneck, that
device profile—not Simulator headline FPS—should choose between further GLES
batching and a Metal translation/backend.

This is a handoff decision, not completion. A signed physical install, a
complete touch race, a repeated three-boost drift chain, hardware cadence,
audio latency, thermals, lifecycle and update/save persistence remain open.

## Exact source and environment

| Item | Observed value |
| --- | --- |
| Campaign branch before this report | `codex/simulator-performance-next` at `cc51f003d52cef0856c40aa2ef0da268b901ecd1` |
| Installed implementation identity | `c56162f68a74` |
| Existing remote `main` | `0fee3da33c045b9b5b6a8d3c75b7947de1e3e99d` |
| Prior publication | PR #23 merged the bounded-control publication record at 2026-08-02 00:32:27 CDT |
| Simulator | `CTRPad Import Validation`, iOS 26.5, UDID `1D19A61F-20B7-46B0-AB52-B3A3406952E2` |
| Booted Simulator count | exactly one |
| App | `io.github.chrissotraidis.ctrpad`, ARM64 iOS Simulator process |
| Renderer | `Apple Software Renderer by Apple Inc.`; GLES 3.0 `APPLE-23.1.1`; GLSL ES 3.00 |
| Drawable | 1,032 x 1,376 points and pixels; framebuffer/renderbuffer 1 |
| PSX feedback path | coherent framebuffer fetch enabled |

The installed executable intentionally names the implementation commit rather
than later documentation-only commits. There was no uncommitted source change
at the start of this audit.

## Timestamped resumed run

All wall times below are Central Daylight Time (`-05:00`). Runtime log times
were retained in UTC and converted by subtracting five hours.

| Time | Action and observation | Result |
| --- | --- | --- |
| 00:57:29 | Cold session opened as implementation `c56162f68a74`; UIKit created the drawable and selected Apple Software Renderer. | Clean initialization; all four PSX shaders and VRAM pipelines became ready. |
| 00:57:55-01:15:05 | Production touch navigation and bounded race recovery exercised Cross, steering, Brake and Pause. | Every cited edge reached `source=retail-poll`; the first race stayed on `LAP 1/3` and repeatedly entered scenery. |
| 01:15:18-01:17:33 | The paused race was inspected, `RESTART` selected with touch steering, and confirmed with Gas/Cross. | Pause menu, restart transition, Time Trial/Crash Cove presentation and green-light start rendered coherently. |
| 01:18:19-01:22:24 | A bounded straight Gas segment was exercised while exact screenshots and FPS samples were retained. | Input was consumed, but complex-scene speed fell from about 7 FPS to 4.32-5.09 FPS. No lap-completion claim was made. |
| 01:21:40 | `/usr/bin/sample` captured the live ARM64 Simulator process for three seconds. | Main thread was rendering, not blocked; the dominant stack was software triangle submission. |
| 01:22 | CTRPad alone was terminated by exact bundle ID to remove renderer load; the sole Simulator remained booted. | No second Simulator was created and no user process was killed. |
| 01:24:27 | Goal API and Git boundary were reread. | Active goal time was 295,248 seconds: 3 days, 10 hours and 48 seconds; branch source was already an ancestor of remote `main`. |

## Visual and input evidence

The following ignored `/tmp` screenshots contain retail-derived pixels and
must not be committed. Their hashes make the local observations auditable:

| Local evidence | SHA-256 | Observation |
| --- | --- | --- |
| `ctrpad-paused-state.png` | `b7e7494980a88ca73e454f5986adfe54fbcb4cbd5966247b76459d93a51287b6` | Coherent Crash Cove, HUD, kart, minimap, pause menu and complete touch overlay. |
| `ctrpad-pause-restart-selected.png` | `655959b98b4d081b365f6db10b505eba082a2667323b5364115834d3bd7afc6b` | `RESTART` selected with production steering input. |
| `ctrpad-race-restarted.png` | `315f58053e2899b218f8c3f752d1c5aae293c73a7641578c2b22c69341530133` | Restart transition flag presentation. |
| `ctrpad-race-countdown.png` | `88f70f76efb39d1011c455956c983fa21d5a56496717dd84d132aedf2be895fa` | Time Trial / Crash Cove track presentation with Crash and kart. |
| `ctrpad-race-start-ready.png` | `0ae793ce83b2e303e3eac03ed87dfce635c2e3a05c363f3cc733ae2ea48f9b9e` | Green light, `LAP 1/3`, timer zero, geometry/textures/HUD present. |
| `ctrpad-race-seg01.png` | `76941b5bd204e50e8d9d91162ddd71bac5e18ac5b5434fc0f21de6db3484ee0d` | Bounded Gas moved the kart from the start and returned to neutral. |

The session log is 19,257 bytes at SHA-256
`94e81785e8aaf63998538de9d3dc238679101db510f4da31335591c13fc48ff3`.
The targeted scan found zero error, fatal, visibility-exhaustion, asset-failure,
shader-failure, abort or assertion rows. Its exact touch records include Gas
`0x4000`, Brake `0x8000`, Left `0x0080`, Right `0x0020` and Pause `0x0008`
down/retail-poll/up boundaries.

The production slot-zero save remained inode `111222179`, size 6,016, mtime
`1785525736` and SHA-256
`6a01b0f5562ed7a279d8f8e51e3b1874ac39a6120f55db4fe3873288950619a3`
across the controlled termination/relaunch work. This accepts local
preservation for the exercised session, not a physical update-install.

## Performance root cause

### 1. Simulator uses a CPU rasterizer

The session opened with:

```text
[CTR Renderer] *Video adapter: Apple Software Renderer by Apple Inc.
[CTR Renderer] *gles version: OpenGL ES 3.0 APPLE-23.1.1
[CTR Renderer] *PSX framebuffer fetch: enabled
```

The sample's loaded images include
`OpenGLES.framework/GLRendererFloat.bundle/GLRendererFloat`, and its main
thread was executing the display-link game loop rather than waiting on a lock.

### 2. Ordered PS1 draw submission dominates the frame

Of 1,768 sampled main-thread stacks, 1,269 were in `RenderSubmit`, 1,236 were
under `DrawOTag`, and the `DrawAllSplits` branches total 1,236. The largest
branch enters:

```text
DrawAllSplits
  NativeRenderer_DrawTriangles
    glDrawArrays_IMM_ES2Exec
      gleDrawArraysOrElements_ExecCore
        GLRendererFloat polygon/triangle fill
```

That is approximately 70% of the sampled main-thread time beneath ordered
draw submission. It agrees with the earlier matched Crash Cove profile:
accepted batching retained 66 renderer calls for the representative
119-split/78-fetch/56-merge state and averaged about 165.879 ms per frame.
Presentation resolve/blit was already reduced to single-digit milliseconds;
screen scaling is no longer the dominant cost.

The PS1 path cannot be collapsed casually. Primitive order, semitransparency,
masking and framebuffer feedback are observable retail semantics. Prior
lower-draw-count prototypes produced exact pixels in fixtures but regressed
the matched live Simulator workload, so they were correctly rejected.

### 3. Host contention amplifies the renderer limit

At 01:20-01:21, the eight-logical-CPU Mac reported load averages of
`88.64 50.16 32.89`. A point-in-time process snapshot included approximately:

- CTRPad 69% CPU;
- WindowServer 62% CPU;
- Codex renderer 92% CPU;
- File Provider 46% CPU;
- Docker backend/virtualization, CrossOver/wineserver and Steam helper work.

These are point samples, not additive normalized utilization, but the load
average alone proves deep oversubscription. Terminating CTRPad removed its
process without touching those user workloads; the one-minute load was still
above 80 immediately afterward because queued work drains over time.

### 4. Game time follows rendered iterations in this route

The Simulator log reports approximately 4.32-11.95 FPS during the tested race
and about 50-56 FPS in lighter early presentations. Because the cooperative
UIKit display loop advances the retail step as frames are produced, a
software-rendered 4-8 FPS scene feels like slow motion and also makes remote
screen-read control loops poor evidence for human steering ergonomics.

## Product state after the audit

### Locally accepted

- native ARM64 products build for macOS, iOS Simulator and iPhoneOS;
- full recorded retail state/parity and GLES pixel oracles remain accepted;
- the user-owned NTSC-U import route, sandbox split and atomic save path exist;
- the exact installed implementation launches and renders coherent copyright,
  title/menu, character, track, kart, HUD, minimap and touch-control assets;
- production touch input reaches the retail consumer, including bounded and
  sustained assistive actions;
- rotating timestamped logs carry build/renderer/input/lifecycle/FPS evidence;
  and
- unsigned IPA and corresponding-source workflows are credential-free and
  bind all 40 source-commit characters.

### Not accepted

- Simulator speed is far below the retail 30-FPS budget in complex scenes;
- this resumed route did not complete even lap one, so it is rejected as a
  complete-race or touch-ergonomics proof;
- repeated three-boost drift chains remain unproven;
- this session's keyboard-capture attempt did not add a new log-confirmed
  keyboard result, although the earlier dedicated keyboard acceptance remains;
- no valid Apple code-signing identity, compatible provisioning profile or
  connected physical iPad exists on this Mac; and
- signed installation, physical cadence/frame pacing/audio/thermals,
  multi-touch feel, Files provider behavior, lifecycle and save/update
  persistence remain unmeasured on target hardware.

## Exact handoff route on another Mac

Start from the published `main` commit named by this report's later package
record. Do not copy build products from a dirty checkout.

```sh
git clone https://github.com/chrissotraidis/ctrpad.git
cd ctrpad
git checkout main
./package-ios.sh --build
./package-source.sh
```

The unsigned IPA can be re-signed with a compatible user-side tool. For direct
Apple signing, follow `docs/INSTALL-IOS.md` and provide a user-owned bundle ID,
Apple Development identity, provisioning profile and device UDID:

```sh
./package-ios.sh --build \
  --bundle-id com.example.yourname.ctrpad \
  --identity "Apple Development: Your Name (TEAMID)" \
  --profile /absolute/path/to/CTRPad.mobileprovision \
  --device YOUR_DEVICE_UDID
```

Then use `tools/ios-device-campaign.sh` for preflight, non-destructive prepare,
human acceptance and collection. The first physical run must record a complete
touch race, repeated three-boost drift chain, sustained cadence, frame pacing,
audio, thermals, lifecycle and save/update persistence. A device failure
becomes the next concrete implementation task; a device pass closes the
performance uncertainty without further Simulator-only optimization.

## Exact package acceptance

Commit `daba106ae988487978547c20ce01aa0656ae2265` froze this report and the
six linked history/roadmap indexes. With CTRPad terminated, exactly one
Simulator still booted, one low-priority compiler job and no parallel test
suite, that clean source produced:

| Artifact | Size | SHA-256 | Result |
| --- | ---: | --- | --- |
| `CTRPad-0.1.0-1-daba106ae988-unsigned.ipa` | 1,459,935 bytes | `67ec61bfa637e2e5d7e6e4e96025bccb647a7c00b9b54bf0d4d79e716a1c34e5` | Seven-member retail-free IPA; thin ARM64 iOS executable; full source and 12-character build identity match; intentionally unsigned. |
| `CTRPad-source-daba106ae988.tar.gz` | 17,735,992 bytes | `da8af630754ec542ee8878ad9ae7e6afcd54a6a0abfca94b80c851c8f4fff3a3` | 3,267 members; retail media, runtime state, packages, profiles and keys excluded. |

Both SHA-256 sidecars passed. Independent IPA extraction found only
`Payload/CTRPad.app` with `CTRPad`, `Info.plist`, GPL license, third-party
notices and installation information. `file` and `lipo` reported a single
ARM64 Mach-O; plist extraction returned the exact full commit and clean short
identity; `codesign` confirmed the expected unsigned state.

The same exact source reconfigured and rebuilt macOS ARM64 with the established
32 warnings and zero errors. Its final serialized test run passed 25/25 with
zero failures in 22.51 seconds, including native state/replay/layout/input/
audio/renderer/lifecycle/storage checks and the iOS entitlement, CoreDevice
JSON and build-identity guards. The iPhoneOS build also completed with the
same established 32 warnings and zero errors.

At 01:37:14 CDT, active goal time was 296,019 seconds: 3 days, 10 hours,
13 minutes and 39 seconds. The artifacts are ignored local handoff products;
their source commit will be published to GitHub. They are not Apple-signed and
cannot launch on an iPad until the user supplies a valid identity/profile or a
compatible re-signing workflow.

Ready PR #24 published the exact two-commit/seven-file handoff. Its protected
head `d44c5b0b06c6...` was clean and mergeable and merged at 01:39:55 CDT as
remote-main `8ed2ce98a9bb...`. A fresh ancestry check passed. The source commit
named by both local artifacts is therefore present on GitHub `main` even
though the generated artifacts themselves remain intentionally ignored.

## Evidence boundary

The runtime log, sample report and screenshots are local diagnostic material.
The screenshots contain retail-derived pixels; the log and sample contain
local container paths. Their hashes and conclusions are durable here, but the
files themselves are intentionally excluded from Git and release packages.
