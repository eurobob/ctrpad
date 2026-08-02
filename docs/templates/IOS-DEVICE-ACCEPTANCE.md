# CTRPad physical iPad acceptance record

Copy this file to `docs/parity/YYYY-MM-DD-ios-device-acceptance.md` only after
running the campaign. Replace every placeholder with observed evidence or
`not tested`; never turn an unchecked item into an inferred pass.

Do not commit a retail image, extracted retail file, save, provisioning
profile, certificate, private key, device UDID/serial number, raw `devicectl`
JSON, or unredacted device log. Keep raw campaign evidence beneath ignored
`dist/` or outside the checkout and cite only redacted summaries and hashes
that are necessary to reproduce the conclusion.

## Identity and environment

| Field | Exact value |
| --- | --- |
| Date/time/time zone | `<value>` |
| Source commit | `<40-character Git SHA>` |
| IPA `CTRNativeSourceCommit` | `<same 40-character Git SHA>` |
| IPA `CTRNativeBuildIdentity` | `<clean 12-character prefix>` |
| CTRPad version/build identity from log | `<value>` |
| Signed IPA filename | `<value>` |
| Signed IPA SHA-256 | `<value>` |
| Matching source archive filename/SHA-256 | `<value>` |
| Bundle identifier | `<value>` |
| Profile name/UUID/expiration | `<redacted-safe values>` |
| Signing team | `<team identifier>` |
| Xcode/devicectl version | `<value>` |
| iPad model/iPadOS version | `<model and OS; omit serial/UDID>` |
| Raw evidence directory | `<local ignored path; do not commit contents>` |

## Offline preflight and initial install

- [ ] `ios-device-campaign.sh preflight` completed successfully.
- [ ] The IPA sidecar, ZIP structure and strict signature verified.
- [ ] The build-identity manifest matched all 40 source-commit characters and
      rejected dirty, missing, truncated or mismatched metadata.
- [ ] The profile CMS signature, signer purpose and certificate chain verified
      to an Apple Root CA from the system root keychain.
- [ ] The app signing-certificate chain verified to an Apple Root CA.
- [ ] The profile authorized the exact App ID, signing team and target UDID.
- [ ] The signature's App ID prefix exactly matched the profile prefix; suffix-
      only matching was not used.
- [ ] The sole signed keychain group and optional `get-task-allow` were
      profile-authorized, with no unexpected CTRPad entitlement.
- [ ] The app leaf signing certificate matched one of the profile's authorized
      `DeveloperCertificates`.
- [ ] The executable was thin ARM64 with platform `IOS` and the expected build
      identity.
- [ ] The matching source archive was produced from the same full commit and
      its SHA-256 sidecar verified.
- [ ] The signed app contained GPL/install resources and no retail/runtime
      data.
- [ ] `ios-device-campaign.sh prepare` completed without uninstalling CTRPad.
- [ ] Every retained `devicectl` result had the expected command type,
      `outcome=success`, an explicit positive integer JSON schema version,
      supported tool version and no error object.
- [ ] The exact installed bundle, version, build and `CTRPad.app` URL verified;
      a text search or empty app result was not treated as proof.
- [ ] The exact `CTRPad.app/CTRPad` process launched with a positive PID.

Exact command/output summary:

```text
<redacted command and decisive output>
```

## Retail Files import

- [ ] With no valid installed image, the native Files chooser appeared.
- [ ] A deliberately invalid replacement was rejected.
- [ ] Rejection did not remove or replace an already accepted retail image.
- [ ] A user-owned NTSC-U single-track raw MODE2/2352 BIN imported.
- [ ] The game continued in the same process after import.
- [ ] Cold relaunch loaded the installed image without reopening the chooser.

Observed timestamps/log rows:

```text
<redacted rows; no local container URL or device identifier>
```

## Touch-first complete race

Record the character, track, mode and result. A menu-only or starting-grid
session is not a pass.

| Requirement | Result and timestamp |
| --- | --- |
| Touch-only menu navigation | `<pass/fail/not tested + evidence>` |
| Sustained analog steering plus gas | `<value>` |
| Hop and held drift on physical glass | `<value>` |
| One three-boost drift chain | `<value>` |
| Item use while steering/accelerating | `<value>` |
| Pause and resume | `<value>` |
| One complete race and finish result | `<value>` |
| No stuck control after customization/rotation | `<value>` |

Human control assessment, including missed inputs or ergonomic problems:

```text
<honest narrative>
```

## Audio, lifecycle and optional controller

- [ ] Music, engine, effects, voice/XA and cinematic/STR audio were audible
      where expected, without sustained breakup.
- [ ] Home/background suspended output and foreground resumed it.
- [ ] Portrait/landscape or window resize reflowed game and controls correctly.
- [ ] A cold termination/relaunch recovered normally.
- [ ] If tested, a connected controller coexisted with touch without ownership
      or stuck-input faults. Otherwise record `not tested`.

Observed issue/timestamps:

```text
<value>
```

## Device cadence, frame pacing and thermal observation

Simulator FPS is not evidence here. State the scene, measurement duration and
tool or log source. Do not replace missing device telemetry with visual feel.
Use `frame-stats-rows.txt` for the app's 120-frame wall-cadence windows and
`device-state-rows.txt` for thermal/Low Power/battery context. Separate loading
or transition windows from sustained race windows; a zero-row file is missing
evidence, not a pass. p95/p99 use the nearest-rank definition.

| Metric | Exact observation |
| --- | --- |
| Track/scene | `<value>` |
| Measurement duration | `<value>` |
| Sustained mean/median/p95/p99/max frame time and FPS | `<value or not measured>` |
| Visible pacing stalls | `<count/description>` |
| Thermal state transitions and surface observation | `<value>` |
| Low Power Mode and battery/energy observation | `<value or not measured>` |

## Save and update persistence

1. Create or update a profile/save through the game.
2. Run `ios-device-campaign.sh collect` and retain the pre-update save hash.
3. Update-install a signed build with the same bundle identifier; never
   uninstall the app for this check.
4. Cold relaunch, load the profile and collect again.

- [ ] Pre-update profile loaded after a cold relaunch.
- [ ] Update installation preserved the accepted retail image.
- [ ] Update installation preserved the game-created save.
- [ ] The post-update Load screen showed the expected profile/progress.
- [ ] Pre/post collected save hashes and observed game state were recorded.

```text
pre-update save SHA-256:  <value>
post-update save SHA-256: <value>
retail revalidation evidence: <value; do not commit retail data>
```

## Collected log review

| Check | Result |
| --- | --- |
| Current plus rotated CTRPad logs collected | `<value>` |
| Structured installed-app result/version/build | `<value>` |
| Structured app-container copy result | `<value>` |
| `[ERROR]` / `[FATAL]` rows | `<count and disposition>` |
| `[CTR AssetRef]` / visibility rows | `<count and disposition>` |
| Import/save/lifecycle faults | `<count and disposition>` |
| Crash report or unexpected termination | `<value>` |

Include only short redacted rows needed to explain a result:

```text
<value>
```

## Acceptance conclusion

Overall result: `<accepted / rejected / incomplete>`

Unmet or weakly evidenced requirements:

1. `<value or none>`

Follow-up defects and their repository issue/commit references:

1. `<value or none>`

Reviewer/tester and date: `<value>`
