# Release re-baseline clean Apple smoke

## Result

Clean documentation commit `d5772375fabc361c58fbee6d35c43d5fcbdf4cd0`
passes the locally available release re-baseline boundary:

- native macOS ARM64 builds and passes 22/22 tests plus the exact renderer
  pixel oracle;
- thin ARM64 iPad Simulator and iPhoneOS bundles build with clean embedded
  identity `d5772375fabc`;
- the actual 1032x1376 iPad Simulator surface passes the established GLES
  pixel hashes;
- an update install replaces the stale exploratory app while preserving the
  retail BIN and memory-card save byte-for-byte and at the same inodes;
- a one-Simulator retail route reaches a live Crash Cove race with coherent
  graphics, keyboard and touch consumption, pause, Home/foreground and
  portrait-to-landscape reflow;
- exact bundle-ID termination leaves no application process, and the current
  log plus four archives contain no targeted asset, visibility, error or fatal
  marker; and
- a retail-free unsigned IPA and exact matching GPL source archive are ready
  for user-owned signing and physical-device validation.

This accepts the clean Simulator correctness/diagnostic refresh described in
`docs/history/RELEASE-REBASELINE.md`. It does not accept Apple Software
Renderer cadence, a full race, physical touch ergonomics, signing or any
physical-device requirement.

## Exact source and publication boundary

The source branch was clean and tracking
`origin/codex/simulator-performance-next`. The release re-baseline was first
committed and pushed as:

```text
d5772375fabc361c58fbee6d35c43d5fcbdf4cd0
Rebaseline release acceptance on physical iPad
```

Draft PR #2 targets GitHub `main` and was open, clean and mergeable at this
head. The GitHub connector repeated its known 404 for the private repository,
so the explicitly repository-qualified authenticated `gh` fallback created
the PR. Its first verification command used unsupported `gh pr view --head`
after the PR was already created; `gh pr view 2 --repo chrissotraidis/ctrpad`
then verified the intended head/base and draft state. No repository was
mutated by the failed verification command.

This report is a later documentation-only record. The binaries and archives
below correctly identify the clean build source as `d5772375fabc`.

## Resource and Simulator discipline

Before compilation, both named CTRPad iPad Simulators were shutdown and the
Simulator GUI and CTRPad process were absent. All three Apple builds used nice
level 15 and one build job. Compilation never overlapped a booted device or
Simulator GUI.

Only `CTRPad Import Negatives`
`26F3DEE8-8840-446D-85FE-C882009C9C06` was booted for runtime work. `CTRPad
Import Validation` remained shutdown. After the smoke, the exact bundle ID was
terminated, the sole device was shutdown, and direct process inspection showed
no Simulator GUI or CTRPad executable. Computer Use briefly reported a stale
`isRunning=true` after Command-Q; the exact process query was authoritative and
empty. No device was erased or application uninstalled.

## Clean macOS ARM64 result

Fresh configuration identified SDL revision
`SDL-3.4.10-beta-7.1-191-gd5772375f`. The sequential build completed with the
established 32-warning set and produced:

```text
CTR Native 0.1.0-beta.7.1 (d5772375fabc)
```

CTest passed all 22 tests in 3.08 seconds. The independent verbose renderer
test passed in 1.33 seconds with:

```text
logical hash       851169f2644a1675
blend hash         0c0d08324ae06c35
presentation hash  a7798c5a6ddee965
fallback draws     12
active draws       12
```

The desktop adapter was Apple M2 / OpenGL 4.1 Metal. No test was skipped.

## Clean iPad Simulator build and actual-surface oracle

Fresh Simulator configuration selected iPhoneSimulator 26.5, ARM64, minimum
iOS 15.0 and shared GLES. The sequential build completed with the same 32
warnings. The raw linker's thin ARM64 executable is:

```text
unsigned executable SHA-256  1699c36d136c7bf5ce173ea5701e6313f9d40abc038b77fc27d83a3a08ca7091
```

