# 2026-07-30 ARM64/i686 Prefix-Parity Result

**Source revision:** `a40a7584c5761ce24b54bbb9280058a1d84b9dd5`

**Status:** accepted for the 2,200-frame startup/menu/loader/early-race
prefix; not acceptance of the full 24,232-frame golden scenario

## Scope and retail input

The user-supplied image was:

```text
ref/CTR/CTR - Crash Team Racing (USA).bin
```

It is 605,698,800 bytes, or 257,525 exact MODE2/2352 sectors, and has
SHA-256:

```text
f780bf2331476aabfc00772fa758b12dd95ebfbc907968132cbd3cdd4e2c07c0
```

The paired CUE names that BIN and declares one `MODE2/2352` track. Retail
payloads, generated replays, checkpoints, and memcards remain ignored and are
not tracked by Git.

Replay version 4 supplied all 2,200 input frames and the complete VSync
boundary. The segment enters the race at replay frame 1,710 and ends after 490
early-race frames. It does not cover a complete race, powerslide/item/lap/save
coverage, multiplayer, end-of-race, or persistence.

## Defects closed by this investigation

Four independent issues were corrected before final evidence was accepted:

1. Replay versions 2 and 3 omitted VSync calls emitted before the next tracked
   frame during asynchronous loading. Version 4 owns those packets in the next
   frame, records the pre-frame encoded-entry count, and run-length-encodes
   repeated packets without enlarging the fixed frame.
2. `CsOpcodeArg` contained a native pointer, widening `CsOpcodeMeta` from the
   retail 20-byte record to 32 bytes on LP64. The decoder then cleared `arg1`
   at the retail rotation offsets, changing six animation endpoints and the
   RNG call sequence. Decoded opcode words are now four bytes on every host.
3. Driver-table entries can remain non-null after their large-stack slots are
   freed. Digest schema 2 hashes only aligned current-pool slots absent from
   the free list, and hashes allocator lifecycle/populations instead of
   pointer-width-dependent physical coordinates.
4. The first final sanitizer attempt found `INSTANCE_Birth` reading beyond the
   short native literal `"akubeam1"`. Native name copying now stops at `NUL`
   and zero-fills the 16-byte destination; the non-native retail copy remains
   unchanged.

The complete diagnostic sequence, including rejected probes, failed
hypotheses, temporary instrumentation, and intermediate reports, is in
`docs/history/ENGINEERING-JOURNAL.md`.

## Final report identities

All three final reports finalized 2,200 frames, eight rolling checkpoints, and
race-driver activation at frame 1,710.

| Property | Ordinary ARM64 | Disposable i686 | ASan/UBSan ARM64 |
|---|---|---|---|
| Report | `build-macos-arm64/debug/reports/20260730/ctr-103032` | `/tmp/ctrpad-i686-final-m32-uGz0OL/debug/reports/20260730/ctr-153116` | `/tmp/ctrpad-arm64-asan-v4-0Z3iyC/debug/reports/20260730/ctr-103044` |
| Executable fingerprint | `d711aa67b79f365c` | `3723d854320c764a` | `dacfb49a867ac4fe` |
| Identity checksum | `0xec7b4691` | `0x77e93ab7` | `0xec3dbecd` |
| Executable SHA-256 | `7d150121b63307d4f7319938c0680f2f37bcc71cfafc550dc96169138bf5efd0` | `93ebb84eee1b30a205818a1749db3b97e81fb2f4172dd9a61bc20eab6871b00a` | `1e43d48c9cab84e30e7ac1c53c7d85b07b193e4fd8187105a9559276d10dfe6c` |
| Replay SHA-256 | `7ab4a53e5dd2f687ace985a8834d783a8e434d9af0b2d71917de5123e2de32ad` | `99cedca46fbfd84b9b1a39e7a5d531ca501a954dca108a61d84e33a7df2b4044` | `90812a6a41b92709df5662a0a4ae833cdbe2ae0defc784e8a4656e02033047a4` |
| Checkpoint SHA-256 | `7edd4527520b74f5d65d86c661928ba6bba78cc53f4af6b954637897799cb9d1` | `651f203adc345dcff98eb007862bbb18e0e62473aece78b60efd7ba3467df19a` | `bd9eedd7a6db23b74b2c89a03f0540c70ffdf545b701ae8ae89c36bff5469df2` |

