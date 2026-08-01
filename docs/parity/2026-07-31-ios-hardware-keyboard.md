# iOS Hardware-Keyboard Acceptance

- Date: 2026-07-31
- Accepted source commit: `e6ba535a9c73`
- Target: iOS 26.5 ARM64 iPad Simulator, portrait window, GLES 3

## Result

The practical keyboard aliases now control player one on iOS while the touch
overlay and Simulator gamepad peer are active. An exact committed build used
only keyboard input to skip the retail presentations, select Time Trial,
select Crash and Crash Cove, choose No Ghost, and reach the starting grid.

The finalized 1,869-frame input report proves that Start, Down and Cross were
recorded only in slot zero's touch-capable analog pad and that every press
returned to neutral on the next frame. Slots one through three remained
disconnected. The report also identifies its platform as `ios`, correcting the
earlier misleading `macos` metadata.

This accepts Simulator hardware-keyboard delivery and player-one composition.
It does not substitute for a physical iPad keyboard test, human multi-touch
ergonomics, Apple signing, or on-device timing/performance acceptance.

## Controls under test

The aliases were already published in commit `2c10b00b34df` and documented in
the top-level README:

| Keyboard | PS1 control |
|---|---|
| `W A S D` | D-pad Up / Left / Down / Right |
| `I J K L` | Triangle / Square / Cross / Circle |
| `Q E` | L1 / R1 |
| `P` | Start |
| `Tab` | Select |

The issue was not missing mappings or missing SDL key events. It was ownership
of the PS1-shaped controller slot after iOS enumerated its gamepad peer.

## Initial live trace and root cause

The first delayed-start report used the previously installed exact Simulator
app at SHA-256
`df2d6249f96497817af14382bfcc122b1795123a388c32bfaa8f95ead2cc41ee`
and implementation identity `560f6dd20963`. Report `ctr-195304` finalized 1,106
frames. Direct binary decoding showed:

- slot zero was the connected `0x73` touch/analog controller but stayed neutral;
- slot one was a connected digital `0x41` keyboard controller;
- `P` produced active-low Start `0xfff7` in slot one at frame 467;
- `S` produced active-low Down `0xffbf` in slot one at frame 640;
- the bounded `K` tap missed the low-FPS sampling interval.

The keyboard transport therefore worked, but retail menus read player one.
`NativeInput_OpenController` unconditionally moved the keyboard to the next
slot whenever a controller opened the same slot. That is useful desktop
multiplayer behavior. On iPhone and iPad, however, touch is deliberately
composed only into the primary pad, so moving the keyboard created an
unintended player-two input path.

Commit `e6ba535a9c73` retains the desktop policy and makes iOS share the primary
pad among keyboard, touch and gamepad input. A media-free self-test exercises
both shared-primary and separate-controller assignment. The existing update
order already resets a slot and composes controller, keyboard and touch
active-low inputs into it, so no UIKit keyboard bridge or duplicate event path
was added.

The same commit orders `SDL_PLATFORM_IOS` before `__APPLE__` in replay report
platform identification. iOS reports now say `ios`; macOS remains `macos`.

## Rejected diagnostic routes

These routes were recorded because they explain apparent stalls and prevent a
future investigator from repeating them as product evidence:

1. A normal LLDB attach to a running Simulator process hung for roughly 60
   seconds and was killed. The app continued normally.
2. Two `--wait-for-debugger` launches remained on a white frame with zero game
   CPU. An interrupt showed the main thread stopped in dyld's external-state
   notification trap, before CTR input initialization. Killing/detaching LLDB
   let each app continue. This was debugger/dyld perturbation, not a game hang.
3. Immediate `--record --detailed` report `ctr-195059` failed at replay frame
   zero with `too many VSync packets in replay frame 0`. Slow iOS bootstrap
   exceeded the fixed 64-run VSync record capacity. The supported delayed
   `--record --toggle --detailed` route armed at launch and began on F9 after
   bootstrap; it finalized normally on F10.

No product source was changed to disguise these diagnostic limitations.

## Exact build matrix

The source fix was committed and pushed before the acceptance artifacts were
configured. All three presets then embedded clean identity `e6ba535a9c73`:

```text
iOS Simulator ARM64  1cef6404aa3c2bf094c3357e71eb1069e81ed3e6307b972d043bfe222c2c62cb
iOS device ARM64     4ca08e9bbc2095f1a05d533e15af688da23f244a464164c24e4265557a199db1
macOS ARM64          cfd3d9b420e8e9592a622112bd368b20857ba427d68c92fab58bd8578d741dca
```

The clean macOS build passed all 21 CTests. Its input test reported
`primary-share=keyboard+touch+gamepad` while retaining the virtual-controller
buttons, axes, rumble, duplicate-add, removal and reconnect gates. Simulator
and device products both linked as thin ARM64 iOS applications.

## Update-install preservation

