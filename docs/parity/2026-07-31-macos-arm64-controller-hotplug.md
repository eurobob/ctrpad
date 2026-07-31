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

The exact committed signed ARM64 app, a separate combined ASan/UBSan build,
and a disposable optimized Linux i686 build all pass the 16 media-free CTests.
The i686 source mount was read-only and its output tree was disposable. The
final i686 compile repeats only four established warnings; two new
signed/unsigned warnings exposed by the first attempt were corrected before
acceptance.

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

## Final clean-tip cross-width validation

The functional commit was followed by test-only commit `764205d4c`, which
casts the historical signed mapping bits back to SDL's unsigned
`SDL_JoystickID` for two comparisons. Documentation checkpoint `359e8d5a0`
then froze the complete process while i686 was still running. With the
worktree clean and synchronized, all final builds identify as:

```text
CTR Native 0.1.0-beta.7.1 (359e8d5a0f07)
```

| Exact clean-tip build | Result | Executable SHA-256 |
|---|---|---|
| signed macOS ARM64 app | 16/16 in 0.61 s; strict signature/plist valid; thin ARM64 | `c97953dddfe682962732aea7a2d2e8ebde6f8083a2add1eb7ef47388924ae446` |
| macOS ARM64 ASan+UBSan | 16/16 in 3.90 s; no sanitizer finding | `ec4d49d5ca1180dc2711bfd7353d58d84e9e142a724e97947940d14dad12adfd` |
| optimized Linux i686 | 16/16 in 3.51 s; ELF32 Intel 80386 | `d21c04bd129399a28a0e983fd26d187f5c98adcc1506983caf18a029992bfd10` |

The final i686 executable is a 32-bit little-endian PIE with interpreter
`/lib/ld-linux.so.2` and GNU Build ID
`e71bd4d9b99bf1efc5214a6d4c250ca22621ef96`. The exact recompile removed both
new sign-comparison warnings; it retained only two established format-string
and two established maybe-uninitialized warnings.

The first final CTest command mounted `/out` read-only. CTest exited 8 before
running a test because it could not create `Testing/Temporary/LastTest.log`.
That invocation is rejected as infrastructure error, not counted as a test
failure. Repeating with only the disposable output tree read/write ran and
passed all 16 tests; the repository source remained read-only throughout.

## Publication and evidence boundary

The functional source and running-history checkpoint were pushed through
`359e8d5a0` on `origin/codex/arm64-apple`. Draft pull request #1 remains open
and unmerged; `origin/main` remains
`95417c723518407d6bfe3c81a37606294963efe2`.

No retail disc image, extracted asset, save file, screenshot, device
credential, or runtime capture was added to Git. The disposable build and
long-running replay verifier remain outside the repository; the repository
stores their commands, identities, hashes, results, and honest acceptance
boundaries only.
