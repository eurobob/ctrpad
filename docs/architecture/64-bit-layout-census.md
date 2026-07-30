# 64-Bit Layout and Pointer Census

**Baseline source:** `868a4e308a1c80222948938efe6b8f9ab86aee73`

**Status:** M2 census complete; migration is assigned by storage owner

## Reproduction

Run:

```sh
tools/audit-64bit-memory-model.sh
```

The command uses DWARF from the clean i686 unity object to follow typedefs,
arrays, anonymous aggregates, and embedded structures. It then forces the same
unity source through the pinned container's compiler as LP64, without changing
the source gate or accepting the build, to collect the actual narrowing and
layout failures.

Generated evidence is ignored build output under `build-64bit-audit/`. The
accepted census has:

```text
i686 object:
7ebc25257acc868616a1616d8a1b4479068b6c9ccb27deb7fc2d703db72c357b

layout-census.tsv:
c1d77ab5ba9bb2f488754a29a5e3c2c06131d0d3343de79aca9ec8fa1d535550
```

## Measured surface

| Measurement | Count |
|---|---:|
| Types named by a `sizeof`/`offsetof` layout assertion | 283 |
| Pointer-bearing pinned root definitions | 104 |
| Root/field storage contexts with a native pointer | 939 |
| Serialized-file contexts | 50 |
| Scratchpad contexts | 50 |
| Resident executable/overlay-map contexts | 575 |
| Runtime-only host contexts | 264 |
| Actual `CTR_SCRATCHPAD_PTR(struct/union ...)` overlay types | 28 |
| `Ptr32` references | 102 |
| Unique pointer-to-integer warning coordinates under LP64 | 162 |
| Unique integer-to-pointer warning coordinates under LP64 | 121 |
| Unique source lines containing either narrowing direction | 203 |
| LP64 static-layout assertion failures | 673 |

The earlier viability estimate of roughly 75 pointer-bearing pinned structs
was directionally correct but low. The DWARF walk finds 104 definitions and
939 distinct storage contexts after expanding embedded maps such as `sData`,
`Data`, and `GameTracker`.

`build-64bit-audit/layout-census.tsv` is the field-level inventory. Every row
contains:

- the pinned root and definition;
- the complete embedded field path;
- the type that directly declares the pointer;
- the field declaration;
- its storage owner; and
- its migration decision.

The generator fails to emit a pointer-bearing root only if it is not named by
a checked layout expression, so additions remain auditable rather than relying
on a hand-maintained count.

## Ownership and migration decisions

The field-level TSV applies these rules:

| Storage owner | Exact scope | Migration decision |
|---|---|---|
| `serialized_file` | `AnimTex`, `BSP`, `Level`, `Model`, `ModelAnim`, `ModelHeader`, `NavHeader`, `QuadBlock`, `SCVert`, `Skybox`, and `SpawnType2` when they are the pinned root | Replace native pointer slots with checked `GuestRef32`; retain offsets and sizes |
| `scratchpad_guest` | A pinned root directly overlaid through `CTR_SCRATCHPAD_PTR` | Move host pointers to a sidecar or host-width local; retain the scratch byte image |
| `resident_map` | `rData`, `Data`, `sData`, `GameTracker`, fixed overlay data/RDATA maps, garage state, credits state, and video BSS | Use a native host-pointer view or sidecar; retain a separate guest schema only where raw retail offsets are consumed |
| `runtime_host` | Every remaining pointer-bearing pinned root | Widen to a native pointer and make its layout assertion native-only or move the assertion to a guest schema |

The 28 directly observed scratchpad overlay types are:

```text
AHSignScratch
CSThreadParentFrameScratch
CameraScratchWork
CsThreadInitData
DisplayBlurTile
DrawConfettiScratch
DrawLevelOvr1PScratchVertex
DrawLevelOvr1PStableScratch
DrawSkyScratch
MainRenderLevelGeometryScratch
MaskHeadScratch
ParticleRenderListScratch
PushBufferSetMatrixVPScratch
RBBannerScratchVertex
RBDefaultScratch
RaceFlagScratch
RedBeakerRainScratch
RenderBucketExecuteScratch
RenderBucketPackedVertex
RenderListsScratchRecord
RenderWeatherScratch
ScratchpadFrustum
ScratchpadStruct
TorchScratch
VehEmitterWallScratch
VehGroundShadowScratch
VehGroundSkidsScratch
VehWarpDustScratch
```

## Integer fields carrying pointer-like values

DWARF correctly reports C pointer fields, but integer fields need a separate
semantic decision:

| Integer field family | Owner | Decision |
|---|---|---|
| `ModelHeader.ptrCommandList` | serialized model asset | `GuestRef32` |
| `InstDrawPerPlayer.ptrCommandList`, `ptrColorLayout`, `ptrDeltaArray` | runtime render work | native pointers |
| `sData.ptrMPK`, `ptrPushBufferUI`, `ptrFruitDisp`, `ptrLoadSaveObj` | resident runtime state | native pointers |
| `sData.howlChainParams[0..1]` | resident runtime state | split the pointer parameters from integer sector/count parameters |
| `Data.voiceSetPtr[]` | resident initialized data | native pointers |
| `GameTracker.DecalMP[].ptrOT1/ptrOT2` | resident runtime state | native pointers |
| `GameTracker.ptrCircle`, `ptrClod`, `ptrDustpuff`, `ptrSmoking`, `ptrSparkle`, `mpkIcons` | resident pointers into loaded assets | `GuestRef32` or typed resolved handles; never host addresses in `u32` |
| 15 named `Ptr32` fields, 25 scalar slots after array expansion | scratchpad overlays | sidecar/local host pointers; retail scratch words stay four bytes |
| overlay bucket/setup/handler addresses, menu jump tables, and retail opcode labels | opaque retail code labels | retain `u32`; dispatch through label translation |
| SPU addresses, CD sector addresses, GPU tokens, colors, and packed flags | device/guest numeric values | retain fixed-width integers |

This supplement prevents a misleading rename-only pass: names containing
`ptr` are not automatically host pointers, and fields without `ptr` in the
name can still contain one.

## Acceptance link

The real-data proof is implemented by
`tools/prototype-guest-references.c` and wrapped by
`tools/verify-guest-reference-prototype.sh`. It uses the M1 report rather than
committing retail-derived bytes. The accepted run:

- allocated the copied MPAK at a native address above `0xffffffff`;
- traversed a real `Level`, `mesh_info`, `QuadBlock`, and texture reference;
- produced the same target for tagged-offset and bounded-arena prototypes;
- verified that each traversed ptrmap field was in the recorded relocation
  inventory; and
- rejected corrupt offset, guest-address, and captured-pointer cases with
  bounded diagnostics.

The design choice and migration invariants are recorded in
`docs/architecture/ADR-0001-guest-references.md`.
