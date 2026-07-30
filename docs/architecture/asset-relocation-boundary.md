# Asset Relocation Boundary

**Milestone:** M4

**Status:** checked relocation boundary wired into both retail ptrmap loader
paths; tagged-reference consumer conversion pending

`NativeAssetRelocation_Relocate` is the only accepted path for converting a
retail ptrmap into tagged guest references. Its inputs make both byte bounds
explicit:

- the asset base and exact asset byte size;
- the patch-entry array and exact patch-map byte size; and
- a diagnostic context naming the asset or load operation.

The owning MEMPACK region must already be registered with
`NativeGuestRef_RegisterRegion`. A nested asset is not a second overlapping
region.

## Atomic validation

The implementation performs a complete validation pass before writing any
asset slot:

1. the patch map size must be a multiple of four;
2. every patch entry must be four-byte aligned;
3. every four-byte slot must fit completely inside the asset;
4. normalized slot indices must be unique;
5. every target offset must be inside the asset; and
6. every target host byte must encode through the live owner region.

A temporary bitset detects duplicate slots without quadratic behavior. Tagged
references are staged separately and copied into the asset only after all
entries validate, so allocation or validation failure leaves the asset
byte-for-byte unchanged.

The self-test relocates an asset nested 16 bytes into a registered owner. Its
first relative target, asset offset 4, becomes `0x07000014`: region tag 7 and
owner-relative byte offset 20. It then proves atomic rejection of duplicate
slots, unaligned entries, an end-of-asset slot, an end-of-asset target, and a
non-word-sized patch map. The same source compiles as a strict C17 object on
Apple ARM64.

## Loader integration

`LOAD_RunPtrMap` now takes the asset byte size and patch-map byte size
explicitly. On LP64 it calls the atomic tagged-reference boundary and reports
the failing patch index, slot, target, and guest-reference status. On i686 it
keeps the existing native-pointer representation while applying the same
alignment and range checks, which preserves the current playable baseline.

Both call paths now provide real bounds:

- the embedded DRAM-file path validates the map header inside the queue slot,
  treats the bytes preceding that header as the asset, and rejects an invalid
  map before publishing the destination; and
- the separate LEV/PTR path retains the LEV byte size from
  `LOAD_Callback_LEV` until `LOAD_Callback_PatchMem` validates and applies the
  later map.

The native MEMPACK backing is registered as one checked guest-reference owner
when the arena initializes. Nested assets therefore encode owner-relative
references without attempting overlapping registrations.

## Remaining boundary work

The LP64 loader can now produce tagged fields, but most model, level,
animation, string-table, and command-list consumers still dereference those
four-byte slots as native pointers. Each consumer must move behind an explicit
checked resolver before an LP64 retail load is runnable. Arena unregister/reset
behavior also needs failure-path coverage across MEMPACK clear, pop, pack
replacement, and rollback.

The live i686 loader and the atomic self-test are passing. This boundary is not
described as M4 acceptance until the full consumer graph resolves tagged
references and the retail asset set loads on ARM64.
