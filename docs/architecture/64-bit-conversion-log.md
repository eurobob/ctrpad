# 64-Bit Conversion Log

This log records measured reductions against the M2 census. A reduction in
compiler warnings is progress, not parity acceptance; each batch must still
pass the i686 tests and the retail golden replay before it is accepted.

## 2026-07-29 — Guest-reference foundation and first runtime batch

**Source base:** `868a4e308a1c80222948938efe6b8f9ab86aee73`

**Status:** implementation validated by forced-LP64 compilation and six
media-free tests; retail replay validation pending

The batch adds the checked eight-bit-tag/24-bit-offset guest-reference
registry selected in `docs/architecture/ADR-0001-guest-references.md`. The
runtime-only conversions then:

- preserve function-pointer width for VBlank and thread callbacks;
- return native pointers from JitPool instead of passing them through `int`;
- use byte-pointer arithmetic for flexible-array payloads, matrix byte
  strides, process objects, lists, primitive memory, OT memory, and MEMPACK;
- carry render-list and OT addresses as native pointers through their call
  boundaries; and
- retain packed low-bit visibility-list tags in `uintptr_t`;
- replace the burst/blowup thread objects' integer pointer slots with native
  pointer arrays while preserving their exact 12-byte i686 allocation; and
- align MEMPACK and JitPool allocations to native pointer width (still four
  bytes on i686, eight bytes on ARM64);
- keep the SPU sample start address in `ChannelAttr` as an explicit 32-bit
  device address while widening HOWL host pointers and its deferred-load
  parameter block; and
- widen the resident voice-set table and three native UI object slots to
  typed host pointers; and
- widen `InstDrawPerPlayer` command, color, animation-delta, and OT range
  values to typed native pointers, with DrawTires carrying its OT pointers in
  a host-width local sidecar instead of widening the preserved retail scratch
  image; and
- give the runtime-owned `CameraDC`, `AudioMeta`, primitive-memory,
  ordering-table-memory, and gamepad structures separate, explicit ILP32 and
  LP64 host-layout contracts; and
- apply `-msse` only to x86 targets and enable compiler-checked
  `-fno-strict-aliasing` and `-fwrapv` semantics.

The intentionally deferred sites include serialized ptrmap assets, the
scratchpad overlays, checkpoint fields that still encode 32-bit native
addresses, and resident integer-pointer fields whose widening changes the
executable data maps.

### Measured compiler reduction

The same forced-LP64 unity compile used by
`tools/audit-64bit-memory-model.sh` produced:

| Diagnostic | M2 baseline | After batch | Removed |
|---|---:|---:|---:|
| Pointer-to-integer warning coordinates | 162 | 0 | 162 |
| Integer-to-pointer warning coordinates | 121 | 0 | 121 |
| Unique source lines with either narrowing direction | 203 | 0 | 203 |
| Static-layout assertion failures | 673 | 610 | 63 |

No serialized or scratch layout assertion was weakened or hidden. One fewer
forced-LP64 failure is `sizeof(struct ChannelAttr) == 0x10`: explicitly
modeling `spuStartAddr` as a four-byte device address makes that asserted
device layout correct on both ILP32 and LP64 hosts. Another 22 failures were
fixed-offset/size checks for the runtime-only `InstDrawPerPlayer`; those
checks remain active on 32-bit hosts, while host-width member assertions
replace them on LP64. The next 40 were fixed retail offsets for six
runtime-owned structure families. They now retain their exact ILP32 contracts
and assert their calculated eight-byte-pointer offsets and sizes on LP64.

### Validation

The changes were built in the pinned amd64-container/i686 environment using an
isolated copy of the baseline build metadata. The baseline executable and the
active golden report were not modified. All six tests passed:

```text
ctr_native_version       passed
ctr_native_state_digest  passed
ctr_native_replay_gate   passed
ctr_native_guest_ref     passed
ctr_native_asset_relocation passed
ctr_native_input         passed
```

The reconfigured i686 compile command contains `-msse`,
`-fno-strict-aliasing`, and `-fwrapv`. The strict-aliasing diagnostics
previously emitted by GCC disappear under the selected aliasing contract; the
two existing format-security and two maybe-uninitialized diagnostics remain
visible.

The ARM64 host compiler also accepts `platform/native_guest_ref.c` as a strict
C17 object with `-Wall -Wextra -Werror -pedantic`, as well as the atomic M4
asset-relocation boundary documented in
`docs/architecture/asset-relocation-boundary.md`. The real-asset prototype
continues to traverse the captured frame-1800 Roo's Tubes MPAK at a host
address above `UINT32_MAX` and rejects all four corrupt-reference probes.

The first strict playback of the clean golden report exposed an older replay
input omission at frame 22,392: native Enter reached the name-entry shortcut
through SDL but was not serialized in the pad snapshot. The exact diagnosis
is `docs/parity/2026-07-29-golden-run-result.md`. Rebuilding this batch and
bypassing replay identity is not a substitute because checkpoint callback
pointers still assume stable code offsets. A state-preserving input migration
then identified unrecorded between-frame loading VBlanks at frame 367. A fresh
recording seeded only by the validated pad trace and memcard seed is required;
this batch is not described as parity-preserving until that report passes the
two unchanged playbacks and deliberate mutation.

## 2026-07-30 — Expanded runtime-layout contracts

**Source base:** `868a4e308a1c80222948938efe6b8f9ab86aee73`

**Status:** implementation validated by the forced-LP64 audit and six
media-free i686 tests; retail replay validation pending

This follow-on batch preserves the 610-failure result above as the first
measured checkpoint. It converts an additional 139 runtime-only assertions
without conditionalizing a serialized-file or scratchpad-guest contract.

The converted runtime families now include:

- `Item`, `LinkedList`, and `GhostPacket`;
- `VoicelineItem`, `ChannelStats`, `SongSeq`, and `Song`;
- `JitPool`, `LoadQueueSlot`, and `Mempack`;
- `Thread`, `BucketSearch`, `DriverCollision`, and `ThreadBucket`;
- `RectMenu`, `Particle`, `Oscillator`, `Emitter`, and `PushBuffer`;
- `Instance` and the runtime profile load/save objects;
- runtime draw-level render lists, `QuipMeta`, `BotData`, `VisMem`, and
  `VisMemBspListNode`;
- overlay 231 weapon/runtime objects (`MaskHeadWeapon`, `TrackerWeapon`,
  `RainLocal`, `Cloud`, `Shield`, `WeaponSlot`, `MineWeapon`, `Baron`,
  `Follower`, `Fruit`, and `Spider`);
- overlay 232 runtime objects (`BossGarageDoor`, `WoodDoor`,
  `AHPauseMember`, `PauseObject`, and `SaveObj`); and
- overlay 233 runtime objects (`CsParticleConfig` and
  `Ovr233InitMatrixTableEntry`).

Each structure retains its exact ILP32 offsets and size. On LP64, the
assertions state the calculated native-pointer offsets and host size rather
than pretending the runtime object is still a retail byte image. Apple
Clang's record-layout dump was used to verify the calculated LP64 offsets.

### Measured compiler reduction

Reproduction:

```sh
tools/audit-64bit-memory-model.sh
```

Result:

| Diagnostic | M2 baseline | First checkpoint | Current | Removed overall |
|---|---:|---:|---:|---:|
| Pointer-to-integer warning coordinates | 162 | 0 | 0 | 162 |
| Integer-to-pointer warning coordinates | 121 | 0 | 0 | 121 |
| Unique source lines with either narrowing direction | 203 | 0 | 0 | 203 |
| Static-layout assertion failures | 673 | 610 | 471 | 202 |

