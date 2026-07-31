# macOS ARM64 Controller Hotplug Result — 2026-07-31

## Result

Read-only review of the SDL gamepad path found that controller slots did not
own the SDL instance that opened them. `s_controllerToSlotMapping` was
initialized, swapped, serialized, restored, and queried, but a successful
open did not write the resolved joystick instance ID and close did not release
the mapping.

This made duplicate add events unsafe. After the first device opened slot 0,
its mapping still appeared free. A second add for the same SDL instance could
therefore open the device in slot 1. Removal closed only the first matching
handle and could leave a ghost controller in the other slot.

Commit `2f9bf4eaedd1ca6c781a9654f9851687b4c7fc18`
(`fix: own controller slots across hotplug`) records the resolved SDL joystick
instance ID only after `SDL_OpenGamepad` succeeds and resets that slot to `-1`
when the controller closes. Duplicate add, removal, and reconnect therefore
use one explicit ownership record.

The exact committed signed ARM64 app and a separate combined ASan/UBSan build
pass all 16 media-free CTests. A disposable optimized Linux i686 build from a
read-only source mount is still running at this documentation draft; its
result will be added only after the executable and test exit are observed.

## Virtual-gamepad integration proof

The input self-test now attaches an SDL virtual gamepad with the standardized
gamepad button/axis layout and a rumble callback. It exercises the production
add, snapshot, vibration, remove, and reconnect functions rather than a
separate model of them.

The accepted sequence requires:

1. the first add to open slot 0 and move the keyboard to slot 1;
2. a duplicate add for the same instance to leave exactly one open handle;
3. South, right shoulder, and right trigger to produce active-low PS1 buttons
   `0xb5ff`;
4. right X/Y and left X/Y to produce analog bytes
   `80 ff 00 80`;
5. PS1 rumble table `40 80` to reach the virtual-device callback as low/high
   magnitudes `32640/16320` exactly once;
6. removal to close the handle and restore mapping `-1`; and
7. reconnect to reclaim the released slot.

The stable success marker is:

```text
virtual-gamepad=buttons+axes+rumble+hotplug
```

This is an integration proof for SDL's virtual standardized gamepad API and
CTR Native's real slot/snapshot/rumble path. It is not evidence that a
particular physical MFi, Bluetooth, USB, Xbox, PlayStation, or Nintendo
controller has been connected and played through a full race. Those device
and manual-play checks remain open.

## Exact ARM64 validation

Both builds identify as:

```text
CTR Native 0.1.0-beta.7.1 (2f9bf4eaedd1)
```

| Build | Result | Executable SHA-256 |
|---|---|---|
| signed macOS ARM64 app | 16/16 CTests passed; strict signature/plist valid; thin ARM64 | `0f23ce4c8c8770caddda4c32d28e84ad6fd0c614c20514dea343cb31ca0967c1` |
| macOS ARM64 ASan+UBSan | 16/16 CTests passed with abort-on-first-finding options | `4be53768ba67f9e6d38bb677146e9bf1b24481cf406afe5565bb9abb0025ecad` |

The targeted sanitizer invocation also ran `ctr_native_input` verbosely and
printed the full marker. It passed in 1.27 seconds. The complete sanitizer
suite passed 16/16 in 2.24 seconds with no address- or undefined-behavior
report.

The signed app executable was written at 04:55:57 CDT and is 4,167,584 bytes.
The sanitizer executable was written at 05:00:36 CDT and is 25,123,304 bytes.
The source commit was created at 04:54:36 CDT.

## Publication and evidence boundary

The source checkpoint was pushed to `origin/codex/arm64-apple`. Draft pull
request #1 remains open and unmerged; `origin/main` remains
`95417c723518407d6bfe3c81a37606294963efe2`.

No retail disc image, extracted asset, save file, screenshot, device
credential, or runtime capture was added to Git. The disposable build and
long-running replay verifier remain outside the repository; the repository
stores their commands, identities, hashes, results, and honest acceptance
boundaries only.