The exact linker-signed Simulator executable installed without a second
signature transform, and the installed SHA-256 remained
`1cef6404...c62cb`. The update migrated the data-container UUID from
`1F53906D-0653-4C99-A0A8-EF38A69CA3E4` to
`501F6DA4-63D0-49D2-B4FC-C0A1F5B7EBC7` and the bundle container to
`2F0F2C2E-9627-4438-982F-B8BC75F47532`. Paths were re-resolved rather than
reusing stale URLs.

Before and after installation, the imported retail image and live save kept
the same filesystem identities:

```text
BIN   inode 111313696, 605698800 bytes, f780bf2331476aabfc00772fa758b12dd95ebfbc907968132cbd3cdd4e2c07c0
save  inode 111309627,      6016 bytes, 6a01b0f5562ed7a279d8f8e51e3b1874ac39a6120f55db4fe3873288950619a3
```

The source validation Simulator and all retail media remained outside the
repository.

## Accepted visual route

PID `11165` launched with `--record --toggle --detailed`. After the retail
Crash-box presentation appeared beneath the complete safe-area touch overlay,
F9 began recording. Computer Use then sent only the documented aliases:

1. `P` skipped directly to the retail main menu.
2. `S` moved the highlight from Adventure to Time Trial.
3. `K` selected Time Trial.
4. `K` selected Crash Bandicoot.
5. `K` selected Crash Cove.
6. `K` selected No Ghost.

The app visibly reached the Crash Cove Time Trial starting grid with the
retail track, kart and full touch overlay rendered. F10 finalized the report.

Two local-only 655-by-903 Computer Use evidence frames remained outside Git:

```text
Time Trial selected  132799 bytes  b50085dda776b0c180efe8c1b8722a5133a91ec589b9342bb5953e63d0de6d37
starting grid        100660 bytes  09764b012c7f2fe1d8cfe5b3e08d61dedff591d7ce7b2ee6ba966a48f5e696dc
```

## Accepted packet evidence

Report directory (local-only):

```text
debug/reports/20260731/ctr-201917
```

Final metadata:

```text
finalized=1
recording_status=finalized
manual_start=1
build_id=e6ba535a9c73
platform=ios
replay_version=4
frame_record_size=440
frame_count=1869
checkpoint_count=7
executable_fingerprint=e9d9b4240372487c
```

Artifact hashes:

```text
input.ctrreplay  07fe7a7e1f1d4c16dc6c60a0a93d7263d74985698116a500f3db66a4e5f9a7fd
state.ctrstates  524d2aa0b11dd3892e23e50417988beb8a28df25ee198bb788ecac270a1c53e5
```

Direct parsing used the source-owned 148-byte replay header, 440-byte frame
record, pads offset 248 and 12-byte `PlatformInputPadSnapshot`. Slot zero had
one stable identity for all frames: status `0x00`, analog ID `0x73`, connected
`0x01`. Its complete non-neutral runs were:

| Frame(s) | Buttons | Meaning | Next frame |
|---:|---:|---|---|
| 361 | `0xfff7` | Start (`P`) | neutral |
| 528 | `0xffbf` | Down (`S`) | neutral |
| 906 | `0xbfff` | Cross (`K`) | neutral |
| 1249 | `0xbfff` | Cross (`K`) | neutral |
| 1376 | `0xbfff` | Cross (`K`) | neutral |
| 1562 | `0xbfff` | Cross (`K`) | neutral |

Slots one, two and three had stable disconnected identity
`status=0xff/id=0xff/connected=0x00`, stayed `0xffff` for all 1,869 frames,
and contained no non-neutral run. This is the decisive correction relative to
the original player-two trace.

## Exact replay

The accepted app was terminated and the same installed executable relaunched
as PID `12141` with the finalized replay path. It validated all seven rolling
checkpoint records, restored frame-zero state, used
`ctr-201917/memcard.playback` rather than the live save, and replayed the
keyboard route. Computer Use visibly observed the Time Trial highlight and
then the Crash Cove starting grid without injecting playback input. The log
closed with:

```text
[CTR Replay] replay finished after 1869 frames
---- LOG CLOSED ----
```

No canonical divergence was reported. The log's frame-zero raw-checkpoint
comparison was deliberately unequal across processes because it contains host
addresses; that line is labeled diagnostic-only and the canonical gate excludes
those addresses by design.

After recording and replay, the clone BIN/save again retained inodes
`111313696`/`111309627`, accepted sizes and exact hashes. The untouched source
validation Simulator retained inodes `111131200`/`111222179` and the same
hashes. The disposable clone was shut down without deletion, and existing
source-validation PID `93637` was returned to the foreground.

## Remaining boundary

Simulator keyboard delivery is accepted for navigation from presentation to a
race grid, including explicit release. Remaining goal work still requires a
real identity/profile and connected iPad for signed installation, physical
Files import/update/save checks, physical keyboard delivery, natural held
steer/Gas/drift ergonomics, a complete race and on-device timing/performance.
The overall goal remains active.