The census reports:

```text
pinned-types=288
pointer-bearing-root-definitions=104
field-contexts=939
resident_map=575
runtime_host=264
scratchpad_guest=50
serialized_file=50
```

The audit exits nonzero at the forced LP64 unity compile by design while any
layout assertions remain. The measured 471 failures are the next work queue,
not an accepted ARM64 build.

### Validation

The exact current worktree was checked with:

```sh
docker exec ctrpad-i686-debug sh -lc \
  'cmake --build /out --parallel 8 && \
   ctest --test-dir /out --output-on-failure'
```

The first invocation rebuilt and linked the unity executable. A second
invocation reported `ninja: no work to do`, proving the build tree matched the
current source, and all six tests passed in 2.46 seconds:

```text
ctr_native_version           passed
ctr_native_state_digest      passed
ctr_native_replay_gate       passed
ctr_native_guest_ref         passed
ctr_native_asset_relocation  passed
ctr_native_input             passed
```

The remaining failures are dominated by serialized navigation/model/level
data, fixed scratchpad overlays, the large runtime `Driver` layout, and
resident `Data`/`sData`/`GameTracker` maps. Those groups require their assigned
guest-reference, sidecar, or host/guest split. They must not be cleared by
blanket assertion suppression.

## 2026-07-30 — Menus, metadata, and render work objects

**Source base:** `a40a7584c5761ce24b54bbb9280058a1d84b9dd5`

**Status:** implementation validated by the forced-LP64 audit and six
media-free i686 tests; retail replay validation pending

The remaining failure list was grouped by asserted type and cross-checked
against `layout-census.tsv` before editing. This batch contains only
runtime-host structures:

- `Title` and `MainMenuCheatCode`;
- `WarpPad`;
- `Terrain` and `Scrub`;
- `MetaDataLEV` and `MetaDataMODEL`;
- `RenderBucketEntry`, plus duplicate renderer-side contracts for `Item`,
  `Thread`, and `CameraDC`;
- the renderer's observed `Driver.driverID` dependency; and
- `DecalMPEntry`.

Apple Clang's record-layout output supplied the LP64 offsets. The code now
asserts the original ILP32 contracts and the actual LP64 native contracts.
No serialized-field or scratchpad assertion was made conditional.

### Measured compiler reduction

```text
forced LP64 static-layout assertion failures: 471 -> 435
removed in this batch:                         36
removed from M2 baseline:                     238
pointer-to-integer coordinates:                 0
integer-to-pointer coordinates:                 0
unique narrowing source lines:                  0
```

The post-edit residual list contains none of the converted types. It is
concentrated in `Driver`, resident overlay maps, serialized model/level
structures, and scratchpad structures. This confirms that the count did not
fall by accidentally disabling a broader assertion block.

### Validation

```sh
tools/audit-64bit-memory-model.sh

docker exec ctrpad-i686-debug sh -lc \
  'cmake --build /out --parallel 8 && \
   ctest --test-dir /out --output-on-failure'
```

The pinned i686 unity executable rebuilt and linked. All six tests passed in
1.75 seconds:

```text
ctr_native_version           passed
ctr_native_state_digest      passed
ctr_native_replay_gate       passed
ctr_native_guest_ref         passed
ctr_native_asset_relocation  passed
ctr_native_input             passed
```

The four pre-existing compiler warnings—two format-security and two
maybe-uninitialized diagnostics—remain visible and unchanged.

## 2026-07-30 — Driver layout and large-pool capacity

**Source base:** `a40a7584c5761ce24b54bbb9280058a1d84b9dd5`

**Status:** implementation validated by the forced-LP64 audit and six
media-free i686 tests; retail replay validation pending

`Driver` accounted for 99 forced-LP64 failures. This batch audited every
allocation and clear before changing the assertions. The audit found four
distinct retail assumptions:

- player and bot thread flags requested the `0x62c` non-ghost prefix;
- player and bot initialization cleared that same prefix;
- ghost initialization cleared the full `0x638` i686 object; and
- the large-stack pool item was `0x670`, leaving `0x668` object bytes after
  its eight-byte i686 free-list header.

The LP64 `Driver` is `0x6e8` bytes. Leaving the pool unchanged would therefore
provide an object region 128 bytes smaller than the structure and corrupt the
following pool item.

### Size model

The source now names the three different concepts instead of reusing a retail
magic number:

| Concept | ILP32 | LP64 |
|---|---:|---:|
| Race object prefix through `ghostTape` | `0x62c` | `0x6d8` |
| Full ghost-capable `Driver` | `0x638` | `0x6e8` |
| Large-stack pool item, including list header and `0x30` spare | `0x670` | `0x728` |

Player and bot creation request and clear `DRIVER_RACE_OBJECT_SIZE`. Ghost
initialization clears `DRIVER_GHOST_OBJECT_SIZE`. The LP64 ghost request now
validates the full object size; the i686 request remains retail value `4` so
the stored thread flags and gameplay-visible i686 behavior remain unchanged.
`MainInit_JitPoolsReset` sizes the large pool with
`DRIVER_LARGE_STACK_ITEM_SIZE`.

The LP64 assertion block records every prior retail anchor plus the widened
pointer boundaries, full size, and pool capacity. The i686 block retains all
original offsets.

### Measured compiler reduction

```text
forced LP64 static-layout assertion failures: 435 -> 336
removed in this batch:                         99
removed from M2 baseline:                     337
pointer-to-integer coordinates:                 0
integer-to-pointer coordinates:                 0
unique narrowing source lines:                  0
```

The post-edit error list contains no `Driver` assertion. A source search also
finds no remaining raw Driver allocation/clear use of `0x62c`, `0x638`, or the
old `PROC_STACK_ITEM_SIZE(0x670)` expression.

### Validation

```sh
tools/audit-64bit-memory-model.sh

docker exec ctrpad-i686-debug sh -lc \
  'cmake --build /out --parallel 8 && \
   ctest --test-dir /out --output-on-failure'
```

The i686 unity executable rebuilt and linked. All six tests passed in 2.02
seconds. The four pre-existing compiler warnings remained unchanged.

## 2026-07-30 — Fixed-width vehicle/render scratch pointer words

**Source base:** `a40a7584c5761ce24b54bbb9280058a1d84b9dd5`

**Status:** implementation validated by the forced-LP64 audit and six
media-free i686 tests; independent retail replay recording still in progress

Four scratch layouts stored host pointers in fields whose byte positions are
part of the retail scratch ABI:

- `VehGroundShadowScratch` stored eight `Driver *` and eight `Instance *`
  values, plus a sentinel pointer;
- `VehGroundSkidsScratch` stored a `PushBuffer *`;
- `DrawTiresScratch` stored an `Icon **`; and
- `ParticleRenderListScratch` stored an ordering-table pointer.

Allowing those fields to widen would shift the following scratch words and
invalidate the retail offsets. Treating the fields as ordinary LP64 native
layout was therefore not an acceptable conversion.

### Representation

The scratch-resident fields are now named fixed `u32` pointer words:

```text
VehGroundShadowEntry.driverPtr32 / instPtr32
VehGroundShadowScratch.sentinelDriverPtr32
VehGroundSkidsScratch.pushBufferPtr32
DrawTiresScratch.wheelSpritesPtr32
ParticleRenderListScratch.otPtr32
```

On i686 they retain the exact raw 32-bit pointer word, preserving the prior
layout and observable scratch contents. On LP64 they are zero, so no truncated
host address is ever treated as meaningful. Native pointers travel separately
through function parameters or small host-width sidecars:

