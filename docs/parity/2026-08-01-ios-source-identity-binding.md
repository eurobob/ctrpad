# Exact iOS package-to-source identity binding

**Date:** 2026-08-01 CDT

**Implementation commit:** `84456cd95a587a1b23a54e37a103e176052c7b46`

**Result:** accepted for local build, package, corresponding-source and
Simulator evidence; Apple signing and physical-iPad acceptance remain open.

## Why this audit happened

The physical-device handoff already authenticated signing chains,
entitlements and structured CoreDevice results, but it did not prove that the
signed application was built from the source commit published beside it. The
device bundle carried only semantic version `0.1.0` / build `1`. Runtime text
inside the pre-audit executable still identified `890ba3f4be57-dirty`, while
the checkout and final `main` source were `931a81065572...`.

At approximately 19:45 CDT, the following real command succeeded:

```sh
./package-ios.sh \
  --app build-ios-device-arm64/CTRPad.app \
  --output dist/CTRPad-source-linkage-gap-931a8106.ipa
```

The resulting IPA and checksum were valid under the old packager even though:

```text
REPO_HEAD=931a81065572
PLIST_SOURCE_COMMIT=missing
BINARY_BUILD_ID=890ba3f4be57-dirty
```

That is a reproduced false acceptance, not a hypothetical concern. The old
packager validated architecture, resources, retail exclusion and optional
signing, but never compared application metadata to the checkout. The old
dirty check also used only `git diff --quiet`, which did not cover staged or
untracked source.

## Accepted contract

1. A Git build embeds the full 40-character commit as
   `CTRNativeSourceCommit` and keeps the 12-character clean/dirty label as
   `CTRNativeBuildIdentity` (`CMakeLists.txt:86-163`,
   `platform/apple/Info-iOS.plist.in:28-31`).
2. Git dirty state comes from `git status --porcelain=v1
   --untracked-files=normal`, covering staged, unstaged and untracked source
   (`CMakeLists.txt:119-134`).
3. `verify-ios-build-identity.sh` requires a 40-character lowercase commit,
   requires the clean 12-character prefix, optionally compares all 40 expected
   characters and writes no manifest on failure
   (`tools/verify-ios-build-identity.sh:67-116`).
4. `package-ios.sh` refuses a dirty checkout, derives or requires the full
   source commit, passes it into CMake for `--build`, verifies the staged plist
   before architecture/signing work and includes the short commit in the IPA
   name (`package-ios.sh:98-120,141-151,181-189,341-375`).
5. Physical preflight requires the full commit, records it in preflight and
   prepare manifests, and carries the verified identity into collection
   (`tools/ios-device-campaign.sh:145-161,282-293,380-403,438-480,524-538`).
6. Corresponding-source packaging requires both the verifier and its fixture
   test (`package-source.sh:110-132`).

The first draft retained only a 12-character source key. Review rejected that
as insufficient for a claim worded “exact”; the accepted plist and campaign
compare all 40 Git characters. A corresponding-source build outside `.git`
must therefore supply the full commit to both CMake and the packager.

## Negative evidence

| Probe | Exact result |
| --- | --- |
| Package from the uncommitted implementation checkout | exit 1: `source checkout is dirty` |
| Generated dirty plist (`931a81065572-dirty`) | exit 1; no manifest |
| Old false-positive IPA | exit 1: no `CTRNativeSourceCommit`; no manifest |
| Current IPA with all-zero expected commit | exit 1 at the full 40-character comparison; no manifest |
| Current unsigned IPA with correct commit | identity manifest written, then exit 1 for missing `embedded.mobileprovision` before any device operation |
| Fixture suite | 2 clean positives; 6 missing/dirty/wrong/truncated/malformed/mismatched negatives |

One validation command initially passed the `.app` directory rather than its
`Info.plist` to the verifier and correctly failed with `Info.plist not found`.
The unchanged command was repeated with `/Info.plist` and passed. This harness
mistake supplied no accepted evidence.

## Clean implementation result

The accepted clean device bundle reports:

```text
SOURCE_COMMIT=84456cd95a587a1b23a54e37a103e176052c7b46
BUILD_IDENTITY=84456cd95a58
EXPECTED_SOURCE_COMMIT_STATUS=verified
BUNDLE_ID=io.github.chrissotraidis.ctrpad
VERSION=0.1.0
BUILD=1
INFO_PLIST_SHA256=31b70d0d8ecaed71d6edb002c5a9199f614cb1d7ec6bc99b193dbb0cbcc12fd5
```

Two independent unsigned packages were byte-identical:

```text
SHA256=25a8f7511e5f792027f011619b9902255e90048c45e0938fa32f2888f111c191
MEMBERS=7
RETAIL_MEMBERS=0
```

The fully reconfigured macOS ARM64 suite passed 25/25 in 19.53 seconds. Test
25 is the new 2-positive/6-negative identity suite
(`CMakeLists.txt:434-452`).
Immediately before documentation publication, the same built implementation
passed all 25 tests again in 26.40 seconds at 20:35 CDT; identity test 25 took
4.85 seconds and again passed all 2/6 cases.

The matching source archive is:

```text
CTRPad-source-84456cd95a58.tar.gz
SOURCE_COMMIT=84456cd95a587a1b23a54e37a103e176052c7b46
SHA256=5ddf285a00611842c4cecce492d992cf904fa3d60641ce582d12f1952c89fbd9
MEMBERS=3263
EXTRACTED_IDENTITY_TEST=2 positives / 6 negatives passed
```

