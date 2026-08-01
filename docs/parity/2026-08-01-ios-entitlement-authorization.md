# iOS profile-to-signature entitlement authorization

## Result and boundary

The signed-IPA preflight previously compared only the suffix after the first
dot in the app's signed `application-identifier`. It verified that the suffix
equaled the bundle ID, but did not prove that the signed App ID prefix matched
the embedded profile. It also did not verify the signed keychain access group,
`get-task-allow`, or the absence of CTRPad-unneeded service entitlements.

That was a real installability gap for externally signed input: a
cryptographically valid app/profile pair could reach the preflight success
manifest even though iOS would reject a prefix or entitlement mismatch during
installation. Apple's TN2415 states that the OS compares the App ID prefix in
the app signature and profile, and documents invalid or profile-disallowed
`keychain-access-groups` as installation failures:
<https://developer.apple.com/library/archive/technotes/tn2415/>.

The new shared verifier binds CTRPad's minimal signature to the already-trusted
profile. Both manual packaging and the physical-device preflight use it. This
closes an offline false-positive path; it does not supply an Apple identity,
profile or iPad and therefore does not prove a real signed install.

## Implementation

`tools/verify-ios-entitlement-binding.sh` accepts a decoded profile plist only
after its caller has established CMS trust, and an entitlement plist only after
the caller has established app-signature trust. It then requires:

- one normalized `ApplicationIdentifierPrefix` matching the prefix in the
  profile's `application-identifier`;
- a valid exact or final-component-wildcard profile App ID authorizing the
  requested bundle ID;
- an exact signed `<prefix>.<bundle-id>` rather than a suffix-only match;
- one profile team and the same signed team entitlement;
- exactly one CTRPad keychain access group, equal to the signed App ID and
  authorized by an exact or terminal-wildcard profile group;
- signed `get-task-allow`, when present, to equal the profile authorization;
  and
- only CTRPad's three required entitlement keys plus optional
  `get-task-allow`.

The verifier writes its success manifest only after all checks pass and refuses
a tracked in-repository output path. `package-ios.sh` applies it once to the
requested minimal entitlements before signing and again to the entitlements
read back from the final signature. `tools/ios-device-campaign.sh` applies it
to any supplied signed IPA and retains the exact prefix, signed App ID,
keychain group and debugger-authorization status in ignored evidence.

`tools/test-ios-entitlement-binding.sh` creates only synthetic plists in a
temporary directory. CMake registers it as `ctr_ios_entitlement_binding` on a
macOS host, so this release check is now part of the ordinary suite rather than
an undocumented one-off probe. `package-source.sh` requires both the verifier
and test in corresponding-source archives.

## Executed evidence

The direct self-test passed two legitimate shapes:

1. exact profile App ID plus wildcard profile keychain group;
2. final-component-wildcard profile App ID plus wildcard keychain group.

It rejected seven isolated mutations at their intended boundary, without a
success manifest:

1. wrong signed App ID prefix;
2. unexpected `aps-environment` entitlement;
3. a second signed keychain group;
4. a profile that did not authorize the signed keychain group;
5. signed `get-task-allow` differing from the profile;
6. a non-final App ID wildcard; and
7. a profile team entitlement differing from `TeamIdentifier`.

After reconfiguring the exact macOS ARM64 tree, CTest enumerated 23 tests and
passed 23/23 in 20.94 seconds; the new test accounted for 15.99 seconds. Bash
syntax and `git diff --check` passed.

Unsigned packaging remained unchanged in behavior. Two packages were
byte-identical, each contained seven members, both sidecars passed, and both
had SHA-256:

```text
7ebb1f4e9a3dc7bcc1d8676bb265301ae420d391ddd21d496834bc8dc9e52a36
```

At 18:48:30 CDT there was still exactly one booted Simulator, zero valid
code-signing identities and no CoreDevice device. Active goal time was 271,519
seconds: 3 days, 3 hours, 25 minutes and 19 seconds. A real Apple profile must
still pass the trust, authorization, package, install and physical acceptance
sequence before this project can be called complete.

After tightening Boolean type checks and final-component keychain wildcard
validation, the focused test passed in 4.51 seconds and the complete suite
again passed 23/23 in 8.13 seconds. At 18:51:23 CDT, active goal time was
271,689 seconds: 3 days, 3 hours, 28 minutes and 9 seconds. These are the final
pre-commit candidate results; the external boundary is unchanged.

## Publication

The exact 16-file checkpoint became commit `c89bdfc235e0` at 18:52:18 CDT.
Its clean 3,258-member source archive passed its sidecar, independent hash and
extracted-source self-test at SHA-256
`db3be6dd4f8ca7fe15c1cfbec9f147431babeb78eb94e3023e68a78c01116892`;
the forbidden-member scan returned zero.

The commit was pushed unchanged. The preferred private-repository connector
returned HTTP 404, then authenticated CLI fallback opened draft PR #12 at
18:54:11. GitHub reported exactly one commit, the intended 16 files, 653
additions, 34 deletions, `MERGEABLE` / `CLEAN` and no configured checks. The
exact protected head merged at 18:54:33 as `d9e3c59440ca`.

Packaging fetched `main` produced another 3,258-member archive at SHA-256
`760dcee662b993ebe417ba0cd3b6b077430538c9f43a72e979648e2bb4eb68d0`.
Its sidecar, independent hash, exclusion scan and extracted-source entitlement
self-test passed. At 18:55:55 CDT, active goal time was 271,962 seconds: 3 days,
3 hours, 32 minutes and 42 seconds. Publication closes this offline defect, not
the real signed-profile or physical-iPad branch.
