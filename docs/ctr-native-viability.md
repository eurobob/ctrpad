# CTR Native: iPadOS Port Viability

Research-only assessment. No code was written, patched, or built. All four upstream repositories were cloned read-only into `ref/` and read locally.

**Assessment date:** 2026-07-29
**ctr-native HEAD assessed:** `2df55dc5a` (2026-07-26), version string `0.1.0-beta.7.1`
**Method:** local source reading with grep-based site counts, `git` history analysis, GitHub API for issues/PRs/releases/forks, and upstream documentation fetches. File paths below are relative to the ctr-native repository root unless stated otherwise.

---

## 1. Verdict

**Wait.** The deciding factor is that iPadOS has no 32-bit runtime and ctr-native cannot produce a 64-bit build at all today, by explicit and deliberate design: `CMakeLists.txt:7-13` raises a `FATAL_ERROR` unless `CMAKE_SIZEOF_VOID_P EQUAL 4`, and two compile-time locks reinforce it at `game/233/D233.c:7` and `include/ovr_233.h:697`. This is not a flag to flip. Behind that gate sits genuine structural work, most importantly `LOAD_RunPtrMap` at `game/LOAD/LOAD_Assets.c:8-20`, which relocates loaded asset files in place by writing a truncated host pointer into 32-bit slots inside the file image (`*(int *)&origin[offset] = *(int *)&origin[offset] + (int)origin;`), plus roughly 75 layout-pinned structs that carry native pointers and roughly 170 pointer-narrowing code sites. My estimate for reaching a clean 64-bit build is 12 to 18 person-weeks, with a real risk of 20 to 30 if parity requirements force a guest-arena redesign; confidence low to medium.

Everything else is in better shape than the brief implies. The renderer, which the brief treats as a second major risk, is not one: all OpenGL lives in essentially one file, and a GLES 3.0 retarget is a small, well-bounded change. The MIPS and GTE integer emulation is written portably. The platform layer is clean, SDL3 3.4.10 is vendored as full source with the iOS backends present, and every codec (SPU-ADPCM, XA, MDEC video) is in-process C. If the 64-bit work were done, the remaining iPad work would be ordinary.

The reason this is "wait" rather than "go" is scope, not feasibility. The requester's existing program takes source ports that are already 64-bit and already ARM-capable and adds a touch layer and an iOS shell. Here that precondition does not hold, so the project is "finish somebody else's 64-bit port, then do the iPad port." The reason it is not "no" is that the blocker is bounded, fully identified, sits in the maintainer's stated roadmap, and about a quarter to a third of it is already done.

One specific trap to flag: the existence of a working Android fork does not de-risk iOS. Simon358's fork builds `armeabi-v7a`, which is 32-bit ARM. It proves the code runs on ARM, but iOS has not accepted 32-bit binaries since iOS 11, so that precedent does not transfer.

---

## 2. Corrections to the brief

Verified against the repositories and the GitHub API.

| Brief stated | Actual |
|---|---|
| Latest tag is `beta-6.1` dated 2026-06-26 | Latest is **`beta-7.1`, 2026-07-07**. `beta-7` was 2026-07-03. Nine releases exist, `beta-1` through `beta-7.1`. |
| CTR-ModSDK has 6,410 commits | **5,480** at ModSDK HEAD `e3f19686`. The 6,000-plus figure matches ctr-native, which has **6,629** and carries the shared decompilation history. |
| Assets required are `BIGFILE.BIG`, `SOUNDS/KART.HWL`, `TEST.STR` and XA streams | Since Beta 7 the **primary** path is a single raw disc image `assets/ctr-u.bin`. The extracted file list is now an optional developer override (`README.md:136-166`). Validation accepts either source per file (`platform/native_assets.c:777-785`). |
| Unity build via `game_includes.h` / `game_unity.h` | `game_includes.h` **does not exist** in ctr-native; it is the pre-refactor name still present in the NyperYuhgard copy. Current path is `game/game_unity.h`. Likewise `platform.h` is at `include/platform.h`, not the repo root. |
| 943 game source files | `README.md:24` still claims 943, but `game/` actually contains **255 `.c` files** (260 files total), of which 249 are unity-included via `game/game_unity.h`. The README is stale. |
| OpenGL 3.3 required | Release notes do say this, but the code requests 3.3 core and **falls back through 3.2, 3.1, and 3.0 core** (`platform/native_renderer.c:190-214`). Shaders are `#version 140`, which is GL 3.1 (`platform/native_renderer.c:923,930`). The true floor is lower than advertised, which helps the GLES case. |
| Some native paths pack host pointers into 24-bit GPU primitive links | **No longer true, and this is the brief's most important stale fact.** The 24-bit field is a genuine token, resolved through a `uintptr_t`-keyed range table (`platform/native_gpu_links.c:11-17,155-186`). It is already 64-bit correct. The 32-bit blocker is real but lives elsewhere: asset relocation, scratchpad overlays, and pinned struct layouts. |
| Builds are Windows x86 and Linux i386 only | Confirmed. All nine releases ship only `windows-x86` and `linux-x86` artifacts. |
| License GPL-3.0 added at Beta 6 | Confirmed. LICENSE added 2026-06-25 in commit `12e2888c0`; `beta-6` tagged 2026-06-26. |
| Release manager appears to be `aalhendi` | Confirmed. All nine releases authored by `aalhendi`. |
| `master` last touched 2026-07-26 | Confirmed exactly. |

Two additions the brief did not anticipate. First, an active Android fork exists (`Simon358/ctr-native-android`) with two upstream PRs, one closed as duplicate and one open and unreviewed for 19 days. Second, the maintainer has publicly declined official macOS builds while permitting source builds, and the macOS source build is currently broken on Apple Silicon.

---

## 3. Blocker inventory

Ordered by severity. "Upstream working on it" reflects written evidence only.

### B1. The 64-bit build gate and the pointer contracts behind it

**What it is.** `CMakeLists.txt:7-13` fails configuration on any 64-bit target. Behind it, host pointers are deliberately stored in 32-bit fields in three families of data structure: in-place relocated asset files (`game/LOAD/LOAD_Assets.c:8-20`, consumers such as `struct ModelHeader.ptrCommandList` at `include/namespace_Instance.h:361` read back as host pointers at `game/RenderBucket/RenderBucket_QueueExecute.c:2296,4356,4643`), scratchpad overlays whose byte offsets are part of the algorithm (`include/namespace_DrawLevel.h:239-267`, size-pinned to `0x1b4` at `:301`; 102 `Ptr32` references across `game/` and `include/`), and integer-typed pointer fields in the resident executable map (`include/regionsEXE.h:3524,3527,4128`).

**Why it blocks.** iOS is arm64-only. There is no 32-bit iOS target to fall back to. Nothing iPad-shaped can compile until this is resolved.

**Effort.** 12 to 18 person-weeks for a competent C systems programmer working with the project's parity discipline. Risk case 20 to 30 weeks if parity forces a full guest-arena-plus-offset-addressing architecture. **Confidence: low to medium.** The uncertainty is not in the site count, which is countable, but in how much of the retail-behavior parity suite has to be re-validated after each structural change.

**Upstream.** Partially. The README roadmap commits to "Keep reducing 32-bit host-pointer assumptions in PSX-shaped data" and frames 32-bit as temporary ("while remaining PSX address-shaped data and host-pointer contracts are audited", `README.md:188`). Beta 6 shipped three real enablers. But no release note has ever mentioned a 64-bit target, no 64-bit branch exists, and `platform/native_memory.c:19-20` carries an open TODO listing the unfinished audit areas.

### B2. Renderer requires desktop OpenGL

**What it is.** The renderer is a PsyCross-derived PS1 GPU reimplementation issuing raw desktop GL calls through a vendored glad loader (`platform/native_renderer.c:1-5,190-214,221`). iOS provides only GLES and Metal.

**Why it blocks.** No desktop GL context can be created on iOS.

**Effort.** Small. Roughly 10 focused edits plus one new repack function across 2 files. The only load-bearing change is the `GL_RG` readback at `platform/native_renderer.c:1821`, which must become an RGBA read plus a repack loop. Everything else is the context request (`:190-208`), two shader version strings (`:923,930`), and ifdef-ing three debug-only features: `glPolygonMode` (`:2240`), a BGRA screenshot (`platform/native_platform.c:159`), and timer queries (six sites, all `CTR_INTERNAL`). **Estimate: 1 to 2 person-weeks including on-device validation. Confidence: medium-high**, because the audit found zero uses of the features that usually break GLES ports (no `GL_TEXTURE_RECTANGLE`, no `glLogicOp`, no geometry shaders, no PBOs, no MRT, no dual-source blending, no `glBlitFramebuffer`), the shaders already carry ES compatibility shims, and depth testing is never enabled.

**Upstream.** Yes, in a fork. Simon358's Android work already added a GLES path, offered upstream as PR #41, unreviewed for 19 days.

### B3. No touch input exists anywhere

**What it is.** Input is SDL3 keyboard and gamepad only (`platform/native_input.c:258-306`). There is no touch code in the tree.

**Why it blocks.** It is the actual product on an iPad.

**Effort.** This is design work, not porting work, and it is the requester's core competency. The engineering hook is unusually clean: `Platform_InputInstallPadSnapshots` (`platform/native_input.c:940-962`) already exists for replay injection and bypasses SDL entirely when active, and the pad state is a plain 12-byte struct (`include/platform/native_input.h:8-16`). **Estimate: not given, since the brief asked only for difficulty judgment. Confidence: n/a.**

**Upstream.** No.

### B4. Filesystem assumptions incompatible with the iOS sandbox