A separate fresh extraction then exercised the user-facing no-`.git` command,
not merely the verifier:

```sh
CMAKE_BUILD_PARALLEL_LEVEL=1 SOURCE_DATE_EPOCH=<commit-time> \
  ./package-ios.sh --build \
  --source-commit 84456cd95a587a1b23a54e37a103e176052c7b46
```

It configured and built all 246 iPhoneOS targets with one low-priority job,
packaged successfully, verified its sidecar and embedded the exact full source
plus clean short build identity. The extracted-build IPA SHA-256 is
`314d2ef050fc1771a46b7605a00de942f6d92fdd96ecee9c4f96ff121240b0e0`.
Its bytes are not claimed equal to the in-place package because RelWithDebInfo
contains different absolute build paths.

The packager's built-in exclusion scan passed. An independent ad-hoc scan
first reported one apparent forbidden member because its expression matched
`.str` at the start of `InfoPlist.strings`; anchoring extensions to end of path
returned zero on the unchanged archive. The false harness result is retained
here rather than silently omitted.

## Exact Simulator recheck

At 20:14 CDT, exactly one iPad Simulator was booted. The guarded updater
installed the clean Simulator build and proved:

```text
INSTALLED_SOURCE=84456cd95a587a1b23a54e37a103e176052c7b46
INSTALLED_BUILD=84456cd95a58
SOURCE_EXECUTABLE_SHA256=ec75262905f524463d4867429feda6b27d1cfe85bb4cbf7ef8a648f1ffdf2ef5
SIGNED_AND_INSTALLED_EXECUTABLE_SHA256=4b1a6c6f36b8094c7686d401c9740035226fd4f06d7993851429b400d0eaefa3
RETAIL=111131200|605698800|f780bf2331476aabfc00772fa758b12dd95ebfbc907968132cbd3cdd4e2c07c0
SAVE=111222179|6016|6a01b0f5562ed7a279d8f8e51e3b1874ac39a6120f55db4fe3873288950619a3
```

Computer Use observed the copyright screen, animated Crash/trophy/title
assets, complete Adventure/Time Trial/Arcade/VS/Battle/High Score menu and all
touch controls. Touch Cross produced down, retail-poll consumption and up log
rows. The current log contains zero `[ERROR]`, `[FATAL]`, `[CTR AssetRef]` or
visibility rows. Heavy menu animation settled near 7.7 FPS under Apple
Software Renderer, so this is a correctness/visibility/logging result—not
physical-device cadence evidence.

At 20:33:38 CDT, `simctl list devices booted --json` again reported only the
same `CTRPad Import Validation` iPad. Bringing the installed app to the
foreground returned PID 25424 and a five-second-later framebuffer capture
again showed the fully textured Crash/trophy mode menu and complete touch
overlay. The ignored local capture
`dist/CTRPad-simulator-current.png` hashed to
`ad7a9098a015d58e20bb3e5c06d89b57939befa918572d5a9f61efe8f49c41b0`.
It is visual inspection evidence, not a committed retail-derived artifact.

An initial line-count command printed one “physical device” for the literal
empty JSON array `[]`. Structural inspection then verified command type,
success, JSON schema version 3 and `result.devices=[]`; the physical count is
zero. Signing identities are also zero.

## Documentation source checkpoint

The timestamped documentation checkpoint was committed at 20:36:40 CDT as
`f4c1300f9bb749ab901689e9b091eefa06e907f1`. Its corresponding-source archive
contained 3,264 members, returned zero retail/runtime/package/profile/key
matches and passed the extracted two-positive/six-negative identity suite. The
first independent `shasum -c` invocation ran from the repository root even
though the sidecar records a basename, so it failed to locate the archive.
Repeating from `dist/` at 20:38:17 passed on the unchanged files; archive
SHA-256 is
`27bd43cc0deaea5fcb6248c095d0eca0e727c126d5197236ab1f7ac178363ed0`.
The failed harness invocation is not counted as acceptance evidence.

PR #18 publication used the authenticated GitHub CLI after the connected
private-repository endpoint returned 404. Audit found exactly the intended
three commits and 18 files, 931 additions/40 deletions, protected head
`4bc222fcb44c614349c4487431490f1ae97cdb27`, clean mergeability and no
configured checks. After conversion from draft, that exact head merged to
`main` at 20:40:38 CDT as
`d29a560e96f0731cc0528cab123b3bee4ce902c9`. The final-main source checkpoint
contained 3,264 members, passed its sidecar, end-anchored exclusions and
extracted 2/6 fixture, and hashed to
`9ad1c1680cbf325f1567e9ae838fd72c5edaf9d6e75f877e5fd8746ce4380577`.

## Remaining boundary

This closes ambiguous IPA/source pairing and confirms the exact build remains
visually coherent in the one Simulator. It does not supply an Apple-issued
identity, provisioning profile, signed CTRPad positive, connected iPad,
physical multi-touch ergonomics, device cadence/thermals/audio latency, full
touch-only race or physical update/save persistence. Those remain required.

At 20:29:27 CDT, after the extracted-source package completed, active goal time
was 277,567 seconds: 3 days, 5 hours, 6 minutes and 7 seconds.
At 20:38:30 CDT, after committing and independently checking the documentation
source checkpoint, active goal time was 278,117 seconds: 3 days, 5 hours,
15 minutes and 17 seconds.
At 20:41:47 CDT, after PR #18 and its final-main source validation, active goal
time was 278,313 seconds: 3 days, 5 hours, 18 minutes and 33 seconds.
