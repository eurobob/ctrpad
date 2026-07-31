# macOS ARM64 Audio/XA Output Probe — 2026-07-30

## Result

The exact accepted ARM64 producer opened the normal macOS CoreAudio device at
the native 44.1 kHz, stereo, signed-16-bit contract. A second ordinary,
non-replay process redirected the same SDL output stream to the built-in disk
driver and emitted non-silent, non-full-scale PCM with zero measured
underrun/overflow deltas across 702 game frames.

LLDB then proved that the initial retail XA request loaded successfully from
the external assets, selected 52 compressed sectors at 37.8 kHz, and decoded
the first sector into 4,032 source frames on SDL's audio thread before the
samples entered the XA interpolation/mix path.

This accepts the macOS device-open boundary, ordinary SPU output, initial XA
load/decode, and host stream delivery for M6. It is not a subjective listening
test and does not accept every music/voice track, reverb behavior, or STR
movie playback.

## Immutable producer

All probes used:

```text
binary:
  build-macos-arm64/ctr_native-cutscene-fix-producer-eee2a8df5b96
build ID:
  eee2a8df5b96
architecture:
  Mach-O 64-bit executable arm64
SHA-256:
  fa9a7d46292ab09b143e0e2b514317daa251af48f6341dc437b1f5d81ded3961
```

No source or binary was rebuilt between the full state-parity report and
these probes.

## Ordinary CoreAudio open

The producer was started with no replay arguments and no audio environment
override. After normal renderer and game initialization it reported:

```text
[CTR Native] SDL audio stream opened:
  driver=coreaudio
  src=44100 Hz/2 ch
  dst=44100 Hz/2 ch
  device=44100 Hz/2 ch
  sampleFrames=1024
```

The process ran for another ten seconds without an audio underrun/overflow
report, then received Ctrl-C. The native signal path shut it down with exit
status 0 and closed the log. This proves that the real CoreAudio stream can
open, but the absence of a short-run warning was not used alone as content
evidence.

## Deterministic PCM inspection

SDL's compiled-in `disk` audio driver was selected for a second ordinary
process so the exact bytes submitted by the host stream could be inspected:

```sh
env \
  SDL_AUDIO_DRIVER=disk \
  SDL_AUDIO_DISK_OUTPUT_FILE=/private/tmp/ctrpad-audio-capture-kOM8mk/perf.raw \
  SDL_AUDIO_DISK_TIMESCALE=1 \
  ./ctr_native-cutscene-fix-producer-eee2a8df5b96 \
  --perf-dir /private/tmp/ctrpad-audio-capture-kOM8mk/perf
```

The run was stopped with Ctrl-C after 702 measured game frames. It exited 0
and wrote its normal performance summary:

```text
frames=702
average_ms=33.320
average_work_ms=4.584
max_work_ms=9.597
work_frames_over_33_33ms=0
```

Summing the named CSV columns over all 702 rows produced:

```text
audio_underrun_delta:
  0
audio_overflow_delta:
  0
```

`audio_queued_frames` remains zero in this mode by design: ordinary playback
mixes directly in `NativeAudio_StreamCallback`; the explicit scheduled queue
is used only by deterministic replay rendering.

`tools/inspect-native-pcm-s16le.mjs` independently enforces a non-empty
multiple of the four-byte stereo frame size, rejects a silent channel, and
reports per-channel amplitude, full-scale samples, stereo difference, and
SHA-256. It uses the fixed contract requested by `NativeAudio_OpenDevice`.

Result:

```text
bytes=6332416
frames=1583104
durationSeconds=35.898050
sampleRate=44100
channels=2
format=S16LE
left:
  min=-16386
  max=19787
  nonzeroSamples=1555132
  fullScaleSamples=0
  rmsDbfs=-23.215531
  peakDbfs=-4.381400
right:
  min=-16929
  max=20672
  nonzeroSamples=1555170
  fullScaleSamples=0
  rmsDbfs=-23.143742
  peakDbfs=-4.001349
stereoDifferentFrames=1530073
SHA-256:
  eac89fd2abc7d1d115070faaf847aa1293c4e32addd4272e75fec99a47f14834
```

The sample-derived duration describes the captured PCM bytes; it is not a
wall-clock cadence acceptance measurement for SDL's diagnostic disk driver.
macOS game cadence is accepted separately in
`2026-07-30-macos-arm64-cadence.md`.

Passing `/dev/null` to the inspector exits 1 with:

```text
[CTR PCMInspect] PCM capture is empty
```

The raw PCM and performance outputs remain disposable files under
`/private/tmp`. No audio, XA, disc-image, or other retail-derived byte is
tracked by Git.

## Initial XA request and load

A name breakpoint on `NativeAudio_PlayXATrack` stopped at the first automatic
request:

```text
categoryID=1
xaID=80
volumeLeft=32640
volumeRight=32640
```

The stack was:

```text
NativeAudio_PlayXATrack
CDSYS_XAPlay
StateZero
CTR_Main
main
```

Stepping out of the complete loader returned 1. The live state immediately
after that success was:

```text
active=1
frameCount=209664
sampleRate=37800
categoryID=1
xaID=80
sectorCount=52
numChannels=1
```

This proves that the external asset lookup, XA metadata lookup, channel
filter, compressed-sector preparation, and stream activation all succeeded.

## Audio-thread sector decode

The optimized producer inlines
`NativeAudio_XaStreamDecodeNextSectorNoLock`. An initial `thread step-out`
from that inline frame appeared to return 0 without advancing the stream.
That observation was rejected because forcing a step-out of an optimized
inline frame does not reliably execute the inline body.

The replacement probe stopped at the actual success boundary,
`native_audio.c:2224`, before `decodedFrames` was incremented. At that point:

```text
nextSector=1
decodedFrames=0
```

One source-level step executed the increment:

```text
decodedFrames=4032
```

The audio-thread stack at the success boundary was:

```text
NativeAudio_XaStreamDecodeNextSectorNoLock
NativeAudio_GetXAPcmSampleAtFrameNoLock
NativeAudio_GetXAPseudo37800SampleNoLock
NativeAudio_ZigZagInterpolateXASampleNoLock
NativeAudio_GetXAMixSampleNoLock
NativeAudio_MixFrame
NativeAudio_RenderFramesNoLock
NativeAudio_StreamCallback
```

This is direct evidence that the first prepared XA sector decoded into the
rolling PCM ring and was requested by the live SDL output callback. It is not
an inference from `NativeAudio_PlayXATrack` returning success.

## Acceptance boundary

Accepted here:

- exact ARM64 producer opens the real CoreAudio device;
- fixed 44.1 kHz stereo S16 output contract;
- ordinary output contains substantial, distinct left/right PCM;
- no full-scale samples in the captured interval;
- zero measured underrun and overflow deltas across 702 game frames;
- initial XA metadata/source/sector preparation succeeds; and
- the live audio callback decodes the first XA sector to 4,032 source frames.

Still open:

- human listening/quality comparison against retail;
- representative music, voice, engine, item, and reverb coverage;
- full XA-track completion and transition behavior;
- long-duration device underrun/overflow soak;
- STR movie audio/video synchronization; and
- all iOS/iPadOS interruption, route-change, and lifecycle cases.
