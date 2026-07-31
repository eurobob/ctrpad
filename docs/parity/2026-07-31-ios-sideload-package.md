# iOS/iPadOS Sideload Package — 2026-07-31

## Result

Commits `6db6116fe67a8cd09d0067c4e8d75e0158db488e`,
`a37cdf2aa5af460c20cd4950f450ab133248077c`, and
`207121134a057e83189a192de5f35706868e03da` add the reproducible, retail-free
iOS/iPadOS packaging and signing-readiness path. The app bundle now carries the
GPL-3.0 text, third-party notices, and source/build/sign/install information.
`package-ios.sh` validates and packages a standard `Payload/CTRPad.app` IPA,
or late-binds a user-supplied Apple identity and provisioning profile without
copying a private key or credential into source control.

Exact current tip `207121134a05` produces two byte-identical unsigned IPAs at
SHA-256 `78b93b01ae92ebb79fb6e92e1b3b29d9cae5984fa7fa5980a96ff0489abe0c63`.
The executable is thin ARM64, targets iOS 15.0 with SDK 26.5, embeds the exact
current source identity, and contains no retail-like file, runtime container,
profile or code signature. This accepts the complete non-secret package path.

It does not accept Apple development/ad-hoc authorization, physical-device
installation, device import/persistence, performance, multi-touch feel, drift/
boost ergonomics or a complete touch-only race. This machine still has no
physical iPad, valid code-signing identity or provisioning profile. A Simulator
ad-hoc signature cannot substitute for those gates.

## Artifact contract

The packager requires:

- `CFBundlePackageType=APPL` and an explicitly valid bundle identifier;
- one Mach-O executable with exactly the `arm64` architecture;
- an iOS `LC_BUILD_VERSION` and the generated minimum OS metadata;
- `LICENSE`, `THIRD_PARTY_NOTICES.md`, and `INSTALL-IOS.md` inside the app;
- a standard top-level `Payload/CTRPad.app` archive; and
- a new output path so an earlier evidence artifact is never silently replaced.

It rejects known retail-media extensions (`BIN`, `IMG`, `ISO`, `CUE`, `CCD`,
`SUB`, `BIG`, `HWL`, `XA`, and `STR`) plus `Documents`, `Application Support`,
or `memcards` directories. It stages only the input `.app`; it never reads the
Simulator/device application container. `dist/` remains ignored.

The unsigned mode removes any inherited `_CodeSignature` and
`embedded.mobileprovision`, then explicitly reports `unsigned`. This archive is
not executable authorization by itself. It is input for a compatible user-side
re-signing tool.

## User-owned signing path

When `--identity` and `--profile` are both present, the script:

1. confirms the identity is visible to the current keychain;
2. decodes the CMS-signed profile with `security cms`;
3. requires iOS in the profile platform list and a non-expired date;
4. matches an exact or wildcard profile App ID to the built bundle ID;
5. optionally requires a specified device UDID in `ProvisionedDevices`;
6. extracts and validates the team/application prefix;
7. constructs exact application, team and keychain group entitlements, plus the
   profile's `get-task-allow` value when present;
8. embeds the profile, requests DER entitlements from `codesign`, and runs a
   strict/deep signature verification; and
9. reads the final signature entitlements back and requires the expected exact
   application identifier before packaging.

CTRPad currently claims no Apple service capability such as iCloud, push,
Game Center, an app group or background execution. The guide explicitly warns
that adding a capability requires regenerating the App ID/profile and updating
this minimal set.