**What it is.** `main.c:168` does `chdir` to a discovered base directory, and saves, logs, perf output, and savestates are then written to relative paths (`platform/native_memcard.c:22`, `platform/native_log.c:82`, `platform/native_perf.c:25`, `platform/native_savestate.c:22-24`). `SDL_GetPrefPath` is never called anywhere in the project.

**Why it blocks.** On iOS the bundle is read-only. Writes must go to the app's Documents or Library directory.

**Effort.** 1 to 2 person-weeks including the Files-app import flow for the user's disc image. **Confidence: medium.** The path plumbing itself is absolute-path-clean; there is simply no external knob, and no CLI or environment override for the asset directory exists (`platform/native_assets.c:19,41-42`).

**Upstream.** No.

### B5. Build flags and entry point

**What it is.** `-msse` is applied to all non-MSVC compilers (`CMakeLists.txt:114`), which Apple Clang rejects outright when targeting arm64. This is exactly the failure reported by a user in issue #20. `main.c:2,138` uses `SDL_MAIN_HANDLED` with a plain `main()`, but iOS requires SDL to own the entry point for the UIKit lifecycle. The CTest smoke test executes the built binary (`CMakeLists.txt:141`), which cannot work when cross-compiling.

**Why it blocks.** Compilation and launch, respectively.

**Effort.** Under one week total. No actual SSE intrinsics exist anywhere in the source, so `-msse` is purely droppable. **Confidence: high.**

**Upstream.** No, though it is a two-line fix that upstream would very likely accept.

### B6. Undefined-behavior flag hygiene

**What it is.** The build sets neither `-fno-strict-aliasing` nor `-fwrapv` (verified absent from `CMakeLists.txt`, `build.sh`, `CMakePresets.json`). Meanwhile the code performs genuine type-punning, for example `int`-typed stores over `s16` matrix fields at `game/PushBuffer.c:360-364,386-390` and `*(s32 *)&sdata->kartSpawnOrderArray[0]` at `game/BOTS.c:184-185`, and relies on signed overflow in the `FP_MULT` family at `include/ctr_math.h:135-149`. GCC gets only the warning `-Wstrict-aliasing=2` (`CMakeLists.txt:125`).

**Why it blocks.** It does not block, but it is a latent miscompilation risk when moving to a new compiler and a new optimizer. A `CTR_MAY_ALIAS` macro already exists at `include/ctr_compiler.h:27` and is not applied at these sites.

**Effort.** Two flags, then re-verification. Under one week. **Confidence: high** that adding the flags is correct and cheap; **low** confidence that no latent miscompilation is already present without running the parity suite.

**Upstream.** No.

### B7. GPLv3 versus the App Store

**What it is.** ctr-native is GPL-3.0 as of 2026-06-25 (`LICENSE`, commit `12e2888c0`).

**Why it blocks.** It forecloses App Store distribution. It does not block sideloaded distribution. Detail in section 4.9.

**Effort.** n/a. It is a distribution-channel constraint, not work.

**Upstream.** n/a.

### B8. Frame pacing hostile to iOS

**What it is.** VBlank pacing uses `SDL_DelayPrecise` plus a spin-wait (`platform/native_platform.c:573-624,608`), and quit handling calls `exit(0)` directly (`platform/native_platform.c:437,447`).

**Why it blocks.** It does not block, but a spin-wait is battery-hostile on a tablet, and `exit(0)` conflicts with iOS app lifecycle expectations (suspend and resume rather than terminate).

**Effort.** Under one week. **Confidence: medium-high.**

**Upstream.** No.

---

## 4. Findings by section

### 4.1 Project health

**Velocity.** 1,162 commits in the last 6 months and 1,200 in the last 12 (`git rev-list --count --since`). Those numbers are nearly identical because the project was effectively dormant before May 2026. The monthly histogram:

| Month | Commits |
|---|---|
| 2025-08 | 2 |
| 2025-09 | 34 |
| 2025-10 | 1 |
| 2025-12 | 1 |
| 2026-02 | 4 |
| 2026-03 | 2 |
| 2026-04 | 3 |
| 2026-05 | 671 |
| 2026-06 | 390 |
| 2026-07 | 92 |

The native port began in earnest in May 2026. The trend since is downward: 671, then 390, then 92. July is not complete, but at 92 commits through the 26th the month will land near 100, roughly a seventh of May. **The project is cooling, not accelerating.** That said, a burn-down after an initial push is normal and 92 commits a month is still active development by any ordinary standard.

**Contributor concentration.** The all-time list is misleading because ctr-native inherited the decompilation history. Niko shows 4,269 commits all-time but only 3 in the last 6 months. Restricted to the last 6 months:

| Contributor | Commits (6mo) |
|---|---|
| Abdulrazaq Alhendi (`aalhendi`) | 1,139 |
| penta3 | 7 |
| TheUbMunster | 4 |
| Niko | 3 |
| Agung Firdaus | 2 |
| Rinnegatamante, Superstarxalien, icebound777, mateusfavarin | 1 each |

**Bus factor is 1.** `aalhendi` wrote 98% of the last six months of work, authored all nine releases, closes nearly every issue, and reviews the PRs. This reads unambiguously as one person's project with a small supporting cast doing triage (`kkv0n`) and occasional contributions. That is the single largest non-technical risk in this assessment: the 64-bit roadmap item exists because one person put it there, and it will land on his schedule or not at all.

**Release cadence.** Nine releases, all pre-release, all authored by `aalhendi`.

| Tag | Date | Gap |
|---|---|---|
| beta-1 | 2026-06-08 | |
| beta-2 | 2026-06-09 | 1 day |
| beta-3 | 2026-06-11 | 2 days |
| beta-4 | 2026-06-12 | 1 day |
| beta-5 | 2026-06-19 | 7 days |
| beta-6 | 2026-06-26 | 7 days |
| beta-6.1 | 2026-06-26 | same day hotfix |
| beta-7 | 2026-07-03 | 7 days |
| beta-7.1 | 2026-07-07 | 4 days hotfix |

A steady weekly cadence from beta-4 onward, with hotfixes following two of the last three releases. Three weeks have now passed since beta-7.1 with no beta-8, which is consistent with the July slowdown.

**Issues and PRs.** 30 issues (13 open, 17 closed) and 19 PRs (3 open, 8 merged, 8 closed unmerged). Median time to close is fast: 15.7 hours for issues, 22.6 hours for merged PRs. But responsiveness has degraded since mid-July, with #43, #45, #38, and #47 unanswered as of this writing, and #47 is a detailed crash report with an offered fix that has sat 3 days. The pattern matches the commit histogram: the maintainer's attention is thinning.

### 4.2 The 32-bit blocker, quantified

**What `docs/MEMORY_MODEL.md` says.** The document is 164 lines and is accurate where it can be checked. It lays out the PS1 memory map, documents that the main executable's data ranges are mapped as C structs in `include/regionsEXE.h`, explains the MEMPACK dual-ended allocator, and describes the native split where the platform layer owns a host backing arena and the game allocator consumes it. On the central question it states, at lines 148-163, that PS1 primitive and OT links store a 24-bit address plus an 8-bit length, and that "Under `CTR_NATIVE`, the 24-bit field is a bridge token, not a truncated host pointer." It closes with a directive to keep game-visible packets retail-shaped and not reintroduce PsyCross's widened packets. It does not state a 64-bit roadmap; the roadmap language lives in `README.md:190-193`.

**How much of the 64-bit work actually completed.** The Beta 6 claims check out.

Verified complete and 64-bit correct:

- **The 24-bit token bridge is real.** `struct NativeGpuLinkRange` at `platform/native_gpu_links.c:11-17` stores `uintptr_t hostStart` and `hostEnd` alongside `uint32_t tokenStart` and `tokenEnd`. Tokens are allocated downward from `0x00f00000` (`:9,21`) within the 24-bit namespace, with `0x00ffffff` as terminator (`include/platform/native_gpu_links.h:7`). Conversion is a range lookup plus offset in both directions (`:155-167` and `:169-186`), and an unregistered pointer aborts the process rather than truncating (`:165-166`). No host pointer is ever narrowed. This compiles and works unchanged on 64-bit. Wired into the retail-shaped macros at `include/psx/libgpu.h:157-166,197` and `include/gpu.h:9-16`, with arenas re-registered every frame at `game/MAIN/MainFrame.c:4-27`.
- **The fixed scratchpad address is gone.** Native uses a process-local static buffer (`platform/native_memory.c:43-52`) and translates retail `0x1f800xxx` constants by constant subtraction (`include/ctr_scratchpad.h:13-22`). The `(u32)` cast there is applied to a retail constant, never to a host pointer.
- **The fixed image base is gone.** No `/BASE`, `/FIXED`, or `-Ttext` anywhere in the build files. Checkpoint restore is relocation-aware across image bases (`platform/native_checkpoint.c:474-550`).
- **Cast hygiene is modern.** `uintptr_t` is used correctly at 205 sites (142 in `game/`, 51 in `platform/`, 12 in `include/`). The problem is never a sloppy cast; it is the width of the destination.

**What remains, classified.**

*Structural (requires redesigning a data structure that mirrors PSX layout):*

