# Structured `devicectl` campaign evidence

## Result and remaining boundary

The physical-iPad campaign now validates CoreDevice results structurally instead
of searching arbitrary JSON text. The former installed-app check could report a
false success when the requested bundle identifier appeared only in
`info.arguments` or `result.matchingBundleIdentifier` while `result.apps` was
empty. The new verifier requires a successful, versioned command envelope and
the exact result object for list/details/install/app/launch/copy operations.

This closes an offline evidence-integrity defect. It does **not** prove a signed
CTRPad install: at 2026-08-01 19:10:02 CDT this Mac still had zero valid signing
identities and `devicectl` reported no physical devices. The signed-profile,
real-install, physical launch, Files, complete race, touch ergonomics, hardware
cadence, audio and thermal gates therefore remain open.

## Defect reproduced

At pre-change head `890ba3f4be57`, prepare and collect accepted installed-app
evidence with:

```text
grep -Fq "$bundle_id" installed-app.json
```

That check was at `tools/ios-device-campaign.sh:385-386` and `:424-425` in the
pre-change source. A real failed local request for nonexistent device
`00000000-0000-0000-0000-000000000000` returned CoreDevice error 1000, outcome
`failed`, command type `devicectl.device.info.apps` and no `result.apps`. The
requested `io.github.chrissotraidis.ctrpad` string remained in
`info.arguments`, so the legacy `grep -Fq` returned success. A synthetic
successful app-query envelope with
`result.matchingBundleIdentifier=io.github.chrissotraidis.ctrpad` and an empty
`result.apps` array reproduced the same false-positive class without a device.

The defect was an evidence claim, not an app renderer or gameplay failure. No
install occurred during either reproduction.

## Supported JSON contract

Local `devicectl 518.33` help states that file-based JSON is versioned and will
remain stable, while standard output is for humans and is not stable. It also
calls `--json-output` the only supported scripting interface. The campaign now
uses only that file interface and validates it in two layers:

1. every invocation must have no top-level `error`, the exact expected
   `info.commandType`, `info.outcome=success`, a numeric tool version and, when
   present, a positive integer `info.jsonVersion`;
2. install, installed-app and launch operations must contain the exact
   operation-specific result rather than merely a matching string.

`tools/verify-devicectl-json.sh:89-138` implements the envelope. Its specialized
checks require:

- exactly one `result.installedApplications[0]` with the expected `bundleID`
  and a `CTRPad.app` installation URL (`:148-167`);
- the expected `matchingBundleIdentifier`, exactly one `result.apps[0]`, exact
  bundle/version/build when preparing, and a `CTRPad.app` URL (`:168-209`); and
- a positive integer `result.process.processIdentifier` plus exact
  `CTRPad.app/CTRPad` executable URL (`:210-230`).

Only after those checks pass does it write a manifest containing the input JSON
SHA-256, command/outcome/tool/JSON versions, bundle/version/build, remote URL and
PID (`tools/verify-devicectl-json.sh:232-249`). In-repository manifests must be
gitignored (`:78-87`).

`tools/ios-device-campaign.sh:385-427` now applies the envelope and exact
install/app/launch checks before writing `prepare-manifest.txt`. Collection
validates details, installed version/build and the copy-from envelope before
writing its result (`:451-502`). Both app queries request `--columns '*'` so the
version, build and URL fields required for proof are present. Collection does
not pin the version/build to the original prepare values because an intentional
same-ID update must be observable rather than rejected.

## Test chronology

The first verifier self-test failed before any source claim because
`plutil -lint` on this macOS rejected JSON at its opening `{`. The parser was
corrected to use `plutil -p`, which accepts both JSON and property-list inputs
and still fails malformed data. That failed attempt is retained here because it
changed the implementation.

The final deterministic fixture suite in `tools/test-devicectl-json.sh` passed
four positive cases:

1. generic successful list envelope;
2. exact installed app/version/build;
3. exact install result; and
4. exact launched process.

It also rejected nine isolated mutations without writing a success manifest:

1. failed JSON containing the requested bundle text;
2. empty `apps` containing matching query text elsewhere;
3. wrong command type;
4. wrong installed bundle;
5. a second exact app result;
6. wrong installed version;
7. wrong install-result bundle;
8. zero launch PID; and
9. wrong launched executable.

