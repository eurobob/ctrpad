# GPL Corresponding-Source Package Acceptance

- Date: 2026-08-01
- Implementation: `4091b602ab2abc74db584a56de06faa23905a96a`
- Artifact: deterministic tracked-source `.tar.gz` plus SHA-256 sidecar
- Result: accepted for exact source-archive generation and extraction;
  clean-machine compilation and physical-iPad release acceptance remain open

## Gap closed

The repository and draft PR already published the complete tracked source, but
M11 had no release command that tied a versioned corresponding-source artifact
to the future IPA. Asking a recipient to infer which working tree, ignored
files or build outputs belonged beside an IPA would be weaker than the app's
exact embedded Git identity and the existing deterministic IPA packager.

`package-source.sh` now creates that explicit artifact from a Git commit. It
does not copy working-directory files into the archive. A tracked-dirty check
fails before packaging, then `git archive` selects only the named commit under a
commit-derived top-level directory. Ignored user media, saves, build trees and
local credentials are therefore outside the input boundary rather than merely
removed after collection.

## Archive contract

The packager requires and verifies:

- CMake build files, `main.c`, `build.sh`, both release packagers, README, GPL
  license, third-party notices and iOS Installation Information;
- roadmap, decision record, engineering journal and progress log documenting
  modifications and limitations;
- complete `game`, `include`, `platform`, `tools` and vendored SDL trees,
  including SDL's license;
- no retail-shaped BIN/IMG/ISO/CUE/CCD/SUB/BIG/HWL/XA/STR/TIM, save or memory-
  card extension;
- no IPA, provisioning profile, certificate, PEM/private-key-like file or
  common private-key filename; and
- no build, distribution, asset, debug, memory-card or `.git` tree.

`git archive` supplies stable commit metadata and ordering. `gzip -n -9`
removes the gzip filename/timestamp fields. The script validates the tar and
gzip streams, prepares a basename-relative SHA-256 sidecar in its private
temporary directory, moves both results only after those checks, then verifies
the published pair. Existing output paths are never overwritten.

README and `docs/INSTALL-IOS.md` now tell a distributor to run
`./package-source.sh` from the same clean committed checkout used for the IPA
and publish the source tarball and checksum beside it. This does not claim that
the current unsigned IPA was rebuilt from this later documentation/packaging
head; a future release must still match both artifacts to one build identity.

## Negative and interrupted-route evidence

Before the implementation commit, syntax and help output passed and a real run
with tracked edits failed before creating output:

```text
ERROR: tracked changes are present; commit them before packaging corresponding source
```

The first committed implementation was published as `95dcb67f177b`. Its first
exact archive completed. The second execution was interrupted after moving its
tarball but before filling the sidecar, leaving a 17-MB archive and zero-byte
checksum in a local temporary directory. That run was rejected rather than
counted as reproducible.

The interruption exposed a publication-order flaw. Commit `4091b602ab2a`
moved archive hashing and sidecar creation into the private staging directory,
checks the 64-character digest there, publishes the already complete pair, and
verifies the pair after both moves. The incomplete 95d artifacts remain local
diagnostic evidence and are not distributed or committed.

An exact invalid-ref check on the corrected script also failed before output:

```text
ERROR: not a Git commit or tag: this-ref-does-not-exist
```

## Exact clean reproducibility

The corrected implementation was committed and pushed before acceptance.
Both exact runs used clean commit
`4091b602ab2abc74db584a56de06faa23905a96a`, nice priority 15 and separate
sequential commands. They completed in approximately 15.42 and 17.84 seconds.

Each archive has:

```text
size        17,482,944 bytes
members     3,233
root        CTRPad-source-4091b602ab2a/
SHA-256     d1c4b4fb119605ab56475d2a6861555a71bceac146ea2b9004c498adf5e64df1
```

`cmp` returned success. Both independent checksum sidecars passed
`shasum -a 256 -c`; `gzip -t` and complete tar listing passed. The executable
bits for `package-ios.sh` and `package-source.sh` survived the archive. A scan
of the real 3,233-member listing found none of the prohibited media, runtime,
package, authorization or credential patterns.

## Extraction smoke check

The first exact artifact was extracted into a fresh temporary directory. It
had no `.git` metadata. Both release scripts passed `bash -n`, and CMake read
the archive's own `CMakePresets.json` and enumerated:

```text
macos-arm64
macos-arm64-app
ios-simulator-arm64
ios-device-arm64
```

A second prohibited-extension scan over the extracted filesystem was empty.
The two exact archives, sidecars and extracted tree remain local-only.

This is deliberately not a clean-machine compile. At the acceptance boundary,
only 3,693 free 16-KiB VM pages (about 58 MB) remained and swap use was about
10.55 GB. No compiler or Simulator was started; final state remained zero
Simulator processes and zero booted devices. A clean extracted-source build
must wait for safe host headroom and remains an explicit M11 gate.

## Acceptance boundary

CTRPad now has an exact, reproducible, retail-free corresponding-source
artifact workflow that includes the build system, vendored dependencies,
licenses, Installation Information, modification record and packaging tools.
This closes source-artifact generation, not legal review, clean-machine build,
Apple signing, physical installation or release-device gameplay. M11 and the
overall goal remain active.