1. **In-place asset relocation.** `LOAD_RunPtrMap` at `game/LOAD/LOAD_Assets.c:8-20` walks a patch table and does `*(int *)&origin[offset] = *(int *)&origin[offset] + (int)origin;`. The host base pointer is truncated to `int` and written into a 32-bit slot inside the loaded file image. Consumers then overlay C structs directly on those bytes: `struct Level` carries roughly 21 to 25 native-pointer fields (`include/namespace_Level.h`), and `struct ModelHeader.ptrCommandList` is a `u32` at file offset `0x20` (`include/namespace_Instance.h:361`, offset asserted at `game/RenderBucket/RenderBucket_QueueExecute.c:48`) that is dereferenced as a host pointer at `:2296,4356,4643,4740,4842,4953` and at `game/231/RB_Banner.c:87-93`. On 64-bit both the truncation and the struct layout break. The same pattern appears for string tables at `game/LOAD/LOAD_Assets.c:243` and `game/233/CS_Credits.c:256`. This is the single largest item.
2. **Scratchpad overlay slots.** `struct DrawLevelOvr1PStableScratch` (`include/namespace_DrawLevel.h:239-267`) stores host pointers in `u32` fields named `...Ptr32`, is size-pinned to `0x1b4` at `:301`, and is overlaid at scratchpad offset 0 (`game/226/226_00_DrawLevelOvr1P.c:215-218`). Parallel cases exist in RenderBucket (`game/RenderBucket/RenderBucket_QueueExecute.c:124-127,166-169`) and Torch (`game/Torch.c:102,131,525`). 102 `Ptr32` references across the tree. These cannot simply be widened, because `docs/MEMORY_MODEL.md:144-146` states that retail scratchpad offsets are themselves part of the algorithm.
3. **Layout-pinned structs carrying pointers.** 243 structs have `sizeof` static asserts and 1,149 `offsetof` asserts exist in total; roughly 75 of the pinned structs contain native pointer fields. `struct Mempack` pins `start` at `0x4` and `lastFreeByte` at `0x8` with a total size of `0x60` while holding `void *` members (`include/namespace_Mempack.h:27-62`). Each of the 75 needs an individual decision: keep 32-bit because it overlays a file or the scratchpad, convert to a handle or guest offset, or relax the assert because it is runtime-only.

*Mechanical (cast and typedef cleanup):*

4. Approximately 85 sites of the `(u32)(uintptr_t)ptr` narrowing idiom across 7 files.
5. Approximately 34 pointer-to-integer round trips, for example `game/MAIN/MainFrame.c:68,87-96`, `game/PushBuffer.c:214,274`, `game/MEMPACK.c:62,135`, `game/PROC.c:225-298`.
6. Approximately 34 integer-typed fields that hold pointers, for example `int ptrPushBufferUI` at `include/regionsEXE.h:3524` (written at `game/UI/UI_Instance.c:384`, read back at `game/MAIN/MainFrame.c:101-103`) and `int howlChainParams[4]` at `include/regionsEXE.h:4128`.
7. Approximately 20 `u32` pointer comparisons and parameters, including `u32 LevRenderList` function parameters at `include/functions.h:1302-1303`.
8. Two compile-time locks at `game/233/D233.c:7` and `include/ovr_233.h:697`.

Total roughly 170 mechanical code sites, 75 struct triage decisions, and 1 subsystem redesign.

*Already bridged or benign (verified not to be blockers):*

Retail `0x80xxxxxx` literals used as never-dereferenced labels, for example the RenderLists corner jump table (`game/RenderLevel/RenderLists.c:352-361`) and `RB_RETAIL_INST_*` constants switched on as `(u32)(uintptr_t)` (`game/RenderBucket/RenderBucket_QueueExecute.c:310-311,691-705`). These survive 64-bit as zero-extended constants. The callback-address heuristic that once masked against `0x80000000` is already replaced under `CTR_NATIVE` with identity comparison (`game/LOAD/LOAD_File.c:146-152`). Of roughly 523 raw `0x80xxxxxx` matches in `.c` files, the large majority are `-0x80000000` overflow sentinels. All 18 `& 0xffffff` masks outside the bridge are color or CD-sector masks, not address masks.

**Estimate.** Roughly 25 to 30 percent of the 64-bit work is complete. Remaining effort 12 to 18 person-weeks: 1 to 2 for the mechanical pass, 3 to 6 for asset relocation, 2 to 4 for scratchpad slots, 2 to 3 for layout triage, and 2 to 4 for tooling and parity validation. **This is an estimate with low to medium confidence.** The dominant risk is that parity requirements push the design toward a full guest 32-bit arena with offset addressing, which would be more like 20 to 30 person-weeks. Note also that the dev-tooling formats serialize host addresses as `u32` (`platform/native_checkpoint.c:56-60`), and that the checkpoint pointer-slot registry (`:106-171`, fed from `game/LOAD/LOAD_Assets.c:16-18`) is precisely the inventory a relocation redesign would need, which is a helpful accident.

### 4.3 The renderer, and whether it can reach iOS

**What it is.** A PsyCross/PsyX-derived hardware-accelerated PS1 GPU reimplementation on raw desktop OpenGL, loaded via a vendored glad 0.1.36 loader, windowed by SDL3. Not SDL_Renderer and not SDL3 GPU; both are explicitly compiled out of the vendored SDL at `CMakeLists.txt:69-70`. Provenance headers at `platform/native_renderer.c:1-5` (from `PsyX_render.cpp`), `platform/native_gpu.c:1-5` (from `PsyX_GPU.cpp`), and `platform/native_glad.c:22-26`.

It is not a software rasterizer blitting a VRAM buffer. PS1 packets become real GL triangles. `DrawOTag` (`platform/native_libgpu.c:274-295`) hands the ordering table to `ParsePrimitivesLinkedList` (`platform/native_gpu.c:1072-1188`), which validates each link through the token bridge and decodes primitives into 20-byte packed vertices (`include/platform/native_renderer_types.h:26-35`), grouped into batches keyed on blend mode, texture format, tpage, clip rect, and mask state (`platform/native_gpu.c:798-867`). `DrawAllSplits` uploads once with `glBufferSubData` and issues one `glDrawArrays` per batch (`platform/native_renderer.c:2243-2266`). PS1 VRAM is emulated as a single persistent 1024x512 `GL_RG8` texture holding packed 15-bit data (`:28-34,1201-1226`), mirrored by a CPU array with dirty-rect upload (`:2084-2108`) and tile-tracked lazy readback (`:1768-1837`); fragment shaders reconstruct 4, 8, and 16-bit CLUT texels directly from that texture (`:712-852`).

**The boundary.** Unusually clean. Zero GL calls, glad includes, or GL types appear anywhere under `game/`. There are exactly two files containing GL calls: `platform/native_renderer.c` with about 252 call sites and `platform/native_platform.c` with one (a `CTR_INTERNAL` screenshot at line 159). 75 unique GL functions in total. The API the rest of the program sees is `include/platform/native_renderer.h`, 44 functions in 45 lines, whose handle types are plain `u32` (`include/platform/native_renderer_types.h:63-64`) so no GL type leaks. The only layering leak is cosmetic: `game/230/MM_Scrapbook.c:18` includes the renderer header to call `NativeRenderer_ClearVRAM` at `:155`, which is still a GL-free API.

**The three routes.**

*ANGLE.* This is a category error as usually framed. ANGLE's client API is GLES, not desktop GL, so an app must become a GLES app before ANGLE can carry it. Upstream ANGLE also has no iOS plans; the community MetalANGLE fork is what covers iOS. ANGLE is therefore not an alternative to route two, it is a possible backend underneath it.

*Rewrite to GLES 3.0.* **Cheapest, and by a wide margin.** The concrete change list, essentially all in one file: switch the context request to `SDL_GL_CONTEXT_PROFILE_ES` 3.0 (`platform/native_renderer.c:190-208`); change two shader version strings from `#version 140` to `#version 300 es` (`:923,930`), where ES compatibility shims for `varying`, `attribute`, `texture2D`, precision qualifiers, and an explicit `out vec4 fragColor` are already present at `:923-935`; fix the one `glReadPixels` that reads `GL_RG` from the RG8 FBO (`:1821`) by reading RGBA and repacking, roughly 20 lines; ifdef out the wireframe `glPolygonMode` (`:2240`), the BGRA screenshot (`platform/native_platform.c:159`), and six timer-query sites that are already inside `CTR_INTERNAL` (`:301,309,333,373,391,1183`); KHR-suffix or drop the two debug-group calls (`:2270-2284`), which are already runtime-guarded; and regenerate glad for GLES 3.0, noting that the existing loader was already generated with a gles2 profile (`platform/native_glad.c:3-20`).

*Rewrite to SDL3 GPU or Metal.* A full rewrite of a 2,284-line file, six shaders re-authored, and the pervasive GL state save-and-restore idioms redesigned around command buffers. Weeks of work plus a re-verification burden on a renderer whose semi-transparency, mask-bit, and framebuffer-feedback semantics are subtle. Cleanest long term, since GLES is deprecated on iOS, but not the first move.

**GL 3.3 features with no GLES 3.0 equivalent.** Only three, all in debug or internal paths: `glPolygonMode` at `platform/native_renderer.c:2240`, `GL_BGRA` readback at `platform/native_platform.c:159`, and `GL_TIME_ELAPSED` timer queries at the six sites listed above. The one that touches shipping behavior is the `GL_RG` readback at `:1821`, which is not absent from ES 3.0 so much as not guaranteed; ES 3.0 promises only `GL_RGBA`/`GL_UNSIGNED_BYTE` plus one implementation-defined combination, so it must be handled rather than assumed.

Explicitly verified absent, which is why this route is cheap: no `GL_TEXTURE_RECTANGLE`, no `glLogicOp`, no `GL_CLAMP_TO_BORDER`, no geometry shaders, no `glTexBuffer`, no PBOs, no buffer mapping, no dual-source blending, no `glBlitFramebuffer`, no `glTexStorage`, no MRT. Depth testing is never even enabled (`:2149-2159`); only the stencil is used, for the PS1 mask bit (`:2161-2180`).

### 4.4 ARM64 and non-x86 portability beyond pointer width

