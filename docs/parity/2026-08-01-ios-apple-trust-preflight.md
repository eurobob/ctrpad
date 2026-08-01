# iOS Apple trust and real-profile preflight

## Result and boundary

This checkpoint closes two locally reproducible defects in the still-unrun
signed IPA path:

1. `security cms -D` authenticated CMS bytes but did not establish signer
   trust. It decoded a self-signed synthetic profile with exit 0 even while
   `security verify-cert` reported `CSSMERR_TP_NOT_TRUSTED`.
2. `package-ios.sh` required `ApplicationIdentifierPrefix.0` itself to end in a
   dot. The signed entitlement must instead have Apple's documented
   `<prefix>.<bundle-id>` form; a profile's bare prefix and the separator are
   distinct inputs.

The new verifier cryptographically verifies the CMS, validates the signer
chain, pins its terminal certificate byte-for-byte to one of the Apple Root CA
certificates in the macOS system root keychain, and requires an Apple iPhone OS
provisioning-profile signing subject. It applies the same chain/root proof to
the signed app under code-signing policy. Both the packager and physical-device
preflight now use it.

This is stronger offline evidence, not physical acceptance. The Mac still has
zero valid Apple code-signing identities, no user provisioning profile and no
CoreDevice device. Therefore a real provisioning-profile positive, signed
CTRPad IPA, install and device launch remain unexecuted.

Apple's TN3125 describes a profile as authorization for who, what, where, when
and how, with the profile entitlements acting as an allowlist:
<https://developer.apple.com/documentation/technotes/tn3125-inside-code-signing-provisioning-profiles>.
TN2415 states the signed `application-identifier` format as
`<prefix>.<bundle_id>` and notes that the prefix is not necessarily the Team
ID: <https://developer.apple.com/library/archive/technotes/tn2415/>.

## Implementation

`tools/verify-ios-signing-trust.sh:31-103` splits PEM chains, asks macOS
Security to validate the requested policy locally, extracts the resulting
root, and compares its DER SHA-256 with every matching Apple Root CA from
`SystemRootCertificates.keychain`. A user-trusted self-signed root therefore
cannot pass merely because local trust settings accept it.

Profile mode at `tools/verify-ios-signing-trust.sh:175-238` uses stock
`/usr/bin/openssl cms -verify -noverify` to prove CMS integrity and extract the
signer/embedded certificates. The separate Security chain check supplies the
trust decision, after which the signer-purpose guard and manifest are written.
App mode at `:239-287` performs deep/strict code verification, extracts the
certificate chain, applies code-signing policy and records the trusted root and
leaf hashes.

`package-ios.sh:188-285` now:

- refuses a profile unless its CMS signer passes the Apple trust verifier;
- normalizes an optional historical trailing dot from
  `ApplicationIdentifierPrefix.0`, requires the remaining bare prefix to equal
  the prefix in `Entitlements.application-identifier`, and constructs
  `<prefix>.<bundle-id>` exactly;
- refuses the final Apple-signed app unless its leaf certificate chain is
  trusted and that exact DER certificate appears in the profile's
  `DeveloperCertificates`; and
- reads back the signed application ID, literal dotted team entitlement and
  first keychain access group.

`tools/ios-device-campaign.sh:175-294` now retains separate app/profile trust
evidence directories and records both trusted-root hashes in its offline
preflight manifest before any device command. `package-source.sh` requires the
new verifier so matching GPL source cannot omit the release gate.

## Executed evidence

### Untrusted CMS reproduction

At 18:11 CDT an isolated RSA key, self-signed email-signing certificate and
encapsulated CMS profile were generated under
`/tmp/ctrpad-cms-trust-audit-20260801`. No keychain or trust setting was
modified. Fixture SHA-256:

```text
09519ad6b57964b617b809904b77af881160f6650e213ebed48b7463ccd43be8
```

The old primitive and an actual trust evaluation disagreed exactly:

```text
security cms -D                         exit 0, payload decoded
security verify-cert -p basic -N -L     exit 1, CSSMERR_TP_NOT_TRUSTED
OpenSSL trusted CMS verification        exit 4, self-signed certificate
```

Changing one byte produced tampered fixture SHA-256
`84b3c02f96ca98c9a3b600098e73cc4926ecb95a586f44904339abffa7f05e63`.
The new verifier rejected it as `provisioning profile CMS signature is
invalid`. The unchanged self-signed fixture passed cryptographic decoding, then
failed the separate chain step as `not trusted under basic policy`.

### Positive Apple chain and negative app paths