- ground shadow keeps the native `Driver *` values in an eight-entry local
  sidecar while entries are transformed;
- ground skids passes `PushBuffer *` through its emission call chain;
- tire rendering carries the sprite table and ordering-table ranges in
  `DrawTiresHostRanges`; and
- particle rendering keeps the native ordering table in a local variable.

The original scratch offsets and sizes remain unconditional assertions.

### Measured compiler reduction

```text
forced LP64 static-layout assertion failures: 336 -> 291
ground-shadow assertions removed:             10
ground-skids assertions removed:                9
tire-render assertions removed:                24
particle-render assertions removed:             2
removed in this batch:                          45
removed from M2 baseline:                      382
pointer-to-integer coordinates:                  0
integer-to-pointer coordinates:                  0
unique narrowing source lines:                   0
```

The remaining 291 failures are concentrated in the collision scratch layout,
resident overlay maps, and serialized model/level structures. No assertion
block was disabled to obtain the reduction.

### Validation

```sh
tools/audit-64bit-memory-model.sh

docker exec ctrpad-i686-debug sh -lc \
  'cmake --build /out --parallel 8 && \
   ctest --test-dir /out --output-on-failure'
```

The exact worktree rebuilt and linked under the pinned i686 environment. All
six tests passed in 2.27 seconds. The two existing format-security warnings
and two existing maybe-uninitialized warnings remained visible and unchanged.

## 2026-07-30 — Collision scratch image and native host registry

**Source base:** `a40a7584c5761ce24b54bbb9280058a1d84b9dd5`

**Status:** implementation validated by the forced-LP64 audit, seven
media-free i686 tests, and a dedicated collision-scratch self-test;
independent retail replay recording still in progress

`ScratchpadStruct` is used at the retail scratch address, inside the resident
map, and in a small number of scratch-work wrappers. Its fixed `0x20c` image
contained pointers to threads, callbacks, mesh data, BSP hitboxes, quadblocks,
level vertices, temporary search vertices, and a 15-entry scrub history. The
extended `0x2f8` image adds 15 pointer-bearing scrub records.

Widening any of these fields changes downstream collision offsets. Simply
accepting an LP64 native layout was therefore rejected.

### Representation and synchronization

All pointer-bearing words in the fixed collision image are now explicit
`u32` fields. The live values reside in `CollScratchHost`, including:

- the active thread and collision callback;
- both mesh pointers and the current BSP hitbox;
- candidate and accepted quadblocks;
- 15 hitbox-history entries;
- accepted level/search triangle vertices;
- nine scratch-vertex-to-level-vertex links; and
- 15 scrub-record quadblocks.

`COLL_Scratch_GetHost` resolves a sidecar by `ScratchpadStruct *`. The
registry is a fixed 64-slot, no-allocation least-recently-used table, suitable
for the single-threaded collision execution model and the small stable set of
scratch/resident addresses. Every producer uses a synchronized setter. On
i686 the setter also writes the exact prior raw pointer or function-pointer
word into the guest image. On LP64 it writes zero, so gameplay never recovers
a native address from a truncated scratch word.

The original `BspSearchVertex`, `BspSearchTriangle`, `BspSearchResult`,
`CollLevelTriangle`, `CollBspSearchTriangle`, `ScratchpadStruct`, and
`ScratchpadStructExtended` offsets and sizes remain unconditional assertions.

### Measured compiler reduction

```text
forced LP64 static-layout assertion failures: 291 -> 235
removed in this batch:                          56
removed from M2 baseline:                      438
pointer-to-integer coordinates:                  0
integer-to-pointer coordinates:                  0
unique narrowing source lines:                   0
```

The post-edit forced-LP64 stderr contains no error for any converted
collision-scratch type and no non-assert compiler error.

### Dedicated regression test

`--self-test-collision-scratch` now checks:

- fixed sizes `0x20c` and `0x2f8`;
- synchronized object and function-pointer words;
- host-width recovery for every sidecar category;
- maximum valid array indices; and
- isolation between two scratch owners.

Its i686 output is:

```text
[CTR CollScratch] self-test passed: guest-size=0x20c extended=0x2f8 pointer-word=raw32 owners=isolated
```

The test is registered as `ctr_native_collision_scratch`, raising the
media-free suite from six to seven tests.

### Validation

```sh
tools/audit-64bit-memory-model.sh

docker exec ctrpad-i686-debug sh -lc \
  'cmake --build /out --parallel 8 && \
   ctest --test-dir /out --output-on-failure'

docker exec ctrpad-i686-debug \
  /out/ctr_native --self-test-collision-scratch

git diff --check
```

The exact unity executable rebuilt and linked. All seven tests passed in 0.99
seconds. The two existing format-security warnings and two existing
maybe-uninitialized warnings remained visible and unchanged.

## 2026-07-30 — Overlay 230 resident host layout

**Source base:** `a40a7584c5761ce24b54bbb9280058a1d84b9dd5`

**Status:** implementation validated by the forced-LP64 audit and seven
media-free i686 tests; independent retail replay recording still in progress

Overlay 230 contains the main-menu/character-select resident data (`D230`) and
video decode runtime state (`V230`). These are native C objects, initialized
and directly traversed by the linked executable. They are not copied from an
on-disc serialized asset.

The native checkpoint system serializes each object using its current
`sizeof` and explicitly relocates:

- every `RectMenu` pointer;
- menu, cheat-handler, character-layout, title, and transition pointers; and
- all video input/output buffers and the active CD location pointer.

No native code addresses a `D230` or `V230` field through a retail absolute
address or byte offset. Under ADR-0001 these objects therefore use native host
pointers and an architecture-specific host layout, while the full ILP32
retail contract remains asserted for the parity build.

### Pointer arithmetic repair

`MM_Video_StartStream` formed its second VLC and output buffers by converting
the first pointer to `int`, adding a byte count, and converting back. Those two
expressions now perform byte-pointer arithmetic directly:

```text
(u8 *)base + byteOffset
```

No pointer bits pass through a 32-bit integer on LP64.

### Explicit contracts

Apple Clang record-layout output supplied the native LP64 contract:

| Resident object | ILP32 retail | LP64 native |
|---|---:|---:|
| `OverlayDATA_230` | `0x1580` | `0x1890` |
| `OVR_230_VideoBSS` | `0x88` | `0xa8` |

Every prior `OverlayDATA_230` retail anchor remains inside the ILP32 block.
The LP64 block mirrors all 67 field anchors, including each widened menu and
character-select boundary. `OVR_230_VideoBSS` records all prior scalar anchors
plus the widened buffer arrays, slice, CD locations, final pointer, and size.

### Measured compiler reduction

```text
forced LP64 static-layout assertion failures: 235 -> 167
OverlayDATA_230 assertions removed:             67
OVR_230_VideoBSS assertions removed:             1
removed in this batch:                          68
removed from M2 baseline:                      506
pointer-to-integer coordinates:                  0
integer-to-pointer coordinates:                  0
unique narrowing source lines:                   0
```

Serialized `Level`, `Model`, `QuadBlock`, and navigation structures were
deliberately left strict for the GuestRef migration.

### Validation

```sh
tools/audit-64bit-memory-model.sh

docker exec ctrpad-i686-debug sh -lc \
  'cmake --build /out --parallel 8 && \
   ctest --test-dir /out --output-on-failure'

git diff --check
```

The exact unity executable rebuilt and linked. All seven tests passed in 2.08
seconds. The two existing format-security warnings and two existing
maybe-uninitialized warnings remained visible and unchanged.

