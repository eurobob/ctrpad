# Exact iOS Simulator installation helper

## Scope

This checkpoint converts the 2026-08-01 stale-installed-bundle correction from
a manual procedure into a repository-owned safety boundary. It does not change
game code, claim a physical-device signature or replace the existing clean
release smoke. It makes later Simulator evidence rejectable unless the
installed executable can be tied to the selected build product.

The source head at the start was
`b80d7b5afc770e03984a138bee7a0030bf04bd26`; the worktree was clean and only
`CTRPad Import Validation`
(`1D19A61F-20B7-46B0-AB52-B3A3406952E2`) was booted. `CTRPad Import Negatives`
remained shut down. The selected build embeds application-source identity
`d5772375fabc`, as already accepted by the clean re-baseline.

## Installation contract

`tools/install-ios-simulator.sh` now:

1. requires macOS and exactly one booted Simulator;
2. optionally requires that sole device to match an explicit UDID;
3. validates `CFBundlePackageType`, bundle ID, executable name, thin ARM64
   architecture, Mach-O type and the `IOSSIMULATOR` load command;
4. rejects retail-like media inside the source bundle;
5. copies the app with metadata suppression to a uniquely named temporary
   directory;
6. ad-hoc signs only that isolated copy, then applies strict/deep signature
   verification;
7. performs an update install through `simctl`;
8. resolves the installed app container and requires the installed executable
   SHA-256 to equal the signed staged executable; and
9. optionally compares the existing retail image and slot-zero save by inode,
   size and SHA-256 before and after the update.

The helper never boots, shuts down, erases or deletes a Simulator. It does not
modify the source `.app`, and its ad-hoc signature is Simulator-only.

## Static and negative checks

At 17:26:08–17:26:10 CDT:

```text
bash -n tools/install-ios-simulator.sh                 PASS
tools/install-ios-simulator.sh --help                  PASS
tools/install-ios-simulator.sh --unknown               rejected, exit 1
tools/install-ios-simulator.sh --app /tmp/not-real     rejected, exit 1
tools/install-ios-simulator.sh --device 26F3...9C06    rejected, exit 1
```

The last case used the known shutdown negative-test device and stopped with:

```text
ERROR: requested Simulator 26F3DEE8-8840-446D-85FE-C882009C9C06 is not the
only booted Simulator 1D19A61F-20B7-46B0-AB52-B3A3406952E2
```

No install occurred in any negative case. `shellcheck` was unavailable on this
machine; this is retained as a tooling absence, not described as a pass.

## Exact live update

The accepted command was:

```sh
./tools/install-ios-simulator.sh \
  --device 1D19A61F-20B7-46B0-AB52-B3A3406952E2 \
  --verify-persistence \
  --launch
```

It completed successfully and reported:

```text
SOURCE_EXECUTABLE_SHA256=1699c36d136c7bf5ce173ea5701e6313f9d40abc038b77fc27d83a3a08ca7091
SIGNED_STAGED_EXECUTABLE_SHA256=c6d40aaf3c0cfc989e47a79b74ff86f9e52b95a74505c58abaf77d323319187f
INSTALLED_EXECUTABLE_SHA256=c6d40aaf3c0cfc989e47a79b74ff86f9e52b95a74505c58abaf77d323319187f
PERSISTENCE_VERIFIED=retail-and-slot-zero
LAUNCH_RESULT=io.github.chrissotraidis.ctrpad: 66389
```

The app bundle moved to container `2BF7ACA4-7075-4CA1-B3E0-A720809B185B`.
The data container moved to `16180269-6639-463B-A3A4-54603D6B14CF`, while the
files retained exact identity:

| File | Inode | Bytes | SHA-256 |
| --- | ---: | ---: | --- |
| Retail `ctr-u.bin` | 111131200 | 605,698,800 | `f780bf2331476aabfc00772fa758b12dd95ebfbc907968132cbd3cdd4e2c07c0` |
| Slot-zero save | 111222179 | 6,016 | `6a01b0f5562ed7a279d8f8e51e3b1874ac39a6120f55db4fe3873288950619a3` |

The raw build-tree executable remained `1699c36d...091`; strict verification
continued to reject its stale resource signature with `code has no resources
but signature indicates they must be present`. This is expected and proves the
helper changed only its isolated copy. Its temporary staging directory was
removed after completion.

## Post-install runtime evidence

The new session opened at `2026-08-01T22:26:46.599Z` (17:26:46 CDT), reported
`0.1.0-beta.7.1 build=d5772375fabc`, initialized the 1032x1376 GLES 3 surface,
enabled framebuffer fetch, activated the touch overlay and returned to the
UIKit display loop. The current 30-line log and four retained rotations had
zero targeted `[ERROR]`, `[FATAL]`, `[CTR AssetRef]` or visibility matches.

A Simulator screenshot at 17:27:06 CDT visibly contained the current
`CONTROLS` and `CHANGE DISC` controls plus the rendered Aku Aku crate/title
sequence and complete touch overlay. The uncommitted local evidence PNG was
`/tmp/ctrpad-installer-verified-2026-08-01.png`, 2064x2752, SHA-256
`11455c4784c0e3a423a93f05b9218ec92713c76db92d4504ea2c1cf6bfb991ba`.

One supplementary `simctl spawn ... ps -ax` diagnostic returned POSIX error 2
because that guest command was unavailable. This did not affect installation:
`simctl launch` returned PID 66389, the new app log advanced, and the screenshot
captured its rendered output.

## Boundary

At the 17:27:33 CDT reading, active goal time was 266,659 seconds: 3 days,
2 hours, 4 minutes and 19 seconds. This closes reproducible exact Simulator
installation and update-preservation proof. It does not close the overall
goal: this Mac still lacks an Apple signing identity, matching provisioning
profile and connected target iPad, so signed physical installation, hardware
cadence/thermals, audio latency and real multi-touch drift/boost acceptance
remain open.

## Publication-source verification

The implementation and history were committed at 17:32:23 CDT as
`67b4c6276640dc59fb4495cf26ce4e86e64ea5b1`. From that clean commit,
`package-source.sh` produced a 3,249-member corresponding-source archive at
17:32:48 CDT. Its SHA-256 was
`116781ee716c0f8d4400fb82c12e5ee8af991c478d058917ebe553edd4cb42c5`; member
inspection found both `tools/install-ios-simulator.sh` and this report, while
the packager excluded retail/runtime/package/profile/key material.

The packager's own checksum verification succeeded. A redundant outer
`shasum -c dist/...sha256` command then failed because the sidecar intentionally
contains the archive basename and the caller remained one directory above
`dist/`. Repeating the same check from inside `dist/` returned `OK`. This was a
verification-caller path mistake, not an archive or sidecar defect.

At the 17:33:01 CDT reading, active goal time was 266,988 seconds: 3 days,
2 hours, 9 minutes and 48 seconds. Publication to GitHub remained the next
step; the physical-device boundary was unchanged.
