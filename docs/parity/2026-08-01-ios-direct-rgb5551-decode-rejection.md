# iOS direct RGB5551 texture decode: rejected experiment

## Result

The direct RGB5551 fragment-decoder experiment is **rejected**. It preserved
the complete desktop and actual-surface iPad Simulator pixel oracle and rendered
the bounded retail route coherently, but it made the matched Crash Cove state
6.10% slower than the accepted lookup-texture renderer. The experimental source
was restored before publication. This report is retained because a
pixel-identical optimization can still be materially worse on Apple's Simulator
software renderer.

The accepted source at the start and end of this experiment is
`07bbc599bccc26680105e63dcb51f90268e31bfa`. The one-file prototype was never
committed. Ignored build products and Simulator evidence remain available on
the development host; no retail-derived file or screenshot is published.

## Why this route was tested

The accepted renderer already resolves packed PS1 VRAM at the logical display
size before scaling to the 1032x1376 host surface. Reducing the main render
target further would reduce fidelity, so it was rejected without implementation.

Every accepted 4-, 8-, and 16-bit PS1 texture fragment instead performed a
dependent sample of the 256x256 RG8-to-RGBA lookup texture. The nearest path
used one dependent lookup and the bilinear path used four. The already-accepted
packed-VRAM presentation shader proved that integer RGB5551 decoding was
available in GLES 3. The bounded hypothesis was therefore: replace only that
dependent lookup with exact high-precision integer decode, leaving primitive
order, vertex ABI, batching, blend/STP, mask, framebuffer feedback, resolution,
and presentation unchanged.

The candidate reconstructed the two packed bytes, decoded 5-bit R/G/B, and
preserved the historical lookup's exact output rather than expanding 5 bits to
the full 8-bit range:

```text
R/G/B = five_bit_channel << 3
STP   = bit_15 << 7
```

`highp int` was required because a default mediump signed integer cannot safely
represent every 16-bit packed value. Existing lookup allocation/binding code
was deliberately left in place during the first performance gate so the only
runtime shader difference was decode versus dependent lookup.

## Resource discipline and implementation chronology

Only `platform/native_renderer.c` changed. Before compilation, the disposable
and protected iPad Simulators were both shut down. Simulator's first Command-Q
attempt did not land because Computer Use no longer had an active window; two
subsequent state requests timed out after device shutdown. Direct inspection
showed the exact Simulator GUI PID, and a normal `SIGTERM` closed that GUI.
No device was erased or uninstalled.

All compiles used nice level 15 and one build job. The dirty macOS build took
71.99 seconds, repeated the established 32 warnings, and passed all 22 tests in
3.61 seconds. The independent renderer-pixel invocation passed in 1.24 seconds
with the accepted desktop marker:

```text
fallback-draws=12 active-draws=12
hash=851169f2644a1675
blend-hash=0c0d08324ae06c35
present-hash=a7798c5a6ddee965
```

The iOS Simulator compile completed and produced a fresh thin ARM64 app at
14:27:03 CDT. The final compiler console chunk was lost when the tool output
was compacted, so no exact iOS build duration or warning count is claimed for
this candidate. Direct artifact inspection, rather than a redundant rebuild,
proved the fresh Mach-O and bundle. Its executable hashes were:

```text
unsigned build executable  ddc83538e6462e2cd178f86f1535e18bafae0859fbc71573a4a2b3b87dc7773d
isolated ad-hoc executable  65d64ad3591a7ddf4a9d57f2264fe4a0827c6e1839b95e856c4a1be2bbce7c86
```

The isolated copy passed strict/deep code-signature verification. It is a
Simulator-only ad-hoc signature, not Apple device-signing evidence.

The first compound boot/install command returned after CoreSimulator startup
messages before reaching its later checks. A second compound attempt proved
the disposable device booted but again ended at install. Running installation
as its own command completed in 2.71 seconds. This was an update install: no
uninstall, erase, or container deletion occurred. The protected validation
device remained shut down, and the data container remapped to:

```text
.../Data/Application/402596C0-524F-4C42-9302-A93E4E8236B5
```

The imported retail image and save retained their accepted identities:

```text
retail inode 111450682, 605698800 bytes
SHA-256 f780bf2331476aabfc00772fa758b12dd95ebfbc907968132cbd3cdd4e2c07c0

save inode 111309627, 6016 bytes
SHA-256 6a01b0f5562ed7a279d8f8e51e3b1874ac39a6120f55db4fe3873288950619a3
```

## Actual iPad Simulator pixel and visual evidence

The real 1032x1376 GLES surface used coherent framebuffer fetch, matched the
forced fallback, and passed:

```text
fallback-draws=12 active-draws=5
hash=851169f2644a1675
blend-hash=0c0d08324ae06c35
oracle-hash=0c0d08324ae06c35
present-hash=172d49a34571b64c
```

