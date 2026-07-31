# macOS ARM64 Audio Mixer/Reverb Oracle — 2026-07-31

## Result

Clean commit `87f8e7a052c29aa8c01eb76263162e24cf2c5d00` adds a
media-free deterministic audio regression test. A synthetic 16-byte PS1 ADPCM
block travels through the production SPU upload, voice attribute, Key On,
streaming decoder, Gaussian interpolation, stereo volume, master volume,
mixer, and frame-rendering paths. A second one-shot render sends that voice
through the production Room reverb and requires a tail after the dry voice has
stopped.

Signed macOS ARM64, combined ASan/UBSan ARM64, and optimized Linux i686 all
produce the same fixed oracle:

```text
dry FNV-1a:  0x132e19d77167fb3d
wet FNV-1a:  0x4bdedc91d1293ad8
tail frames: 6905
```

This accepts deterministic cross-width SPU voice decode, panning, one-shot
termination, Room reverb, and wet-tail behavior. It complements rather than
replaces the real CoreAudio/XA probe. Subjective listening, representative
retail voice/engine/item/music coverage, every reverb preset, XA transitions,
and iOS audio routes remain open.

## Why this test was added

The earlier exact-producer evidence proved a real 44.1 kHz stereo CoreAudio
open, substantial non-silent output, zero short-run transport deltas, and the
first retail XA sector reaching the audio callback. It did not provide a
repeatable architecture oracle for the native mixer or exercise reverb.

The narrowest automatable gap was therefore inside `platform/native_audio.c`,
where the same source file owns the decoder and mixer. This avoids an audio
device, wall-clock timing, subjective microphone capture, and any retail
asset dependency while testing the production implementation rather than a
duplicate reference mixer.

## Production path under test

The CLI route is `--self-test-audio-mixer` (`main.c:201-204,314-317`) and the
CTest contract is registered at `CMakeLists.txt:244-247`.

`NativeAudio_MixerSelfTestStartVoice` (`platform/native_audio.c:2773-2815`):

1. builds a 16-byte ADPCM block from fixed synthetic nibbles;
2. uploads it at SPU address `0x2000` through
   `NativeAudio_SpuSetTransferStartAddr` and `NativeAudio_SpuWrite`;
3. installs voice 0 volume, pitch, start address, and stable sustain fields
   through `NativeAudio_SpuSetVoiceAttr`;
4. assigns the reverb send through `NativeAudio_SpuSetReverbVoice`; and
5. invokes `NativeAudio_SpuSetKey(SPU_ON, ...)`.

Key On calls the real block decoder. That decoder reads SPU RAM live, decodes
all 28 samples, retains ADPCM history, and honors loop flags
(`platform/native_audio.c:1386-1485`). The test holds the already-keyed voice
at a unity sustain level so the oracle isolates decoder, pitch/interpolation,
panning, termination, and reverb rather than depending on a chosen attack
curve.

## Dry stereo oracle

The dry phase uses one block marked Loop Start + Loop End + Repeat, pitch
`0x1000`, master volume `0x7fff/0x7fff`, and voice volume
`0x6000/0x2000`. It renders 4,096 stereo frames through
`NativeAudio_RenderFrames` (`platform/native_audio.c:2840-2862`). Acceptance
requires:

- at least one nonzero stereo frame;
- nonzero right-channel energy;
- left absolute energy greater than twice right absolute energy; and
- exact byte digest `0x132e19d77167fb3d`.

The digest is 64-bit FNV-1a over each signed sample's explicit low byte then
high byte (`platform/native_audio.c:2755-2771`). It therefore does not depend
on hashing an in-memory host structure.

## One-shot Room reverb oracle

The wet phase resets all native audio state, selects
`SPU_REV_MODE_ROOM | SPU_REV_MODE_CLEAR_WA`, sets left/right depth to
`0x7fff`, enables voice 0's reverb send, and uses a centered
`0x6000/0x6000` one-shot block with Loop End but no Repeat
(`platform/native_audio.c:2864-2877`).