The first positive attempt used all of Xcode.app. Deep verification continued
for almost three minutes, so that task-owned process was terminated rather than
spending more host resources; it produced no accepted result. The same verifier
then completed against the much smaller Apple-signed Calculator.app in about
two seconds:

```text
CODE_SIGNATURE_STATUS=verified
CERTIFICATE_TRUST_STATUS=verified
APPLE_ROOT_PIN_STATUS=verified
LEAF_CERTIFICATE_SHA256=d84db96af8c2e60ac4c851a21ec460f6f84e0235beb17d24a78712b9b021ed57
EXTRACTED_CHAIN_CERTIFICATE_COUNT=3
TRUSTED_CHAIN_CERTIFICATE_COUNT=3
TRUSTED_ROOT_CERTIFICATE_SHA256=b0b1730ecbc7ff4505142c49f1295e6eda6bcaed7e2c68c5be91b5a11001f024
```

This proves the code-signing chain/root implementation with an Apple-signed
app, not an iOS development identity. The exact ad-hoc CTRPad Simulator app
failed with `app signature has no certificate chain`, as required.

The first verifier candidate also exposed and corrected two Bash 3.2 issues:
top-level `--help` fell through to the output-dir requirement, and an empty
chain array expanded as unbound under `set -u`. After correction, help passed;
unknown mode, a tracked evidence path and a pre-existing output path fail
before mutation. The tracked-path test created no directory. `shellcheck` was
unavailable and is not reported as a pass.

### Real-profile identifier mechanics and package regression

A synthetic decoded plist with bare prefix `SYNTH12345` exercised the exact
canonicalization used by the packager:

```text
RAW_PREFIX=SYNTH12345
PROFILE_APP_ID=SYNTH12345.io.github.chrissotraidis.ctrpad
SIGNED_APP_ID=SYNTH12345.io.github.chrissotraidis.ctrpad
```

This proves the delimiter/prefix mechanics only; the synthetic profile remains
untrusted. A real Apple profile must still pass all trust and authorization
checks.

At 18:23 CDT, two unsigned packages from the unchanged device app compared
byte-for-byte. Each contains seven members, excludes a profile, signature,
retail media and runtime data, and has SHA-256:

```text
3354bb3e69ef7664665c3d378f221cbdbc0aa7507b2f639e250037947e7149ed
```

Both sidecars passed. Feeding that unsigned IPA to device preflight passed ZIP
and sidecar validation, then failed for missing `embedded.mobileprovision`
before any device operation.

The complete macOS ARM64 regression suite passed 22/22 in 5.56 seconds after
integration. Bash syntax/help and `git diff --check` passed. Inventory retained
one booted Simulator at the previously accepted product and reported zero valid
signing identities and no physical device.

An attempted convenience `stat` lookup failed because `stat` was not on this
shell's PATH; repeating with exact `/usr/bin/stat` returned the retained file
timestamps. This harness correction changed no source or runtime state.

At 18:26:33 CDT, active goal time was 270,194 seconds: 3 days, 3 hours, 3
minutes and 14 seconds. The trust/profile compatibility defects are locally
corrected; the first real Apple-profile positive and every physical result
remain open.

At 18:31 CDT, the pre-commit candidate repeated all 22/22 macOS ARM64 tests in
2.68 seconds. A fresh temporary-output run accepted Calculator.app with the
same leaf/root hashes above, and a fresh run against the unchanged self-signed
profile again failed certificate trust. At 18:31:32 active goal time was
270,498 seconds: 3 days, 3 hours, 8 minutes and 18 seconds. The temporary trust
output was removed after its manifest and rejection were inspected.

## Publication

The 14-file checkpoint was committed at 18:33:14 CDT as `5af382409d8d`. Its
clean source archive passed at 3,255 members and SHA-256
`2c3a2d6f5ff9083fabe7019b77d2c14897c297c9598f49f06ee55a0a9ced6dce`.
After the preferred private-repository connector returned HTTP 404,
authenticated CLI fallback opened PR #10 at 18:35:07. GitHub reported the exact
commit/files, `MERGEABLE` / `CLEAN` and no configured checks. The protected
exact head merged at 18:35:27 as `8f3b2be37c99`.

Packaging that fetched `main` merge produced 3,255 members at SHA-256
`34f600e64a4885154348ca011c290e76a04b228932a8f6a91aba3bab38236326`.
Its checksum, required-file inspection and retail/runtime/profile/key/
certificate/package exclusion scan passed. At 18:36:44 active goal time was
270,811 seconds: 3 days, 3 hours, 13 minutes and 31 seconds. This publication
closes the locally actionable trust correction only; it does not change the
real-profile and physical-device boundary.