## 2026-07-30 — Overlay 232 resident host layout

**Source base:** `a40a7584c5761ce24b54bbb9280058a1d84b9dd5`

**Status:** implementation validated by the forced-LP64 audit and seven
media-free i686 tests; retail parity replay still in progress

`OverlayDATA_232` is the adventure-hub and pause-menu resident state. Before
splitting its layout assertions, all uses of `D232` and the checkpoint code
were searched. The object is linked and initialized as native C data; native
code traverses it directly, checkpoint region sizing uses the current
`sizeof(D232)`, and restore explicitly relocates both `RectMenu` objects, the
hub-item pointer array, `ptrPauseObject`, and all `PauseObject` pointers.
No native code was found loading this object from a serialized retail image or
addressing its fields through retail absolute offsets.

Under ADR-0001, `D232` therefore retains native host pointers and receives
separate, explicit architecture contracts:

| Resident object | ILP32 retail | LP64 native |
|---|---:|---:|
| `OverlayDATA_232` | `0x898` | `0xa50` |

All 13 original absolute-address anchors remain active on i686. The LP64 block
asserts the measured host offsets for the same 13 fields, including both menu
boundaries, the hub pointer table, mask state, and the widened pause-object
region. The already-established `PauseObject` contracts remain `0xe4` on
i686 and `0x158` on LP64.

### Measured compiler reduction

```text
forced LP64 static-layout assertion failures: 167 -> 154
removed in this batch:                          13
removed from M2 baseline:                      519
pointer-to-integer coordinates:                  0
integer-to-pointer coordinates:                  0
unique narrowing source lines:                   0
```

The remaining failures are intentionally concentrated in serialized
`Level`/`Model`/navigation layouts, overlay 233, resident executable-map
state, and the render-bucket queue. This batch did not relax any serialized
type.

### Validation

```sh
tools/audit-64bit-memory-model.sh

docker exec ctrpad-i686-debug sh -lc \
  'cmake --build /out --parallel 8 && \
   ctest --test-dir /out --output-on-failure'

docker exec ctrpad-i686-debug sh -lc \
  'stat -c "%y %s %n" /out/ctr_native; \
   ctest --test-dir /out --output-on-failure'

git diff --check
```

The fresh i686 binary timestamp was `2026-07-30 07:25:05.790301064 +0000`;
its size was 8,979,860 bytes. All seven tests passed against that binary in
2.32 seconds.

## 2026-07-30 — Overlay 233 source-owned and resident layouts

**Source base:** `a40a7584c5761ce24b54bbb9280058a1d84b9dd5`

**Status:** source-owned `R233`, mutable `D233`, and garage layout contracts
validated; serialized-model consumer migration remains pending

Overlay 233 was split by ownership rather than treated as one assertion
group:

- `R233` is a linked, `const`, source-owned initializer. Its opcode byte
  streams retain retail-relative branch words, but the native interpreter
  translates those words through the explicit
  `CS_OVR233_TranslateRetailOpcodePointer` field/range table.
- `D233` is mutable native state copied from `R233`; its checkpoint capture
  uses the current host `sizeof`, omits the derived matrix-table pointers, and
  reconstructs them after restore.
- `gGarage` is linked native menu state. Checkpoints size it with the current
  host layout and relocate its `RectMenu` pointers.
- `creditsBSS` and `CreditsObj` remain outside this batch because they embed
  serialized `Model` and `ModelHeader` copies. Their remaining assertions
  will be handled with the model GuestRef conversion rather than hidden by a
  transient host-layout split.

The checkpoint region list does not serialize or restore `R233`; it uses the
current `R233` address range solely to relocate live pointers into its
source-owned scripts. No consumer was found indexing the native object by a
retail absolute field offset.

### Explicit contracts

Apple Clang record layouts supplied:

| Object | ILP32 retail | LP64 native |
|---|---:|---:|
| `OverlayRDATA_233` | `0xbd90` | `0xc340` |
| `OverlayDATA_233` | `0x1818` | `0x1840` |
| `OVR233_Garage` | `0x00ac` | `0x00c0` |

The i686 block retains every existing retail field/array assertion. The LP64
block mirrors every `R233` and garage anchor, including the widened particle
configuration, script-pointer, matrix-table, boss-cutscene, model-pointer,
and menu boundaries. `D233` records the two model pointers, matrix data,
matrix table, and full size on both architectures.

### Measured compiler reduction

```text
forced LP64 static-layout assertion failures: 154 -> 74
removed in this batch:                          80
removed from M2 baseline:                      599
pointer-to-integer coordinates:                  0
integer-to-pointer coordinates:                  0
unique narrowing source lines:                   0
```

The 74 remaining failures are:

```text
35 overlay-233 credits state
16 serialized Level/QuadBlock/BSP/Spawn/SCVert layouts
11 resident executable-map anchors
11 serialized Model/ModelHeader/ModelAnim render assertions
 1 serialized Model layout assertion
```

### Validation

```sh
tools/audit-64bit-memory-model.sh

docker exec ctrpad-i686-debug sh -lc \
  '(cmake --build /out --parallel 8 && \
    ctest --test-dir /out --output-on-failure) \
   > /tmp/ctrpad-ovr233-resident-build.log 2>&1'

git diff --check
```

The fresh i686 executable linked at
`2026-07-30 07:37:48.860981673 +0000`, size 8,979,860 bytes. All seven tests
passed in 2.62 seconds. The two existing format-security and two existing
maybe-uninitialized warnings were unchanged.

## 2026-07-30 — Serialized model references and credits runtime headers

**Source base:** `a40a7584c5761ce24b54bbb9280058a1d84b9dd5`

**Status:** forced-LP64 compilation and all seven media-free i686 tests pass
the batch gates; retail replay comparison and an ARM64 runtime load remain
pending

`Model`, `ModelHeader`, and `ModelAnim` are serialized retail asset structures,
not native pointer-bearing runtime layouts. Their relocated fields now remain
four-byte `CtrAssetRef32` values on both ILP32 and LP64
(`include/namespace_Instance.h:316-418`). The render-time and gameplay
consumers resolve those references through range- and alignment-checked
helpers (`platform/native_asset_ref.c:96-218`) rather than casting the stored
bits to host pointers.

The migrated boundary covers:

- model header arrays and animation-reference arrays;
- frame data, texture-reference tables and entries, color tables, animation
  textures, and compressed-animation delta arrays;
- render-bucket queue setup/execution and the instance, vehicle, hub,
  cutscene, texture-cycling, banner, burst, and blowup consumers; and
- the cutscene frame and visible-LOD paths, which now reject absent or
  out-of-range model data instead of directly indexing serialized reference
  words (`game/233/CS_Instance.c:35-184`,
  `game/233/CS_Thread.c:612`).

On i686, `CtrAssetRef32.bits` continues to contain the raw 32-bit pointer
written by the existing loader. On LP64, it contains the tagged guest
reference emitted by the atomic relocation boundary. This preserves i686
behavior while making truncation impossible on LP64.

### Credits ownership exception

Credits copy up to two serialized headers into five resident, runtime-owned
model copies. Writing an eight-byte native header pointer into
`Model.headers` would corrupt the required `0x18` serialized `Model` layout.
Instead, a fixed eight-slot sidecar maps those five resident `Model` addresses
to native header arrays (`platform/native_asset_ref.c:7-94`).

Credits now:

- cap each runtime copy to the two headers actually allocated;
- install and clear the sidecar with the ghost lifecycle
  (`game/233/CS_Credits.c:76-134`);