The file hashes are intentionally different because replay headers bind exact
binary/platform identity and native checkpoints contain host-width physical
state. Cross-width acceptance uses the fixed-width canonical per-frame
components.

The i686 producer is an ELF32 Intel 80386 executable with build ID
`6191c605c1bebcfd0e4192e1b5131a8ebf291ca2`. Both ARM producers are thin
ARM64 Mach-O executables.

## Exact component result

The final comparison was:

```sh
node tools/compare-replay-state-components.mjs \
  --require timing,rng,drivers,world,allocation,root,pads,vsync \
  build-macos-arm64/debug/reports/20260730/ctr-103032/input.ctrreplay \
  /tmp/ctrpad-i686-final-m32-uGz0OL/debug/reports/20260730/ctr-153116/input.ctrreplay
```

It reported:

```text
timing:     equal=2200 mismatched=0 ranges=none
rng:        equal=2200 mismatched=0 ranges=none
drivers:    equal=2200 mismatched=0 ranges=none
world:      equal=2200 mismatched=0 ranges=none
allocation: equal=2200 mismatched=0 ranges=none
root:       equal=2200 mismatched=0 ranges=none
pads:       equal=2200 mismatched=0 ranges=none
vsync:      equal=2200 mismatched=0 ranges=none
required components match: timing,rng,drivers,world,allocation,root,pads,vsync
```

The ordinary ARM64 and i686 reports therefore match both deterministic
transport channels and all six canonical state components for all 2,200
frames. The ordinary and sanitizer ARM64 reports also match all six state
components for all 2,200 frames. The corrected ordinary ARM64 report matches
the earlier pre-name-fix ARM64 and i686 reports for all six state components,
confirming that zero-filling unused instance-name bytes did not change
canonical gameplay.

## Sanitizer, tests, and static audit

The rejected sanitizer report
`/tmp/ctrpad-arm64-asan-v4-0Z3iyC/debug/reports/20260730/ctr-102022`
reached checkpoint 7, then failed with a global-buffer-overflow in
`INSTANCE_Birth` through the mask-use path. It is failure evidence, not a
passing run.

After the name-copy correction:

- ordinary ARM64 CTest: 13/13;
- ASan/UBSan ARM64 CTest: 13/13;
- disposable i686 CTest: 13/13;
- ASan/UBSan 2,200-frame replay: completed with no sanitizer report; and
- normal ARM64 and i686 2,200-frame replays: completed normally.

The final forced-LP64 audit reported:

```text
forced_lp64_compile_exit=0
forced_lp64_compile_errors=0
forced_lp64_static_assert_failures=0
pointer_to_integer_coordinates=0
integer_to_pointer_coordinates=0
unique_pointer_narrowing_source_lines=0
i686_object_sha256=7ebc25257acc868616a1616d8a1b4479068b6c9ccb27deb7fc2d703db72c357b
layout_census_sha256=60cd7bb0f4ec3975aee782949faa76e4769393d300def2a9231b9ea9e1961662
```

The protected historical baseline executable remained byte-identical:

```text
build-linux-i686-baseline/ctr_native
SHA-256: afe7b3d264bd2674192e71485037842ab329d1eca9c0c34ca50da5d4487c76a7
size: 8,888,128 bytes
modified: 2026-07-29T17:54:11-0500
```

## Acceptance boundary

This result closes the known timing, RNG, driver-liveness, and allocator
canonicalization defects for the measured prefix. M1 remains in progress
until a fresh version-4 recording covers all eight behaviors in
`NTSC-U-GOLDEN-RUN.md`, replays twice at different address layouts, and fails
at the selected deliberate live-driver mutation.
