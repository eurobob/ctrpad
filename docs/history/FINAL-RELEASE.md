# CTRPad final release phase

## Purpose

This chapter continues the initial three-day campaign history after the
2026-08-02 checkpoint. It records the small final phase that turned the proven
Apple port into the v0.1.0 public-release package without rewriting the dated
roadmap, journal, or parity reports.

## 2026-08-02 — Handoff, customizable touch controls, and public documentation

- PR #29 published the final three-day build handoff after PR #28's complete
  history checkpoint.
- The shared iPhone/iPad touch layer gained separate safe-area-aware layouts,
  movable and resizable controls, opacity and handedness options, resettable
  defaults, held-input cleanup, and an original CTRPad app icon.
- The README and installation/legal documentation were reorganized around the
  supported Apple Silicon Mac, iPhone, and iPad experience.
- macOS gained first-launch disc selection; iPhone/iPad retained Files import.
  Both paths validate a user-supplied NTSC-U single-track raw MODE2/2352 BIN.
- Retail assets, saves, device profiles, and signing material remained outside
  Git and the generated packages.
- PR #30 merged the public-release README, platform onboarding, packaging, and
  legal boundary as `ddb0544b4`.

## 2026-08-04 — Final Apple-platform acceptance and v0.1.0

- The current build was installed in place and accepted on the attached iPad
  and iPhone without replacing their imported game data or saves.
- Connected controllers use the same standard mapping on macOS, iOS, and
  iPadOS.
- macOS gained racing-first keyboard defaults, native click-to-edit bindings,
  resettable controls, controller guidance, and a top-right options panel.
- macOS joined iPhone and iPad in exposing persistent 1x, 2x, 3x, and 4x
  internal geometry resolution; 4x is the Apple Silicon Mac default.
- The repository passed its Apple builds and 26-test suite, and the release
  workflow produced retail-free Apple artifacts with matching source and
  SHA-256 sidecars.
- PR #31 merged the final controls, display, installation, and package metadata
  work as `ca18efbbc`; annotated tag `v0.1.0` points to that merge.
- PR #32 merged the checksum portability correction as `7a2921542`. Published
  sidecars were corrected so each records only its artifact filename.

## Released product boundary

CTRPad v0.1.0 supports Apple Silicon macOS 11 or newer and one iOS/iPadOS 15+
device build for both iPhone and iPad. The public release contains an unsigned
IPA for user-owned signing, an ad-hoc-signed Apple Silicon Mac archive,
corresponding source, and checksum sidecars. It contains no Crash Team Racing
disc data, saves, provisioning profiles, certificates, or private keys.

The earlier history remains authoritative for what was known and still open at
each dated checkpoint. This chapter is the current summary of what followed and
what shipped.
