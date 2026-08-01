# iOS framebuffer-fetch split batching

Date: 2026-08-01
Status: dirty-source implementation, desktop, byte-oracle and one-Simulator
profile accepted for publication; exact post-commit replay still required
Parent checkpoint: `2026-08-01-ios-framebuffer-fetch.md`

## Purpose and gate

The accepted coherent-framebuffer-fetch checkpoint made every PS1 textured
semitransparent logical split one ordered GLES draw instead of two. Its matched
Crash Cove state still submitted 126 renderer calls and averaged 172.324 ms,
well outside the 33.333-ms 30-FPS budget. Inspection showed that the PS1 parser
intentionally leaves consecutive semitransparent primitives as separate
logical splits even when their complete host render state is identical.

This checkpoint reduces only draw submission. It does not change primitive
parsing, retail state, physics, timing, input, logical split boundaries or the
render-trace oracle. It is acceptable only if:

1. coherent GLES preserves API primitive order within the joined vertex run;
2. only adjacent, contiguous, state-identical fetch splits join;
3. the portable two-pass renderer remains unchanged;
4. an overlapping-pixel oracle proves sequential PS1 blending, not merely a
   non-overlapping screenshot;
5. the logical renderer and actual iOS presentation hashes remain stable;
6. a one-Simulator retail run is visually coherent, diagnosable and faster in
   a structurally matched scene.

This is not Simulator acceptance and does not reopen the physical-device gate.

## Design and specification basis

`DrawAllSplits` now scans consecutive logical splits immediately before host
submission. A run joins only when all of the following match:

- coherent `GL_EXT_shader_framebuffer_fetch` is active;
- both splits are native PS1 textured-semitransparent draws;
- the first vertex range ends exactly where the next begins;
- blend mode, texture format, texture object and primitive/mask mode match;
- output-STP state and debug-label identity match;
- the complete copied `DRAWENV` and `DISPENV` bytes match.

The first split's start vertex and the aggregate vertex count are submitted as
one `glDrawArrays`. Vertex order is unchanged. Logical `gpu_splits`,
`gpu_split_vertices`, `gpu_semitrans_splits` and
`gpu_framebuffer_fetch_splits` counters remain logical rather than being
rewritten to host-call counts. The new
`gpu_framebuffer_fetch_merged_splits` counter records the host calls removed.
`renderer_draw_calls` continues to count actual renderer submissions.

This relies on the coherent extension, not its non-coherent sibling. Khronos
extension revision 8 states that `GL_EXT_shader_framebuffer_fetch` reflects
previous overlapping samples in API primitive order. That contract makes a
single contiguous draw equivalent to the prior sequence for this shader. The
source comment records the reason beside the batching predicate. Primary
specification:

<https://registry.khronos.org/OpenGL/extensions/EXT/EXT_shader_framebuffer_fetch.txt>

The fallback path cannot batch and still performs its established non-STP and
STP passes separately for every logical split.

## Oracle expansion

The media-free blend fixture now adds two identical, fully overlapping PS1
textured-semitransparent quads after the four blend-mode quads. The fallback
must issue 12 draws: six logical quads times two passes. Coherent fetch must
issue five draws: the four different-mode quads plus one joined draw for the
two same-mode overlapping quads.

The fixture checks both overlap pixel classes:

- non-STP remains opaque;
- the STP pixel contains the result of applying the average equation twice.

The entire fetch buffer must still match the forced two-pass buffer byte for
byte. This rejects state-blind joining, reordered overlap and a renderer that
merely reports fewer calls.

The new blend/fallback hash is `0c0d08324ae06c35`. The logical renderer hash
remains `851169f2644a1675`. Desktop staged presentation remains
`a7798c5a6ddee965`; iOS actual-surface presentation remains
`172d49a34571b64c`.

## Dirty build and automated evidence

All compilation occurred with both named Simulator devices shut down, the
Simulator GUI closed, process priority `nice 15`, and one build job.

The initial final-source macOS configure/build took 1.47 / 69.65 seconds. After
static review added only the coherent-order rationale comment, a final
one-job rebuild took 69.74 seconds and repeated the established 32 warnings.
The resulting executable is Mach-O ARM64, embeds
`0b9c7125cf40-dirty`, and hashes to:

```text
7b670e2f6fb455fbddbe670e09a5c50c6e9e545cf59fc878f8c4f435eede1a53
```

All 22 native CTests passed in 3.07 seconds. The desktop marker was:

```text
[CTR Renderer] pixel self-test passed: api=gl size=32x16 formats=4,8,16 clut=4,8 transparency=zero,stp blend=average,add,subtract,quarter bilinear=mixed-stp ordered-overlap=match fallback-draws=12 active-draws=12 mask=output-bit framebuffer=feedback vram=rgb5551 hash=851169f2644a1675 blend-hash=0c0d08324ae06c35 blend-oracle=two-pass oracle-hash=0c0d08324ae06c35 framebuffer-fetch=two-pass present=resolve+blit@64x32 present-hash=a7798c5a6ddee965
```

The initial iOS Simulator configure/build took 1.38 / 66.67 seconds, repeated
the same 32 warnings and produced a thin ARM64 executable. The unsigned hash
is:

```text
e4c29176d96b7654cfb132662e70aeadf2435475217688a4c3d81fdce4cd93e9
```

An isolated ad-hoc-signed copy in `/tmp/ctrpad-batch-test.Nigu44` passed
strict/deep verification, identifies `io.github.chrissotraidis.ctrpad`, and
has executable hash:

```text
5c236fb5c7050bd719bf4a2a76be7d8c4f1615fca7750cb3d5518302ba39c87a
```

On the actual 1032x1376 iOS Simulator surface, the enabled GLES marker reported
`ordered-overlap=match`, `fallback-draws=12`, `active-draws=5`, logical hash
`851169f2644a1675`, blend/fallback hash `0c0d08324ae06c35`,
`blend-oracle=match`, `framebuffer-fetch=enabled`, and presentation hash
`172d49a34571b64c`. The known immediate-test UIKit appearance/accessibility
diagnostics followed app-owned success output. Correct-ID termination ended
the resident self-test shell.

## One-Simulator retail profile and visual inspection

Only disposable device `CTRPad Import Negatives`
(`26F3DEE8-8840-446D-85FE-C882009C9C06`) booted. Protected device
`CTRPad Import Validation` remained shut down. The signed dirty app launched
as PID 7636 with:

```text
--perf --perf-dir perf-batch-dirty-0b9c
```

Computer Use enabled visible Simulator keyboard capture and used the published
player-one route `S K K K K I` to enter Time Trial, select Crash and reach
Crash Cove. K gas, D/K turns and later A/K/K motion moved the kart from the
grid through the fence/water edge and into the canyon/palm section. Direct
screenshots and the user's open Simulator view showed coherent logo/menu,
Crash model and portraits, track preview, ghost prompt, fly-in, grid, kart,
headlights, cliffs, canyon textures, sky/horizon, palm, animated water, HUD,
minimap and touch overlay. This is broad bounded evidence, not an every-track
or every-effect claim.

The app log continued to publish 120-frame cadence readings rather than
freezing. Late values were about 7.4-7.5 FPS before a scene change produced
6.84-6.94 FPS. Correct-ID termination after at least 588.165 seconds flushed:

```text
frame CSV       1,456,292 bytes / 4,333 lines including header
frame CSV SHA   eedb1181af6c1d3ae4ab3bb309215ee9887890ad1dcd88eef26178c10a9b590a
app log         24,534 bytes / 224 lines
app log SHA     f0d6035c9c23afa821d003d41102367a4fa4cb6e6d6a8b4c14d5b54cd7a3c955
targeted faults 0
```

The run requested frame performance but not `--gpu-trace`, so no
`gpu_trace.csv` was expected or created. Logical per-frame GPU counters remain
in `frame_times.csv`.

## Matched-scene result

The comparison selects frames with identical logical structure rather than
comparing unrelated track positions: 119 logical splits, 78 semitransparent
fetch splits, 5,060.93 submitted renderer vertices and 5,042.93 logical split
vertices. The exact unbatched checkpoint contributes 151 frames; this batching
run contributes 428.