- clear all entries when overlay-233 credits state resets
  (`game/233/D233.c:203-206`); and
- rebuild the five derived mappings after checkpoint restoration
  (`game/233/D233.c:93-101`,
  `platform/native_checkpoint.c:1733`).

Apple Clang record-layout output measured the resulting mixed
serialized/runtime credits contracts:

| Object | ILP32 retail | LP64 native |
|---|---:|---:|
| `ModelAnim` | `0x18` | `0x18` |
| `ModelHeader` | `0x40` | `0x40` |
| `Model` | `0x18` | `0x18` |
| `CreditsObj` | `0x340` | `0x380` |
| `Ovr233_Credits_BSS` | `0x374` | `0x3d0` |

The LP64 credits expansion therefore comes only from genuine runtime pointers;
the embedded serialized model arrays remain byte-for-byte retail-sized
(`include/ovr_233.h:1133-1206`).

### Test extension

The asset-relocation self-test now traverses a synthetic serialized
model/header/animation graph through the checked accessors on both pointer
widths. It separately installs, reads, clears, and rejects the cleared runtime
header override (`platform/native_asset_relocation.c:195-333`). Its accepted
output is:

```text
[CTR AssetRelocation] self-test passed: first=0x07000014 duplicate=duplicate-patch target=target-out-of-range atomic=yes model=checked override=checked
```

### Measured compiler reduction

```text
forced LP64 static-layout assertion failures: 74 -> 28
removed in this batch:                         46
removed from M2 baseline:                     645
pointer-to-integer coordinates:                 0
integer-to-pointer coordinates:                 0
unique narrowing source lines:                  0
non-layout compiler errors:                     0
```

The 28 remaining failures are exactly 17 serialized
`Level`/`NavHeader`/`QuadBlock`/`BSP`/`SpawnType2`/`SCVert` assertions and 11
resident executable/data-map assertions. There are no model or credits
failures left.

### Validation and rejected first run

The first fresh i686 build passed all seven tests in 0.91 seconds, but it added
two `-Wunused` diagnostics in the pointer-width-specific self-test. That run
was rejected as the accepted batch result. Moving the LP64-only temporary
inside its conditional and explicitly consuming the i686-only parameter
removed both warnings.

The correction rebuild used a new log and produced:

```text
binary timestamp: 2026-07-30 07:55:43.543918623 +0000
binary size:      9,001,344 bytes
binary SHA-256:   5695056d5b033a0dd5464ebb263bd7ea3cf8d18dbeb65c855f41f7766036ca98
i686 tests:       7 / 7 passed
i686 test time:   0.88 seconds
warnings:         2 format-security + 2 maybe-uninitialized (pre-existing)
```

The final forced-LP64 audit and `git diff --check` also passed their intended
gates. Retail parity remains unclaimed until the independent replay completes
and the converted LP64 runtime can consume the retail model set.

## 2026-07-30 — Serialized Level/Nav graph

**Source base:** `a40a7584c5761ce24b54bbb9280058a1d84b9dd5`

**Status:** forced-LP64 consumer compilation and the media-free i686 regression
gate accept this batch; the 11 resident-map assertions and retail ARM64
runtime validation remain open

The serialized `Level`, `mesh_info`, `QuadBlock`, `PVS`, `BSP`, `SCVert`,
`WaterVert`, `SpawnType2`, `Skybox`, `LevTexLookup`, `InstDef`, and
`NavHeader` pointer slots now remain four-byte `CtrAssetRef32` values. A
central accessor boundary resolves roots and checked arrays without widening
or rewriting the retail file image (`include/namespace_Level.h`,
`include/namespace_Instance.h`, `include/namespace_Bots.h`,
`platform/native_asset_ref.c`).

The runtime exceptions are explicit:

- `InstDef.ptrInstance` stores historical raw pointer bits on i686 and an
  instance-pool index plus one on LP64.
- The fixed `0x90` `LevelVisMemAsset` is materialized into a bounded native
  `VisMem` sidecar on LP64. Native-width BSP list nodes are allocated outside
  the retail image.
- `NavHeader.last` is derived from the inline frame array and `numPoints`; no
  host pointer is written into the serialized header.
- Visibility references preserve their low two retail tag bits. Resolution
  masks the tags, validates the aligned guest reference, then reapplies the
  tags to the native pointer value expected by the visibility walkers.

### Consumer migration inventories

The first struct-layout conversion deliberately exposed 307 non-static
compiler errors. After converting the first model/icon/spawn/navigation
group, an intermediate compiler inventory contained 213 errors. After
collision, drawing, visibility, UI, checkpoint, vehicle, hazard, warpball,
and state-digest migration, the final pre-validation audit is:

```text
forced LP64 static-layout assertion failures: 28 -> 11
removed in this batch:                         17
removed from M2 baseline:                     662
non-static compiler errors:                   307 -> 0
pointer-to-integer coordinates:                 0
integer-to-pointer coordinates:                 0
unique narrowing source lines:                  0
```

One intermediate edit introduced an unmatched parenthesis in the collision
checkpoint condition, and the potion consumer briefly omitted its
`teethInst` local. Both were compiler-detected and corrected before reaching
the zero-consumer-error audit.

### Validation and rejected first run

The first uniquely logged i686 build passed all seven tests and the new Level
accessor self-test, but added three unused-function warnings: one LP64-only
visibility helper and two obsolete DrawLevel pointer helpers. That binary
(`077682dbc443868d5251e90ae2c2c9728835aba90066508b758d8c643d42a2b2`)
was rejected.

The correction made the visibility materializer LP64-only, deleted the two
dead helpers, and fixed the raw DrawLevel texture-byte path to reconstruct a
retail PSX address from a checked guest reference. The independently logged
accepted run produced:

```text
binary timestamp: 2026-07-30 08:47:31.176164301 +0000
binary size:      9,107,604 bytes
binary SHA-256:   bf0b49f28e3e31ad46e03d35398baece79370a57285b3a483b06eceee49e4b24
i686 tests:       7 / 7 passed
i686 test time:   0.74 seconds
warnings:         2 format-security + 2 maybe-uninitialized (pre-existing)
self-test:        model=checked override=checked level=checked
```

The Level test traverses mesh, quad, BSP, checkpoint, direct/indirect texture,
packed visibility, and LP64 visibility-sidecar boundaries. Cross-process
checkpoint rebinding of derived sidecar pointers remains assigned to the
checkpoint-format work in M5. Retail parity remains unclaimed.

## 2026-07-30 — Resident Data/sData maps

**Source base:** `a40a7584c5761ce24b54bbb9280058a1d84b9dd5`

**Status:** forced-LP64 compilation and the media-free i686 regression gate
accept this batch; macOS ARM64 runtime and retail parity remain open

The last failures were resident-map anchors, not serialized contracts. Apple
Clang 21 measured the LP64 `Data` offsets for `rowsQuit`, `menuQuit`,
`playerIconAdvMap`, and `bakedGteMath` as `0x4c38`, `0x4c50`, `0x7050`, and
`0x8bb0`. The measured LP64 `sData` offsets for the six failing anchors are
`0xa28`, `0xa30`, `0xbf0`, `0xbf8`, `0xbfc`, and `0xc00`.

The existing ILP32 absolute-address assertions remain intact. LP64 now has
separate measured runtime-layout assertions. `BakedGteMathEntry` explicitly
contains a `MatrixND *` and an entry count, with eight-byte and 16-byte
contracts on ILP32 and LP64 respectively.

The same inventory exposed one unconverted `PLYROBJECTLIST` model lookup and
missing checkpoint rebasing for `bakedGteMath[].physEntry`; both are corrected
as part of this boundary.

