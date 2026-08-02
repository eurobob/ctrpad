# Remote-main fresh-clone proof — 2026-08-02

## Outcome

A new shallow HTTPS clone of GitHub `main` at exact commit
`cb459a198798d58adc345e4a4997820d2b580cf9` independently configured, built,
tested and packaged on the Apple Silicon host without using the development
worktree's ignored files, CMake caches or Git objects.

The clone produced:

- a native ARM64 macOS build with the established 32 warnings and zero errors;
- 26/26 serialized tests passing in 10.85 seconds;
- a thin ARM64 iPhoneOS app targeting iOS 15.0 with SDK 26.5, built through all
  247 steps with the same 32 warnings and zero errors;
- a seven-member, retail-free unsigned IPA; and
- a 3,270-member corresponding-source archive bound to the same full commit.

This proves same-host remote-main rebuildability and removes hidden checkout
state as a handoff explanation. It is not an independent second-Mac result,
Apple signing evidence or physical-iPad acceptance.

## Resource preflight

All timestamps are CDT on 2026-08-02.

- host: macOS 26.5 build 25F71, `arm64`;
- Xcode 26.6 build 17F113, iPhoneOS SDK 26.5;
- CMake/CTest 3.27.1;
- initial load averages: `8.25 12.99 19.25`;
- initial free space: approximately 12 GiB;
- development checkout size: 7.9 GiB, including 3.4 GiB of accumulated
  `build-macos-arm64/debug/reports` history; and
- one Simulator was booted but CTRPad was stopped; it was shut down before the
  proof, leaving zero booted Simulators.

No historical output was deleted. A local-object inventory warned about a
268.50 MiB orphan temporary pack in the development `.git`; it was not pruned
because repository cleanup was outside this test. The fresh shallow clone was
81 MiB total with a 19 MiB `.git` directory. Every compile used one job at
`nice -n 15`.

## Timestamped execution

| Time | Action and result |
| --- | --- |
| 02:30:16-02:30:18 | `git clone --depth 1 --branch main --single-branch https://github.com/chrissotraidis/ctrpad.git` created `/tmp/ctrpad-main-fresh.L7QJUr/repo`, resolved exact `cb459a198798...` and reported clean `main...origin/main`. No local reference/shared clone option was used. |
| 02:30-02:31 | Began `macos-arm64` configure. The orchestration wrapper yielded while CMake was still alive; an early build attempt returned `Error: could not load cache`. Process inspection found the sole configure still running. It completed and wrote the cache at 02:31:20. No source changed and the early command was not counted as a build result. |
| 02:31-02:33:28 | `CMAKE_BUILD_PARALLEL_LEVEL=1 nice -n 15 cmake --build --preset macos-arm64` completed all 242 steps with 32 established warnings and zero errors. |
| 02:33:38-02:33:49 | `ctest --preset macos-arm64 -j 1 --output-on-failure` passed 26/26 in 10.85 seconds. |
| 02:34-02:34:58 | Configured `ios-device-arm64`: iPhoneOS, ARM64, GLES, RelWithDebInfo and deployment target 15.0. |
| 02:34:58-02:38:04 | Built all 247 iPhoneOS steps serially. `main.c`, Files import, telemetry, touch and `CTRPad.app/CTRPad` linked with 32 established warnings and zero errors. |
| 02:38:24-02:38:25 | `package-ios.sh` wrote the full-source-identity unsigned IPA and checksum sidecar. |
| 02:38:32-02:38:50 | `package-source.sh` wrote the exact-commit source archive and checksum sidecar with 3,270 members. |
| 02:39 | The IPA checksum passed from repository root. The source sidecar contains a basename, so the first root-directory check could not locate the tarball. Repeating both checks from `dist/` passed without modifying either artifact. |
| 02:39-02:40 | Verified sizes/member counts, full plist identity, thin ARM64, iOS 15.0/SDK 26.5 load command, intentionally unsigned state and negative retail/profile/key scans. A first profile diagnostic accidentally used an unset helper variable and printed root-level paths; the corrected explicit per-user paths were both absent. |
| 02:40:02 | Active goal time was 299,789 seconds: 3 days, 11 hours, 16 minutes and 29 seconds. This is Codex goal time, not uninterrupted compute or person-hours. |

## Exact artifacts

```text
source commit
cb459a198798d58adc345e4a4997820d2b580cf9

CTRPad-0.1.0-1-cb459a198798-unsigned.ipa
size      1,462,090 bytes
members   7
sha256    c16223884f7d49fc269e605133feb761fe2c15677557446f073df041432bc2ad

CTRPad-source-cb459a198798.tar.gz
size      17,756,079 bytes
members   3,270
sha256    38a55c9247625dbaf370e40972beaa146b8bc387a0e57090261672a760bda531

bundle identifier       io.github.chrissotraidis.ctrpad
bundle version          0.1.0 (1)
full source commit      cb459a198798d58adc345e4a4997820d2b580cf9
short build identity    cb459a198798
architecture            arm64
LC_BUILD_VERSION        platform IOS, minos 15.0, sdk 26.5
signature               intentionally absent
IPA sensitive scan      clean
source sensitive scan   clean
```

The outputs remain ignored in the disposable clone. The previously published
implementation pair for `2bfa532074eb` remains in the development checkout;
commits between it and `cb459a198798` were documentation/publication records.

## Claim boundary

At 02:40 the host still reported zero valid code-signing identities, both
standard per-user provisioning-profile directories absent and no physical
CoreDevice connection. The next release evidence therefore requires a valid
Apple identity/profile and the target iPad. The signed guarded-update campaign
in `docs/INSTALL-IOS.md` must then prove real-device cadence/thermal/audio,
touch race/drift, lifecycle, Files import and save/update persistence.
