# Level-Visibility Cache Lifetime Correction

- Date: 2026-08-01
- Discovery target: `CTRPad Import Validation`, iOS 26.5 ARM64 Simulator
- Discovery build: published pre-correction branch state
- Correction status: implementation complete; build/runtime acceptance pending

## User-visible defect

The user pointed out that many game assets were not rendering in the iPad
Simulator. A read-only inspection first caught the expected animated
checkerboard **LOADING** transition, which was not counted as a defect. The
next live frame rendered the kart, HUD/start lights and some instances but
showed the characteristic incomplete scene. That distinction matters: the
loading frame alone would have been weak evidence, while the running scene and
log establish a real defect.

The protected app's 2,623-line production log contains exactly 268
`[CTR AssetRef]` errors. All 268 are the same failure class:

```text
134  level visibility cache exhausted: context=LOAD_TenStages ... capacity=8
134  level visibility cache exhausted: context=MainInit ... capacity=8
```

There are no other asset-reference, missing-file, model-reference or texture-
reference errors in that log. This does not prove that every remaining visual
is retail-correct, but it isolates the observed large-scale disappearance to
one deterministic failure rather than a general claim that “GLES is rough.”

## Root cause

The LP64 port cannot use serialized 32-bit `VisMem`/BSP pointer storage
directly. `platform/native_asset_ref.c` therefore builds a host-sized sidecar
per `Level *` in `s_levelRuntimeVisMem`. The fixed cache has eight entries.

`Level_GetVisMem` returns null once all eight entries have non-null level keys.
Ordinary level transitions call `MEMPACK_PopToState`, Adventure/boss loaders
call `MEMPACK_ClearLowMem`, and bookmark rollback can call `MEMPACK_PopState`.
Those operations invalidate the LEV allocations but, before this correction,
did not release the matching sidecars. `LOAD_Callback_LEV` invalidated only an
entry whose key exactly matched the new destination. Different level sizes
produce different destination addresses, so demo/level churn eventually filled
all eight slots.

The render consequence is direct. `MainInit_VisMem` stores the null result in
`gGT->visMem1`, and `RenderAllLevelGeometry` returns immediately when
`visMem1 == NULL`. Instances such as karts or HUD elements can continue to
render while BSP terrain, water/scenery visibility and level geometry vanish.
That matches the observed “many assets missing” appearance.

## Correction

The first draft added invalidation at `LOAD_TenStages` and `LOAD_Hub_ReadFile`.
A complete `MEMPACK_ClearLowMem` call-site audit found additional boss/level
paths, so that call-site-only approach was replaced before build acceptance.

The final design adds `LevelRuntime_InvalidateRange(start, end)`. It compares
host addresses as `uintptr_t` values and releases only cache entries whose
`Level *` lies in the allocation range being discarded. Native memory-pack
operations invoke it immediately before changing allocator state:

- `MEMPACK_ClearLowMem`: `[pack.start, firstFreeByte)`;
- `MEMPACK_PopState`: `[previous bookmark, firstFreeByte)`; and
- `MEMPACK_PopToState`: `[requested bookmark, firstFreeByte)`.

This central boundary covers ordinary levels, Adventure hub replacement,
boss/connected-level loading and future callers. It preserves a level in a
different live pack. `LOAD_Hub_ReadFile` additionally clears `visMem2` after
the inactive pack reset so no freed host pointer remains during replacement.
The existing exact-destination invalidation in `LOAD_Callback_LEV` remains as
a defensive same-address reload boundary. Null targeted invalidation is now a
no-op.

## Regression coverage

The media-free asset-relocation self-test now constructs nine distinct level
fixtures in one registered guest-reference region. It fills all eight cache
slots, verifies targeted recycling into the ninth fixture, fills the cache
again, verifies that range recycling preserves the five out-of-range entries
while admitting the ninth fixture, then verifies full-cache recycling. Its
success marker is:

```text
cache-recycle=targeted+range+all
```

This covers the host-side primitives used by the callback, memory-pack reset
and checkpoint/global reset paths. A build is still required to validate the
unity-game call sites, and a long-running post-correction level/demo churn is
required to prove the live log remains free of exhaustion.

## Resource boundary and current status

At the user's request, only one Simulator may be open. The disposable clone
was shut down, leaving only the protected validation device booted. It is not
an acceptable install target for a dirty correction build. A low-priority,
single-job desktop compile was attempted, but it was stopped when the Mac had
roughly 64 MB of free VM pages and the user reported system slowness. No build
or compiler process remains.

A follow-up attempt to compile only the changed asset-reference object was
rejected before compilation: the configured project unity-builds these sources
through `main.c.o`, so Ninja has no standalone
`platform/native_asset_ref.c.o` target. That attempt added no system load and
does not count as validation.

The root cause and source correction are complete. Compilation, deterministic
tests, a disposable one-Simulator runtime, absence of the 268-error signature
after more than eight distinct level allocations, exact-build publication and
post-fix visual acceptance remain pending. Until those pass, the renderer/
asset defect is diagnosed and corrected in source but not accepted.
