# ADR-0001: Tagged Guest References for 32-Bit Retail Layouts

**Status:** accepted

**Date:** 2026-07-29

**Milestone:** M2

## Context

CTR Native currently makes a 32-bit host pointer do three different jobs:

1. a native runtime pointer;
2. a pointer written into a 32-bit field in a relocated retail asset; and
3. a temporary pointer stored at a retail scratchpad offset.

Those representations happen to have the same width on the supported i686
build. They cannot remain interchangeable on Apple ARM64.

`LOAD_RunPtrMap` is the load-bearing case. A retail file contains 32-bit
offsets from the file origin. The current loader adds the native allocation
address and writes the resulting host pointer back into the same four-byte
slot. Widening the C field breaks the file layout, while leaving it four bytes
truncates an ARM64 address.

The current checkpoint pointer-slot registry is useful evidence but is not a
safe long-term owner registry. In the accepted golden report:

| Replay frame | Registered slots | Still-valid MPAK targets | Stale/out-of-range slots |
|---:|---:|---:|---:|
| 300 | 3,444 | 3,444 | 0 |
| 1,800 | 38,313 | 26,289 | 11,046 |
| 22,200 | 65,536 (capacity) | 14,462 | 49,062 |

The registry accumulates slots from freed or reused allocations and eventually
reaches its fixed capacity. M4 may use its capture data as an inventory, but
must not reuse its current lifetime model.

The prototype command is:

```sh
tools/verify-guest-reference-prototype.sh \
  build-linux-i686-baseline/debug/reports/20260729/ctr-225420
```

On the Apple ARM64 host it copied the captured two-MiB MPAK to a native
allocation above `UINT32_MAX` and traversed the real Roo's Tubes path:

```text
GameTracker.level1
  -> Level.ptr_mesh_info
  -> mesh_info.ptrQuadBlockArray
  -> QuadBlock[0].ptr_texture_low
```

The asset contained 1,806 quad blocks, 8,507 vertices, and 1,011 BSP nodes.
Both prototypes reached the same texture bytes without narrowing the host
pointer. Offsets exactly at the arena end, bounded guest addresses exactly
past the arena, and captured pointers below the recorded base all failed with
the field name, offending value, access size, and valid range.

## Considered designs

### A. Tagged 32-bit guest references with checked translation

Keep every serialized pointer slot four bytes. Replace the C pointer type with
an explicit `GuestRef32`, and resolve it only through a central region
registry.

Proposed encoding:

```text
31                 24 23                              0
+--------------------+--------------------------------+
| nonzero region tag | byte offset within that region |
+--------------------+--------------------------------+
```

- zero is null;
- 255 non-null region tags are available;
- each region can be up to 16 MiB;
- the current expanded native MPAK is 8 MiB and the retail-pressure MPAK is
  2 MiB;
- the encoded value never contains host-address bits.

Each non-overlapping allocation owner, such as the live MPAK arena, owns one
region registration for exactly its live byte range. A nested ptrmap asset
records its owner tag and its byte offset within that region. Loading validates
the patch map and converts each asset-origin-relative file offset into a tagged
region-relative reference. Clearing or replacing the owning MPAK unregisters
the region and its relocation-slot inventory. Nested asset ranges are never
registered as overlapping regions and therefore cannot consume a tag apiece.

Advantages:

- preserves every four-byte file slot;
- works at any native allocation address;
- provides a field-aware bounds/alignment diagnostic;
- supports references to MPAK, resident data, or another registered asset;
- does not require native globals to move into emulated RAM;
- can be introduced on i686 first and checked by the golden parity gate.

Costs:

- every asset-pointer consumer must cross a typed resolver;
- region lifetime must follow MPAK and asset lifetime exactly;
- array-of-reference fields need explicit element resolution;
- pointer-writing code must call checked host-to-guest conversion.
- a reference has no generation bits, so an owner tag may be reused only after
  every dependent reference and sidecar has been cleared; the registry catches
  use after unregister but cannot distinguish an old reference after the same
  tag has been registered for a replacement arena.

### B. A bounded PS1-address guest arena