Only the disposable `CTRPad Import Negatives` device booted. Computer Use
inspected the retail copyright screen, main menu, Crash character picker,
Crash Cove list/preview/map, No Ghost prompt, animated course fly-in, starting
lights, and live grid. Keyboard Down/C and accessible touch Cross both reached
the retail input consumer. Crash, kart, portraits, track, banner, waterfall,
sky, HUD, timer, lap counter, minimap, transparency, and touch overlay were
present together.

One fly-in capture showed vertically clipped/inverted letterbox labels. A
settled grid capture 12 seconds later showed the complete normal HUD and scene;
the earlier image was the retail transition animation, not an asset failure.
This bounded route found no recurrence of the missing-assets report. It does
not prove every track, character, effect, visibility-cache recycle, rotation,
or background/foreground sequence.

The first normal launch mistakenly used
`--profile-renderer=perf-direct-decode-dirty-07bbc`. The profiler accepts
`--perf-dir PATH`, so that launch produced application/FPS logs but no frame
CSV. This was detected from source and log evidence rather than silently
treated as a profile. The app was restarted once on the same booted device
with the exact writable `--perf-dir` path. The correct log explicitly recorded
both output files before the route was repeated.

The rotated initial-session log is 12,141 bytes / 108 lines at SHA-256
`a4914173e20679d0f9099eb3cba2efff616c216f33412d889d3f6faab3b913be`.
The profiled-session log is 6,870 bytes / 53 lines at SHA-256
`68eec948982a72cce952de2107abdfddec8c6568f31cb10f6d369f3b87dd789f`.
Targeted AssetRef, visibility-cache, ERROR/FATAL, unbalanced-render, assert,
signal, exception, and crash scans found zero lines in both.

## Matched performance result

Correct bundle-ID termination stopped the profiled process after the grid had
accumulated more than 500 complete Crash Cove rows. The final CSV is 644,630
bytes / 1,932 newline-terminated lines at SHA-256
`7b574948d40bd57b9a529add3253d15bebd77f11df44057427ab53d2d17c240b`.
It contains 1,931 complete 59-field records and one 54-field partial final row;
the partial row is excluded. The GPU CSV is the expected header-only 28-byte
file at SHA-256
`af0f3466758a717080c8ac7d955fb6c432274898fb065e304a8d36cf3c9d91f6`.
Simulator GLES exposes no optional GPU timer-query samples on this route.

The comparison selects only `level_id=3` rows with identical renderer
structure: 66 draw calls, 119 logical splits, 78 framebuffer-fetch splits, and
56 merged splits.

| Metric | Accepted `perf-exact-125966b21` | Direct decoder | Change |
|---|---:|---:|---:|
| Complete matched frames | 200 | 248 | — |
| Total frame | 187.002 ms | 198.412 ms | +11.410 ms / +6.10% |
| Reciprocal throughput | 5.348 FPS | 5.040 FPS | -5.76% |
| Non-wait work | 156.406 ms | 164.397 ms | +7.991 ms / +5.11% |
| Renderer triangles | 144.237 ms | 153.395 ms | +9.158 ms / +6.35% |
| Present VRAM | 8.990 ms | 8.455 ms | -0.535 ms |
| Framebuffer store | 4.583 ms | 4.068 ms | -0.515 ms |
| Swap window | 3.990 ms | 3.816 ms | -0.174 ms |

The candidate slightly reduced final presentation-related buckets but added
far more cost to triangle rendering. That is consistent with integer
bit-manipulation being slower than the tiny cached lookup texture in Apple's
software rasterizer. Host timing varies, but the same scene and exact draw/split
population make this a substantially stronger result than unmatched FPS logs.

## Decision and restored boundary

The candidate is rejected because exact pixels are necessary but not
sufficient; the matched runtime cost regressed. `apply_patch` restored the
lookup uniform, lookup helper, four bilinear lookup calls, and nearest lookup.
`git diff --check` then passed and the repository returned byte-for-byte to
published accepted source at `07bbc599bccc`.

No second compile is needed to prove the restoration itself: the restored file
has no Git diff from the exact revision whose macOS/iOS builds, 22 tests,
desktop/iOS pixel oracles, retail replay, and profile are already recorded in
`2026-08-01-ios-unified-fetch-state-rejection.md`. The ignored build tree and
installed Simulator app still contain the rejected candidate and are not a
release artifact. A clean source rebuild is required after the documentation
checkpoint before further renderer work or device packaging.

The rejection reading was 258,100 seconds: 2 days, 23 hours, 41 minutes,
40 seconds cumulative, 2,216 seconds (36 minutes, 56 seconds) after the prior
exact-replay boundary. Goal time includes user-visible Simulator inspection,
the corrected profile, analysis, restoration, and documentation preparation;
it is not a build benchmark or a person-hour estimate. The overall goal
remains active.