**Endianness: not an issue.** There is no byte-swapping code at all. The project has a portable little-endian access layer instead: `CTR_ReadU16LE`, `CTR_ReadU32LE`, and their write counterparts are byte-wise shift-composed and therefore both endian-safe and alignment-safe (`include/macros.h:68-98`). Remaining host-little-endian assumptions, all correct on ARM64, are the GTE register `PAIR` union punning (`include/psx/gtereg.h:15-36`), raw asset reads without swapping (`platform/native_assets.c:672`, `game/LOAD/LOAD_File.c:92,119`), and halfword bit-splicing (`game/PushBuffer.c:353-357`).

**Unaligned access: low risk.** Packed regions exist (`include/psx/libgpu.h:269,693` wrapping the GPU primitive structs; `include/psn00bsdk/include/psxpad.h:19-22,271`), but Clang emits safe accesses for packed members and ARM64 tolerates unaligned normal loads regardless. There are 78 cast-based deserialization sites in `game/`, but they inherit PS1's own 4-byte alignment invariants: MEMPACK is 4-byte aligned (`include/namespace_Mempack.h:8-19`, `game/MEMPACK.c:95-134`), PS1 file formats were 4-aligned because MIPS `lw` traps otherwise, the scratchpad emulation is a union with a `u32` member guaranteeing alignment (`platform/native_memory.c:34-44`), and all 95 `CTR_SCRATCHPAD_PTR` offsets in use are 4-aligned. No atomics and no SIMD anywhere, so the ARM64 exceptions do not apply.

**MIPS and GTE integer semantics: written portably, not relying on x86.** This is the finding that most contradicts the brief's implicit worry. `include/ctr_math.h:151-220` implements the MIPS semantics explicitly: `CTR_MipsSll` masks the shift count with `& 0x1f` and shifts through `u32`, so no shift-count or signedness UB (`:153-156`); `CTR_MipsSra` reimplements arithmetic shift with an unsigned shift plus an explicit sign mask, so it does not even depend on implementation-defined signed right shift (`:158-163`); `MulLo`, `AddLo`, `SubLo`, and `NegLo` wrap through unsigned arithmetic (`:170-193`); `MulHiU` uses a `u64` intermediate (`:175-178`); and `CTR_MipsDiv`/`DivU` explicitly handle divide-by-zero and `INT_MIN / -1` (`:200-220`), which incidentally neutralizes the real x86-versus-ARM64 divergence, since ARM64 `sdiv` does not fault where x86 raises SIGFPE. These helpers are used 418 times, concentrated in `game/Vehicle/`, which corroborates the Beta 5 note. The GTE core uses `s64` accumulators throughout (`platform/native_gte_core.c:38-40,79-106,306-320`).

Residual technical UB does exist and behaves identically on Apple Clang arm64: `(1 << 31)` flag constants (`platform/native_gte_core.c:142-159`), left-shifting a negative `s64` in `gte_shift` (`:79-91`), signed right shift of negatives (`platform/native_libgte.c:38,45`, `game/CAM.c:43`), and the `FP_MULT` family doing raw `(x*y) >> 12` at 12 sites (`include/ctr_math.h:135-149`).

**Inline assembly and intrinsics: none reachable.** Every block of MIPS inline assembly is behind `#ifndef CTR_NATIVE` with a native replacement: `game/PushBuffer.c:319-336`, `game/Vehicle/VehEmitter.c:460-482`, `game/226/226_00_DrawLevelOvr1P.c:2744-2766`. `game/psyq_start.c:61` contains raw `asm` but the file is not in the unity build. `include/regionsEXE.h:5164` uses a `register ... asm("$gp")` binding, guarded at `:5162-5167`. The PSn00bSDK assembly headers are reachable only through the `#else` branch of `#if defined(CTR_NATIVE)` at `include/psx/psx_prelude.h:4-21`. There are zero x86 intrinsics anywhere, which is proven by construction since the MSVC build works and MSVC rejects GCC-style assembly. The only x86 artifacts are compiler flags: `-msse` at `CMakeLists.txt:114` and `/arch:SSE2` at `:105`.

**Compiler specifics: already POSIX-clean.** Every `_WIN32` block has a POSIX `#else` (`main.c:8-13`, `platform/native_savestate.c:49-56`, `platform/native_disc_image.c:6-8,44-60`, `platform/native_memcard.c:320`, and others), and an `__APPLE__` branch already exists returning `"macos"` at `platform/native_replay_scheduler.c:248-256`. No structured exception handling, no `alloca`. `setjmp`/`longjmp` is used for the PS1 cooperative thread-tick system (`game/PROC.c:4-14,596,672`), which is upward-only within a live frame and therefore valid and portable. MSVC pragmas are confined to `include/ctr_compiler.h:16-21` with `_Pragma` fallbacks.

**The C17 claim is true.** `CMakeLists.txt:88-90` sets `C_STANDARD 17`, `C_STANDARD_REQUIRED ON`, and `C_EXTENSIONS OFF`, so Clang gets `-std=c17` rather than `gnu17`, and Clang builds additionally opt into `-Wc23-extensions` (`:127-128`). The code honors it: no statement expressions, no case ranges, no `typeof`, no zero-length arrays. GNU-isms are fenced behind `__GNUC__` at `include/ctr_compiler.h:26-40` with portable fallbacks.

**`long` width: clean.** All project types come from `stdint.h` (`include/macros.h:8,12-22`) and no game data is `long`-typed. Bare `long` appears only in `fseek`/`ftell` plumbing, where LP64 makes it wider and therefore safer; `platform/native_checkpoint_file.c:217` has a `> 0xffffffffUL` guard that is vacuous on Windows and becomes meaningful on macOS.

**Threading and timing: portable.** The project creates no threads. The only concurrency is SDL3's audio callback (`platform/native_audio.c:2928`), synchronized exclusively with `SDL_LockAudioStream` (`:374-387`). Timing is `SDL_GetPerformanceCounter` plus `SDL_DelayPrecise` plus a spin-wait (`platform/native_platform.c:573-624`). PS1 critical sections are stubbed to no-ops (`main.c:17-19`).

### 4.5 Platform layer surface area

29 `.c` files in `platform/`, all unity-included from `main.c:38-66`. The core API is `include/platform.h:13-31`: `Platform_Init`/`Shutdown`, `InitScratchpad`, `InitMempackArena`/`GetMempackArena`, `BeginFrame`/`BeginScene`/`EndScene`/`EndFrame`, `PresentVRAMDisplay`, `PinVRAMDisplayFrames`/`Rect`, `GetVBlankCount`, `WaitUntilVBlank`, `PollHostEvents`, `PollInput`, and `NikoGetEnterKey`. Subsystem APIs live in `include/platform/native_*.h`.

| Subsystem | Implemented with today | SDL3 iOS coverage | What an iOS implementation needs |
|---|---|---|---|
| Audio | Full in-process SPU emulation, 24 voices with software ADPCM, reverb, CD mix, plus XA streaming; output through one `SDL_OpenAudioDeviceStream` at S16 stereo 44.1kHz (`platform/native_audio.c:2925-2928`) | Yes, CoreAudio backend | Nothing beyond enabling it. The decoder is pure C. |
| Input | SDL3 keyboard and gamepad to PS1 pad packets, rumble via `SDL_RumbleGamepad` (`platform/native_input.c:703,1107`) | Partial: MFi and Bluetooth controllers yes, touch is native to SDL but unused here | On-screen touch controls are entirely new code. Keyboard debug hotkeys become irrelevant. |
| Memcard and saves | Raw `fopen`, `mkdir`, and dirent under a relative `memcards/` directory (`platform/native_memcard.c:22,182-187`) | `SDL_GetPrefPath` exists but is never called in this project | Redirect all writes. Today everything is relative to a `chdir`'d base (`main.c:168`), which on iOS is the read-only bundle. |
| CD, disc, streaming | `fopen` on extracted files plus an in-process ISO9660 reader over `assets/ctr-u.bin` (`platform/native_cd.c:11-12`, `platform/native_disc_image.c:24-26`) | Bundle and Documents reads are fine | Asset discovery walks the exe directory and two parents (`platform/native_assets.c:535-574`), driven by `SDL_GetBasePath` (`main.c:152`), so it adapts. The real work is the import flow. |
| Renderer and video | Desktop GL 3.3 down to 3.0 core with glad; SDL vendored with `SDL_OPENGLES OFF` and `SDL_GPU OFF` (`CMakeLists.txt:69,77`) | No desktop GL on iOS | The GLES retarget in section 4.3, plus flipping the SDL options back on. |
| Timing | `SDL_GetPerformanceCounter`, `SDL_DelayPrecise`, and a spin-wait pacing a synthetic NTSC VBlank at 897619 GPU cycles, about 59.817Hz (`platform/native_platform.c:539-541,573-627`) | Yes | Replace the blocking spin with a display-link or callback-driven loop for battery and lifecycle reasons. |
| Logging | `fopen` on a log next to the executable, plus `OutputDebugStringA` on Win32 (`platform/native_log.c:82,92`) | n/a | Point at the pref directory. The Win32 branch compiles out. |
| Memory and arena | malloc-backed 2MiB mempack with the retail NTSC-U window layout, plus scratchpad and resident-pointer repair (`platform/native_memory.c:19-27,89-106`) | n/a | Pure C, but bound to the 32-bit pointer contract of section 4.2. |