As expected, the raw linker signature was incomplete (`Info.plist=not bound`,
no sealed resources). An isolated copy under
`/tmp/ctrpad-d5772375f.O2ZNZX/CTRPad.app` was ad-hoc re-signed for Simulator
only and passed deep/strict verification:

```text
signed executable SHA-256    c6d40aaf3c0cfc989e47a79b74ff86f9e52b95a74505c58abaf77d323319187f
```

This ad-hoc signature is not physical-device signing evidence.

The actual iPad surface reported framebuffer/renderbuffer 1, Apple Software
Renderer, OpenGL ES 3.0, GLSL ES 3.00 and framebuffer fetch enabled. The pixel
self-test passed:

```text
surface              1032x1376
logical hash         851169f2644a1675
blend hash           0c0d08324ae06c35
oracle hash          0c0d08324ae06c35
presentation hash    172d49a34571b64c
fallback draws       12
active draws         5
```

After the success line, the Simulator console emitted an unbalanced UIKit
appearance-transition warning during the self-test's immediate exit, followed
by the Simulator runtime's duplicate accessibility-loader class warning. The
normal retail run's persistent application log contains neither warning and
the subsequent Home/foreground route retained correct state. This report does
not silently classify the console warning as fixed; it is a focused self-test
exit observation to reproduce on physical lifecycle work if it appears there.

The console-PTY attachment stayed open after the self-test process had exited.
An empty poll produced no output, then Control-C closed only the attachment.
Exact bundle-ID termination correctly returned `found nothing to terminate`,
consistent with the already-exited self-test. The normal product was launched
separately afterward.

## Update preservation

Before installation, the existing data container was
`4CB99800-8032-4A8E-A4E3-88D781D2A71E`. The clean update install completed in
18.16 seconds and remapped the data-container path to
`B3DC8851-3E19-4501-A8D8-3DF4A64FA2F9`, while the protected objects retained
their exact identities:

```text
retail inode  111450682
retail size   605698800
retail SHA    f780bf2331476aabfc00772fa758b12dd95ebfbc907968132cbd3cdd4e2c07c0

save inode    111309627
save size     6016
save SHA      6a01b0f5562ed7a279d8f8e51e3b1874ac39a6120f55db4fe3873288950619a3
```

The installed executable matched the isolated signed hash and contained clean
identity `d5772375fabc`. The same retail/save inode, size and hash values were
rechecked after the complete smoke and termination.

## Bounded visual, input and lifecycle route

Computer Use directly inspected the Simulator rather than inferring appearance
from logs. The clean product rendered:

- copyright text and the animated CTR title/logo;
- Adventure and main mode menus;
- Time Trial character selection with Crash, kart and eight portrait tiles;
- the Crash Cove level list, preview and minimap;
- No Ghost selection and animated track fly-in;
- the grid, kart, bridge/banner, sky, course geometry and exhaust;
- live timer, lap counter, start lights, HUD, minimap and touch overlay;
- the Pause menu over the live course; and
- the same paused course after Home/foreground and portrait-to-landscape
  rotation, followed by resumed rendering.

Touch Cross entered the first menu. Hardware keyboard `Z`, Down, `C` and `P`
navigated back, selected Time Trial/Crash/Crash Cove/No Ghost and paused or
resumed the race. Race-time `C`, `D` and `E` edges were published, and a final
accessible touch Gas activation resumed on the landscape route. The current
log records every relevant edge reaching the exact retail poll, including:

```text
touch    0x4000 consumed
keyboard 0x1000 consumed
keyboard 0x4040 consumed
keyboard 0x0820 consumed
keyboard 0x0008 consumed
```

Home/foreground logged ordered lifecycle transitions from active to
will-background/background and will-foreground/active, with audio suspended
then active. The app remained on the paused race and reflowed controls and
rendering to the full landscape window. It then resumed and consumed touch Gas.