The direct result was:

```text
DEVICECTL_JSON_SELF_TEST=passed positives=4 negatives=9
```

The macOS CTest registration is in `CMakeLists.txt:415-420`. A focused run
passed, and the final full run passed 24/24 in 46.57 seconds on 2026-08-01,
including entitlement binding and the new device-JSON test. Bash syntax,
`git diff --check` and corresponding-source required-file integration also
passed. `package-source.sh` requires both new tools so a release archive cannot
silently omit the verifier.

## Real local schema probes

A real credential-free list call produced:

```text
commandType  devicectl.list.devices
outcome      success
version      518.33
jsonVersion  3
devices      []
JSON SHA-256 71bd029c873932f032a8128c91e00474364295a5d0329e3210a94a23e36c6d8c
```

The new envelope verifier accepted it. The real failed app query described
above was rejected with `ERROR: devicectl JSON contains an error object` and no
success manifest. A real failed copy probe independently established command
type `devicectl.device.copy.from`. These ignored `/tmp` probes contain no
successful physical-device evidence.

The unsigned-IPA campaign negative still stopped before device access with:

```text
ERROR: signed app has no embedded.mobileprovision
```

No `preflight-manifest.txt`, install, launch or collection result was created.

## Exact one-Simulator recheck

This work kept exactly one Simulator booted: `CTRPad Import Validation`. After
the tooling regression passed, the current Simulator product was rebuilt at
low priority with one build job and installed through the guarded isolated-sign
helper. At 19:15 CDT:

```text
source executable SHA-256  9e85ca4a99355e4f7f118129104563c96daefe357654c22c6731c8576e291074
staged/installed SHA-256    88b5e79f2cb5606cf2c9cabf12180627b484359a425923f55d8812e1fd400979
launch PID                  4571
retail persistence          inode/size/SHA-256 unchanged
slot-zero save persistence  inode/size/SHA-256 unchanged
```

Computer Use then observed the exact freshly launched build rendering the
copyright screen, Naughty Dog intro and complete main-menu graphics with the
safe-area touch overlay. A touch Gas press reached the retail poll and opened
the main menu. The current log initialized GLES 3.0, framebuffer fetch, touch
and the UIKit display loop and contained zero targeted `[ERROR]`, `[FATAL]`,
`[CTR AssetRef]` or visibility rows. The Apple Software Renderer later settled
near 7.8 FPS; that is retained as Simulator behavior, not substituted for
physical-iPad cadence evidence. The screenshot is ignored under `dist/` because
it contains user-supplied retail pixels and is not part of published source.

## Time accounting

The historical active-time boundary requested by the user remains exactly
262,238 seconds: 3 days, 50 minutes and 38 seconds at 15:56:54 CDT. Work later
resumed. At 19:16:58 CDT, after the structured evidence work and exact
Simulator recheck, the active goal reading was 273,210 seconds: 3 days,
3 hours, 53 minutes and 30 seconds. These are Codex active-goal readings, not
invented wall-clock implementation durations.

## Publication closure

The exact 16-file checkpoint became commit `55932c2c1510` at 19:26:17 CDT.
Its 3,261-member corresponding-source archive passed its sidecar at SHA-256
`da7c7d22c7e3ac227ea13c1bedc8a50549ac79585c5fc924ce74afc0e8bcc3f7`.
The first independent member check accidentally used zsh's special `path`
array as a loop variable and lost command lookup after the checksum/member
count; the unchanged retry used `member_path`, found the required verifier,
test, report and timeline, found zero forbidden members and passed the extracted
four-positive/nine-negative verifier.

The preferred private-repository GitHub connector returned HTTP 404. The
authenticated CLI fallback created PR #14, whose audit found one exact commit,
16 intended files, 865 additions, 25 deletions, `MERGEABLE` / `CLEAN` and no
configured checks. GitHub merged protected head `55932c2c1510...` at 19:28:59
CDT as `94f4d40eda4b...`; fetched `origin/main` contained both tools and this
report. At 19:29:27 CDT active goal time was 273,968 seconds: 3 days, 4 hours,
6 minutes and 8 seconds. One Simulator, zero identities and no physical device
remained, so publication is accepted while signed hardware acceptance stays
open.