Keep retail-looking values such as `0x80000000 + arenaOffset` in four-byte
slots. Resolve them against a host allocation whose actual address is
unrelated.

The prototype proves this works for the captured two-MiB MPAK and rejects an
address at `0x80200000`. It does not require the host allocation itself to be
low.

Advantages:

- values resemble retail addresses;
- cross-object references inside one arena need no region tag;
- a single subtraction resolves an address after validation.

Costs:

- resident C globals, overlay globals, scratchpad, and the native-expanded
  MPAK do not currently share one emulated address space;
- moving them into one arena would require a new global-state ownership model
  or continuously synchronized mirrors;
- the expanded eight-MiB MPAK cannot fit in the retail two-MiB window;
- every dereference still needs translation, so it does not eliminate the
  consumer conversion;
- it makes the 64-bit port depend on a broader memory-system rewrite before a
  real asset can be converted.

Allocating the arena at a low host virtual address was rejected outright.
Apple platforms do not promise such an allocation, it would still leave
serialized layouts conflated with host pointers, and it would hide rather than
remove truncation defects.

## Decision

Use design A: tagged `GuestRef32` values and a checked region registry for
serialized/file-layout references.

Use a split representation for the other ownership classes:

- **Serialized asset fields:** four-byte `GuestRef32`, translated at typed
  boundaries. Their byte offsets and sizes remain unconditional.
- **Scratchpad fields:** preserve the exact 1 KiB scratch byte layout. Native
  host pointers live in a sidecar or host-width local for the duration of the
  routine. Never widen a field inside the scratch byte image.
- **Resident executable/overlay maps:** native runtime pointers become
  host-width fields or sidecar fields. Retain an explicit 32-bit guest schema
  only where raw retail offsets or PS1 backfeed actually consume it.
- **Runtime-only structures:** use ordinary native pointers and native layout.
  Retail layout assertions become PS1/guest-schema assertions rather than
  global host-layout assertions.
- **Opaque retail code/address labels:** remain `u32` labels and go through
  their existing dispatch translation. They are not `GuestRef32` values.

The first implementation API should make unsafe operations difficult:

```c
struct GuestRef32
{
    u32 bits;
};

int GuestRegion_Register(...);
void GuestRegion_Unregister(...);
struct GuestRef32 GuestRef_FromRegionOffset(...);
int GuestRef_FromHostPointer(...);
void *GuestRef_Resolve(...);
```

Resolution requires the expected byte count, alignment, and a diagnostic
context string. Null handling must be explicit. No implicit cast exists
between `GuestRef32` and a native pointer.

## Migration rules

1. Change `LOAD_RunPtrMap` to accept the asset size and patch-map byte size.
2. Reject unaligned patch entries, slots outside the asset, target offsets
   outside the asset, integer overflow, and duplicate patch entries.
3. Register the owning allocation arena before converting its slots. Roll back
   a new registration on any validation failure.
4. Convert file offsets to owner-region-relative `GuestRef32` values; never
   write `origin` into the file.
5. Convert consumers subsystem-by-subsystem through typed helpers.
6. Tie registrations and pointer-slot inventories to MPAK clear/pop/swap and
   asset unload operations. A global append-only slot list is forbidden.
   Before reusing an owner tag, clear every native sidecar and dependent guest
   reference to prevent an ABA-style stale reference from resolving into the
   replacement arena.
7. Run the conversion on i686 and require the unchanged golden digest before
   using it on ARM64.
8. Version checkpoint persistence. A new checkpoint must serialize guest
   references or region/offset pairs, never native addresses or 32-bit file
   offsets that can overflow.

## Consequences

This decision deliberately separates retail byte layout from native object
layout. It creates more explicit access sites in M4, but it confines the
complexity to representation boundaries and makes corrupt data fail
deterministically.

M3 may now widen runtime-only pointers and split resident layouts without
waiting for every asset consumer. M4 owns the complete ptrmap conversion. M5
owns scratchpad sidecars and remaining pinned-layout splits.

The bounded arena prototype remains useful as a test oracle and as a possible
future representation for a full PS1-memory emulator. It is not the selected
dependency for the Apple port.
