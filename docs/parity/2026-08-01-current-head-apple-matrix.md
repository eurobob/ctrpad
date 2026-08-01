# Current-head Apple build, test, and unsigned-package refresh

**Date:** 2026-08-01

**Source commit:** `bbb17478c76d7f8868ebfd3a6bf1b6d4cc90bfa5`

**Branch:** `codex/arm64-apple`

**Result:** accepted as a bounded current-head refresh; physical signing and
device acceptance remain open

## Purpose and boundary

Earlier exact build evidence predated the isolated-keychain documentation
checkpoint, and several later audits had deliberately deferred another large
unity compile while the host had very little immediately free memory. This
checkpoint refreshes the ordinary macOS ARM64 product, the macOS ASan/UBSan
product, both iOS ARM64 SDK products, and the reproducible unsigned IPA against
the exact GitHub head above.

This was an in-place reconfiguration and rebuild of established build
directories, not a fresh extracted-source or clean-machine build. It does not
claim an Apple signature, physical installation, physical iPad performance,
hardware lifecycle behavior, or human touch ergonomics. Those acceptance gates
remain open.

## Host-protection procedure

The user had reported system slowdown and required at most one Simulator. The
entire matrix therefore ran sequentially with:

- zero `Simulator` application processes;
- zero booted `simctl` devices;
- one build at a time;
- `nice -n 15` for configure, compile, and packaging work; and
- `--parallel 1` / `-j 1` for all builds and tests.

The initial `memory_pressure` sample reported 53% system-wide memory free.
During the large translations the sampled range was 42–47%; every sample
reported zero throttled pages. Raw immediately free pages briefly fell to the
low thousands, so work remained serialized and no Simulator was opened. The
closing sample reported 47% system-wide free, zero throttled pages, and
9,836.94 MiB of 11,264 MiB swap in use. Swap use was high but did not increase
across the accepted matrix.

Background CoreSimulator services remained available even with the Simulator
application closed. They were not mistaken for an open Simulator window or a
booted device.

## Exact configure and build routes

The standard presets define thin ARM64 macOS and iOS products in
`CMakePresets.json:117-180`. iOS presets explicitly select iOS 15.0, GLES,
UIKit-compatible ARM64, and either the Simulator or iPhoneOS SDK. The iOS
bundle receives only the executable and distribution resources from
`CMakeLists.txt:118-131,169-193`; retail data is deliberately external.

The accepted commands were:

```text
nice -n 15 cmake --preset macos-arm64
nice -n 15 cmake --build --preset macos-arm64 --parallel 1
nice -n 15 ctest --test-dir build-macos-arm64 --output-on-failure -j 1

nice -n 15 cmake -S . -B build-macos-arm64-sanitizers -G Ninja \
  -DBUILD_TESTING=ON -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DCMAKE_OSX_ARCHITECTURES=arm64 \
  '-DCMAKE_C_FLAGS=-fsanitize=address,undefined -fno-omit-frame-pointer' \
  '-DCMAKE_EXE_LINKER_FLAGS=-fsanitize=address,undefined'
nice -n 15 cmake --build build-macos-arm64-sanitizers --parallel 1
ASAN_OPTIONS=symbolize=0:abort_on_error=1:detect_leaks=0 \
UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 \
  nice -n 15 ctest --test-dir build-macos-arm64-sanitizers \
    --output-on-failure -j 1

nice -n 15 cmake --preset ios-simulator-arm64
nice -n 15 cmake --build --preset ios-simulator-arm64 --parallel 1
nice -n 15 cmake --preset ios-device-arm64
nice -n 15 cmake --build --preset ios-device-arm64 --parallel 1
```

Apple's ARM64 AddressSanitizer does not support LeakSanitizer. Leak detection
was disabled for that platform limitation only; address and undefined
behavior findings remained fatal.

All four binaries contain version `0.1.0-beta.7.1`, exact build ID
`bbb17478c76d`, and vendored SDL marker
`SDL-3.4.10-beta-7.1-169-gbbb17478c`.

## Artifact results

| Product | Identity | SHA-256 | Result |
|---|---|---|---|
| macOS CLI | thin ARM64 Mach-O | `279a7965a616995f1c4525322568ad81a4a66519890fb349fb73c05fcfa4aea2` | built |
| macOS ASan/UBSan | thin ARM64 Mach-O | `5172e794d772adc76bffd397f96790fe999cb0bd6a479f7a057ea869df3ca28d` | built |
| iOS Simulator | thin ARM64 Mach-O | `47984fe3f417a3c19060ec4a69d3c26093f24110100dbc1fcc31fefc54c81333` | built |
| iPhoneOS | thin ARM64 Mach-O | `7229ffd73d3e86f45f3c853bfee8270467e68f3a77e9e078c85f0f9088112490` | built |