### Measured compiler result

```text
forced LP64 static-layout assertion failures: 11 -> 0
removed in this batch:                         11
removed from M2 baseline:                     673
forced LP64 compiler errors:                    0
pointer-to-integer coordinates:                 0
integer-to-pointer coordinates:                 0
unique narrowing source lines:                  0
```

Only the two historical format-security warnings remain in the forced-LP64
diagnostic log.

### Validation

The uniquely logged i686 run produced:

```text
binary timestamp: 2026-07-30 08:54:45.852280708 +0000
binary size:      9,109,080 bytes
binary SHA-256:   93748f6826040a3d02c60a522c07163fd0927799500e65b6de3d9eab6381d085
i686 tests:       7 / 7 passed
i686 test time:   0.76 seconds
warnings:         2 format-security + 2 maybe-uninitialized (pre-existing)
self-test:        model=checked override=checked level=checked
```

This is the accepted zero-layout-failure mechanical-conversion boundary.
Native ARM64 linking and runtime parity are separate downstream gates.

## 2026-07-30 — ARM64 runtime-only stride, reference, and queue corrections

**Source base:** `a40a7584c5761ce24b54bbb9280058a1d84b9dd5`

**Status:** sanitizer and 2,000-frame retail smoke boundary accepted; full
golden parity remains open

The first real macOS ARM64 retail launch exposed three assumptions that static
layout conversion and media-free tests could not exercise:

1. The HOWL channel free-list used the retail `0x20` byte stride even though
   native `ChannelStats` widens to `0x28` on LP64. The allocator now uses the
   host type size and array count (`game/HOWL/HOWL_Voiceline.c:22-25`).
2. MPK word zero was still loaded as `LevTexLookup **`. It is a serialized
   four-byte relocated reference and now resolves as `CtrAssetRef32`
   (`game/LOAD/LOAD_TenStages.c:338-350`).
3. The render-bucket queue reused unaligned byte scratch sized for retail
   eight-byte entries. LP64 entries contain two pointers. Native now owns an
   aligned host-width queue with retail-equivalent entry capacity and writes
   a full pointer terminator (`game/MAIN/MainInit.c:3-18,286-292`;
   `game/MAIN/MainFrame_RenderFrame.c:675-708`).

These choices preserve serialized bytes and retail capacity while widening
only host-runtime storage. They also demonstrate why successful forced-LP64
compilation was a boundary rather than acceptance.

The initial normal binary exited during startup. The ASan/UBSan build then
localized the fault sequence in order:

```text
ASan SEGV:  LIST_RemoveMember -> Channel_AllocSlot (bad channel stride)
UBSan:      LOAD_TenStages.c:341 (four-byte MPK ref read as host pointer)
UBSan:      RenderBucket_QueueExecute.c:2146 (unaligned host queue)
```

After all three corrections, the sanitizer binary completed 2,000 retail
frames at 31.24 FPS without another diagnostic. The normal ARM64 binary
completed 2,000 frames at 31.25 FPS and was deliberately stopped. Its SHA-256
is `f2ac8e010ac83c5b4894c24d5f12b644a4ddcf1e33f02ff246c823c3f5ef9cc5`.
The forced-LP64 audit remains at zero compile errors, static assertions, and
narrowing coordinates. The i686 build remains 7/7 with the established four
warnings and SHA-256
`1e46e4ef61f91e518b313f4feee5d8e14b809b45d27fb98f58a47bdefac04e23`.

This does not close M3–M6: renderer-heavy scene coverage, cross-process
checkpoint rebinding, input/play evidence, persistent saves, and exact
24,232-frame state parity are still required.

## 2026-07-30 — Checkpoint format v3 and process-local pointer closure

**Source base:** `a40a7584c5761ce24b54bbb9280058a1d84b9dd5`

**Status:** ARM64 frame-zero capture and a 178-frame restore under a different
ASLR layout pass. Full rolling-checkpoint and golden-run parity remain open.

Checkpoint v2 encoded address-range starts, code anchors, and pointer values as
`u32`, so a normal ARM64 process produced `NativeCheckpoint_GetSize() == 0`.
Version 3 records `pointerSize`, uses `u64` for persisted host addresses, uses
`uintptr_t` for live slots, and validates both range arithmetic and producing
pointer width (`platform/native_checkpoint.c:19-105,390-559,2181-2265`).
Checkpoint files are architecture-specific developer artifacts; serialized
retail asset references remain four bytes.

Cross-process testing exposed nested ownership not covered by the original
explicit relocation inventory:

- gamepad packet and wheel pointers inside `GamepadSystem`;
- HOWL `Song.CseqSequences[]`, `SongSeq` note cursors, song-set bits, and the
  end-of-HOWL pointer;
- resident audio, character-name, and terrain-emitter pointers;
- process-stack object slots inside mempack; and
- the native render-bucket arena, which is rebound because it is transient
  static host storage rather than captured retail memory.

Those owners are handled at
`platform/native_checkpoint.c:707-731,1368-1501,1544-1628,1886-1888` and
`game/MAIN/MainInit.c:21-31`.

The initial v3 restore also attempted a whole-region safety invariant: after
explicit relocation and repair, it rejected any aligned native-width word
that happened to fall inside an old-process range. That raw scan safely found
real early omissions, but later exact-binary playback proved that it could
also classify scalar bytes as a pointer. The typed applied-relocation ledger
documented in the later section below supersedes the raw scan.

The validation sequence was:

```text
v2 ARM64 capture: rejected before frame 0 (checkpoint size 0)
v3 first restore: SIGSEGV in GAMEPAD_ProcessHold (nested packet pointer)
v3 second restore: SIGBUS in HOWL_Channel.c:409 (nested song pointer)
v3 safety scan: safely rejected 172 remaining stale pointers
v3 corrected restore: 178/178 replay frames, zero stale pointers, exit 0
```

The passing report is
`/tmp/ctrpad-macos-arm64-checkpoint-v3b-LnvgJP/debug/reports/20260730/ctr-044204`.
Its capture and playback `sdata` bases differ
(`0x102e3e318` versus `0x104b1a318`), proving that the result did not depend
on identical ASLR placement.

This work does not make replay-seeded fresh boots deterministic. Replay
version 2 records VBlank packets only inside tracked frames and omits loading
VBlanks between them. The visibly stuck 1,900-frame ARM64 run was rejected;
its timing and root digests differed from the i686 source at frame zero.

## 2026-07-30 — Runtime-capacity and retail-offset closure through first race

**Source base:** `a40a7584c5761ce24b54bbb9280058a1d84b9dd5`

**Status:** the ARM64 runtime now crosses intro skip, race-load allocation,
visibility materialization, and the first 489 Roo's Tubes race frames. Full
cross-architecture state parity remains open.

### Host object capacity without gameplay-memory inflation

Retail small/medium process-stack slots could not contain widened LP64
objects. The first visible symptom was a missing `CutsceneObj`: process birth
failed, no cutscene thread existed, and valid Start input at frame 307 had no
consumer. Native LP64 stack strides are now derived from the largest supported
host object while preserving retail item counts. Compile-time fit assertions
cover all known users (`game/MAIN/MainInit.c:24-72`).

Wider stack, thread, instance, driver, particle, and rain records add at most
`0x9ec0` bytes at the four-player maximum. Native retail-pressure memory now
opens exactly that much prefix space while retaining the historical arena
end. `MainInit` reserves the unused portion for smaller configurations, so:

```text
actual LP64 pool overhead + explicit unused reservation = 0x9ec0
remaining gameplay allocation budget = retail-equivalent
native physical backing = unchanged 0x200000
```

The bound and compile-time formula are
`include/platform/native_memory.h:11` and
`game/MAIN/MainInit.c:74-135`; arena geometry is
`platform/native_memory.c:28-29`.

### Visibility sidecar preserves optional retail player slots

`LevelVisMemAsset` is a fixed four-player table, but one-player LEVs leave
inactive slots null. The first LP64 materializer treated null inactive
destinations as corruption and rejected the entire sidecar. It now:

- resolves every non-null serialized reference through the guest-ref boundary;
- preserves null inactive slots;
- logs guest status, region, offset, list kind, player, counts, and context on
  a populated-slot failure;
- validates all destination/BSP lists required by the active player count in
  `MainInit`; and
- prevents level rendering from dereferencing an absent sidecar.

The implementation is
`platform/native_asset_ref.c:1068-1195`,
`game/MAIN/MainInit.c:152-244`, and
`game/MAIN/MainFrame_RenderFrame.c:863-883`.

### Serialized byte offsets are also an ABI boundary

Pointer conversion was not the only LP64 hazard. The 65-entry `MetaPhys`
table stores literal NTSC-U byte offsets into the retail `Driver`. Applying
those offsets to the widened host struct wrote the complete physics constant
block `0x68` bytes before its native location. This left the actual gravity,
speed, handling, turbo, and collision constants uninitialized and produced an
explicit divide trap when two zero collision weights were combined.

`VehBirth_ConstDestination` translates each retail offset relative to the
named native `Driver.const_Gravity` anchor. The block's internal relative
layout is compile-time asserted through `const_CollisionWeight` and
`const_prototypeKey`. The new `ctr_native_vehicle_constants` test applies all
65 values for all four engine classes and checks offset bounds on both ILP32
and LP64 (`game/Vehicle/VehBirth.c:580-716`,
`CMakeLists.txt:191-194`).

This establishes a second conversion rule:

> A serialized field offset is guest ABI data even when it contains no
> pointer. Never apply it directly to a widened host object; translate it at
> a named layout boundary.

The same audit found raw retail driver offsets in `UI_VsQuipReadDriver`.
Inspection of the 51 real NTSC-U VS/battle metadata records showed that its
46 generic reads target three compact scalar ranges: `0x4f0..0x4f7`,
`0x514..0x56b`, and `0x574..0x57f`. The reader now translates those ranges
from retail anchors to the named native `quip1`, `timeElapsedInRace`, and
`NumMissilesComparedToNumAttacks` anchors, accepts only the recorded
one-/two-/four-byte widths, and bounds-checks each access. The one eight-byte
attacked-player record remains on its existing named-field path. Compile-time
range-layout assertions and `ctr_native_vs_quip_offsets`, which walks all 51
records and resolves all 46 generic readers, cover the conversion on ILP32
and LP64 (`game/UI/UI_VsQuip.c:3-293`, `CMakeLists.txt:195-198`).

This closes the known raw-offset consumer from the source audit. Actual
multiplayer end-of-race/VS execution remains a runtime coverage requirement.

### Host render-list offsets must follow native pointer width

The level render-list walkers were another non-serialized offset boundary.
They selected `bspListStart` as `slot * 8 + 4` and the full-dynamic head as
`0x28`, which are the ILP32 positions. Native LP64 slots are 16 bytes,
`bspListStart` is eight bytes into each slot, and the full-dynamic head is at
`0x50`. The old unaligned pointer stores could straddle two native fields even
though the ARM64 CPU allowed the game loop to continue.

`RenderLists_GetHead` now returns the named native members for all five normal
slots and the full-dynamic slot, with explicit bounds checks. Both traversal
paths use it. The `ctr_native_render_lists` test writes through all six
resolved addresses and reports the expected `4/8/0x28` i686 geometry or
`8/16/0x50` LP64 geometry
(`game/RenderLevel/RenderLists.c:160-426`, `CMakeLists.txt:199-202`).

This is distinct from translating serialized ABI metadata: render-list
storage is native host state, so it must use native typed fields and must not
retain retail byte arithmetic at all.

### Composite instance/draw-record offsets require semantic translation

Red-beaker rain treated an `Instance` followed by its per-player
`InstDrawPerPlayer` records as one retail byte array. Its
`playerIndex * 0x88 + 0x8c/0x90/0x94` reads select each retail draw record's
MVP translation, but on LP64 the `Instance` and draw records widen to `0xa0`
and `0xb0`. The same code then read signed byte `+0x50`, which aliases
player-zero `Instance.depthBiasNormal` and, after each retail stride, the
preceding draw record's `lodIndex` low byte.

Native now resolves the current draw record with `INST_GETIDPP`, reads
`mvp.t[]` by name with retail halfword truncation, and expresses the depth/
LOD alias with named fields. Bounds checks cover all player indices.
`ctr_native_red_beaker_layout` validates all four record addresses, transform
triplets, and alias values on both `0x74/0x88` ILP32 and `0xa0/0xb0` LP64
layouts (`game/RenderWeather/RedBeaker_RenderRain.c:20-356`,
`CMakeLists.txt`).

The existing input prefix does not activate the item-effect path, so this is
structural rather than runtime-effect coverage. A post-change full segment
still verified no covered-path regression.

### Validation boundary

The final report
`/tmp/ctrpad-arm64-input-diag-VYgBb3/debug/reports/20260730/ctr-060443`
completed 2,200 frames, entered the race at frame 1711, captured eight rolling
checkpoints, and exited 0. It matches the preceding ARM64 report for all 2,200
frames in all six digest components. macOS ARM64 and fresh isolated i686
builds each pass 11/11 tests. The forced-LP64 audit also remains at zero
compiler errors, static-assert failures, and pointer-narrowing coordinates.

The i686 source and ARM64 coverage report still do not match timing/root state
because the current input-only fresh-boot automation does not reproduce
asynchronous loading VBlanks. M6 parity therefore remains open despite the
accepted runtime-capacity and physics-initialization fixes.

## 2026-07-30 — Sanitizer-discovered packed-word/host-pointer boundaries

**Source base:** `a40a7584c5761ce24b54bbb9280058a1d84b9dd5`

**Status:** the covered 2,200-frame ARM64 path passes combined ASan/UBSan;
full renderer, multiplayer, and game parity remain open.

A clean ordinary run concealed four additional LP64 violations. Fresh
sanitizer retries rejected them in this order:

| Report | Last recorded state | Finding |
|---|---:|---|
| `ctr-060956` | frame 0, no checkpoint | four-byte-aligned audio byte stream cast to an eight-byte-aligned snapshot |
| `ctr-061439` | frame 1,500, six checkpoints | native render-list offset `0x50 / 4` used as bucket index 20 |
| `ctr-061954` | frame 1,500, six checkpoints | water texture reconstructed from a truncated scratch `Ptr32` |
| `ctr-062437` | frame 1,500 metadata; race active at 1,711 in log | clipped primitive OT pointer reconstructed from a four-byte record |

The corrections establish four reusable rules:

1. **Serialized byte streams have no native alignment guarantee.** Audio state
   is captured/restored through aligned scratch plus `memcpy`; its on-disk
   bytes, size, and version are unchanged.
2. **Host field offsets are not canonical table indices.** Render dispatch
   walks the retail bucket identity (10..0 for 1P/2P and 7..0 for 4P), while
   each bucket's named host member provides the widened pointer.