The retained landscape screenshot is 850x708 with SHA-256:

```text
c70f0c968ba219f278db90ae33168dc98375cfd7b219739600457b54af4450f2
```

It is task-owned evidence under `/tmp/ctrpad-d5772375f.O2ZNZX`, not a committed
retail artifact.

## Log and shutdown result

Correct-ID termination succeeded and a launchd search found no bundle or
executable process. The current structured log has 82 lines / 9,369 bytes:

```text
current SHA-256  467de09b754c80e7729504b277596c59fc83b863b551169d6190670e540ac555
archive .1       68eec948982a72cce952de2107abdfddec8c6568f31cb10f6d369f3b87dd789f
archive .2       a4914173e20679d0f9099eb3cba2efff616c216f33412d889d3f6faab3b913be
archive .3       25a66dd9b163a0b346ff1d8873ab65d884816c5d04abae157c1d4e1896626e2f
archive .4       a1d98d657307f7751dc2b7e09d5d09d214747130934d0c1aabd1feb0ec41107b
```

An exact scan of the current log plus four archives found zero `[ERROR]`,
`[FATAL]`, `[CTR AssetRef]`, or visibility invalid/mismatch/failure/error
markers. The current log identifies build `d5772375fabc`, the three sandbox
roots, framebuffer fetch, touch-first overlay and UIKit display loop.

Observed FPS samples ranged from menu peaks near 59.8 to a late race sample of
4.79 under Apple Software Renderer. This remains the known Simulator
performance limitation and is not promoted to physical-device evidence.

## Clean iPhoneOS and paired package result

With both devices and Simulator GUI closed, the clean iPhoneOS configuration
selected the 26.5 SDK, ARM64, minimum iOS 15.0 and shared GLES. The sequential
build completed with the established 32 warnings. The thin ARM64 executable
embeds `d5772375fabc`:

```text
device executable SHA-256  6f1aaefa69645c82da9f5e06222de9ff18c2bd0e4f0ed823b195271c9ae4095b
```

The existing clean app was packaged without rebuilding:

```text
dist/CTRPad-0.1.0-1-d5772375f-unsigned.ipa
SHA-256  41e00a5d258b87ef775f07cb9566f1a31138cf1cc8c48d7513054610bccb13ed
```

ZIP validation passed. The IPA has exactly the standard Payload/app directory,
executable, Info.plist, LICENSE, THIRD_PARTY_NOTICES and INSTALL-IOS members. It
is thin ARM64, minimum iOS 15.0, bundle
`io.github.chrissotraidis.ctrpad`, version 0.1.0 (1), unsigned, and contains no
retail media.

The exact matching corresponding-source package is:

```text
dist/CTRPad-source-d5772375fabc.tar.gz
SHA-256  117dd291e35c1e13d1d65767fc24e13202941c5a1486b4bef26e2ad6a0d6352d
members  3245
```

The packager accepted the clean exact commit and excluded retail media,
runtime state, packages, profiles and keys. Both artifacts are ignored local
outputs; the source repository remains the publication surface.

## Remaining release boundary

A fresh audit still found:

```text
valid Apple code-signing identities  0
provisioning profiles                0
connected physical devices          0
```

Therefore the clean Simulator and unsigned-package prerequisites are complete,
but the signed physical campaign remains open. Required next evidence is a
user-owned Apple identity/profile and target iPad performing signed install,
retail Files import, a complete touch race and three-boost drift, audio,
controller coexistence, lifecycle/update/save persistence, physical cadence
and thermals. If the physical device reproduces the Simulator bottleneck, the
renderer must be profiled there before considering a Metal translation or
backend.

The close reading was 261,192 seconds: 3 days, 33 minutes, 12 seconds
cumulative, 1,326 seconds (22 minutes, 6 seconds) after the release re-baseline
documentation boundary. The overall goal remains active.