It renders 24,000 stereo frames. The dry voice must be inactive at the end,
and frames 256 onward must contain a wet tail. The exact result is 6,905
nonzero tail frames and digest `0x4bdedc91d1293ad8`
(`platform/native_audio.c:2882-2903`). All three values are compiled into the
test and CTest regex, so future output drift fails.

## Development sequence

The first implementation deliberately printed but did not yet lock the two
digests. It passed ordinary ARM64, ASan/UBSan ARM64, and optimized i686 with
identical bytes and 6,905 tail frames. Only after that independent
cross-width observation were the values promoted to required constants.

The fixed-oracle source then passed complete pre-commit ARM64 and sanitizer
suites. Commit `87f8e7a05` was created, and all three producers were rebuilt
again so the final evidence embeds a clean source identity. No failed product
run was relabeled as a pass, and the preserved long verifier's executable and
report were not modified.

## Exact clean matrix

### Signed macOS ARM64 app

```text
build ID:       87f8e7a052c2
architecture:   Mach-O 64-bit executable arm64
CTest:          17/17 passed in 0.57 seconds
signature:      strict/deep verification passed
Info.plist:     plutil passed
SHA-256:        85c03b1a557ad379a869ded940b1ed37879c918d96401a7c6f860d99e7611d93
compile output: 32 established warnings; no new audio-test warning
```

### Combined ASan/UBSan macOS ARM64

```text
build ID:       87f8e7a052c2
ASAN_OPTIONS:   symbolize=0:abort_on_error=1:detect_leaks=0
UBSAN_OPTIONS:  halt_on_error=1:print_stacktrace=1
CTest:          17/17 passed in 3.96 seconds
finding:        none
SHA-256:        5382e363bd6c9872674d3a65077c47e7f23dbaf4e96a1f01e82f6b4ff1601dc6
compile output: 59 established warnings; no new audio-test warning
```

### Optimized Linux i686

```text
build ID:       87f8e7a052c2
architecture:   ELF 32-bit LSB PIE, Intel 80386
interpreter:    /lib/ld-linux.so.2
GNU Build ID:   eb4975f6df60b41738c657827bbe5ed38038e54a
CTest:          17/17 passed in 3.74 seconds
SHA-256:        abc019636ca9e8f5081584b7fcc12b36b918299148df7118bc2bd0e07394ef48
compile output: four established warnings; no new audio-test warning
```

The disposable i686 build remained at
`/private/tmp/ctrpad-i686-controller-gXAeRV`; source was mounted read-only at
`/src` and output read/write at `/out` in pinned image
`ctrpad-linux-i686:ubuntu-24.04`.

## Retail-byte and acceptance boundary

The test contains only authored synthetic nibbles. It opens no disc image,
XA, STR, BIG, HOWL, or other retail file; writes no PCM capture; and adds no
retail-derived artifact to Git.

Accepted here:

- streaming SPU ADPCM decode produces exact cross-width PCM;
- asymmetric volume produces the expected left pan;
- a nonrepeating block stops the voice;
- the Room preset and voice send produce a deterministic wet tail; and
- the path is free of ASan/UBSan findings in this test matrix.

Still open:

- human listening and comparison with retail hardware/emulation;
- representative retail music, voice, engine, item, and multi-voice mixes;
- reverb presets other than Room and retail mode-transition sequences;
- broader XA track selection, completion, and transition behavior;
- long CoreAudio soak; and
- iOS/iPadOS route changes, interruptions, backgrounding, and device output.

## Time and concurrent verifier

The audit began at approximately 05:50 CDT and the exact clean matrix
completed by 06:24 CDT. The goal timer advanced from 141,675 seconds to
143,697 seconds during that interval and reported a cumulative
1 day, 15 hours, 54 minutes, 57 seconds at the documentation checkpoint. This
is product-task elapsed time, not a labor estimate or audio benchmark.

The independent alternate-layout i686 verifier remained running, unpaused,
and not OOM-killed. Its empty machine status file prevents any completion
claim. During this work it crossed its third and fourth 2,000-frame markers
(through frame 8,000), reporting 1.06 then 1.40 FPS windows, and observed
driver transitions at 6,959/7,012. Playback 2, captured exit zero, raw-layout
separation, and the deliberate mutation remain open.
