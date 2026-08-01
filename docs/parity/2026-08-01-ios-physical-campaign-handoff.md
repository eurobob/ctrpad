# Physical iPad campaign handoff

## Result and honest boundary

This checkpoint makes the remaining signed-iPad campaign executable from
another Mac without claiming that the campaign ran here. It adds offline
signed-IPA verification, non-destructive device install/launch evidence,
post-test log/save collection and a human acceptance template. The local
machine still reports:

```text
security find-identity -v -p codesigning    0 valid identities found
xcrun devicectl list devices                No devices found.
provisioning-profile inventory              no files
```

Therefore no Apple-authorized signature, physical installation, device launch,
Files import, complete race, touch ergonomics, cadence, audio, thermal or update
result is accepted here. The new workflow reduces ambiguity at that external
boundary; it does not move the boundary by assertion.

The work began from clean local branch head
`f9b5d4fa0a3ed0baea7c55557edac984edeb53ab`. That head was already merged into
GitHub `main` through PR #6 at
`e2dfe97099613f105b71d6d2246bbf6804004026`. The sole booted Simulator remained
`CTRPad Import Validation`; this work did not boot a second Simulator, install
to either Simulator or alter the running app.

## Local device-tool contract

The installed CoreDevice tool is `devicectl 518.33`. Its local help explicitly
states that human standard output is not stable for automation and that
versioned JSON written with `--json-output` is the supported scripting
interface. The retained commands therefore provide both JSON and log paths for:

- device inventory and exact-UDID details;
- app update installation;
- exact-bundle installed-app lookup;
- foreground launch with any prior instance terminated; and
- Application Support collection after human testing.

`devicectl device install app` accepts a `.app`, not an IPA. The helper first
extracts and validates exactly one `Payload/*.app`, retains that temporary app
through installation, and cleans the extraction afterward. It never invokes
`devicectl uninstall`.

`devicectl device copy from` supports the `appDataContainer` domain. CTRPad's
retail image is under `Documents/CTRPad/assets`, while logs, saves and private
diagnostics are under `Library/Application Support/chrissotraidis/CTRPad`.
Collection deliberately requests only the latter. It scans the copied tree for
retail-like files and fails if one appears.

## Repository workflow

`tools/ios-device-campaign.sh` has three phases:

1. `preflight` hashes and tests the IPA; validates its one-app layout, thin
   ARM64 `IOS` executable, resources and retail exclusion; requires a strict
   signature and embedded non-expired iOS profile; and ties profile App ID,
   signed App ID, signing team, leaf certificate membership in the profile's
   `DeveloperCertificates` and optional physical UDID together.
2. `prepare` requires the authorized UDID, runs preflight, captures versioned
   device JSON/logs, update-installs without deletion, confirms the bundle is
   listed and launches it. The manifest calls its hash
   `SIGNED_PACKAGE_EXECUTABLE_SHA256`; iPadOS does not expose an installed-
   executable readback, so no installed hash is invented.
3. `collect` runs after a human session. It captures device/app information,
   first requires a successful prepare manifest with the same exact device and
   bundle, copies only Application Support, writes save hashes, and extracts
   targeted fault, FPS and campaign-event rows. It does not automate or infer
   touch feel, race completion, Files, audio quality, thermals or update
   persistence; FPS rows still require a stated scene/duration and human
   interpretation.

New evidence directories must not already exist. If a requested directory is
inside the checkout, `git check-ignore` must prove it ignored; examples use
`dist/`. Raw files can include a UDID and user save data and are not suitable
for Git. `docs/templates/IOS-DEVICE-ACCEPTANCE.md` is the redacted durable
record, with explicit checklists for retail import, a touch-only complete race,
three-boost drift, audio/lifecycle, physical cadence/thermals, save/update
persistence and log review.

## Executed local tests

### Syntax and argument controls

At 17:44 CDT:

```text
bash -n tools/ios-device-campaign.sh                         PASS
--help                                                       PASS
no subcommand                                                rejected, exit 1
--timeout 0                                                  rejected, exit 1
prepare without --device-udid                               rejected, exit 1
tracked docs/ evidence path                                 rejected, exit 1
nonexistent tracked evidence parent                         rejected, exit 1
```

The tracked-path guard created no directory. An ignored `dist/` evidence path
was accepted. `git diff --check` passed. `shellcheck` is unavailable and is not
reported as a pass.

The unchanged macOS ARM64 build also passed all 22/22 native tests in 5.39
seconds after the handoff changes. This is a shared-source regression check,
not device execution evidence. Exactly one Simulator remained booted and its
installed executable stayed `c6d40aaf...187f`; the physical tooling did not
touch it.

The profile certificate decoder was independently checked with a synthetic
plist data-array entry: `plutil` raw extraction plus `base64 -D` reproduced the
input byte-for-byte (`certificate_count=1`, `cmp=0`). This accepts the local
data extraction loop only. No real Apple certificate/profile pair exists here,
so leaf-certificate membership still requires a real signed-IPA preflight.

The first `codesign` certificate probe passed the prefix as a separate argument
and failed because codesign treated that prefix as another code path. The local
long-option parser requires `--extract-certificates=PREFIX`. The corrected form
extracted all three certificates from the installed Xcode signature and exited
0; the helper retains the corrected equals form. This proves command syntax,
not CTRPad device authorization.

The signed team entitlement has a literal dotted key. Reading it with a
`plutil` key path would reinterpret the dots, repeating the earlier packaging
pitfall. The helper instead uses `/usr/libexec/PlistBuddy`; a synthetic
`com.apple.developer.team-identifier=TESTTEAM` plist read back exactly.