The ordinary macOS unity compile repeated 32 established conversion,
enumeration, format, and deprecated-API warnings. The sanitizer compile
repeated 59 established warnings because its instrumentation configuration
exposes additional deprecation diagnostics. Each iOS compile repeated the 32
established Apple-target warnings. No compile or link error occurred, and no
warning was relabeled as a new failure.

## Test results

The exact ordinary macOS binary passed all 22 CTests in 3.52 seconds; the
outer wall measurement was 3.69 seconds. The exact sanitizer binary passed the
same 22/22 tests in 6.53 seconds; the outer wall measurement was 6.59 seconds.
There was no ASan or UBSan report.

The suite covers version/build identity, state digest mutation sensitivity,
replay, guest references, asset relocation and cache recycling, input,
collision scratch, vehicle constants and bounds, render lists and pixel
semantics, audio alignment/mixing, lifecycle, sandbox storage, durable memory-
card replacement, checkpoint pointer validation, and particle-emitter layouts
(`CMakeLists.txt:251-341`). iOS presets deliberately set `BUILD_TESTING=OFF`,
so their result is compile/link and static product inspection rather than a
relabeled CTest run.

## iOS bundle and retail-data boundary

Both generated applications report bundle ID
`io.github.chrissotraidis.ctrpad`, version `0.1.0` build `1`, minimum iOS 15.0,
and all four supported iPad orientations. Each bundle is about 3.5 MiB and
contains only:

```text
CTRPad
Info.plist
INSTALL-IOS.md
LICENSE
THIRD_PARTY_NOTICES.md
```

The Simulator linker also emits `_CodeSignature/CodeResources`. Its executable
has an ad-hoc linker signature, identifier `CTRPad`, and no team identifier.
As in prior accepted Simulator work, deep/strict bundle verification reports
`code has no resources but signature indicates they must be present`; the
Simulator accepts this development form, but it is not physical-signing
evidence. The iPhoneOS bundle reports `code object is not signed at all`, which
is the required input state for the guarded unsigned/re-signable packager.

The absence of retail textures, audio, and tracks from either application is
intentional. The native loader searches for `assets/ctr-u.bin`
(`platform/native_assets.c:19-24,189-208,470-524`), and the Files importer
copies the user-selected image to the app's Documents-side
`assets/ctr-u.bin` (`platform/apple/native_ios_import.m:63,316-381`). Therefore
bundle contents cannot prove or disprove runtime scene visibility. The earlier
missing-scene report was separately traced to and corrected in the level-
visibility cache; that visual/device evidence is not reopened by this
retail-free bundle audit.

## Exact unsigned IPA regression

Two sequential packages used exact app input
`build-ios-device-arm64/CTRPad.app` and fixed
`SOURCE_DATE_EPOCH=1785581547`:

```text
env SOURCE_DATE_EPOCH=1785581547 nice -n 15 ./package-ios.sh \
  --app build-ios-device-arm64/CTRPad.app --output <temporary>/one.ipa
env SOURCE_DATE_EPOCH=1785581547 nice -n 15 ./package-ios.sh \
  --app build-ios-device-arm64/CTRPad.app --output <temporary>/two.ipa
```

The runs took 14.32 and 5.02 seconds. Each IPA was exactly 1,449,260 bytes and
hashed to:

```text
05ff4601973413f279b7295f1fd885f74b52ddf011360aae7adb2a8f41c97dab
```

`cmp` succeeded, both sidecars passed independent `shasum -a 256 -c`, and ZIP
validation passed. The archive contained exactly seven members: `Payload/`,
`Payload/CTRPad.app/`, and the five files listed above. It contained no retail
media, runtime save/container, provisioning profile, or code signature. Both
IPAs and sidecars were deleted from the temporary directory after acceptance;
only this reproducible identity is retained in Git.

## Rejected and corrected routes

- `cmake --preset macos-arm64-sanitizers` failed before compilation because no
  such preset exists. The established explicit sanitizer configuration above
  was inspected, reconfigured, and used instead. The absent convenience preset
  is not described as a product-build failure.
- The first signature-status wrapper assigned to zsh's read-only `status`
  parameter and aborted before producing a result. The repeated wrapper used
  task-specific variables and recorded the exact Simulator and device results
  above.
- A first temporary cleanup command passed multiple operands to macOS
  `unlink`, which accepts one path. It removed nothing. The bounded temporary
  files were then enumerated, unlinked one by one, and the empty directory was
  removed. No repository or user media was targeted.

## Acceptance boundary

This accepts exact-current ordinary and instrumented macOS behavior, both iOS
compile/link products, and deterministic retail-free unsigned packaging while
obeying the one-Simulator/resource constraint. It does not accept:

- a fresh extracted-source or clean-machine build;
- the unbuilt `macos-arm64-app` bundle at this source head;
- a real Apple identity/profile or team signature;
- install, launch, cadence, lifecycle, controller, Files-provider, save, or
  touch behavior on a physical iPad; or
- final human multi-touch/drift-boost ergonomics and prolonged device play.

M8, M10, M11, and the overall goal remain active at those boundaries.