Apple documents that non-macOS third-party code is authorized by a provisioning
profile, that an iOS app embeds it as `embedded.mobileprovision`, and that the
profile binds the signer, App ID, devices, lifetime and entitlements in
[TN3125](https://developer.apple.com/documentation/technotes/tn3125-inside-code-signing-provisioning-profiles?changes=_5).
Apple's
[current signature-format guidance](https://developer.apple.com/documentation/xcode/using-the-latest-code-signature-format)
describes DER entitlements for current iOS/iPadOS. The corresponding commands,
direct `devicectl` install route, import steps and source information are in
`docs/INSTALL-IOS.md`, which is also embedded in every Apple app bundle.

## Exact clean package

The final device build was explicitly reconfigured after commit so generated
identity did not retain a parent or dirty cache. It linked with the 32
established warnings and no error.

```text
source commit     207121134a057e83189a192de5f35706868e03da
device executable build-ios-device-arm64/CTRPad.app/CTRPad
device SHA-256    62e8148d0e32697d09f8f59d40ab34a477ca4cc05df0530c4a1fc264e7e64be9
architecture      arm64
platform          IOS
minimum OS        15.0
SDK               26.5
embedded identity 207121134a05; no dirty suffix
```

Two independent commands wrote different output filenames from the same app.
Both archives had this exact digest and `cmp` returned success:

```text
78b93b01ae92ebb79fb6e92e1b3b29d9cae5984fa7fa5980a96ff0489abe0c63
```

The seven archive members were:

```text
Payload/
Payload/CTRPad.app/
Payload/CTRPad.app/CTRPad
Payload/CTRPad.app/INSTALL-IOS.md
Payload/CTRPad.app/Info.plist
Payload/CTRPad.app/LICENSE
Payload/CTRPad.app/THIRD_PARTY_NOTICES.md
```

`unzip -t` passed. After extraction, `cmp` proved each of the three distribution
resources byte-identical to its source file. The app had no
`embedded.mobileprovision` or `_CodeSignature`. A case-insensitive member scan
found none of the retail extensions. The archive and SHA sidecars remain local
ignored evidence; no IPA, disc image, save, profile, certificate or private key
entered Git.

## Deterministic timestamps

The first corrected-layout archive and a repeat contained identical files but
different ZIP bytes. `cmp` failed at byte 11. The varying values were temporary
`Payload/` mtimes, so content equality was rejected as reproducibility.

Commit `a37cdf2aa5af` sets every staged file and directory mtime to
`SOURCE_DATE_EPOCH`. If the variable is absent, the packager uses the current
source commit timestamp; outside a Git checkout it uses 2000-01-01 UTC. Values
before the ZIP epoch or non-integers are rejected. A later exact pair passed
both SHA-256 equality and byte-for-byte `cmp`.

## Signed-branch structural probe

No Apple profile/identity exists locally, so a device-valid signature was not
manufactured or claimed. A local-only ad-hoc Simulator copy instead exercised
the intended entitlement construction and current `codesign` format without
pretending it could run on hardware. The first probe failed before signing:
`plutil` interpreted `com.apple.developer.team-identifier` as a nested key path.
That exposed a real signed-mode bug.

Commit `207121134a05` switches dotted-key creation to Apple's
`/usr/libexec/PlistBuddy` and validates bundle/team/prefix characters first.
The repeated probe emitted and read back exactly:

```text
application-identifier                 TESTTEAM.io.github.chrissotraidis.ctrpad
com.apple.developer.team-identifier    TESTTEAM
keychain-access-groups[0]              TESTTEAM.io.github.chrissotraidis.ctrpad
get-task-allow                         true
CodeDirectory                          v=20400
```

The ad-hoc signature passed strict/deep verification. `TeamIdentifier=not set`
correctly exposes that it was not an Apple signature. This proves the local
entitlement file, DER request, signature verification and readback mechanics;
it does not prove profile authorization or installability.

## Runtime and persistence regression

An exact parent app at `a37cdf2aa5af` was built for Simulator ARM64 with the
same bundle resources; the only later source change was the packaging script's
dotted entitlement construction. The unsigned Simulator executable SHA-256 was
`21d67a1bc642e672b47aa94c74296bf4fb7c95358eb51e6b74456d11820565b8`.
An ad-hoc signed disposable copy passed strict/deep verification and had signed
executable SHA-256
`ed53ba9f26eba0a8e501f1e02aaabead1016e3b729751ce97d34c0b28879c11c`.

The update installed and launched. A native screenshot showed the retail
copyright screen, rendered text and every touch control; its local-only
SHA-256 is `dff60f6d43e36aff4c252c6848030df2065bd29a20ce407da39484c27be3da8d`.
The Simulator reassigned the data-container UUID but retained the imported BIN
and memory-card inodes, sizes and hashes:

```text
BIN   inode 111131200   605698800 bytes   f780bf2331476aabfc00772fa758b12dd95ebfbc907968132cbd3cdd4e2c07c0
save  inode 111222179        6016 bytes   6a01b0f5562ed7a279d8f8e51e3b1874ac39a6120f55db4fe3873288950619a3
```

The exact final macOS ARM64 build at `207121134a05` passed 21/21 CTests in
1.17 seconds and had SHA-256
`9e90e9adffecc323f1d63e96ddb903f6e2d936711e845328a58ec85daf3fb246`.
This protects the shared input, renderer, lifecycle, storage, save and parity
oracles after packaging changes.

## Rejected and corrected routes

1. An exploratory signing-disabled CMake Xcode-generator configure spent
   several minutes executing individual SDL feature probes through Xcode and
   had not completed by the `atan` check. It was interrupted with no source
   change. The established Ninja device build plus explicit late signing is the
   retained route; the probe is not release evidence.
2. The first IPA used `ditto` without `--keepParent`, so it contained
   `CTRPad.app/` at the ZIP root. The verifier rejected its own output because
   `Payload/CTRPad.app/Info.plist` was absent. `--keepParent` fixed the standard
   layout before any artifact was accepted.
3. The first repeatability check found changing temporary-directory timestamps.
   It produced different SHA-256 values and was rejected despite equal file
   content. `SOURCE_DATE_EPOCH` normalization produced byte-identical output.
4. An injected empty `ctr-u.bin` in a disposable app was rejected before
   packaging. Supplying only `--identity` was also rejected because identity
   and profile must be paired.
5. The first DER probe revealed the dotted-entitlement `plutil` bug described
   above. Its failed output was rejected; `PlistBuddy`, strict verification and
   entitlement readback are retained.
6. `shellcheck` was not installed. `bash -n`, successful/negative runtime
   probes, `git diff --check`, exact builds, archive extraction and direct tool
   readback are the recorded gates; no absent linter is reported as a pass.

## Publication and remaining boundary

All three implementation/fix commits were pushed to
`origin/codex/arm64-apple` and remain in draft pull request
[#1](https://github.com/chrissotraidis/ctrpad/pull/1), not `main`. The complete
source, build script, license, notices, modification history and Installation
Information are public on that branch. The ignored local IPAs are not a binary
release and are not attached to GitHub.

The next decisive gate needs a connected iPad plus a user-owned Apple identity
and provisioning profile. Run the documented signed command, inspect the exact
profile/signature, install the resulting app, import the user's retail image,
verify an update preserves it and the save, then complete natural multi-touch,
drift/boost, full-race, cadence, thermal and performance acceptance. The active
goal is not complete until those hardware results exist.