**The PsyQ facade.** Implemented across `native_libapi.c`, `native_libetc.c`, `native_libgpu.c`, `native_libgte.c`, `native_libpad.c`, `native_libspu.c`, plus `native_cd.c` for the `Cd*` family, `native_gte_core.c`, and `native_inline_c.c`. Most entry points are real: the GPU, GTE, SPU, pad, and CD paths all route into working subsystems, and the root counters genuinely emulate RCNT1 from emitted VBlanks (`platform/native_libapi.c:10-23`). The stubs are `OpenEvent`, `CloseEvent`, `EnableEvent`, and `TestEvent` returning 0 and the CARD functions no-oping (`:77-121`), `StopCallback` (`platform/native_libetc.c:20-23`), `PadInfoAct` and `PadSetMainMode` returning 0 with `PadSetActAlign` returning 1 (`platform/native_libpad.c:32-53`), and `SetRCnt`/`StartRCnt`/`StopRCnt` accepting and ignoring (`platform/native_libapi.c:25-67`). None of these stubs are iOS-relevant.

PsyCross derivation is acknowledged per-file, for example `platform/native_gpu.c:1-5`, `native_gte_core.c:1-5`, `native_renderer.c:1-5`, `native_glad.c:22-25`, and in `THIRD_PARTY_NOTICES.md:6-47`.

**Codecs are entirely in-process.** No host or OS codec dependency exists anywhere. XA-ADPCM is decoded at `platform/native_audio.c:1382-1407,1813,1853,1905` with the console's 37800 to 44100Hz zig-zag interpolation table transcribed from psx-spx at `:420-422`. STR video is a full CPU MDEC implementation: Huffman AC tables at `platform/native_str.c:125-141`, fixed-point IDCT at `:265-332`, macroblock decode at `:382-475`, YCbCr to RGB555 at `:373-380`. This matters a great deal for iOS, because it means there is no AVFoundation or codec-licensing surface to bridge.

### 4.6 Build system

`CMakeLists.txt` defines a single target built from exactly one source file, `main.c` (`:86`), sets C17 with extensions off (`:88-90`), defines `CTR_NATIVE`, `BUILD=926`, and `CTR_INTERNAL` (`:94-100`), applies `/W4 /bigobj /arch:SSE2` on MSVC (`:102-111`) and `-msse` elsewhere (`:113-122`), and links `SDL3::SDL3` statically (`:131`, with `SDL_STATIC ON` at `:64`). SDL3 arrives as vendored full source via `add_subdirectory(externals/SDL)` (`:83`) with several subsystems disabled, notably `SDL_GPU OFF`, `SDL_RENDER OFF`, `SDL_OPENGLES OFF`, and `SDL_VULKAN OFF` (`:69-78`). `CMakePresets.json` defines presets only for Windows MSVC Win32, Windows MinGW i686, and Linux GCC i686; there is no macOS, iOS, or ARM preset. `build.sh:14` passes `-m32`, `build.bat:39` requires an i686 MinGW compiler, and `build-msvc.bat:14-26` drives the `windows-msvc-x86` preset.

**SDL3 version: exactly 3.4.10**, confirmed at `externals/SDL/include/SDL3/SDL_version.h:47,56,65` and corroborated at `THIRD_PARTY_NOTICES.md:75`. It is the complete upstream source tree, not a prebuilt binary: `externals/SDL/src/video/uikit/`, `externals/SDL/src/audio/coreaudio/`, and `externals/SDL/Xcode/` are all present. SDL3 supports iOS through CMake with `-DCMAKE_SYSTEM_NAME=iOS`, per upstream's own documentation, so the vendored copy as shipped is capable of an iOS build once the project's own options stop disabling GLES.

**The unity build helps.** `game/game_unity.h` has 250 `#include` lines covering 249 `.c` files; `main.c:32-34,38-66` adds 3 data files and the 29 platform files. The entire program is one translation unit. For Xcode or iOS CMake integration this is a net positive: there is exactly one source file to add to any toolchain, and symbol collisions are already resolved because the project has never built any other way. The `/bigobj` requirement (`CMakeLists.txt:104`) is MSVC-specific and Clang has no equivalent limit. The costs are compile memory and time for one large TU, which is a non-issue on modern Macs, and the fact that whole-program `static` conventions would make splitting the build later a real piece of work. `include/platform/native_win32.h:20-23` documents the single-TU design constraint explicitly.

**What breaks under `-DCMAKE_SYSTEM_NAME=iOS`,** in order of severity:

1. `CMakeLists.txt:7-13` fails configuration outright on 64-bit. iOS is arm64-only.
2. `-msse` at `:114` is a hard error on Apple Clang targeting arm64. No SSE intrinsics exist in the source, so the flag is droppable.
3. `SDL_OPENGLES OFF` at `:69` and `SDL_GPU OFF` at `:77`, combined with the desktop-GL renderer.
4. `SDL_MAIN_HANDLED` with a plain `main()` at `main.c:2,138`. iOS requires SDL to own the entry point.
5. The CTest smoke test executes the built binary at `:141`, impossible when cross-compiling. Needs `BUILD_TESTING=OFF`.
6. Correctly gated and therefore harmless: `-static-libgcc` (`:132-134`), MinGW `-static` (`:135-137`), MSVC static CRT (`:18`).

Runtime rather than configure-time: `chdir(base)` at `main.c:168` with relative writes throughout, and `exit(0)` on quit at `platform/native_platform.c:437,447`.

**No linker tricks remain.** There is no fixed image base, no `/BASE`, no `-Ttext`, and no linker script anywhere. PSX addresses survive only as struct layout maps in `include/regionsEXE.h` and in the documentation. `CMAKE_EXPORT_COMPILE_COMMANDS` is on in every preset.

**The realistic iOS path** is CMake with the standard iOS toolchain, not a hand-built Xcode project. The project is already a single CMake executable target with vendored SDL3 as a subdirectory, so an `ios-arm64` preset plus conditioning the three or four hostile flags is mechanically straightforward. A hand-built Xcode wrapper around a static library would be more work for no benefit, though Xcode will still be needed for the app bundle, signing, launch storyboard, and `Info.plist`, which CMake can generate.

### 4.7 Assets

**Required file list, confirmed in code.** Canonical names are defined at `platform/native_assets.c:19-24`: the directory `assets`, `BIGFILE.BIG`, `SOUNDS/KART.HWL`, `TEST.STR`, `XA/ENG.XNF`, and the disc image `ctr-u.bin`. `NativeAssets_Validate` (`:891-912`) requires all four data files, then `NativeAssets_ValidateXA` (`:791-889`) parses the `ENG.XNF` manifest, checking magic `XINF` and version 102 at `:814`, and requires every `S%02u.XA` the manifest references across `XA/MUSIC`, `XA/ENG/EXTRA`, and `XA/ENG/GAME` (`:793-797,872-884`). `main.c:174-177` exits with code 1 on failure.

The critical nuance the brief missed: **each file passes if it exists either extracted on disk or inside the disc image** (`:777-785`, XA variant at `:880`). So the practical requirement is either `assets/ctr-u.bin` alone, or the full extracted set, and mixing works per file, with extracted files taking precedence (`README.md:138-141`; host-first open order at `platform/native_cd.c:99-118`). The full extracted list is `BIGFILE.BIG`, `SOUNDS/KART.HWL`, `TEST.STR`, `XA/ENG.XNF`, `XA/ENG/EXTRA/S00.XA` through `S05.XA`, `XA/ENG/GAME/S00.XA` through `S20.XA`, and `XA/MUSIC/S00.XA` through `S01.XA`, which is 29 XA streams (`README.md:158-166`).

**Total size: unknown.** No size figure appears anywhere in the README or `docs/`. I am not going to estimate it from disc geometry, since the requirement is a specific NTSC-U image and I have not measured one.

**Disc image constraints.** `.bin` only, no `.cue` parsing. `platform/native_disc_image.c` implements a read-only ISO9660 reader over a single-track raw MODE2/2352 image with the data track at byte 0 (`:14-26,86-104,340-365,409-447`), including raw-sector reads for XA and STR (`:512-541`). The filename must be `ctr-u.bin` (`:14`), matched case-insensitively on POSIX (`:42-78`). `README.md:124` states that a cooked 2048-byte `.iso` will not work because it discards the XA and STR sector data.

**Path handling on iOS.** The base directory comes from `SDL_GetBasePath()` (`main.c:152-156`), and `NativeAssets_Init` probes the executable directory and two ancestors for an `assets` directory containing `BIGFILE.BIG` or `ctr-u.bin` (`platform/native_assets.c:535-574`), then `chdir`s to the winner (`main.c:168-172`). There is **no environment variable, no command-line argument, and no configuration file** for the asset path; `assets` is a hardcoded relative constant (`:19,41-42`). The only public CLI flag is `--version` (`main.c:132-147`). The internal plumbing is absolute-path-clean, so an iOS port would redirect at `NativeAssets_Init` rather than refactor. Practically, an iPad user would import `ctr-u.bin` through a `UIDocumentPicker` or Files-app drop into the app's Documents directory, and the app would point the asset root there. On-device extraction from the disc image is unnecessary, since the ISO9660 reader already reads the image directly.

**Codecs are in-process,** as covered in 4.5. No host codec dependency, so nothing here constrains the App Store or sideload story.

### 4.8 Touch control design surface

Not designing controls, per the brief. Enumerating what the game demands.

**Full input map.** Bindings are hardcoded in `NativeInput_DefaultMappings` at `platform/native_input.c:290-349`; there is no user binding config. The raw-to-logical conversion table is `data.gamepadMapBtn` at `game/zGlobal_DATA.c:2915-3000`, consumed at `game/GAMEPAD.c:335-341`.

