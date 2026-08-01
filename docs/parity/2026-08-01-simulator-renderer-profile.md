# Simulator renderer profile and staged presentation

Date: 2026-08-01 CDT

This checkpoint responds to the explicit product gate that CTRPad must be
stable, diagnosable and graphically coherent in one iOS Simulator before any
physical-device work resumes. It does **not** close that gate. It identifies
and removes most of one presentation bottleneck, proves the replacement is
pixel-identical to the prior path, and leaves the larger per-split submission
cost open.

## Scope and source boundary

The profiling and optimization were performed from a dirty diagnostic working
tree based on the already-published `codex/arm64-apple` history. The first
profile embedded `25976cd77c72-dirty`; subsequent documentation-only commits
advanced the published base, and the optimized profile embedded
`867dbd264fb4-dirty`. The product-code difference between the two measured
paths is the staged presentation change recorded here. Neither dirty build is
release or exact post-commit acceptance evidence.

The previous exact clean Simulator replay remains the behavioral baseline:
commit `ba80d153ae55` consumed 20/20 keyboard actions through the retail poll,
rendered coherent Crash Cove and Roo's Tubes scenes, survived rotation and
Home/resume, and retained five clean log generations. Its 63 reported FPS
samples still rejected the gate because they averaged 7.37 and settled near
4.61-5.30 FPS.

All compilation in this checkpoint used `nice -n 15` and one job while both
named Simulator devices and the Simulator GUI were shut down. Only disposable
device `CTRPad Import Negatives`
(`26F3DEE8-8840-446D-85FE-C882009C9C06`) was later booted. Protected device
`CTRPad Import Validation` remained shut down. The app was explicitly
terminated with its verified plist identifier
`io.github.chrissotraidis.ctrpad`; both devices and Simulator were shut down at
the end.

## Instrumentation before optimization

The existing frame profiler was extended without changing game-visible state:

- separate packed-VRAM restore and final-presentation timing buckets;
- renderer draw-call and submitted-vertex counters;
- GPU split, split-vertex and semitransparent-split counters;
- per-frame CSV columns plus totals and maximum-per-frame summary fields.

The unoptimized iOS diagnostic ran the same menu-to-Crash-Cove route and
preserved 2,772 valid 57-column frames in
`/tmp/ctrpad-perf-dirty-frame-times.csv`:

```text
bytes    920,708
SHA-256  17cc1316847113a5354e32c840c4e83dd36ae93cbaa3163719596ecd7682fee3
log      7,493 bytes / 60 lines
log SHA  482ecefe410e63f944363e0d5d2131a48bf39c0307196e235687077fdb40d126
```

Crash Cove averaged 230.729 ms total and 198.501 ms non-wait work across 731
frames. Two costs dominated that work: 137.739 ms in split submission and
47.201 ms in final packed-VRAM presentation. The scene averaged 112.29 splits
and 70.41 semitransparent splits per frame. Semitransparent PS1 primitives use
two native passes so opaque/non-STP and blended/STP texels preserve retail
semantics; this explains why renderer draw calls exceed split count.

## Staged presentation design

The old final path executed the integer packed-PS1-VRAM decode shader directly
over the complete host viewport. The live iPad Simulator surface was
1032×1376, so the software renderer decoded roughly 1.42 million destination
pixels even though the logical game display was normally 320×240.

The new path:

1. uploads dirty packed VRAM exactly as before;
2. decodes the selected VRAM display rectangle once into a reusable RGBA
   framebuffer at the logical display dimensions;
3. scales that resolved image into the host presentation viewport with
   `glBlitFramebuffer`, `GL_NEAREST`;
4. restores the renderer's tracked GL state at the same boundary as the prior
   presentation path.

The direct path remains internal as a test oracle. The GLES loader contract now
requires `glBlitFramebuffer`, so an unsupported implementation fails during
renderer initialization rather than presenting a silent black frame.

The renderer pixel self-test now opens a 64×32 host window for a 32×16 logical
fixture, captures both the old direct output and the staged output, and requires
byte-for-byte equality. The test passed with:

```text
logical hash  851169f2644a1675
present hash  a7798c5a6ddee965
marker        present=resolve+blit@2x
```

This covers every byte of the scaled test framebuffer. It supplements rather
than replaces live retail-scene inspection.

## Build and automated verification

The optimized macOS ARM64 build linked in 68.06 seconds with the established
32 source warnings and no new warning. The focused renderer pixel test passed
in 3.64 seconds. The complete native suite then passed 22/22 in 5.66 seconds,
including input, lifecycle, storage, atomic memory-card and renderer tests.

With Simulator still closed, the iOS Simulator ARM64 build linked in 74.03
seconds with the same 32 established warnings. Its unsigned executable was a
thin ARM64 Mach-O, imported `_glBlitFramebuffer`, embedded
`867dbd264fb4-dirty`, and hashed to:

```text
48c0382240ac908a150a40628544f313f6d542b43d3d21a4f6899c6c1d50d534
```

A disposable ad-hoc-signed copy passed strict/deep verification. Signature
replacement changed its executable SHA-256 to:

```text
3b54a227c7257fab539939dd33b67753b791d25e62e8ad6f3b812c29ef0dbe51
```

The ordinary build artifact was not modified by signing.

## One-Simulator live route and visual inspection

Computer Use opened the one disposable device and enabled Simulator hardware
keyboard capture. The published keyboard mapping then performed:

```text
S  move from Adventure to Time Trial
K  enter Time Trial
K  select Crash
K  select Crash Cove
K  choose No Ghost
I  skip the course fly-in
```

The rotating app log recorded all six actions as eight retail-poll
consumptions when a later K/D tap stream was included. The tap stream collapsed
to combined held masks `0x4000` and `0x4020`, as expected at the slow retail
consumer cadence; it did not constitute a sustained driving test. The previous
exact run remains the accepted keyboard driving/pause/level-change evidence.

The following live frames were visually inspected and coherent under the new
path:

- Naughty Dog crate/logo and glow;
- main CTR logo, checkered background and complete mode menu;
- Crash model, kart and all eight character portraits;
- track names, Crash Cove preview and minimap outline;
- No Ghost prompt and lettering;
- Crash Cove fly-in with kart, track, sky and water;
- grid/race view with kart, headlights, start lights, CTR banner, lap/timer
  HUD, turbo meter, minimap, road, cliffs, sky and water;
- the touch overlay and its labelled controls.

No retail-derived screenshot was committed. This bounded route shows no
recurrence of the reported missing-assets pattern; it does not prove every
track, character, effect or asset-cache recycling sequence.

## Measured result

The optimized session was explicitly stopped after 461 seconds. It preserved
3,000 valid 57-column frames and the flushed application log:

```text
frame CSV       990,063 bytes / 3,001 lines including header
frame CSV SHA   e103671647db8d9a94fb72b5f696754b27f892c5c595775646a00a50d69de034
GPU CSV         28 bytes / header only
GPU CSV SHA     af0f3466758a717080c8ac7d955fb6c432274898fb065e304a8d36cf3c9d91f6
app log         11,145 bytes / 99 lines
app log SHA     9a0b455eb50a3cfb5d86f802448327cf5419c8f5caf8ec5ef43644c87761603a
```

The GPU CSV remains header-only because this iOS GLES/Simulator route does not
expose the optional GPU timer-query samples. CPU-side bucket and counter data
are complete.

| Scene | Frames | Total before | Total staged | Present before | Present staged | Split staged |
|---|---:|---:|---:|---:|---:|---:|
| Intro Cortex, level 41 | 264 / 287 | 111.998 ms | 111.173 ms | 23.807 ms | 8.338 ms | 80.266 ms |
| Intro Polar, level 39 | 1,777 / 1,305 | 170.002 ms | 107.663 ms | 43.134 ms | 7.783 ms | 81.568 ms |
| Crash Cove, level 3 | 731 / 1,408 | 230.729 ms | 181.024 ms | 47.201 ms | 7.737 ms | 131.012 ms |

For Crash Cove, staged presentation reduced the measured present bucket by
83.6%, non-wait work by 24.1%, and total frame time by 21.5%. Reciprocal average
throughput improved from approximately 4.33 to 5.52 frames per second despite
the staged sample carrying more average renderer vertices (5,535 versus
5,062). Intro Polar total frame time fell 36.7% and its present bucket fell
82.0%. Intro Cortex's total was effectively flat because split work varied
upward between these content samples, while its present bucket still fell
65.0%.

All five retained app-log generations were present. A targeted scan found zero
AssetRef, visibility-cache-exhaustion, application ERROR/FATAL,
unbalanced-render, assert, signal or crash markers. No new `CTRPad` diagnostic
report existed. The unified log contained the same five Simulator-framework
messages seen previously: one CoreFoundation plug-in factory registration and
four CoreAudio hardware/acoustic-profile limitations. None named the renderer
or asset pipeline.

## Gate decision and next bottleneck

The optimization is retained because it is pixel-identical and removes a
measured 39.464 ms from the average Crash Cove presentation bucket. The full
Simulator gate remains **open**: 181.024 ms per Crash Cove frame is still about
5.4 times the 33.333-ms 30-FPS budget, and this run covered only one track.

The profiler now localizes the next engineering step. Crash Cove spends
131.012 ms per frame submitting an average 116.64 GPU splits and 72.64
semitransparent splits. Future work should reduce renderer state/draw overhead
while preserving PS1 ordering, blend/STP, mask and framebuffer-feedback
semantics, then repeat the pixel oracle, 22-test suite, one-Simulator profile
and broader level/effect churn. A clean post-commit build is required before
any resulting checkpoint can be accepted. Physical-device work remains gated.

The evidence-analysis goal reading was 239,829 seconds: 2 days, 18 hours,
37 minutes, 9 seconds cumulative. This is 2,835 seconds (47 minutes, 15
seconds) after the preceding published 236,994-second boundary. Goal time
includes pauses/resumes and is neither a build benchmark nor a person-hour
estimate. The goal remains active.
