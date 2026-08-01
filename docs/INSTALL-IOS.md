# Build, Sign, and Sideload CTRPad on iPhone or iPad

CTRPad is GPL-3.0 software. It does not include Crash Team Racing, a disc
image, extracted retail data, saves, an Apple certificate, a private key, or a
provisioning profile. Obtain the source corresponding to this build from
<https://github.com/chrissotraidis/ctrpad>. The app reports its exact source
commit in its build identity; rebuild that commit to reproduce or modify it.

This document is Installation Information for the sideloaded build. It is not
legal advice and does not grant rights to retail game data or Sony/Naughty Dog
marks. Use only your own compatible NTSC-U retail disc image.

## Requirements

- an ARM64 Mac with the current Xcode command-line tools;
- CMake 3.20 or newer and Ninja;
- an iPhone or iPad running iOS/iPadOS 15 or newer;
- Developer Mode enabled on the device for development-signed apps;
- for direct signing, an Apple signing identity in the login keychain and a
  non-expired iOS provisioning profile that authorizes the bundle ID and
  target device; or
- a compatible user-side IPA re-signing tool such as AltStore Classic.

Apple explains that third-party iOS/iPadOS code needs a provisioning profile,
that the profile belongs at `CTRPad.app/embedded.mobileprovision`, and that the
profile constrains the signer, App ID, devices, lifetime, and entitlements in
[TN3125](https://developer.apple.com/documentation/technotes/tn3125-inside-code-signing-provisioning-profiles?changes=_5).
Current iOS releases require signatures with DER entitlements; see Apple's
[latest code-signature format](https://developer.apple.com/documentation/xcode/using-the-latest-code-signature-format).

## Build and produce an unsigned IPA

From a clean source checkout:

```sh
cmake --preset ios-device-arm64
cmake --build --preset ios-device-arm64
./package-ios.sh
```

Or let the packager run both build commands:

```sh
./package-ios.sh --build
```

The result is written under ignored `dist/` with a SHA-256 sidecar. The script
requires a thin ARM64 device executable, an iOS load command, the legal/source
installation resources, and the standard `Payload/CTRPad.app` IPA layout. It
fails if it sees known retail-media extensions or runtime save directories.

Package the exact corresponding source from the same clean committed checkout:

```sh
./package-source.sh
```

This writes `dist/CTRPad-source-<commit>.tar.gz` and its SHA-256 sidecar. The
source packager includes the complete tracked source and build inputs, vendored
SDL,
build/package scripts, GPL license, notices, modification history and this
Installation Information. It fails if tracked changes are present or if the
archive contains retail/runtime data, an IPA, a profile, a certificate or a
private-key-like file. Publish this archive alongside the matching signed or
unsigned IPA; the app's build identity identifies the source commit.

To use a different App ID, configure and verify it explicitly:

```sh
./package-ios.sh --build \
  --bundle-id com.example.yourname.ctrpad
```

The unsigned IPA has no executable authorization and cannot be launched as-is.
A compatible re-signing tool must apply the user's own identity and profile.
AltStore's official documentation describes Classic as a user-side IPA
sideloading system and records current account, expiry, active-app, and
Developer Mode constraints:
<https://faq.altstore.io/altstore-classic/your-altstore> and
<https://faq.altstore.io/altstore-classic/how-to-install-altstore-macos>.
Follow that project's current import/install UI; do not give Apple credentials
to CTRPad or store them in this repository.

## Produce a manually signed IPA

If Xcode has already created or downloaded a compatible development/ad-hoc
profile and its identity is present in the keychain:

```sh
security find-identity -v -p codesigning

./package-ios.sh --build \
  --bundle-id com.example.yourname.ctrpad \
  --identity "Apple Development: Your Name (TEAMID)" \
  --profile /absolute/path/to/CTRPad.mobileprovision \
  --device YOUR_DEVICE_UDID
```

The script decodes the signed profile, rejects expiration/platform/App-ID/
optional-device mismatches, embeds it, constructs only CTRPad's minimal
application/team/keychain entitlements, requests DER entitlements from
`codesign`, verifies the signed app strictly, and packages it. It never copies
the private key out of the keychain. The profile is necessarily embedded in a
directly installable signed app; it is authorization metadata, not the signing
private key.

CTRPad uses no Apple service capability such as Game Center, iCloud, push, an
app group, or background execution. If a later modification adds one, do not
use this minimal entitlement set unchanged: enable the capability in the App
ID, regenerate the profile, and update the signing workflow deliberately.

## Install a signed app directly

Unpack the signed IPA and install its app bundle with Xcode's device tool:

```sh
mkdir -p /private/tmp/ctrpad-install
ditto -x -k dist/CTRPad-0.1.0-1-signed.ipa /private/tmp/ctrpad-install
xcrun devicectl list devices
xcrun devicectl device install app \
  --device YOUR_DEVICE_ID \
  /private/tmp/ctrpad-install/Payload/CTRPad.app
```

The connected device must be unlocked, trusted, in Developer Mode, and covered
by the embedded profile. Do not use the example temporary path as a permanent
backup. Apple documents registered-device development/distribution at
<https://developer.apple.com/documentation/Xcode/distributing-your-app-to-registered-devices>.

## Import the retail image and verify persistence

1. Launch CTRPad. If no valid image is installed, the native Files chooser
   appears.
2. Select your own NTSC-U, single-track raw MODE2/2352 BIN image. CTRPad stages,
   validates, and installs it in the app's Documents container; the image is
   never part of the IPA.
3. Start the game using touch, create or update a memory-card save, background
   and relaunch the app, and verify the profile remains visible.
4. Before installing a replacement build, preserve the same bundle ID and use
   an update install. Deleting the app deletes its container, retail import,
   and saves unless the sideload tool separately backs them up.

For source-build details, evidence boundaries, modifications, and known
limitations, read `README.md`, `docs/ROADMAP.md`, `docs/DECISIONS.md`,
`docs/history/ENGINEERING-JOURNAL.md`, and the reports under `docs/parity/` in
the corresponding source checkout. Physical-device performance, touch feel,
full-race completion, drift/boost ergonomics, and final user-owned signing must
be verified on the actual target iPad; Simulator evidence does not replace
those gates.