Finally, the exact linkage loop extracted Xcode leaf certificate SHA-256
`d84db96af8c2e60ac4c851a21ec460f6f84e0235beb17d24a78712b9b021ed57`,
imported that DER value into a synthetic one-entry `DeveloperCertificates`
array and reported `profile_count=1`, `match=1`. This accepts byte comparison
and loop termination, not CTRPad Apple authorization.

### Unsigned IPA rejection

The clean re-baseline unsigned IPA was used only as a negative input:

```text
input  dist/CTRPad-0.1.0-1-d5772375f-unsigned.ipa
SHA    41e00a5d258b87ef775f07cb9566f1a31138cf1cc8c48d7513054610bccb13ed
sidecar verification   pass
ZIP member test        pass
result                 ERROR: signed app has no embedded.mobileprovision
exit                   1
```

The rejection occurred before any device command. The ignored local evidence
is `dist/device-unsigned-preflight-f9b5d4fa0/`. No `_CodeSignature` or profile
was manufactured to turn this package into false signed evidence.

### Missing physical device rejection

`collect` used a deliberately nonexistent selector and 10-second timeout. It
exited 1 with CoreDevice error 1000 and still wrote supported JSON version 3:

```text
commandType  devicectl.device.info.details
version      518.33
outcome      failed
error        The specified device was not found.
```

The raw probe is local under `/tmp/ctrpad-device-probes.hOc1fC`. No install,
launch or container copy command ran after details failed.

During the initial JSON-schema probe, a BSD `mktemp` template with `.json`
after the `XXXXXX` suffix produced literal-X filenames. An attempted direct
cleanup was rejected by the command execution policy; both exact temporary
files were then moved to macOS Trash, a recoverable cleanup. A later static
test orchestration also passed a string in a numeric output-limit slot and was
rerun with corrected tuple arguments. Neither harness error changed source or
device state; both are recorded to prevent confusing them with campaign
failures.

## What remains to run elsewhere

A connected, trusted, Developer-Mode iPad plus its user-owned identity and
matching profile must run:

1. `package-ios.sh` in signed mode;
2. `ios-device-campaign.sh preflight` and `prepare`;
3. real Files import and invalid-replacement preservation;
4. a touch-only complete race including sustained analog steer/gas and a
   three-boost drift chain;
5. audio, Home/foreground, resize/rotation and optional controller checks;
6. pre-update `collect`, same-ID update install, cold profile load and
   post-update `collect`;
7. device cadence/frame-pacing/thermal measurement; and
8. a filled redacted acceptance report plus exact matching source archive.

At the 17:45:00 CDT reading, active goal time was 267,693 seconds: 3 days,
2 hours, 21 minutes and 33 seconds. This checkpoint is not completion evidence;
the overall goal remains open until the physical results above exist.

The final offline-claim audit closed at 17:53:34 CDT with active goal time
268,214 seconds: 3 days, 2 hours, 30 minutes and 14 seconds. By that point the
certificate extraction syntax, DER/profile membership loop, literal dotted
team-entitlement readback, 22-test suite, privacy guard and unsigned/missing-
device rejection paths above had all run. Signed CTRPad preflight and every
physical step remained unexecuted.

The final working-tree audit at 17:55–17:56 CDT reran Bash/package syntax,
`git diff --check` and all 22 macOS ARM64 tests; the suite passed in 4.87
seconds. It again found one booted Simulator, zero signing identities and no
physical device. An initial non-mutating installed-app lookup used the obsolete
`com.chrissotraidis.ctrpad` bundle ID and returned POSIX error 2. Simulator
inventory provided the exact `io.github.chrissotraidis.ctrpad` identifier; the
corrected lookup confirmed installed executable SHA-256
`c6d40aaf3c0cfc989e47a79b74ff86f9e52b95a74505c58abaf77d323319187f`.
The 17:55:51 goal reading was 268,363 seconds (3 days, 2 hours, 32 minutes and
43 seconds). None of these checks substitutes for a signed physical campaign.

The final scope audit then required `collect` to bind to the exact successful
prepare manifest, UDID and bundle. At 17:59 CDT, the unsigned-preflight root
failed for no prepare manifest without creating a collection directory. An
ignored, explicitly synthetic matching-manifest negative reached the fake
device, failed with CoreDevice error 1000 and stopped before app lookup or
copy. The post-change syntax/help/package syntax and diff checks passed. The
17:59:07 active-time reading was 268,549 seconds (3 days, 2 hours, 35 minutes
and 49 seconds). The synthetic manifest and raw JSON/log remain ignored and
are not signed-device evidence.

At 18:00 CDT, clean commit
`4349bae8dd938447b9f67fff04b34dd4b574f09b` produced matching corresponding
source `CTRPad-source-4349bae8dd93.tar.gz`: 3,253 members, SHA-256
`b9dfcc85c35b9cb36d82f9eed942e7a0ab2449d2b36cb5f8de1170ff77c9cba4`.
Its internal and independent sidecar checks passed. The archive contained this
report, the acceptance template and `ios-device-campaign.sh`; direct scans
found zero retail/runtime-like or credential/package-like members. The
18:00:26 goal reading was 268,628 seconds (3 days, 2 hours, 37 minutes and 8
seconds).

At 18:01–18:02 CDT the two handoff commits were pushed. The preferred GitHub
connector returned HTTP 404 for the private repository; authenticated `gh`
fallback created draft PR #7. GitHub then reported exactly two commits and 12
intended files, head `e7af0bc323034e51ba8e4e0426abd0c7c28f0e79`, base
`main`, `MERGEABLE`/`CLEAN`, and no configured checks. The 18:02:05 goal reading
was 268,724 seconds (3 days, 2 hours, 38 minutes and 44 seconds); this was the
pre-merge boundary.
