# iOS coherent framebuffer-fetch semitransparency

Date: 2026-08-01 CDT

This checkpoint tests the next measured Simulator optimization after staged
logical-resolution presentation. It replaces the two native draws used for a
PS1 textured semitransparent split with one ordered GLES draw when coherent
`GL_EXT_shader_framebuffer_fetch` is available. The portable desktop GL and
unsupported-GLES path retains the established two-pass implementation.

The result is pixel-equivalent and measurably reduces draw submission. Exact
post-commit desktop and iOS oracles plus one-Simulator live replay accept this
implementation checkpoint, but it does **not** close the Simulator performance
gate. The matched diagnostic still averages only 5.80 FPS. Physical-device
work remains closed.

## Source and device boundary

The diagnostic source tree was based on published commit `ff26c0815a04` and
identified itself at runtime as `ff26c0815a04-dirty`. The preceding staged-
presentation profile is the comparison source. No dirty binary is release or
publication evidence. The implementation and this initial record were then
committed and pushed as `d3b5bd410e9ae3aefcb439b105e67ff95e1ca534` on
`codex/arm64-apple`; the existing draft PR is
<https://github.com/chrissotraidis/ctrpad/pull/1>.

Compilation was sequential at nice 15 with one job while both named Simulator
devices and the Simulator GUI were shut down. Live testing booted only the
disposable `CTRPad Import Negatives` device
(`26F3DEE8-8840-446D-85FE-C882009C9C06`). Protected device
`CTRPad Import Validation` (`1D19A61F-20B7-46B0-AB52-B3A3406952E2`) stayed
shut down. The existing data container, imported NTSC-U image, extracted
assets, and memory card were preserved through update installation.

At the end, bundle `io.github.chrissotraidis.ctrpad` was terminated explicitly,
the app log was allowed to flush, the disposable device was shut down, and the
Simulator GUI received a normal `SIGTERM` only after Computer Use could no
longer attach to its window. Both named devices were rechecked as shut down
before documentation or compilation continued.

## Why two native passes existed

PS1 textured semitransparency applies the selected blend equation only to
visible texels with bit 15 (STP) set. Visible non-STP texels remain opaque. A
single fixed-function blend state cannot express both rules, so the portable
renderer draws each affected primitive-sized split twice:

1. blend disabled, shader discards STP texels;
2. selected blend enabled, shader discards non-STP texels.

That path preserves opaque/non-STP and blended/STP behavior, including the
mixed contribution created by bilinear sampling, but duplicates draw calls and
some submitted vertices. The staged Crash Cove profile measured an average
72.64 such splits per frame.

## Coherent one-pass design

Runtime extension discovery enumerates the ES 3 extension list with
`glGetStringi`; it does not use substring matching. When
`GL_EXT_shader_framebuffer_fetch` is advertised, the PSX fragment shaders are
compiled with a required extension directive and a location-zero `inout`
color. The shader reads the existing destination and evaluates the retail
average, add, reverse-subtract, or quarter-source equation only for the sampled
STP contribution. Visible non-STP contribution remains opaque. Ordinary GL
blending is disabled for this path.

The Khronos extension contract is the required semantic basis: coherent fetch
observes prior overlapping samples in API primitive order, defines ES 3
`inout` fragment outputs, and remains orthogonal to fixed-function blending:

<https://registry.khronos.org/OpenGL/extensions/EXT/EXT_shader_framebuffer_fetch.txt>

This avoids reordering or merging retail primitives. The implementation also:

- retains the two-pass fallback without changing desktop GL behavior;
- reports `*PSX framebuffer fetch: enabled` or the fallback reason at startup;
- exposes `gpu_framebuffer_fetch_splits` in the opt-in frame CSV so use of the
  optimized route is proved rather than inferred;
- returns to the neutral semitransparency pass after each draw.

## Strengthened pixel oracle and rejected attempts

The renderer self-test now draws all four PS1 semitransparency equations over a
common blue destination, verifies opaque non-STP and blended STP pixels, and
adds an exact mixed-STP/non-STP bilinear fixture. On an extension-capable GLES
context, it first forces the portable two-pass implementation, then renders the
same fixture with framebuffer fetch and compares every RGBA byte.

Three failures were diagnosed rather than waived:

1. The first iOS self-test reported failures for both direct and staged
   presentation readback. An exact clean `ff26c0815a04` control failed the same
   way. iOS ignored the requested 64×32 SDL window and created a 1032×1376
   surface; the old test supplied 64×32 buffers and rejected before readback.
   The test now allocates checked buffers from the actual host dimensions.
2. Selected blend pixels then passed but the complete blend hash differed.
   The test was strengthened again to run forced fallback and fetch paths in
   the same iOS context and report the first differing byte.
3. That comparison found red channel expected 0, actual 62 at `(21,7)`. At
   this bilinear edge, combined visibility was at least one half while STP and
   non-STP visibility were individually below one half. The portable path
   discarded both passes; the prototype wrote a quarter-source fragment. The
   one-pass discard rule now rejects the same edge when both contributions are
   below the individual visibility threshold.

After the correction, the optimized GLES result matched the two-pass oracle
byte-for-byte. This history matters: checking only representative pixels would
have accepted a real texture-edge divergence.

## Dirty build and automated evidence

After the final shader correction, the macOS ARM64 incremental build linked in
75.79 seconds with the established 32 warnings and no new warning. The pixel
marker was:

```text
[CTR Renderer] pixel self-test passed: api=gl size=32x16 formats=4,8,16 clut=4,8 transparency=zero,stp blend=average,add,subtract,quarter bilinear=mixed-stp mask=output-bit framebuffer=feedback vram=rgb5551 hash=851169f2644a1675 blend-hash=f6dc5a2e558bc7b5 blend-oracle=two-pass oracle-hash=f6dc5a2e558bc7b5 framebuffer-fetch=two-pass present=resolve+blit@64x32 present-hash=a7798c5a6ddee965
```

The full native suite passed 22/22 in 1.90 seconds. With Simulator still
closed, the iOS Simulator ARM64 build linked in 65.07 seconds and repeated only
the same 32 warnings. Its unsigned thin-ARM64 executable hashed to:

```text
76473dce8f82aa23a0969b4f35b784036f70f374066cbe26ccaf5e8f72425a54
```

A disposable ad-hoc-signed copy passed strict/deep verification; signature
replacement produced executable SHA-256:

```text
ff1a87b41ae711627a405433548a7c3215b3e4e2a37173fc5e1ada66410b1a08
```

The iOS oracle marker was:

```text
[CTR Renderer] pixel self-test passed: api=gles size=32x16 formats=4,8,16 clut=4,8 transparency=zero,stp blend=average,add,subtract,quarter bilinear=mixed-stp mask=output-bit framebuffer=feedback vram=rgb5551 hash=851169f2644a1675 blend-hash=f6dc5a2e558bc7b5 blend-oracle=match oracle-hash=f6dc5a2e558bc7b5 framebuffer-fetch=enabled present=resolve+blit@1032x1376 present-hash=172d49a34571b64c
```

The immediate self-test teardown repeated the already-documented UIKit
unbalanced-appearance diagnostic and duplicate accessibility-loader framework
warning after app-owned test work returned. They are harness/framework
diagnostics, not renderer or retail-path failures.

## One-Simulator visual and keyboard route

The signed diagnostic was installed as an update and launched with
`--perf --perf-dir perf-fetch-dirty-ff26`. Simulator hardware keyboard capture
was initially off; Computer Use read the toolbar state and explicitly enabled
it. The keyboard path then reached live Crash Cove:

```text
S  select Time Trial
K  confirm Time Trial
K  select Crash
K  select Crash Cove
K  choose No Ghost
I  skip the fly-in
```

Repeated `K` gas taps moved the kart from the grid to the first coastal opening;
paired `D`/`K` taps turned it along the shoreline. The persistent log records
the keyboard edges and combined `0x4020` masks at the retail-poll consumer.

Computer Use inspected coherent retail pixels at every transition: complete
mode menu and CTR art, Crash model and eight portraits, track list and preview,
ghost prompt, course fly-in, start grid, kart, headlights, HUD, speedometer,
CTR banner, track, cliffs, horizon, animated water, fences, and touch overlay.
This is bounded Crash Cove/menu evidence, not a claim for every level, effect,
character, or cache-recycling sequence. No retail-derived screenshot is
committed.

## Final dirty-session evidence

The flushed app session ran for at least 720.887 seconds and preserved:

```text
frame CSV       1,722,887 bytes / 5,173 lines including header
frame CSV SHA   f8fbef54174cf7fe13cc0435ac1d6b7310612db4d23ed95f46b47d3c471c4ee5
app log         18,813 bytes / 173 lines
app log SHA     096615ca85f8067601db36bbadc876bffb9f7df8aa5c2694c4868e588c95cc76
targeted faults 0
```

The application log identifies build `ff26c0815a04-dirty`, iOS, OpenGL ES 3.0,
the Apple Software Renderer, enabled framebuffer fetch, ready shaders and
pipelines, active UIKit display loop, touch overlay, frame capture, and retail-
poll keyboard consumption. The targeted scan covered error, failure, missing,
corruption, assertion, fatal/exception, texture-failure, shader-failure, and
pipeline-failure terms.

## Matched-scene performance result

The comparison below selects a structurally identical Crash Cove state from
the preceding staged profile and this fetch profile: 123 logical GPU splits,
79 semitransparent splits, and 5,405.94 split vertices per frame. That avoids
comparing unlike track positions.

| Metric | Two-pass, 279 frames | Fetch, 700 frames | Change |
|---|---:|---:|---:|
| Total frame | 187.614 ms | 172.324 ms | -8.15% |
| Non-wait work | 154.872 ms | 147.464 ms | -4.78% |
| Renderer triangles | 144.520 ms | 137.016 ms | -5.19% |
| Split submission | 135.412 ms | 127.764 ms | -5.65% |
| Presentation | 7.574 ms | 7.689 ms | +1.52% |
| Renderer draw calls | 205 | 126 | -38.54% |
| Renderer vertices | 5,885.87 | 5,423.94 | -7.85% |
| Reciprocal average throughput | 5.33 FPS | 5.80 FPS | +8.82% |

All 79 semitransparent splits report the framebuffer-fetch counter. The exact
79-call reduction proves each former second draw was removed. Presentation is
effectively unchanged, as expected; the small increase is sample noise outside
the modified path.

After keyboard movement reached the coastal fence/water scene, 942 frames with
at least 279 draw calls and 219 fetch splits averaged 130.356 ms total, 122.704
ms non-wait work, 110.650 ms renderer triangles, 101.120 ms split submission,
8.755 ms presentation, 280.52 draw calls, and 220.53 fetch splits: reciprocal
throughput 7.67 FPS. No matched two-pass sample exists for that position, so it
is runtime characterization rather than an optimization comparison.

## Gate decision

The one-pass path is accepted at the exact implementation checkpoint because:

- every tested output byte matches the portable two-pass oracle;
- desktop and unsupported GLES retain the portable route;
- live extension use is directly logged and counted;
- the matched scene removes 38.54% of renderer draw calls and improves total
  frame time 8.15% without a visual or logged fault.

It is not enough to accept the Simulator gate. The matched 172.324-ms frame is
still about 5.17 times the 33.333-ms 30-FPS budget. The next dependency is
additional submission/state reduction and wider scene/effect churn. Physical-
device signing, installation, performance, multi-touch ergonomics, and final
sideload acceptance remain closed.

## Exact post-commit acceptance

After `d3b5bd410` was pushed, both named devices and the Simulator GUI remained
off while the exact products were reconfigured and built sequentially at nice
15 with one job.

The exact macOS configure took 2.00 seconds and embedded
`d3b5bd410e9a`. Its incremental revision-dependent build took 65.03 seconds,
repeated only the 32 established warnings, and produced executable SHA-256:

```text
1a15c3345e16a03b674a527115a615d4ef934cc0920bccccce19153cb5178c44
```

The complete native suite passed 22/22 in 2.54 seconds. A separate exact pixel
run from a disposable directory retained the portable marker and hashes:
logical `851169f2644a1675`, blend/fallback `f6dc5a2e558bc7b5`, presentation
`a7798c5a6ddee965`, `framebuffer-fetch=two-pass`, and
`present=resolve+blit@64x32`. Its temporary log directory was deleted after the
marker was captured.

The exact iOS Simulator configure took 1.78 seconds and its one-job build took
77.44 seconds with the same 32 warnings. The product is thin ARM64, identifies
bundle `io.github.chrissotraidis.ctrpad`, and embeds `d3b5bd410e9a`. The
unmodified unsigned executable SHA-256 is:

```text
565ff4417722a1c7e6b31be64bab47ad4bff11f7b134e86906bc017871e2d8b8
```

