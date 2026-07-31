# macOS ARM64 Quick Keyboard-Tap Result — 2026-07-31

## Result

The apparent input stall in the signed macOS ARM64 application was a host
input-transport defect, not a frozen game loop. A synthetic press could
deliver both SDL key-down and key-up events between two approximately
29.9 Hz retail pad polls. `Platform_PollHostEvents` drained both events, while
the later input snapshot read only SDL's final released state. The complete
tap could therefore disappear.

Commit `24aff7d88db66e241bb1277fe9f9b316d62a19bc`
(`fix: preserve quick keyboard taps`) retains mapped keyboard press edges for
exactly one PSX-shaped pad snapshot. Direct LLDB observation proved that one
synthetic `C` press reached the retail bus as active-low Cross, and the
running application subsequently advanced from the textured main menu into
the Adventure sequence.

This accepts the macOS keyboard tap-to-retail-pad transport boundary. It does
not yet accept complete manual keyboard play, controller coverage, touch
input, or any iOS/iPadOS input path.

## Root cause and correction

The SDL event loop now passes each non-Alt keyboard event to
`Platform_InputKeyboardEvent` before handling host-only shortcuts. The input
layer maps a key-down to the corresponding active-low PSX button bit and
retains it in `s_keyboardLatchedButtons`. Key-up does not erase an unconsumed
press. `NativeInput_ConsumeKeyboard` combines the latched edges with current
held-key state and then resets the latch to `0xffff`.

The latch is host transport state, not game state:

- it is deliberately absent from checkpoint and replay serialization;
- replay-installed snapshots clear and bypass it;
- disabled pad communication clears it;
- initialization, shutdown, and state restore clear it;
- Alt-modified host shortcuts never enter it; and
- the existing held-key path and PSX packet shape remain unchanged.

The media-free input self-test simulates key-down and key-up for both `C` and
Right before a poll. The first consume must contain active-low Cross and Right
and the second must be exactly `0xffff`. CTest's required output now includes:

```text
tap-latch=c+right one-snapshot
```

## Direct live trace

The first live diagnostic used the newly rebuilt but not-yet-committed app.
Its version string was:

```text
CTR Native 0.1.0-beta.7.1 (88ae012d5875-dirty)
```

The dirty suffix is important historical context: the source change was
committed only after the live trace. The four-file diff inspected in that
binary is exactly the source later committed as `24aff7d88`; no source edit
was made between the trace, final diff review, and commit.

Computer Use delivered one ordinary `C` press to the visible signed app. An
LLDB breakpoint at `Platform_InputKeyboardEvent` observed:

```text
key=6
down=1
```

`6` is `SDL_SCANCODE_C`. After the key-down handler:

```text
s_keyboardLatchedButtons=0xbfff
```

The following key-up arrived with `down=0` and did not clear the retained
press. `NativeInput_ConsumeKeyboard` entered with `0xbfff`, returned decimal
`49151` (`0xbfff`), and cleared the latch only after copying it. The completed
pad snapshot and retail packet were:

```text
snapshot.buttons[0]=0xff
snapshot.buttons[1]=0xbf
pad bytes=00 41 ff bf 80 80 80 80
```

Bit `0x4000` is therefore active-low exactly once: the macOS event crossed
the SDL boundary and reached the packet consumed by the retail input code.
After detaching LLDB and resuming, the visible application advanced from the
fully textured CTR main menu into the Adventure cutscene.

An initially unchanged screen was rejected as gameplay evidence because the
debugger was paused at the consume breakpoint: the game could not advance
while its process was stopped. Later no-debugger `C` taps visibly progressed
the cutscene, but time-based cutscene progress makes those observations weaker
than the direct packet trace. The packet trace is the authoritative one-tap
evidence.

## Exact committed-build validation

After commit and push, every relevant build reconfigured to identify
`24aff7d88db6`.

| Build | Result | Executable SHA-256 |
|---|---|---|
| macOS ARM64 Release | 16/16 CTests passed | `4181de9b2f55fc6251343e29633558ca779db4746f977b0c2f00d43f00904e57` |
| macOS ARM64 ASan+UBSan | 16/16 CTests passed with abort-on-first-finding options | `d0691133460b049627c3389d929483b2bf2330e1accd3367a3a51910431dc8ad` |
| optimized Linux i686 | 16/16 CTests passed | `2e6f2bbbb1945c91ad6742fd68c2e34b35339f24d6b583294a53737daf7a1de5` |
| signed macOS app executable | thin Mach-O ARM64 | `bab0099c7a9d68a92a0cd7dd96445ce537e98716c1ba726d7303f7baa78ad15e` |

The application bundle also passed:

```text
codesign --verify --deep --strict --verbose=2
plutil -lint CTRPad.app/Contents/Info.plist
file CTRPad.app/Contents/MacOS/CTRPad
```

The result was a valid ad-hoc signature, a valid plist, identifier
`io.github.chrissotraidis.ctrpad`, and a Mach-O 64-bit ARM64 executable.
The established compiler warnings were unchanged; no test or sanitizer
failure was emitted.

## Repository publication boundary

The source fix was pushed to branch `codex/arm64-apple` and draft pull request
[#1](https://github.com/chrissotraidis/ctrpad/pull/1). It is **not merged**.
At this checkpoint:

```text
origin/main=95417c723518407d6bfe3c81a37606294963efe2
draft branch source commit=24aff7d88db66e241bb1277fe9f9b316d62a19bc
pull request state=OPEN, draft, merge state CLEAN
```

Only the earlier viability-document work is present on `main`. All native
implementation, test, and parity-evidence work remains reviewable on the
draft branch until the user explicitly approves a merge.

No disc image, extracted retail asset, raw audio capture, memory-card file, or
other retail byte was added to Git.
