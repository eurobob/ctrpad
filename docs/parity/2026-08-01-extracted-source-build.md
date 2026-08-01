# Fresh extracted corresponding-source build

**Date:** 2026-08-01

**Implementation commit:** `4673e1f9fcabbb27526f8422dc9c35aaf356f50e`

**Result:** accepted on the development host; independent clean-machine and
physical-device gates remain open

## Purpose

The corresponding-source packager had already produced byte-reproducible,
retail-free archives and passed extraction/syntax/preset smoke checks. A full
build from an extracted archive had remained deferred while memory headroom was
unsafe. After the bounded Apple matrix completed without throttling, this
checkpoint performed that missing compile and test from a fresh directory with
no `.git` tree.

The gate found and corrected one release-identity defect before the expensive
compile: CMake previously obtained the app build identity only from Git. An
extracted corresponding-source archive would therefore embed `unknown-dirty`
even though the deterministic archive root already named its source commit.

## Pre-correction observation

The first resumed archive came from documentation head
`34ea4415cda845d3bf58e1493811c0f30887444b`:

```text
size      17,505,157 bytes
members   3,236
SHA-256   369152ea0f8eefda1c04f010074b2a2ed6b7010dd61234e6cc558f47a493d80d
root      CTRPad-source-34ea4415cda8/
```

It passed gzip/tar validation, contained no `.git` tree, passed both packager
syntax checks and exposed all four Apple presets. The first independent
sidecar command ran from the repository rather than beside the basename-
relative sidecar and failed with `source.tar.gz: No such file or directory`.
Running it from the archive directory passed. This was a verification-command
working-directory error, not an archive or sidecar defect.

Before compiling, inspection of the extracted `CMakeLists.txt` showed that
`git rev-parse` would return no hash and `git diff` would return a nonzero
result, producing `unknown-dirty`. That route was rejected without spending a
large compile on a knowingly wrong identity.

## Source-identity correction

Commit `4673e1f9fcab` adds `CTR_NATIVE_SOURCE_COMMIT` and makes the identity
selection explicit (`CMakeLists.txt:11-13,58-137`):

1. In a real source-root Git checkout, CMake uses the checkout commit and
   preserves the tracked-dirty suffix. It verifies that the Git top level is
   exactly the source directory, so an extracted tree nested inside an
   unrelated parent repository cannot inherit the parent's identity.
2. Without source-root Git metadata, a 12-hex
   `CTRPad-source-<commit>` directory supplies the packager-derived clean
   identity.
3. A renamed no-`.git` tree can provide 12–40 hexadecimal characters through
   `-DCTR_NATIVE_SOURCE_COMMIT=...`; CMake normalizes and uses the first 12.
4. Invalid input fails configuration. In a real checkout, a nonmatching
   override also fails rather than replacing the checkout identity.
5. An unrecognized no-`.git` tree retains the conservative
   `unknown-dirty` fallback.

README and Installation Information document the normal generated-root and
renamed-root routes (`README.md:184-201`, `docs/INSTALL-IOS.md:52-69`). The
archive checksum remains the integrity proof; the directory name and explicit
override are identity inputs, not cryptographic authentication by themselves.

Candidate Git-checkout configuration produced
`CTR_NATIVE_BUILD_ID="34ea4415cda8-dirty"` while the three tracked edits were
present. A `000000000000` override failed with the exact checkout-mismatch
diagnostic. `not-hex` failed the 12–40 hexadecimal validation. Neither rejected
route generated a build graph.

## Exact archive reproduction

Implementation commit `4673e1f9fcabbb27526f8422dc9c35aaf356f50e` was
pushed before the longer proof. Two separate nice-15 packages then produced
byte-identical results:

```text
size      17,505,454 bytes
members   3,236
SHA-256   5cdbbf9d8939b4aa63b157bdcd1e865328531909da3763862aa9e37bcb51a2cd
root      CTRPad-source-4673e1f9fcab/
```

The first package took 19.83 seconds. `cmp` succeeded, both basename-relative
sidecars passed from their containing directory, gzip validation passed, and
the packager's retail/runtime/package/profile/key exclusions passed. Fresh
extraction had no `.git`, both shell scripts passed `bash -n`, and all four
Apple configure presets enumerated.

## Fresh no-`.git` configure, build, and tests

The untouched generated root was configured and built sequentially with no
retail media:

```text
nice -n 15 cmake --preset macos-arm64
nice -n 15 cmake --build --preset macos-arm64 --parallel 1
nice -n 15 ctest --test-dir build-macos-arm64 --output-on-failure -j 1
```

Configure emitted:

```text
CTR Native: source identity 4673e1f9fcab from corresponding-source root
```

It completed in 73.09 seconds. The fully fresh one-job build compiled 242
targets in 209.27 seconds, repeated the 32 established legacy warnings and
linked successfully. Product identity was:

```text
file      Mach-O 64-bit executable arm64
version   CTR Native 0.1.0-beta.7.1 (4673e1f9fcab)
SHA-256   a645cdece83a697ff1cd628ec93dee4c393fe353fa8476f3dd6b471e80bfade2
```

All 22 CTests passed in 3.06 seconds; the outer wall time was 3.29 seconds.
No retail image or generated result from the checkout was used.

Vendored SDL's own diagnostic revision string was
`SDL-3.4.10-HEAD-HASH-NOTFOUND` because its revision helper also expects Git
metadata. CTRPad's application build ID is exact and the complete SDL source is
inside the hashed archive, so this does not invalidate the source-build gate.
The SDL diagnostic is recorded rather than misreported as matching the Git-
checkout marker; preserving an archive-native SDL diagnostic remains optional
release polish.

## Renamed-root override

A second fresh extraction was renamed to `renamed-source` and remained free of
`.git`. Configuration with the full implementation commit completed in 76.72
seconds:

```text
cmake -S renamed-source -B renamed-source/build-override -G Ninja \
  -DBUILD_TESTING=OFF -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DCMAKE_OSX_ARCHITECTURES=arm64 \
  -DCTR_NATIVE_SOURCE_COMMIT=4673e1f9fcabbb27526f8422dc9c35aaf356f50e
```

CMake emitted `source identity 4673e1f9fcab from CTR_NATIVE_SOURCE_COMMIT`,
and the generated compile definition was exactly
`CTR_NATIVE_BUILD_ID="4673e1f9fcab"`. The already-completed generated-root
build proves the same source content; a redundant second unity compile was not
run.

## Resource and cleanup record

The entire checkpoint used nice priority 15 and one configure/build/test at a
time. No Simulator application was opened and no simulated device was booted.
The closing sample reported 48% system-wide memory free, zero throttled pages,
and 9,700.94 MiB swap used, lower than at the start of the prior matrix.

After evidence collection, three task-owned temporary trees measuring about
79 MiB, 1.4 MiB, and 199 MiB were enumerated and deleted by exact path. This
removed the candidate archives, extracted source, build objects, binaries and
sidecars. It did not target the repository, `ref/CTR`, a Simulator container,
or user data.

## Acceptance boundary

This accepts:

- byte-reproducible corresponding source at implementation commit
  `4673e1f9fcab`;
- an exact application identity in a no-`.git` generated archive root;
- a fully fresh same-host ARM64 configure/build and 22/22 test run; and
- the documented renamed-tree override plus fail-closed Git/format controls.

It does not prove that an independently provisioned clean Mac has all required
tools, does not refresh the macOS app bundle or iOS products at this new
implementation commit, and does not supply Apple signing or physical-iPad
evidence. M11 and the overall goal remain active at those boundaries.
