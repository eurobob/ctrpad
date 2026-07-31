# Full Same-Commit ARM64/i686 Parity Acceptance — 2026-07-31

## Result

Clean macOS ARM64 and optimized Linux i686 producers built from the same
committed source generated complete version-4 reports that match on all eight
required game-state and transport components for every one of 24,232 frames.
Both reports finalized 81 rolling checkpoints. The i686 recording process
exited 0 without an OOM kill.

This is the first accepted full same-commit cross-width trajectory. It closes
the full ARM64/i686 comparison that remained open after the potion and
cutscene emitter corrections.

At this report's initial publication, the two-process/mutation portion was
still running. It subsequently completed naturally and passed in full, as
recorded in **Independent i686 process/mutation completion** below. The
cross-width comparison and the independent-process proof remain distinct
claims even though both are now accepted.

## Shared source identity

```text
source commit:
  eee2a8df5b9605d27c7b20e943bba76174a4f6fc
subject:
  fix: preserve cutscene emitter layout across widths
embedded build ID:
  eee2a8df5b96
```

The immutable producers were:

```text
macOS ARM64:
  build-macos-arm64/ctr_native-cutscene-fix-producer-eee2a8df5b96
  SHA-256 fa9a7d46292ab09b143e0e2b514317daa251af48f6341dc437b1f5d81ded3961

Linux i686:
  /private/tmp/ctrpad-i686-cutscene-run-4hjAQW/ctr_native-cutscene-fix-producer-eee2a8df5b96
  SHA-256 d2e6f06023ccaedae689f11b36b33e005cb30d7bbc70d2a5e3e036f57b276c8e

i686 toolchain manifest:
  /private/tmp/ctrpad-i686-vehlap-8IzCKm/toolchain-packages.txt
  SHA-256 7533bb723143fa947795ded64dbde4c82ffb7bb902b246929a11ac725ec22cd6
```

The i686 container mounted the producer under `/run`, the input report
read-only under `/source-report`, and the user's raw disc image read-only at
`/run/assets/ctr-u.bin`. A later development rebuild under an unrelated
`/out` mount could not alter the running immutable producer.

## Finalized reports

| Property | macOS ARM64 | Linux i686 |
|---|---|---|
| Report | `build-macos-arm64/debug/reports/20260730/ctr-215303` | `/private/tmp/ctrpad-i686-cutscene-run-4hjAQW/debug/reports/20260731/ctr-025812` |
| Platform | `macos` | `linux` |
| Replay version | 4 | 4 |
| Build ID | `eee2a8df5b96` | `eee2a8df5b96` |
| Frames | 24,232 | 24,232 |
| Checkpoints | 81 | 81 |
| Finalized | yes | yes |
| Powerslide marker | present | present |
| Save SHA-256 | `6a01b0f5562ed7a279d8f8e51e3b1874ac39a6120f55db4fe3873288950619a3` | same |

The i686 process ran from 2026-07-30 21:58:11 -0500 through
2026-07-31 00:21:14 -0500. Docker reported:

```text
status=exited
exitCode=0
oomKilled=false
```

The log ended with:

```text
[CTR State] checkpoint #80 replayFrame=24000 checksum=0x440b2ebd
[CTR Replay] replay-seeded recording finished after 24232 frames
---- LOG CLOSED ----
```

## Definitive eight-component comparison

Command:

```sh
node tools/compare-replay-state-components.mjs \
  --require timing,rng,drivers,world,allocation,root,pads,vsync \
  build-macos-arm64/debug/reports/20260730/ctr-215303/input.ctrreplay \
  /private/tmp/ctrpad-i686-cutscene-run-4hjAQW/debug/reports/20260731/ctr-025812/input.ctrreplay
```

Result:

```text
timing:      equal=24232 mismatched=0 ranges=none
rng:         equal=24232 mismatched=0 ranges=none
drivers:     equal=24232 mismatched=0 ranges=none
world:       equal=24232 mismatched=0 ranges=none
allocation:  equal=24232 mismatched=0 ranges=none
root:        equal=24232 mismatched=0 ranges=none
pads:        equal=24232 mismatched=0 ranges=none
vsync:       equal=24232 mismatched=0 ranges=none
```

The command exited 0 and reported that every required component matched.

The stricter transport-semantic comparison also exited 0:

```text
padTransport:             equal=24232 mismatched=0
elapsedTime:              equal=24232 mismatched=0
vblankTotal:              equal=24232 mismatched=0
rawVblankBlock:           equal=24232 mismatched=0
expandedPreFrameVblank:   equal=24232 mismatched=0
expandedInFrameVblank:    equal=24232 mismatched=0
```

Unlike the earlier promoted-input audit, even the raw version-4 VBlank blocks
are identical here.

## Evidence hashes

macOS ARM64 report:

```text
input.ctrreplay  dfd06c677f29d9c2155cb01cf00fd958dddfc06127d9b6c67937651039029e09
state.ctrstates  a0ea4a59e7e26716e99249430aed02bd45794a096c4b929dcfd323e8ecd03632
metadata.txt     9ed1ba92c55da00135e5329bce682b2d82a8530a4daab3607bdd6b10eb0d9e4d
ctr-native.log   bea2c87b0c6694f139561ce5f2ba94eba7d46a6daa54b8bce17b39aba3d0de1a
```

Linux i686 report:

```text
input.ctrreplay  2c1d72d8b545982a7293ea02f32addfee909c92aac59794ab822e82c2f58adb1
state.ctrstates  4311a1d1d508cbf13b0d707fdf4849cd9e16911e79638ec5e50716d5607d3664
metadata.txt     e23368c84849fe33dadd81bcea32ff71f3a033a7b4b4c9f96a86ac09677d36d2
ctr-native.log   9f5231a844fe720f0112b31dd747e57cf932046bad67da6ab0714a56624463cc
```

Raw report files are ignored evidence and are not committed. Their different
file hashes are expected: checkpoint headers and native state bodies contain
pointer-width-specific representation, while the canonical per-frame
components deliberately compare game-visible semantics.

## Acceptance boundary

Accepted:

- exact same source and embedded build ID across ARM64 and i686;
- complete normal finalization on both platforms;
- exact equality of all six canonical game-state digests;
- exact equality of pad and VSync transport;
- exact elapsed-time and expanded VBlank semantics;
- recorded powerslide behavior on both runs; and
- the same checksum-valid game-generated memory-card result.

The checked-in verifier's `CTRPAD_REQUIRE_COVERAGE=0` mode omits only the
separately documented manual coverage form and is described as
process-determinism/mutation verification, not a second complete
golden-coverage run.

No retail image, replay report, checkpoint, save, or extracted asset is
tracked by Git.

## Independent i686 process/mutation completion

The long-running verifier completed without intervention at 12:05:03 CDT on
2026-07-31. Its `--rm` container `ec58fcd7069c` therefore no longer existed,
and the machine-owned `container-exit-status.txt` changed from empty to the two
bytes `0\n`. It produced all three expected logs.

Both unchanged processes ran the complete 24,232-frame replay and logged normal
completion. The direct process restored the recording's raw checkpoint
checksum while the copied i386 loader produced a different raw layout:

```text
playback 1 raw: recorded=0xd4c950a8 restored=0xd4c950a8 equal=yes
playback 1 host: sdata=0x403cd040 gGT=0x403d6bf4 mempack=0x408c8bc0

playback 2 raw: recorded=0xd4c950a8 restored=0x46478f61 equal=no
playback 2 host: sdata=0x3efaf040 gGT=0x3efb8bf4 mempack=0x3f4aabc0
```

Canonical playback nevertheless remained equal through each normal exit. The
automatic mutation selected the first active driver at replay frame 1,711,
changed `driver[0].posCurr.x` from `-165632` to `-165631`, exited through the
required parity-failure route, and reported `drivers mask=0x00000004` as the
first canonical difference. Pads, VBlank transport, timing, RNG, world and
allocation remained equal at the divergence.

A clean detached worktree at `7872f7e61ad6` ran the repository's
`CTRPAD_FINALIZE_ONLY=1 CTRPAD_REQUIRE_COVERAGE=0` verifier against the captured
artifacts. This mode launched no game process; it rechecked source/binary
identity, frame/checkpoint counts, both normal completions, address/raw-layout
separation, the exact mutation result, and then wrote the evidence manifests.
It exited 0 with:

```text
Replay process-determinism and mutation verification passed.
```

Exact producer and final evidence identity:

```text
source commit:       eee2a8df5b9605d27c7b20e943bba76174a4f6fc
i686 binary:         d2e6f06023ccaedae689f11b36b33e005cb30d7bbc70d2a5e3e036f57b276c8e
alternate loader:    eccfafa93226e52e32aff3f97f5f779e7d02306cda017eae6d9c358142d4278d
playback 1 log:      26a801935f3b0ed3749c77047b982d9fa0a6f59c4fd9830e8bf7871b27589249
playback 2 log:      a579d85653751c322a3232a73aa6d4785ca3ffd4525929f7ecf8ea2cee24f728
mutation log:        affa27f8e1f1673dbff38d2f3a160b0948e0bbe5a255a0de2547fd64b1f02c8c
evidence manifest:   19fce2859213c28f9cc5ad6c7a28981b6f9ac1c9db7abf0c52dffc3a54bfdcdf
container image:     sha256:633753dde557f377e580536a32acefb4d11ac6fc8621644602acbddc6f1c6cb5
```

The earlier manual coverage form, accepted cross-width pair, and this
process/mutation result together close M1's deterministic parity-harness gate.
They do not by themselves close M6's broader physical-controller, full-race,
human-audio, renderer, savestate, or natural-quit evidence.