| Metric | Exact fetch | Batched fetch | Change |
|---|---:|---:|---:|
| renderer calls | 122.00 | 66.00 | -45.90% |
| merged logical splits | n/a | 56.00 | 56 calls removed |
| total frame | 171.223 ms | 164.187 ms | -4.11% |
| non-wait work | 147.173 ms | 144.513 ms | -1.81% |
| renderer triangles | 136.333 ms | 133.964 ms | -1.74% |
| split submission | 126.900 ms | 124.429 ms | -1.95% |
| presentation | 7.928 ms | 7.767 ms | -2.03% |
| reciprocal throughput | 5.84 FPS | 6.09 FPS | +4.29% |

The final 120 captured frames averaged 133.167 ms / 7.51 reciprocal FPS. Two
longer batched scene states also demonstrate that the counter is live:

- 1,805 frames: 46 logical / 20 semitransparent / 15 merged splits, 34 calls,
  126.934 ms and 7.88 reciprocal FPS;
- 543 frames: 260 logical / 201 semitransparent / 163 merged splits, 100 calls,
  122.855 ms and 8.14 reciprocal FPS.

Those states have no structurally identical unbatched position and are runtime
characterization only. The matched result is the performance claim.

## Retail-data and save invariants

After update installation, self-test, keyboard motion and correct-ID shutdown:

```text
NTSC-U BIN inode 111450682, 605,698,800 bytes
SHA-256          f780bf2331476aabfc00772fa758b12dd95ebfbc907968132cbd3cdd4e2c07c0
memory card      inode 111309627, 6,016 bytes
SHA-256          6a01b0f5562ed7a279d8f8e51e3b1874ac39a6120f55db4fe3873288950619a3
```

Both named devices are shut down and the Simulator GUI is closed. App data,
retail input, saves, logs, normal build trees and repository files remain.

## Transparent command chronology and corrections

- An early live analysis tailed the active CSV while the app was writing, so
  one displayed line was naturally partial. Final analysis occurred only
  after correct-ID termination and used the flushed file.
- The first final artifact command also requested `gpu_trace.csv`. `wc`,
  `shasum` and `rg` reported it absent because this run intentionally used
  `--perf` without `--gpu-trace`. The corrected evidence scope uses the frame
  CSV and app log; no evidence file was deleted or lost.
- A size-based preservation search correctly found the primary retail/save
  files but also printed historical diagnostic copies. The final invariant
  command named the two authoritative primary paths explicitly.
- Computer Use's first close attempt was rejected because a fresh app-state
  read was required. The retry read state, released keyboard capture and quit
  Simulator normally.
- A subsequent best-effort `simctl shutdown` returned code 405 because quitting
  Simulator had already shut down the disposable device. The same command's
  status output proved both devices were already off and no Simulator GUI PID
  remained.
- The first staged-diff check rejected two Markdown hard-break spaces as
  trailing whitespace. They were removed, the paths were restaged, and the
  check was repeated before commit.

These are retained because the project history is intended to show how the
evidence was obtained, including harmless rejected routes.

## Decision and remaining boundary

The dirty implementation is accepted for an intentional commit because the
specification contract, byte oracle, portable fallback, 22-test suite, enabled
iOS oracle, matched profile, visual route, targeted log scan and retail/save
hashes agree. It still requires exact post-commit desktop/iOS rebuild and
one-Simulator replay before the checkpoint itself is final.

Even the matched 164.187-ms frame is about 4.93 times the 30-FPS budget. Broad
scene/effect churn, full touch race, rotation/Home/resume soak, physical-iPad
performance, human multi-touch ergonomics, user-owned signing/install and
final sideload-package acceptance remain open. No physical device should be
used on this evidence alone.

The dirty-evidence goal reading was 248,826 seconds: 2 days, 21 hours,
7 minutes, 6 seconds cumulative, 1,817 seconds (30 minutes, 17 seconds) after
the prior 247,009-second exact-fetch boundary. Goal time includes user viewing,
profiling, review, low-priority compilation and documentation; it is not a
build benchmark or person-hour estimate. The goal remains active.