A disposable ad-hoc-signed copy passed strict/deep verification and changed
only the signed-copy executable hash to:

```text
80d3b864b1750d8505a695a6fd9430ee6cda101d83490f01320a6fde7044b785
```

Only the disposable device booted. The exact iOS oracle again advertised the
extension, compiled all four PSX shaders and both VRAM pipelines, and matched
every fallback byte at blend hash `f6dc5a2e558bc7b5`. The logical and actual-
surface presentation hashes remained `851169f2644a1675` and
`172d49a34571b64c`. The UIKit shell stayed resident after the short-lived
`SDL_main` test returned; explicit correct-ID termination ended PID 3541. The
same documented immediate-test appearance/accessibility framework diagnostics
followed app-owned output.

The normal exact retail app then launched with
`--perf --perf-dir perf-fetch-exact-d3b5bd410`. Computer Use enabled keyboard
capture and repeated `S K K K K I` to reach Crash Cove. Repeated K followed by
paired D/K taps moved and turned the kart into the first inside cliff. Logo,
mode menu, portraits/model, track chooser/preview, prompt, fly-in, grid, kart,
lighting, HUD, map, banner, terrain, horizon, water, fence and overlay remained
coherent. The exact app log correlates the keyboard masks at the retail poll.

Correct-ID termination after at least 285.035 seconds flushed:

```text
frame CSV       705,266 bytes / 2,125 lines including header
frame CSV SHA   1c74e036b8fa21f21c0346169727dde449fed90d5b7a7c3af078c54ce1b7104b
GPU CSV         28 bytes / header only
GPU CSV SHA     af0f3466758a717080c8ac7d955fb6c432274898fb065e304a8d36cf3c9d91f6
app log         14,753 bytes / 130 lines
app log SHA     f517e967e6453b4d353c74af5c644b997c27d6d1efba75f3a93d77ca331b7fbf
targeted faults 0
```

The exact CSV contains 976 normal Crash Cove frames, averaging 144.598 ms
(6.92 reciprocal FPS). Its last 300 average 134.157 ms (7.45 FPS), 128.675 ms
non-wait work, 117.697 ms renderer-triangle time, 166.13 draw calls, 163.14
splits, 120.28 semitransparent splits, and exactly 120.28 fetch splits. This
scene mix is not structurally identical to the earlier 126-call matched state,
so it is exact runtime acceptance rather than a second before/after claim.

An initial exact analysis query requested the dirty profile's absent
126-draw/79-semitransparent group and divided by zero after reporting the group
distribution. The corrected query characterized the actual captured states
and did not alter either CSV. During final cleanup, the first best-effort
shutdown line contained an extra terminal `6` in the disposable UDID; stderr
was intentionally ignored, the immediately following line used the verified
UDID, and the final device list proved both named devices shut down. Neither
mistake launched, deleted, or changed a device or product.

Update installation preserved the retail/save identities:

```text
NTSC-U BIN inode 111450682, 605,698,800 bytes
SHA-256          f780bf2331476aabfc00772fa758b12dd95ebfbc907968132cbd3cdd4e2c07c0
memory card      inode 111309627, 6,016 bytes
SHA-256          6a01b0f5562ed7a279d8f8e51e3b1874ac39a6120f55db4fe3873288950619a3
```

Computer Use released keyboard capture and quit the Simulator GUI before the
device shutdown. Both named devices were confirmed off. Six explicitly named
task-owned temporary signed-app directories were then deleted; repository,
build products, retail data, saves and the preserved app evidence were not
removed.

The evidence-analysis goal reading was 245,597 seconds: 2 days, 20 hours,
13 minutes, 17 seconds cumulative. This is 3,253 seconds (54 minutes, 13
seconds) after the previous 242,344-second in-progress boundary. Goal time
includes builds, rejected attempts, live user inspection, pauses/resumes, and
documentation; it is not a build benchmark or person-hour estimate. The goal
remains active.

The exact-acceptance reading was 247,009 seconds: 2 days, 20 hours, 36 minutes,
49 seconds cumulative, adding 1,412 seconds (23 minutes, 32 seconds). This
includes publication, exact builds, Simulator boot/oracles/live replay,
cleanup and evidence analysis. It is not a build benchmark or person-hour
estimate. The goal remains active.
