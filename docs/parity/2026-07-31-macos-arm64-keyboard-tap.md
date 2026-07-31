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

## Additive two-hand test layout

The one-tap transport fix made the original keyboard map reliable, but that
map remained obscure and awkward for a complete race. Commit
`2c10b00b34df4f0eb61aa8b72cbe99588a930ed6` adds an ergonomic alias for
every input needed in a basic race while preserving every prior binding:

```text
W/A/S/D       D-pad
I/J/K/L       Triangle/Square/Cross/Circle
Q/E           L1/R1
P             Start
Tab           Select
```

Left/Right Ctrl and `[`/`]` remain the L2/R2 and L3/R3 bindings. Arrow keys,
`Z/X/C/V`, Left/Right Shift, Space, and Return remain unchanged. The aliases
enter the same active-low PS1 button mapper and pad snapshot as the original
keys, so they do not create a keyboard-only physics or replay path.

The input self-test now verifies all 12 aliases individually. It also proves
that held `K+D+E` produces Cross + D-pad Right + R1 simultaneously and that a
complete `K+D` down/up pair is latched for one retail snapshot. The stable
older success marker was retained for CI compatibility, with the new result
appended:

```text
tap-latch=c+right one-snapshot aliases=12 held=k+d+e alias-tap=k+d
```

The exact committed application identified as:

```text
CTR Native 0.1.0-beta.7.1 (2c10b00b34df)
Mach-O 64-bit executable arm64
executable SHA-256 6e3171d1619fbc34ee1985679604a018107ac62039e6de5d8489b1729224f9e9
```

It passed 16/16 CTests, strict deep code-signature verification, plist
validation, and the thin-ARM64 architecture check. Computer Use then launched
that exact app through normal startup. `K` advanced the retail legal/splash
sequence into the real seven-row main menu; at the stable menu, `S` visibly
moved the highlight from Adventure to Time Trial and `W` moved it back to
Adventure. The window was then closed normally.

Synthetic key pulses sent during noninteractive title transitions are
intentionally ignored by the retail state machine, so bounded retries were
needed while crossing the boot sequence. This is not a lost-pad-edge result:
the stable main-menu `S` press moved on its first attempt, and the media-free
tests cover both tap latching and held combinations.

The temporary UI captures were visually inspected but were automatically
removed by the desktop capture service during normal app close. No screenshot
hash is claimed and no retail-derived pixel is in Git. The source checkpoint
was committed at 04:39:42 CDT, the exact application executable was written
at 04:40:48 CDT, visible verification completed by 04:43:44 CDT, and the
source branch had already been pushed to GitHub. The goal API reported
1 day, 14 hours, 8 minutes, 23 seconds of cumulative goal time during this
checkpoint; that timer is not benchmark or labor time.

This accepts a documented, practical desktop keyboard test layout and its
main-menu behavior. A complete human-driven race, gamepad coverage, touch
input, and iOS/iPadOS input remain separate acceptance tasks.

## Live Time Trial acceleration and steering follow-up

After controller ownership validation, the later clean-tip signed app
`359e8d5a0f07` was launched again through the ordinary retail path. Its
executable SHA-256 was
`c97953dddfe682962732aea7a2d2e8ebde6f8083a2add1eb7ef47388924ae446`.
The visible sequence included the SCEA presentation, coherent Naughty Dog
crate animation, textured seven-row main menu, Time Trial character and ghost
selection, Crash Cove intro, and the normal kart/HUD/minimap race view.

The first route is rejected as race evidence. `S` visibly highlighted Time
Trial, but a single `K` confirmation arrived while the menu was not accepting
it and the game entered its normal attract/story sequence. Code inspection
confirmed that both Cross and Circle are valid on the relevant selection
screens; the issue was the attempted state/timing boundary, not a different
binding. `L` returned to the title/menu, then bounded `S` plus `L` input
entered Time Trial. Bounded repeated Cross input selected Crash and no ghost.

At the first stable race observation, the timer read `0:12:21`, lap 1/3, and
the kart was stationary below the CTR start banner. Forty-five sparse
automation taps advanced the timer to `0:35:80` but left the kart at the line.
That attempt is rejected as held-acceleration evidence: the desktop automation
pressed and released too slowly to maintain CTR's held Cross state.

A dense stream through the same ordinary keyboard event path then produced
these visible changes:

- dense `K` advanced the kart from the line to the left cliff/tree area and
  moved the minimap marker by timer `1:05:66`;
- dense alternating `K+D` changed heading/location and advanced the minimap
  marker by `1:23:95`; and
- dense `K+D+E` input moved the kart out from the wall to the exposed
  rock/ocean section by `1:40:48`.

The final inspected capture is local-only at SHA-256
`e6e97000036efe26b162cba3022d3a6e3842656c2c63a488b322b3dc532f0644`
(139,137-byte JPEG, captured 05:45:21 CDT). It shows coherent Crash Cove
geometry, sky/ocean, kart, HUD, minimap, and the moved player marker. The
capture remains outside Git and contains retail-derived pixels.

The dense automation emits sequential key events; it cannot prove that
`K+D+E` overlapped in one retail poll. The deterministic input test remains
the authoritative simultaneous-chord proof. This live follow-up accepts
normal-startup Time Trial entry plus basic keyboard acceleration and steering.
It does not accept a natural human-held full lap, powerslide/boost execution,
or full-race completion. The app closed through its window control and the OS
reported CTRPad no longer running.

Goal elapsed at the 05:46:33 CDT close checkpoint was 1 day, 15 hours,
17 minutes, 19 seconds. The preserved alternate-loader i686 verifier remained
running, unpaused, and not OOM-killed; it had passed driver transitions at
frames 4,636/4,689 but had not yet reached its next 6,000-frame marker.
