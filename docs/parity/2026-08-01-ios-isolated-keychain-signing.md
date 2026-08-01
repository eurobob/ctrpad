# iOS Isolated-Keychain Signing Preflight

- Date: 2026-08-01
- Implementation: `37a5e16760ba25ac948edc19f5eb7774c356d9f4`
- Device-app input: ARM64 `2c78c040bf2a`, SHA-256
  `3dc6e3c7706e996d59668ffb2c91024473d22b068c0565317fabdc87ccd9ebc1`
- Result: isolated-keychain selection, fail-closed identity handling, unsigned
  regression and downstream synthetic CMS/DER mechanics accepted; Apple-
  authorized signing and physical installation remain open

## Gap addressed

`package-ios.sh` previously searched only the user's default keychain list.
That is correct for a normal login-keychain identity but made an isolated CI or
release keychain impossible to select without changing global keychain search
state. It also made a safe local signing-path diagnostic require exactly the
state mutation that the audit was trying to avoid.

The new optional `--keychain PATH` requires `--identity` and `--profile`,
requires the keychain file to exist, limits `security find-identity` to that
keychain, and passes the same keychain explicitly to `codesign`. It does not
set the default keychain, append to the user's search list, unlock a keychain,
or weaken the existing valid-code-signing-identity gate. Installation
Information documents that the selected keychain must already be unlocked.

## Candidate and exact fail-closed checks

`bash -n` and help output passed before commit. These incomplete invocations
failed before packaging:

```text
--keychain without identity/profile
ERROR: --keychain requires --identity and --profile

identity/profile with an untrusted identity in the selected keychain
ERROR: codesign identity is not available in the requested keychain search
```

Neither failure created an IPA or checksum. The implementation was committed
and pushed as `37a5e16760ba25ac948edc19f5eb7774c356d9f4`; both failures were then
repeated at that exact clean head with the same no-output boundary.

## Synthetic fixture and rejected trust route

A private temporary keychain, RSA key, self-signed code-signing certificate and
synthetic profile were created outside the repository. No Apple credential,
account, team or device was used. OpenSSL 3's default PKCS#12 encoding was
rejected by macOS Security as a MAC/password verification failure; the same
synthetic key and certificate imported after using OpenSSL's legacy PKCS#12
encoding. This compatibility detour is retained rather than hidden.

The imported identity was present under X.509 basic policy but correctly
reported `CSSMERR_TP_NOT_TRUSTED`; code-signing policy reported zero valid
identities. Adding user trust would have required an authorization interaction.
That route was not approved, no prompt was accepted, and subsequent trust-
settings inspection reported none. The real packager therefore rejected the
identity exactly as intended instead of adding a synthetic bypass.

The synthetic CMS profile itself decoded successfully through the same
`security cms -D` primitive used by the packager. Its relevant values were:

```text
Platform                iOS
application-identifier  SYNTH12345.io.github.chrissotraidis.ctrpad
TeamIdentifier          SYNTH12345
ProvisionedDevices      00008110-SYNTHETIC0001
ExpirationDate          2026-08-03T10:00:00Z
```

The CMS fixture SHA-256 was
`b0cc01e9e4a6579d7c3b289f6ce32b9633df8f59fad90b91522d36a1ad0bc8ca`.
It is local synthetic evidence, not a provisioning profile that Apple or a
device will authorize.

## Downstream DER-entitlement diagnostic

Because macOS would not expose the untrusted identity to `codesign`, the real
identity-signed branch was not falsely marked as exercised. A separate local
diagnostic used the same generated entitlement plist and embedded synthetic
CMS profile, then applied an ad-hoc signature with
`--generate-entitlement-der --timestamp=none`. Deep/strict verification passed.

The signed entitlement plist retained all four intended values:

```text
application-identifier                 SYNTH12345.io.github.chrissotraidis.ctrpad
com.apple.developer.team-identifier    SYNTH12345
keychain-access-groups[0]              SYNTH12345.io.github.chrissotraidis.ctrpad
get-task-allow                         true
```

The extracted plist hashed to
`518d231212e54e7d66c3d7e4b8deee5cfb664a1892b3c22f84f784c1ee4d716e`.
`codesign -d -vv` explicitly reported CodeDirectory v20400,
`flags=0x2(adhoc)`, `Signature=adhoc` and `TeamIdentifier=not set`. This proves
DER entitlement construction and strict resource sealing while also proving
that the diagnostic is not an Apple/team signature.

## Exact unsigned regression

To ensure the new option did not perturb the established default path, exact
head `37a5e1676` packaged the same device app twice with one fixed commit-time
`SOURCE_DATE_EPOCH`. `cmp` passed. Both seven-member IPAs hash to:

```text
582b8491ecab23cd0ebb944a806a21beb8fba8eb7270cd2153121762f9e2b929
```

ZIP validation passed. The archive remained thin ARM64/iOS 15.0, unsigned and
free of retail/runtime data, `embedded.mobileprovision` and `_CodeSignature`.
The IPAs, sidecars, synthetic profile, certificate, private key, diagnostic app
and entitlements all remained local-only.

The temporary keychain was deleted through `security delete-keychain` after the
checks. The user's keychain search list contained only the original login
keychain, `security find-identity -v -p codesigning` still reported zero valid
identities, and user trust settings remained empty. No Simulator or compiler
was opened.

## Acceptance boundary

Explicit isolated-keychain discovery/signing support and its fail-closed
behavior are accepted. The downstream profile/entitlement/resource-sealing
mechanics gained stronger synthetic evidence without weakening the real gate.
No valid Apple identity or profile was available, and no physical device was
connected; the real signed-IPA branch, Apple authorization, installation and
device runtime remain open. M11 and the overall goal stay active.