| Action | PS1 button | Game-side reader | Keyboard | Gamepad |
|---|---|---|---|---|
| Accelerate | Cross, held | `game/Vehicle/VehPhysProc.c:634,886-891` | `K` or `C` (`:295-299`) | South (`:328`) |
| Analog throttle and reverse | Right stick Y | `VehPhysProc.c:798-806,897` | none | Right stick Y (`:304-305`) |
| Brake and reverse | Square, held | `VehPhysProc.c:635,952-1015` | `J` or `X` (`:292,296`) | West (`:325`) |
| Hop and drift | L1 or R1 | `VehPhysProc.c:16-17,737-786` | `Q`/`E` or Left/Right Shift (`:301,305,308-309`) | Shoulders (`:330,334`) |
| Fire or use item | Circle, tapped | `VehPhysProc.c:644-728` | `L` or `V` (`:293,297`) | East (`:326`) |
| Item aim modifier | D-pad up forward, down backward | `game/Vehicle/VehPickupItem.c:951,734` | `W`/`S` or arrows (`:311-318`) | D-pad (`:338-341`) |
| Steering | D-pad left/right or left stick X | `VehPhysProc.c:1124-1227` | `A`/`D` or arrows | Left stick X or D-pad |
| Camera near/far | L2, tapped | `game/CAM.c:1853-1871` | Left Ctrl (`:302`) | Left trigger (`:331`) |
| Skip race-start fly-in | Triangle | `game/CAM.c:1665-1669` | `I` or `Z` (`:294,298`) | North (`:327`) |
| Pause | Start | `game/MAIN/MainFrame.c:397,435-444` | `P` or Return (`:321,323`) | Start (`:344`) |
| Menu navigation | D-pad plus Cross, Triangle or Square to cancel | `game/230/MM_Characters.c:38-40`, `game/RECTMENU.c:826` | | |

**There is no look-behind control.** R2 is present in the mapping table (`game/zGlobal_DATA.c:2967-2970`) but has no gameplay consumer; grep for `BTN_R2` in `game/` finds only the map table and cheat-code data. The brief lists look-behind as part of the input map; in this codebase it does not exist. That removes one control from the touch budget.

**Steering is genuinely analog.** The platform layer opens gamepads with `analogEnabled = 1`, presenting pad ID `0x73` (DualShock analog), and writes stick bytes into the PSX pad packet (`platform/native_input.c:16,377,470-473,711`; packet layout at `include/platform/native_input.h:8-16` and `include/namespace_Gamepad.h:102-131`). The game accepts analog only from analog-like controller IDs (`game/GAMEPAD.c:369-373`) and feeds `stickLX`, a 0 to 255 value centered at `0x80`, into `VehPhysJoystick_GetStrengthAbsolute` (`game/Vehicle/VehPhysProc.c:1192`), which applies a `0x30` deadzone, a `0x7f` range, and a two-slope piecewise-linear response curve (`game/Vehicle/VehPhysJoystick.c:5-11,50-87`). The output is a continuous signed magnitude, not quantized to steps; effective resolution is roughly 79 levels per side after the deadzone. Digital input emulates the stick by ramping `stickLX` by up to `0xff` per frame (`game/GAMEPAD.c:375-465`), so keyboard play is effectively full-lock steering. **For a touch port this is good news: a virtual analog control has somewhere real to send its value.**

**Polling is per-frame and pull-based.** `VSyncCallback(MainDrawCb_Vsync)` is registered at `game/MAIN/MainMain.c:647`, and each VBlank `MainDrawCb_Vsync` calls `Platform_PollInput()` followed by the retail `GAMEPAD_PollVsync` (`game/MAIN/MainDrawCb.c:46-52`). `Platform_PollInput` pumps SDL events and then reads current keyboard and gamepad state into PSX-shaped pad bytes (`platform/native_platform.c:519-524`, `platform/native_input.c:794-827`). Edge detection into `buttonsTapped` and `buttonsReleased` happens once per game frame at `game/GAMEPAD.c:914-967`.

**There is an existing abstraction for touch to sit behind, and it is a good one.** `Platform_InputInstallPadSnapshots(src, 4)` and `Platform_InputClearInstalledPadSnapshots` (`platform/native_input.c:940-962`) already exist to serve replay playback: when installed snapshots are active, `Platform_InputUpdate` writes them to the pad bus and skips SDL entirely (`:804-810`). This is all-or-nothing per frame. A subtler option, and probably the better one, mirrors the keyboard model: the per-slot loop at `:820-826` applies `NativeInput_ApplyKeyboard` as a mask over the slot's buttons (`:564-583`), so a peer `NativeInput_ApplyTouch` would compose naturally with a connected MFi controller and could write the analog bytes directly.

**The boost chain, with exact constants.** The core function is `VehPhysProc_PowerSlide_Update` at `game/Vehicle/VehPhysProc.c:2002-2160`. Meter quantities are milliseconds; frame constants convert via `<< VEH_PHYS_PROC_FRAME_TIME_SHIFT` where the shift is 5, so one frame is 32ms (`:73`). Per-character stats are identical across all four engine classes in the `data.metaPhys` table:

- `const_turboMaxRoom` = 30 frames (`game/zGlobal_DATA.c:7193`)
- `const_turboLowRoomWarning`, the red zone = 15 frames (`:7194`)
- `const_turboFullBarReserveGain` = 60 frames (`:7195`)
- `const_DriftBoostDurationFrames` = 15 frames (`:7196`)
- `const_Drifting_FramesTillSpinout` = 60 frames (`:7182`)

The mechanic, as implemented: drift entry requires ground contact, steering beyond half the turn rate, the shoulder button held, and speed at least half the class stat (`VehPhysProc.c:1300-1328`), with a 10-frame hop input buffer (`:18,752,782`). `PowerSlide_Init` sets the meter to `30 << 5` = 960ms (`:2208`), decremented by about 32ms per frame (`:2026`). A shoulder tap (`:2009`) grants a boost only while the meter is below `15 << 5` = 480ms (`:2059-2072`), so **the success window is the final 480ms, about 15 frames, of a 960ms fill.** Tapping early produces a failed-boost exhaust puff lasting 8 frames (`:2105`). The reward scales linearly with precision: reserves gained are `MapToRange(meterLeft, 0, 480ms, 1920ms, 0)` (`:2071-2072`), so perfect timing yields 1920ms of reserves and the value decays to zero at the red-zone boundary. Fire level escalates per successive boost (`:2085-2088`), the visual boost lasts `15 << 5` = 480ms (`:2098`), and a maximum of 3 boosts per drift is enforced (`:81,2015,2091`). Holding a drift beyond 60 counted frames triggers spinout with a 1.0-second input lockout (`:85,2118-2136`).

**Frame-rate context for touch latency.** The NTSC build defines 30 FPS with 32ms elapsed per frame (`include/macros.h:36-40`), logic is clamped to 32 to 64ms per frame (`game/MAIN/MainFrame.c:190-205`), and the flip is locked to 2 VBlanks (`game/MAIN/MainFrame_RenderFrame.c:1203-1211`), with native pacing at about 59.817Hz giving roughly 29.9 effective FPS (`platform/native_platform.c:532-541`). The code explicitly warns against 60 FPS patching in velocity math (`game/Vehicle/VehPickupItem.c:751-752`).

**The difficulty judgment this supports:** the boost window is 480ms wide, which is forgiving in absolute terms, and the hop buffer is 10 frames or about 333ms, which is generous for touch latency. The genuinely hard part is not the tap timing but that the tap must occur **while a drift is held and steering is sustained**, meaning a player's thumb must maintain an analog steering deflection and a shoulder hold while a second input fires three times. That is a two-handed simultaneous-hold problem, not a reaction-time problem, and it is the crux of the control design.

### 4.9 Licensing and distribution

**ctr-native is GPL-3.0.** `LICENSE` is the stock 674-line GPLv3 text with no project-specific copyright holder substituted; the FSF template placeholder remains. It was added on 2026-06-25 in commit `12e2888c0`, authored by Agung Firdaus with the message "chore(license): adopt CTR-ModSDK GPLv3 license". Note that `README.md` never states the license; the only rights language in it is the trademark line at `:200`.

**CTR-ModSDK is also GPL-3.0**, added the same day in commit `530a072e` with the message "Niko chose GPL".

**On the pre-license commit question.** The brief's premise is roughly right but its number is wrong. CTR-ModSDK has 5,480 commits, of which 5,476 predate the license grant, so more than 99.9 percent of the history is pre-license, across 24 contributors. ctr-native has 6,629 commits with 6,472 predating its grant, across 27 contributors. Neither repository has a CLA, a DCO, a CONTRIBUTING file, or any recorded relicensing consent, and no source file in `game/`, `platform/`, or `include/` carries a copyright or GPL header. The objective situation: before the grant, the code was under default all-rights-reserved; the grant was applied by one contributor to a tree containing two dozen people's work. Whether every contributor's consent is implied by the ordinary act of submitting a PR to a public project is a legal question I am not equipped to answer, and I will not pretend otherwise. For a downstream port the practical exposure is low, because the upstream project is the party who would need to resolve any dispute, but this should be flagged to counsel rather than waved through.

**Third-party licenses are compatible.** `THIRD_PARTY_NOTICES.md` lists exactly three components: PsyCross/Psy-X under MIT with the full text reproduced and copyright to the REDRIVER2 Project (`:6-47`), a PSn00bSDK header subset under MPL-2.0 (`:49-66`), and SDL3 3.4.10 under zlib with full text (`:68-93`). MIT and zlib are standardly GPLv3-compatible, and MPL-2.0 is compatible where files are not marked "Incompatible With Secondary Licenses", which these are not. Two loose ends worth noting rather than worrying about: the glad 0.1.36 loader is not separately attributed, being covered only implicitly as a PsyCross-derived file (`platform/native_glad.c:3,22-25`), and the MPL full text is referenced by URL rather than included (`THIRD_PARTY_NOTICES.md:64-66`). **PsyCross specifically is MIT and therefore poses no compatibility problem.**