3. **Guest scratch pointers are observability state, not LP64 storage.** Clip,
   rendered-list, visibility, and water-map cursors keep full-width host
   sidecars and continue mirroring low 32-bit values into retail scratch.
4. **A packed pointer may be relocatable without widening its record.** The
   clipped OT record is resolved as a validated low-word byte offset from the
   current viewport's full-width OT base. This preserves the four-byte retail
   record and supports unsigned low-word wrap.

The render-list self-test now proves canonical dispatch order, four sidecars,
and clip-OT rebasing on both pointer widths. The audio self-test deliberately
uses a blob with address modulo eight equal to four.

Accepted validation:

```text
ASan/UBSan report ctr-062801: finalized, 2,200 frames, 8 checkpoints, exit 0
normal report ctr-062951:     finalized, 2,200 frames, 8 checkpoints, exit 0
race activation:             frame 1,711 in both
ARM64 normal CTest:          12/12
ARM64 ASan/UBSan CTest:      12/12
i686 CTest:                  12/12, verified ELF32 i386
forced LP64:                 0 errors, 0 assertions, 0 narrowing coordinates
git diff --check:            passed
```

Compared with pre-sanitizer `ctr-060443`, current normal `ctr-062951` matches
all 2,200 RNG, driver, world, and allocation hashes. Timing/root change only
from frame 1,756 onward, where the invalid old renderer and corrected renderer
no longer execute the same path. This is evidence that simulation state stayed
stable over the covered segment; it is not a substitute for the still-open
golden parity and visual gates.

## 2026-07-30 — Exact-binary replay identity and typed relocation validation

**Source base:** `a40a7584c5761ce24b54bbb9280058a1d84b9dd5`

**Status:** an exact-binary frame-zero checkpoint restore completes the
2,200-frame input segment in a second ARM64 process. A different dirty binary
is rejected before restore. Restoring non-bootstrap rolling checkpoints and
cross-architecture parity remain open.

The human build label was not a sufficient native-state compatibility key.
Every uncommitted build used `a40a7584c576-dirty`, even though unity-build code
and data offsets changed. Replay header version 3 therefore replaces two
reserved words with a 64-bit FNV-1a fingerprint of the executable file. That
fingerprint participates in the header identity checksum and is emitted in
report metadata. The header remains 148 bytes and the frame record remains
440 bytes, but version-2 playback is deliberately rejected.

This separates two concerns:

- ASLR changes the base of one exact executable and is handled by region/code
  relocation.
- Rebuilding changes the image contents/layout and is not made safe by an
  image-base delta.

The original post-restore safety scan also needed correction. It interpreted
every pointer-width-aligned word in all captured regions as a possible native
pointer. At frame zero, SDATA offset `0x538` is the beginning of four adjacent
16-bit `RaceFlag_*` scalars. Their legitimate values concatenated to
`0x1012c1388`, an aligned address inside the producing process's mempack. The
scan rejected it even though no pointer field exists there.

Restore now records each relocation it actually applies as:

```text
slot address
recorded process value
expected relocated value
```

After native storage rebinding, pointer-map application, and resident repair,
the validator inspects those typed slots and rejects only a slot that has
reverted exactly to its recorded-process value. The ledger is bounded at
65,536 entries and overflow rejects restore. This preserves a hard stale-value
check without classifying arbitrary scalar bytes as pointers. The
`ctr_native_checkpoint_pointer_validation` test proves both sides: the
address-shaped scalar is ignored and a relocated slot changed back to its old
value is detected.

End-to-end evidence:

```text
accepted report:       ctr-065640
replay version:        3
executable fingerprint b212b34099a6dbca
record mempack base:   0x1009218b8
playback mempack base: 0x102f298b8
frames/checkpoints:    2,200 / 8
race activation:      frame 1,711
exact-binary playback: exit 0

rejected executable:   dd9fef13fb4ae04a
visible build label:   a40a7584c576-dirty (same)
rejection point:       replay header, before checkpoint restore
```

The current normal and ASan/UBSan ARM64 suites pass 13/13. A fresh isolated
`-m32` build passes 13/13 and is verified as ELF32 Intel 80386, Build ID
`f99490697baa07f778fcb5aa8faa37647ee78cbc`. The forced-LP64 audit remains at
zero errors, static-assert failures, and pointer-narrowing coordinates.

## 2026-07-30 — Rolling-checkpoint allocation, sidecar, and overlap closure

**Source base:** `a40a7584c5761ce24b54bbb9280058a1d84b9dd5`

**Status:** all eight checkpoints in the 2,200-frame ARM64 segment restore with
their exact ordinary producing binary. A repeated ASan/UBSan checkpoint-6
campaign also passes across five process layouts, including partially
overlapping recorded/live mempack ranges. Wider gameplay and parity acceptance
remain open.

Frame-zero restore did not exercise the live race allocation graph.
`--replay-start-checkpoint INDEX` was added so a validated rolling checkpoint
can restore at its mapped replay frame and seek the input stream to the same
record. The new replay-gate assertion covers the checked frame-offset
calculation.

The rolling investigation exposed these boundaries in order:

1. `PROC_BirthWithObject` removes live threads from `JitPools.thread.free` but
   does not add them to `taken`. The checkpoint walker therefore relocated no
   live `Thread.object`, instance, or callback state when it trusted the empty
   taken list. Threads are now enumerated as every fixed pool slot not present
   in the free list.
2. LP64 `VisMem` tables and model-header overrides are heap-backed,
   process-local caches derived from fixed-width level/model assets. Restore
   clears both cache systems and reconstructs `gGT->visMem1/visMem2` from the
   relocated level assets.
3. `INSTANCE_LevInitAll` has the same direct-free-list allocation behavior for
   level instances, while dynamic instances do use `taken`. The free-list
   complement is the one invariant covering both paths, so every allocated
   instance and its per-player draw records are relocated through that view.
4. `CameraDC.ptrQuadBlock` is live collision traversal state. It was added to
   the camera field map after ASan/UBSan restored frame 1,800 at a different
   mempack base and trapped in `COLL_FIXED_QUADBLK_TestTriangles`.
5. A typed slot can be reached by more than one semantic walker. When old and
   live address ranges overlap, a correctly relocated live value may still
   numerically belong to the old range. Applying relocation again corrupts the
   pointer by a second base delta. The existing 65,536-entry applied-relocation
   ledger now has a fixed open-addressed index by slot address; resident,
   image, and mixed relocation are idempotent for the restore transaction.

The media-free checkpoint test models allocated/free thread and instance
slots, the camera quad-block pointer, and two overlapping synthetic address
ranges. Its accepted suffix is:

```text
scalar=ignored relocated=reversion-checked overlap-idempotence=checked
pool-allocation=free-list-complement camera-quad=checked
```

The overlap case was not theoretical. Sanitizer report `ctr-081155` recorded
the mempack backing at `0x106ddb3a0`. Its fifth exact checkpoint-6 repeat used
`0x106c3b3a0`; the approximately 2 MiB ranges overlap, yet playback completed
frame 2,200 without an ASan/UBSan report. This is stronger than merely choosing
widely separated ASLR bases.

Final closure used ordinary report `ctr-081618`: its exact producing binary
restored all eight checkpoints at frames 0 through 2,100 and every run reached
frame 2,200. Normal ARM64, fresh ASan/UBSan ARM64, and isolated Docker `-m32`
builds each pass 13/13 CTests. The finalized i686 executable is ELF32 Intel
80386 with Build ID `155f5086a4f576fc2b65367915dfa4b59d3832c2`; the forced
LP64 audit remains at zero compile errors, assertion failures, and narrowing
coordinates.
