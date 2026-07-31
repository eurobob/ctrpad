# macOS ARM64 Save/Relaunch Result — 2026-07-30

## Result

An actual current ARM64 recording wrote a valid NTSC-U profile file from an
empty memory-card seed. A later independent process using the exact immutable
producer opened that file from the normal `memcards/slot0` root, read all
5,760 payload bytes after the 256-byte icon header, passed the retail CRC, and
returned the game's success code to `RefreshCard`.

This accepts file persistence and checksum-valid reload across macOS process
launches for M6. It does not accept iOS sandbox placement or Files-app import,
which remain later milestones.

## Written artifact

Producer and recording:

```text
producer:
  build-macos-arm64/ctr_native-cutscene-fix-producer-eee2a8df5b96
producer SHA-256:
  fa9a7d46292ab09b143e0e2b514317daa251af48f6341dc437b1f5d81ded3961
build ID:
  eee2a8df5b96
report:
  build-macos-arm64/debug/reports/20260730/ctr-215303
frames/checkpoints/finalized/exit:
  24232 / 81 / 1 / 0
```

The report began with an empty `memcard.seed` and ended with:

```text
memcard.recording/slot0/BASCUS-94426-SLOTS
bytes:
  6016
SHA-256:
  6a01b0f5562ed7a279d8f8e51e3b1874ac39a6120f55db4fe3873288950619a3
```

Fresh current-build lap report `ctr-223221`, also produced by
`eee2a8df5b96`, independently wrote the same file hash from an empty seed.
No memory-card or retail byte is tracked by Git.

## Static format validation

`tools/inspect-native-memcard-save.mjs` implements the same bit recurrence as
`MEMCARD_CRC16` at `game/MEMCARD/MEMCARD_Checksum.c:4-23`, validates the fixed
native wrapper consumed by `platform/native_memcard.c:792-844`, and checks the
retail profile header defined at `include/namespace_Memcard.h:610-646`.

Command against the retained relaunch copy:

```sh
tools/inspect-native-memcard-save.mjs \
  /private/tmp/ctrpad-memcard-relaunch-root-20260730/slot0/BASCUS-94426-SLOTS
```

Result:

```text
bytes=6016
iconBytes=256
payloadBytes=5760
blocks=1
profileVersion=-18
profileSize=0x1600
crcRemainder=0x0
sha256=6a01b0f5562ed7a279d8f8e51e3b1874ac39a6120f55db4fe3873288950619a3
```

The file is exactly one native memory-card block: a 0x100-byte `SC` icon
header plus the 0x1680-byte buffer passed by `RefreshCard` at
`game/RefreshCard.c:299-310`. The payload declares retail version `-18`
(`0xffee`) and 0x1600 bytes of structured `MemcardProfile`; the remaining
buffer bytes include the stored CRC. A complete CRC remainder of zero is the
success condition used by `MEMCARD_ChecksumLoad`.

As a negative check, passing the 988-byte report metadata file exits 1 with
the exact-size rejection. The validator does not accept arbitrary files based
only on their name.

## Independent relaunch procedure

Before the test, `build-macos-arm64/memcards` did not exist. The recorded file
was copied without modification to:

```text
build-macos-arm64/memcards/slot0/BASCUS-94426-SLOTS
```

The exact producer was then started with no replay or recording arguments
under LLDB. This is a distinct process with a new ASLR layout. A breakpoint on
`NativeMemcard_ReadSaveData` stopped on the automatic normal startup load:

```text
save_name:
  bu00:BASCUS-94426-SLOTS
byte_count:
  5760
data_offset:
  256
```

Stepping out of that function produced:

```text
w0:
  0x00000000
nativeResult:
  NATIVE_MEMCARD_OK
```

The enclosing `MEMCARD_Load` at
`platform/native_memcard_adapter.c:130-158` then ran the retail checksum loop.
Stepping out of the complete adapter returned:

```text
Return value:
  (u8) 0
```

`MC_RETURN_IOE` is value 0 at `include/namespace_Memcard.h:29-40`. The adapter
returns that value only when `NativeMemcard_ReadSaveData` succeeds and
`MEMCARD_ChecksumLoad` reaches its zero-remainder success result; not-found,
read errors, and checksum failures return different codes. The live payload
header in the second process was:

```text
0xffee 0x1600
```

This is direct read-and-validate evidence, not merely a filesystem existence
check.

## Cleanup and acceptance boundary

After the result was captured, the diagnostic process was intentionally
killed under LLDB; its exit is not lifecycle evidence. The temporary default
memcard root was moved intact to:

```text
/private/tmp/ctrpad-memcard-relaunch-root-20260730
```

The ordinary build root was restored to its pre-test absent state, so later
clean runs will not silently inherit this profile. The authoritative
recording copy remains in ignored report `ctr-215303`.

Accepted here:

- a real game-driven save write from an empty seed;
- exact native container and retail CRC validity;
- persistence of the bytes after the producing process ended;
- automatic read by a second exact-producer process; and
- the game adapter's checksum-valid success return.

Still open:

- complete manual desktop play;
- clean normal-app quit after the diagnostic reload;
- iOS `SDL_GetPrefPath`/sandbox placement;
- relaunch on a physical iOS/iPadOS device; and
- persistence after iOS suspension/termination.