**GPLv3 versus the App Store, stated plainly.** GPLv3 and the Apple App Store are incompatible, and this is settled in practice rather than theoretical. Two independent conflicts: Apple's terms impose usage rules on the end user that GPLv3 section 10 forbids adding, and the App Store's DRM plus device signing conflict with GPLv3's anti-tivoization requirement that a recipient of a "User Product" receive the Installation Information needed to run modified versions. The VLC removal from the App Store is the well-known precedent. **The practical implication: App Store distribution is off the table unless every copyright holder relicenses, which for a project with 27 contributors, no CLA, and a partly anonymous history is not realistic.**

Sideloaded distribution is a different matter and is **not** blocked by GPLv3. For AltStore, a developer-signed IPA, or direct Xcode install, complying with GPLv3 means shipping the corresponding source and the Installation Information sufficient for a recipient to build, sign, and install their own modified version. Since sideloading inherently requires the user to sign with their own Apple ID or provisioning profile, the Installation Information requirement is satisfiable in a way it simply is not through the App Store. TrollStore is a further case that depends on an exploit rather than a supported distribution path, which introduces durability risk unrelated to licensing. Note also that GPLv3 compliance obliges publishing the port's source, which for a fork is the expected posture anyway.

**Provenance, and how the project asserts it.** No repository contains the phrase "zero original Naughty Dog code", "no leaked source", or "clean room". What they do assert is methodology. `CTR-ModSDK/rewrite/README.md:2` describes a non-byte-matching decompilation using a "Ship of Theseus" strategy: rewritten functions are loaded into PCSX-Redux's expanded memory alongside the intact original, called in place of the originals, and their outputs compared against the original function's. `CTR-ModSDK/decompile/DecompUnitTester/README.md:5` gives a standard reverse-engineering definition. Notably, `CTR-in-C/src/README.md` states that the org **also** runs an explicitly byte-matching track whose "only goal is to achieve matching decomp", which is a different and more legally exposed activity than behavior-matching. The strongest copyright-hygiene statement anywhere in the org is `CTR-in-C/src/matching/staging/README.md:8-10`, which labels the extracted `SCUS-94426.s` disassembly "copyrighted game disassembly" and forbids committing it, keeping it gitignored.

**Assets: none are shipped.** I searched ctr-native for `.big`, `.hwl`, `.xa`, `.str`, `.tim`, and large `.bin` files and found nothing. The user must supply their own NTSC-U disc image. This matches the posture of every project the requester has already shipped. One caveat outside ctr-native: CTR-ModSDK does contain `.tim` image files in mod directories with names mirroring original title-screen assets, for example `mods/Standalones/CrashBall/CML_Splash/title01_usa.tim`, whose provenance is not documented in-repo. That is a ModSDK concern, not a ctr-native one, but it is worth knowing that the upstream org's hygiene is not uniform.

**Exposure comparison.** I can state ctr-native's facts with citations. For DevilutionX, OpenRCT2, Wargus, and Fallout 2 CE I am relying on general knowledge rather than research performed here, and I flag that distinction rather than dress it up.

The structural similarities are strong: all of these projects require user-supplied original game assets, ship none themselves, and are derived from reverse engineering rather than licensed source. On the specific axis of derivation method, ctr-native sits closest to DevilutionX and Fallout 2 CE, which also originate in decompilation, and furthest from Wargus, which is a data-driven engine reimplementation with no derived code at all and therefore the lowest exposure of the four.

Two differences are worth naming. First, the IP holder. Crash Team Racing is Activision-held, and Activision Blizzard was acquired by Microsoft in October 2023, so the current holder is Microsoft. That is the same corporate parent as Diablo and Warcraft II, and as Bethesda for Fallout 2, so the requester has shipped against this counterparty before and the exposure is not novel. Second, and more materially, **there is an active commercial product line.** Crash Team Racing Nitro-Fueled shipped in 2019 and has sold over 10 million units, and there are credible trade reports of a PC and current-generation port in development for 2026. I could not verify that report against a primary source and treat it as rumor, but the 2019 remake is a matter of record. This is a real difference from Diablo 1 and RollerCoaster Tycoon 2, whose commercial lives are largely in the GOG back catalogue. A rights holder with a live remake and a rumored new port has more reason to care about a free native port of the original than one whose title has been dormant for two decades. **This is the strongest non-technical argument for caution, and it is independent of every code-level finding in this report.**

None of the above is legal advice. It is a factual comparison for counsel to work from.

### 4.10 Upstream posture and competing work

**Prior platform requests, all found and read.** GitHub Discussions are disabled on both repositories, so issues and PRs are the complete written record.

- **Issue #20, "Mac Port?"**, open. The maintainer replied within about an hour: "Nope. MacOS users can build from source." Follow-up comments report that the source build actually fails on Apple Silicon with `clang: error: unsupported option '-msse' for target 'arm64-apple-darwin25.5.0'`, exactly the flag identified at `CMakeLists.txt:114`, and that no macOS build instructions exist. **No maintainer reply since 2026-06-29.**
- **Issue #25, "Android version"**, open. A contributor deflected toward Winlator-style emulation and expressed doubt that phones could run it. The Android fork author self-linked in the thread and was not challenged.
- **PR #41, "Added Android GLES compatibility and 32-bit build support"**, open since 2026-07-10 with **zero maintainer review in 19 days**. Not rejected, not engaged.
- **PR #40, "Add Android support"**, closed as a duplicate of #41, with a contributor voicing a concern about renderer changes conflicting with recent optimization work.
- **PR #15, "Macos arm64 port"**, closed by its own author within a minute as opened by mistake. No maintainer signal either way.
- **PR #5, "Make sized enums more portable"**, merged, with the maintainer writing that he agreed portability there was useful.
- **PR #6, "Don't use hardcoded scratchpad address"**, closed with the maintainer saying it was in the right direction and that he would handle it in an ongoing refactor. **He then delivered it in Beta 6**, which is a meaningful data point about how this maintainer works.

There are **zero** mentions of iOS, aarch64, or bare ARM anywhere in either repository's issues or PRs.

**Existing forks attempting ARM or mobile.** One, and only one, is real: **`Simon358/ctr-native-android`**, last pushed 2026-07-26, with a `feature/add-android-support` branch. Its README states that Android builds 32-bit `armeabi-v7a` and `x86` APKs and requires an OpenGL ES 3 capable device. It adds a Gradle project and a GLES renderer path, and it is the source of PRs #40 and #41. **This is the most valuable single artifact for anyone attempting an iOS port**, because it is a working GLES retarget of this exact renderer. Its limitation, again, is that `armeabi-v7a` is 32-bit ARM, which sidesteps rather than solves the 64-bit problem and is not available on iOS.

`NyperYuhgard/CTR-PC-Port` turns out not to be a GitHub fork at all; it is a detached copy created 2026-06-12 that adds netplay, specifically 8-player UDP, item sync, and lobby chat, with documentation in Spanish. It predates the MSVC and CMakePresets refactor, is Windows and Linux x86 only, and does no ARM or mobile work. Its README preserves an older and more explicit version of the roadmap language: "remove the 32-bit constraint". It also redistributes GPLv3-derived code with no license text in its tree, which is a compliance gap on their side and a reason not to build on that copy.

**Assessment: tolerated, edging toward welcomed for the portability work specifically, but unwanted as an official platform.** The evidence in favor of a welcome is concrete: the README roadmap explicitly commits to reducing 32-bit host-pointer assumptions (`README.md:193`), the architecture note frames 32-bit as a temporary state (`:188`), Beta 6 shipped three ARM-enabling changes, an outside porter's portability PRs were merged or superseded-then-delivered, and first-party code targets portable C17. The evidence for mere tolerance is equally concrete: a flat "Nope" on official Mac builds, silence when told the Apple Silicon build is broken, an Android PR sitting unreviewed for nearly three weeks, and no mention of any other platform in nine releases or on the project website.

Nothing hostile exists in the written record. The only stated constraints are retail fidelity, with one contributor noting he is "against modifying vanilla game", and renderer performance. **The realistic model, following the Simon358 precedent, is: do the work in a fork, upstream the portability fixes because those do get merged, and expect the platform shell to live downstream permanently.**

**Who to talk to.** `aalhendi` is the day-to-day maintainer, with 1,152 commits, all nine releases, and near-exclusive authorship of July's commits. `DCxDemo` owns the org, though the public members list is empty so ownership is inferred rather than API-confirmed. `kkv0n` does frontline triage and contributed the audio and performance PRs. `Rinnegatamante`, a well-known Vita and homebrew porter, filed the portability PRs in the project's first week and would be a knowledgeable ally. GitHub issues are the documented contact path; the Discord invite is in the org profile and the ModSDK README. I did not join the Discord. Its widget API is enabled but exposes only the server name and presence counts with an empty channel list, so no message content is readable without joining, and no public archive exists.

### 4.11 Sequencing

**64-bit must land before the renderer work, but not for the reason one might assume.** The dependency is not conceptual, since the two touch almost entirely disjoint code. It is mechanical: `CMakeLists.txt:7-13` refuses to configure on a 64-bit target, so on Apple Silicon you cannot produce any build at all, GLES or otherwise, until the gate is addressable. You can develop the GLES changes on a 32-bit Linux host in parallel and validate them there, but you cannot validate them on Apple hardware until 64-bit configures.

There is one shortcut worth knowing about: the gate can be lifted temporarily for experimentation before the underlying pointer work is finished, because the failure modes are runtime corruption rather than compile errors for most of the mechanical sites. That would allow early renderer bring-up on macOS against a knowingly-broken game state. That is a debugging convenience, not a milestone.

**Yes, there is a viable interim milestone, and it is the right one: a 64-bit macOS ARM64 desktop build.** It de-risks essentially everything iOS-specific except touch, and it does so on a platform with a real debugger, real profiling, no signing friction, no sandbox, and no app lifecycle. It exercises the same compiler (Apple Clang), the same architecture (arm64), the same pointer width, the same UB-flag exposure, and if you take GLES rather than desktop GL on macOS, largely the same renderer path. It also has independent value: it closes issue #20, which is an open user request that upstream declined to serve, which is a good-faith way to open a relationship with the maintainer.

