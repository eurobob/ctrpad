# Level-Visibility Cache Lifetime Correction

- Date: 2026-08-01
- Discovery target: `CTRPad Import Validation`, iOS 26.5 ARM64 Simulator
- Discovery build: published pre-correction branch state
- Correction status: accepted from exact ARM64 products embedding
  `4a4b148dd8d1`; implementation first published at `eeaf2c72c`

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
and checkpoint/global reset paths. The exact unity-built ordinary and
ASan/UBSan executables both pass the focused test and complete 22-test suite.
The exact UIKit/GLES app also completed a bounded title/menu/demo churn with
full scene geometry visible and no recurrence of the exhaustion signature.

## Resource boundary before resumed validation

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

At that checkpoint the root cause and source correction were complete, while
compilation, deterministic tests, a disposable one-Simulator runtime and
post-fix visual acceptance remained pending. Checkpoint `eeaf2c72c` was
therefore published only as a recoverability/review boundary, not presented as
acceptance.

A GitHub-hosted ARM64 compile was also considered to avoid local pressure.
Actions is enabled and `macos-15` is available as a standard ARM64 runner for
private repositories, but those jobs consume the account allowance and can
incur charges. The available token could not read the remaining allowance
without a new `user` scope. No workflow or hosted job was created; remote cost
was not assumed as validation authority.

A subsequent ownership audit safely stopped the stale goal-owned
`ctrpad-i686-debug` container after proving it ran only Xvfb, `sleep infinity`
and a diagnostic polling shell; its writable output was bind-mounted on the
host. Four unrelated production containers remained untouched. Free pages
still settled near 96 MB with about 11.66 GB swap used, so the unity build was
again not started. The stopped container ultimately reported exit 137 after
stop-timeout escalation of its idle process group; no verifier or host-mounted
evidence was lost. The final free-page reading was about 87 MB.

## Resumed exact build and deterministic acceptance

After the user explicitly resumed the goal and confirmed the Simulator was
unblocked, every compilation ran at nice priority 15, with one Ninja job and no
booted Simulator. No unrelated service was changed. All products embed exact
validated pre-documentation head `4a4b148dd8d1` and version
`0.1.0-beta.7.1`:

| Product | SHA-256 | Result |
| --- | --- | --- |
| macOS ARM64 desktop | `7fe474c5e1299445e97ba0bd3d64a38346d0097f6b6509e51180a9a210786c56` | ARM64 Mach-O; version exact; 22/22 CTest in 2.66 s |
| macOS ARM64 ASan/UBSan | `da62529752f29f231f1566afc69035b0a32ae78bfec84441366238b81ecf1d5f` | ARM64 Mach-O; focused test and 22/22 CTest in 7.19 s; no sanitizer finding |
| iOS Simulator ARM64 | `6c0189fafa44de71ce8aadf01c53a186bccd862ec8244f428aedbbde2468d78a` | ARM64 Mach-O; UIKit/GLES bundle compiled and linked |
| iPhoneOS ARM64 | `7dc667e7523e761806b488356824082515eef43604872c8d8f3badc1a93f63f5` | ARM64 Mach-O; UIKit/GLES bundle compiled and linked |

The focused ordinary CTest completed in 0.04 seconds and emitted exactly:

```text
[CTR AssetRelocation] self-test passed: first=0x07000014 duplicate=duplicate-patch target=target-out-of-range atomic=yes model=checked override=checked level=checked cache-recycle=targeted+range+all
```

The sanitizer form completed in 0.21 seconds with the same gate. Reconfiguring
before each build prevented an earlier commit's generated build ID from being
mistaken for the accepted implementation. The compilers repeated established
legacy conversion/deprecation warning classes; no build or test failed.

The two iOS bundles contain only the executable, `Info.plist`, GPL license,
installation guide, third-party notices and `_CodeSignature/CodeResources`;
no retail media is bundled. Their identifier is
`io.github.chrissotraidis.ctrpad`, minimum iOS is 15.0 and both iPhone/iPad
families are declared. The linker emitted an ad-hoc signature, but
`codesign --verify --deep --strict` truthfully fails with `code has no
resources but signature indicates they must be present`; `codesign -dv`
reports identifier `CTRPad`, no team identifier and no sealed resources. The
Simulator accepted the bundle. This does **not** close physical-device signing
or the final sideloadable-package gate.

## One-Simulator live visual acceptance

Both Simulators were shut down before compilation. For runtime acceptance only
disposable `CTRPad Import Negatives`
(`26F3DEE8-8840-446D-85FE-C882009C9C06`) was booted. Protected
`CTRPad Import Validation`
(`1D19A61F-20B7-46B0-AB52-B3A3406952E2`) remained shut down and was not
installed to, launched or reset. The exact unsigned Simulator bundle installed
successfully on the disposable device.

Computer Use inspected fresh accessibility state after every UI operation.
One rotation action was issued; when its immediate refresh failed, the action
was not repeated blindly. The next fresh state proved that it had completed.
Observed live frames included:

- the legal/title scene with Crash, trophy, textured floor/walls and complete
  touch overlay;
- the complete CTR title/menu with background and menu text;
- a **Race Today** introduction with sky, grass, foliage, sign and course
  geometry; and
- Crash and his kart in a forest scene with textured ground, trees, foliage,
  building, sky and the touch overlay.

This is direct evidence that the missing BSP/terrain/scenery class is rendering
again. It is deliberately not a claim that every game asset, animation or
location is pixel-perfect.

The cold-launch active log has 42 lines: 14 initialization lines followed by
28 periodic 120-frame FPS samples. It reports Apple Software Renderer, GLES
3.0, all four PSX shaders, VRAM pipelines, touch overlay and UIKit display loop.
The software-rendered samples ranged from 3.37 to 7.72 FPS. Most importantly,
the complete log contains zero `[CTR AssetRef]`, `visibility cache exhausted`,
`ERROR` or `FATAL` lines. The attached console likewise showed no recurrence
before bounded termination. A local-only window screenshot is 655 by 903
pixels and hashes to
`5637af8065ed6f29acbfe89045e9088cc27ff25aeb6df4575df8f13059626a87`;
it was reviewed visually but excluded from Git so the source repository remains
free of retail-derived pixels.

Installing and running the update preserved the canonical data exactly:

```text
BIN   inode 111450682, 605698800 bytes, mtime 1785358888
save  inode 111309627, 6016 bytes, mtime 1785525736,
      SHA-256 6a01b0f5562ed7a279d8f8e51e3b1874ac39a6120f55db4fe3873288950619a3
```

The active BIN had the same inode, size and modification time before install,
after container migration and after runtime. Its previously established
SHA-256 remains
`f780bf2331476aabfc00772fa758b12dd95ebfbc907968132cbd3cdd4e2c07c0`;
no redundant multi-gigabyte post-run traversal was used as a substitute for
the stronger retained-identity observation. The save retained its exact hash.

After evidence capture the app was terminated, its attached console exited,
and the disposable Simulator was shut down. Final `simctl` state showed both
named devices shut down, returning the Mac to zero booted Simulators.

## Acceptance boundary

The diagnosed eight-entry sidecar-lifetime regression is accepted as corrected
for the deterministic host boundary and the observed UIKit/GLES title/demo
runtime. The original 268-error signature did not recur, complete course/scene
geometry is visible, exact ordinary/sanitizer matrices pass, and canonical
media/save state is preserved. M7's broader accepted replay/pixel evidence is
restored; physical-iPad execution, strict/team signing, device performance,
real multi-touch/controller/keyboard play, completed-race observation and
remaining device lifecycle gates stay open. The overall goal remains active.
