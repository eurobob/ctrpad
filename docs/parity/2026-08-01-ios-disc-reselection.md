# iOS disc re-selection and save-preservation acceptance

## Accepted boundary

Checkpoint `300499d7cd00` adds an explicit in-game path for replacing the
user-owned retail NTSC-U disc image on iOS/iPadOS. The path is reachable from
the touch overlay, requires confirmation before stopping the game, reuses the
existing security-scoped Files importer, preserves the memory-card root, and
requires a cold relaunch after the new image is installed. A disposable ARM64
iPad Simulator exercised confirmation cancellation, picker cancellation, an
invalid image, a complete valid replacement, exact inode/hash preservation,
and cold startup from the replacement.

This accepts the software-side Simulator asset-reselection portion of M9. It
does not claim physical-device Files behavior, Apple development signing,
device termination/background delivery, or device save persistence.

## Why this flow stops instead of hot-swapping

The pre-change runtime had no user-facing route back to the importer. A direct
hot swap was rejected during the implementation audit because production asset
and disc-image readers are global and remain live throughout gameplay. Running
the existing validator against a staging root while the display loop continued
to read the installed disc would also reuse global asset/disc state. Neither
route offered a truthful atomic-media guarantee.

The accepted boundary is deliberately narrower:

1. **CHANGE DISC** presents a native confirmation. Cancel leaves the running
   display loop and game state untouched.
2. **Stop Game & Choose** schedules a request back to the main display-loop
   boundary. The next display callback stops the link, removes touch input,
   shuts down the platform/audio/renderer and closes the current disc image.
3. Only after shutdown does the app show the replacement importer. Its copy is
   private, coordinated, same-volume and validate-before-replace, exactly like
   fresh-install import.
4. A successful replacement disables further picker input and tells the user
   to fully close and reopen CTRPad. This avoids pretending an unimplemented
   in-process retail reset is safe. Memory-card files remain outside the media
   transaction.

The confirmation text explicitly warns that unsaved race progress is lost and
that memory-card saves are kept. Cancellation and every import error explicitly
state that the current disc remains installed.

## Implementation anatomy

`include/platform/native_ios_import.h` now distinguishes initial setup from
re-selection and separates three completion results: failure, runtime started,
and relaunch required. `platform/apple/native_ios_import.m` uses that purpose
to select accurate headings, button/status copy and completion behavior while
retaining the existing security-scoped coordinated-copy transaction.

`include/platform/native_ios_touch.h` and
`platform/apple/native_ios_touch.m` add a disc-reselection callback plus the
accessible `ctrpad.touch.disc` control. The callback and userdata are cleared
whenever the overlay ends or cannot attach. The alert dismissal is allowed to
finish before the game hierarchy is removed.

`main.c` owns the one-shot request flag. It observes the flag at the top of the
UIKit display callback, performs ordered runtime shutdown, closes the old disc,
and begins re-selection only from that safe main-thread boundary. Initial
onboarding still starts the game in-process after a successful import; only an
active-media replacement requires relaunch.

## Precommit live proof and refinement

The first full dirty-source proof used only the disposable **CTRPad Import
Negatives** iPad Simulator (`26F3DEE8-8840-446D-85FE-C882009C9C06`). The
protected **CTRPad Import Validation** Simulator was left booted and untouched.

The dirty build visibly rendered the animated CTR title/demo and exposed
`Change retail disc image` through accessibility. Canceling the confirmation
returned to a later animated frame. Confirming logged:

```text
[CTR Import] confirmed runtime disc re-selection request
[CTR Import] stopping the current game before disc re-selection
```

and showed the coherent replacement screen without an appearance-transition
warning. The first Files presentation reproduced the previously documented
local File Provider blank sheet; dismissing its accessibility region returned
`No file selected. Your current disc remains installed.` A second presentation
loaded **On My iPad → CTRPad**. Selecting the full 605,698,800-byte NTSC-U
fixture replaced inode `111313696` with `111448009`, retained exact disc hash
`f780bf23...07c0`, and kept save inode `111309627`, size 6,016 and hash
`6a01b0f5...619a`. Cold relaunch rendered CTR from the new inode.

Review then added the central error suffix `Your current disc remains
installed.` for every re-selection failure. Both iOS Simulator and iPhoneOS
ARM64 rebuilt before the five-file implementation was committed and pushed as:

```text
300499d7cd00050d83dbb2dc2236a25b0529473b
feat: add safe iOS disc reselection
```

Draft PR #1 resolved to that exact SHA before final acceptance began.

## Exact cross-target matrix

Every final directory was reconfigured after the implementation commit. The
five ARM64 binaries all contain build ID `300499d7cd00`:

```text
iOS Simulator ARM64   a09932ffc3ad9ca6759c89778cefc4595133ca73c4e7791f643f5a9327be2c08
iPhoneOS ARM64        20dfcf892ffecdd2c23e602acc451aebe77cd9c4bec55fa5ce1f5aba431e5c11
macOS desktop GL      89eb5d224542819c261c490f902391c06231937efbfb40d415b39ec13a9140f3
macOS GLES config     2fc21b1008e4ff30d1b26435585610373618d9b9e91fa03a7157bf995cbd194c
macOS ASan/UBSan      1900f3ea57d6989cad1db9532031ce039e5b1cc9eedfdb9058c8c32a0d6a9f46
```

The desktop GL suite passed 22/22 in 4.28 seconds. The ASan/UBSan suite passed
22/22 in 10.50 seconds with no sanitizer finding. The iOS Simulator,
iPhoneOS, macOS desktop and macOS GLES products all compiled/linked as ARM64
Mach-O. Regenerating all five exact optimized products concurrently took about
12.5 minutes because the large production `main.c` translation unit was
optimized independently for each configuration; the jobs remained CPU-active
and completed normally.

The clean Simulator app was 3.5 MB and contained only the executable,
`Info.plist`, GPL license, installation instructions and third-party notices.
No retail asset was in the bundle. Its unsigned executable hash was
`a09932ff...2c08`; the isolated ad-hoc-signed copy verified successfully and
hashed to `688865c7...f2e`. Installing it as an update changed container paths
but retained the already accepted disc and save at their original inodes and
hashes.

## Exact Files and relaunch sequence

Computer Use drove the real Simulator UI and refreshed accessibility state
after each action:

1. Exact build `300499d7cd00` cold-launched the existing disc, activated the
   UIKit display loop and visibly rendered coherent textured animation with
   the touch overlay and accessible disc-change button.
2. Opening the confirmation exposed both **Stop Game & Choose** and **Cancel**.
   Cancel returned to a later animated game frame; no shutdown marker appeared.
3. Confirming stopped the active runtime and presented the replacement screen.
   It promised validate-before-replace behavior and save preservation.
4. The 118-byte `not-a-disc.bin` fixture was selected through the real Files
   picker. The app displayed the raw-MODE2/2352 error plus `Your current disc
   remains installed.` The chooser re-enabled immediately.
5. After that rejection, disc inode `111448009`, disc size/hash, save inode
   `111309627`, save size/hash, and the absence of new staging directories were
   all unchanged.
6. The complete NTSC-U fixture was selected through the same picker. The app
   displayed `Replacement installed`, disabled the chooser, required full
   close/reopen, and explicitly reported that memory-card saves were unchanged.
7. Atomic replacement produced disc inode `111450682`, retained size
   605,698,800 and SHA-256 `f780bf23...07c0`, while the save retained inode
   `111309627`, size 6,016 and SHA-256 `6a01b0f5...619a`. No new reserved stage
   survived.
8. After bounded termination, the same exact executable cold-launched, read the
   new disc inode, activated UIKit/GLES and visibly rendered animated textured
   output with the complete overlay. The 610,956-byte local-only screenshot
   hashed to `a33b033b...ca70`.

The attached production console contained the expected Simulator-only duplicate
accessibility-bundle diagnostic but no UIKit unbalanced appearance-transition
warning through stop, importer presentation, replacement or relaunch. Retail
fixtures, screenshots, temporary signatures and Simulator containers remained
outside Git.

## Acceptance and remaining work

M9's explicit asset-reselection and save-preservation criterion is accepted on
Simulator. The gate proves that cancel does not interrupt gameplay, invalid
media cannot replace the current disc, valid media is installed atomically,
the save is not rewritten, and a cold start consumes the replacement.

M9 remains in progress for inaccessible-provider callback coverage and the
physical-device repetition of Files import, signing, background/termination
behavior and save retention. M8/M10 also remain open for user-owned Apple
signing, real iPad keyboard/controller/multi-touch delivery, a completed race,
audio/video playtesting, cadence and energy. The overall goal remains active.

The preceding published timer was 208,828 goal seconds. The documentation-open
reading was 211,878 seconds: 2 days, 10 hours, 51 minutes, 18 seconds
cumulative, adding 3,050 seconds (50 minutes, 50 seconds). The documentation-
close reading was 212,262 seconds: 2 days, 10 hours, 57 minutes, 42 seconds,
adding 3,434 seconds (57 minutes, 14 seconds) from the preceding published
checkpoint and 384 seconds (6 minutes, 24 seconds) during the closing document
audit. Goal time is cumulative across pauses and resumes; it is not a build
benchmark or a person-hour estimate.