---

## 5. Sequencing

The ordered path, on the "wait" verdict. Phases 0 and 1 are prerequisites to deciding whether to proceed at all.

**Phase 0. Decide by watching, not building. Cost: near zero.**
Watch three signals for one to two months: whether beta-8 ships and at what commit volume, whether PR #41 gets reviewed, and whether any 64-bit commits appear. `aalhendi`'s own delivery on PR #6 shows he does complete roadmap items he takes ownership of, so a public statement of intent about 64-bit would materially change this assessment. Concurrently, get counsel's read on the two non-technical items: the pre-license contributor question and the live-commercial-product exposure in section 4.9. **If counsel says no, stop here and save the engineering entirely.**

**Phase 1. Cheap technical validation, roughly 1 to 2 weeks.**
Build the current 32-bit tree on Linux, verify it runs, and establish a behavioral baseline. Then attempt a macOS arm64 build with the pointer gate temporarily lifted and `-msse` removed, purely to measure how far it gets and to convert my structural-versus-mechanical classification from grep counts into observed compiler and runtime failures. **This is the step that converts my low-to-medium-confidence 12-to-18-week estimate into something you can actually plan against**, and it is cheap enough to run regardless of the phase 0 outcome on the technical side.

**Phase 2. The 64-bit port, 12 to 18 weeks, possibly 20 to 30.**
The dependency order within it, driven by which work unblocks the rest:
1. Mechanical pass first, roughly 170 sites, because it is low-risk and shrinks the noise floor for everything after.
2. Layout triage of the 75 pointer-bearing pinned structs, because the decisions there determine the design of steps 3 and 4.
3. Asset relocation redesign, the largest item and the one with the highest chance of forcing an architecture change.
4. Scratchpad overlay slots, which are constrained by retail offset semantics.
5. Dev-tooling formats and, throughout, parity validation against retail behavior.
Upstream every piece of this that is separable. Portability PRs have a good merge record here, and this is work the maintainer has said he wants.

**Phase 3. GLES 3.0 renderer retarget, 1 to 2 weeks.**
Can begin in parallel with phase 2 on a 32-bit Linux host, since the code is disjoint. Start from Simon358's Android GLES branch rather than from scratch. Merge onto the 64-bit branch once phase 2 configures on arm64.

**Phase 4. macOS ARM64 desktop milestone.**
The de-risking gate. Everything above validated on Apple hardware, native pointer width, Apple Clang, with `-fno-strict-aliasing` and `-fwrapv` added and the parity suite re-run. **Do not proceed past here until this build is correct**, because every bug found after this point will be harder to diagnose behind the iOS toolchain.

**Phase 5. iOS shell, 2 to 4 weeks.**
An `ios-arm64` CMake preset, SDL_main entry-point adoption, pref-path redirection for saves and logs, the Files-app import flow for `ctr-u.bin`, replacing the spin-wait pacing with a display-link loop, and app lifecycle handling in place of `exit(0)`.

**Phase 6. Touch controls.**
The actual product work, behind `NativeInput_ApplyTouch` composing with the existing per-slot mask model. The design problem, per section 4.8, is sustaining analog steering plus a drift hold while tapping boost three times, not the 480ms timing window.

**Phase 7. Sideload distribution with GPLv3 source publication.**

---

## 6. Open questions

Things I could not determine, and what it would take.

1. **Whether `aalhendi` intends to do the 64-bit work, and on what timeline.** This is the highest-leverage unknown in the report; it is the difference between a 15-week project and a wait. The written record shows intent in the README roadmap but no schedule, and no release note has ever mentioned it. **To determine:** ask him directly in an issue, or on Discord. He answered issue #20 within an hour.
2. **What is in the Discord.** The project's real technical conversation almost certainly happens there, and the widget API exposes only presence counts with an empty channel list. Per instructions I did not join. **To determine:** join and read the development channels. This would likely resolve question 1 as well.
3. **Whether my 12-to-18-week estimate survives contact.** It is derived from grep counts and structural reading, not from attempting the work. The specific uncertainty is how much parity re-validation each structural change triggers. **To determine:** phase 1 above.
4. **Total asset size.** Not stated anywhere in the repo, and I did not measure an NTSC-U image. Matters for the iPad import flow and App-size expectations. **To determine:** measure a retail image directly.
5. **Whether the parity test suite is usable as a regression gate for the 64-bit work.** I found the checkpoint, replay, and savestate infrastructure and the ASM-verified annotations in the source, but I did not evaluate coverage or determine whether the suite can run headless in CI. This matters a great deal for phase 2's real cost. **To determine:** read `platform/native_replay_scheduler.c` and the CTest setup in depth and try running it.
6. **Whether Simon358's GLES branch is directly reusable.** I confirmed it exists, targets GLES 3, and is offered upstream, but I read its README rather than diffing its renderer changes against upstream. **To determine:** clone the fork and diff `platform/native_renderer.c`.
7. **The legal weight of the pre-license contributor question.** Out of my competence. Facts are in section 4.9; the judgment is counsel's.
8. **Whether the rumored CTR Nitro-Fueled PC port is real.** Reported by trade press citing an anonymous source, not confirmed by Microsoft or Activision. It affects the exposure assessment materially. **To determine:** watch for an official announcement.
9. **Whether the game is playable end-to-end today at beta-7.1.** I read the source and the release notes but did not build or run it. The issue tracker contains active crash reports, including an unanswered one from three days ago. A port inherits whatever correctness the base has. **To determine:** build and play it, which is phase 1.

---

## 7. Contacts and links

**Primary repositories, all cloned to `ref/`:**

- https://github.com/CTR-tools/ctr-native: the port. GPL-3.0, 278 stars, 20 forks, created 2026-05-16. Assessed at `2df55dc5a`.
- https://github.com/CTR-tools/CTR-ModSDK: the decompilation it builds on. GPL-3.0 since 2026-06-25. 5,480 commits.
- https://github.com/CTR-tools/CTR-in-C: the disassembly and decompilation research repo. **No LICENSE file.** Contains the byte-matching track.
- https://github.com/NyperYuhgard/CTR-PC-Port: a detached copy adding netplay, not a GitHub fork. No LICENSE file despite deriving from GPLv3 code. Not a recommended base.

**The most important fork for this project:**

- https://github.com/Simon358/ctr-native-android: the only active mobile effort. Branch `feature/add-android-support` contains a working GLES 3 renderer path for this exact codebase. Builds 32-bit `armeabi-v7a`, which does not transfer to iOS, but the renderer work does.

**Issues and PRs that matter:**

- https://github.com/CTR-tools/ctr-native/issues/20: "Mac Port?" Maintainer declined official builds; the Apple Silicon build failure is documented in the thread. Unanswered since 2026-06-29.
- https://github.com/CTR-tools/ctr-native/pull/41: Android GLES plus 32-bit build support. Open, unreviewed 19 days. The state of this PR is a live signal about upstream's appetite.
- https://github.com/CTR-tools/ctr-native/pull/6: the scratchpad-address PR the maintainer closed and then implemented himself in Beta 6. Useful precedent for how to contribute here.
- https://github.com/CTR-tools/ctr-native/pull/5: merged portability PR. Evidence that portability work is welcome.
- https://github.com/CTR-tools/ctr-native/issues/25: Android request, deflected toward emulation.

**People:**

- https://github.com/aalhendi: day-to-day maintainer, 1,152 commits, author of all nine releases. **The person to talk to.**
- https://github.com/DCxDemo: org owner and founder, inferred rather than API-confirmed.
- https://github.com/Rinnegatamante: experienced Vita and homebrew porter who filed the early portability PRs. A knowledgeable ally on exactly this class of work.

**Project surfaces:**

- https://ctr-tools.github.io: the org website. No roadmap, no vision statement, no platform commitments.
- https://discord.gg/WHkuh2n: the project Discord. **Not joined per instructions.** The widget API at `https://discord.com/api/guilds/527135227546435584/widget.json` is enabled but exposes only server name and presence, with an empty channel list, so no content is readable without joining. Almost certainly where the real technical discussion lives.
- https://github.com/CTR-tools/ctr-native/releases: all nine releases. Every one is Windows x86 and Linux x86 only, and every one states an OpenGL 3.3 requirement.

**Upstream dependencies:**

- https://github.com/libsdl-org/SDL: SDL3, vendored at 3.4.10 as full source with iOS backends present.
- https://github.com/libsdl-org/SDL/blob/main/docs/README-ios.md: iOS support: Metal and OpenGL ES, CoreAudio, native touch, MFi controllers, sandboxed filesystem via `SDL_GetUserFolder` and `SDL_GetPrefPath`, single fullscreen window only, iOS 11.0 minimum, and `SDL_main` implemented inline in the header.
- https://github.com/libsdl-org/SDL/blob/main/docs/README-cmake.md: confirms CMake iOS support via `-DCMAKE_SYSTEM_NAME=iOS`, with `-DCMAKE_OSX_SYSROOT=iphoneos` and `-DCMAKE_OSX_ARCHITECTURES=arm64`, plus simulator targets.
- https://github.com/OpenDriver2/PsyCross: MIT-licensed origin of the platform layer, PsyQ facade, GTE core, and renderer.
- https://github.com/google/angle: upstream ANGLE. Note it exposes GLES, not desktop GL, and has no iOS plans.
- https://github.com/kakashidinho/metalangle: MetalANGLE, the community GLES-to-Metal fork that does cover iOS. Insurance option, not a first move.
