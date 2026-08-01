# iPad Orientation-Hint Correction

- Date: 2026-08-01
- Implementation: `2c78c040bf2a766d3a12e8fdbff11306e04eb517`
- Target: iOS 26.5 ARM64 iPad Simulator and iPhoneOS ARM64 compile/link
- Runtime device: disposable `CTRPad Import Negatives`
- Protected device: `CTRPad Import Validation`, shut down and untouched
- Result: accepted for current Simulator portrait/landscape/portrait layout;
  physical-iPad rotation/windowing remains open

## Reopened visible defect

The level-visibility correction restored missing world geometry, but its exact
runtime evidence exposed a separate layout defect. After one Simulator
rotation, the iPad shell was portrait while the app logged a landscape
`1376x1032` window. The landscape controller view was clipped into the top of
the portrait scene, leaving a large black region and placing the right action
cluster outside the visible app surface.

The local-only pre-correction window frame was 655 by 903 pixels and hashed to
`5637af8065ed6f29acbfe89045e9088cc27ff25aeb6df4575df8f13059626a87`.
It clearly showed restored textured forest/building/ground geometry, but only
the steering stick and top utilities were visible. That frame is valid evidence
for the level-geometry fix and invalid evidence for complete portrait layout.

This re-audit narrows two earlier reports. The UIKit root-controller lifecycle
fix still balances appearance transitions and preserves rendering across
rotation. The earlier claim that every portrait control had reflowed was too
broad for the current path and is superseded by this exact correction.

## Root cause

`platform/apple/Info-iOS.plist.in` intentionally has two masks:

- iPhone: `LandscapeLeft`, `LandscapeRight`;
- iPad: portrait, upside-down portrait and both landscape orientations.

`Platform_Init`, however, unconditionally set SDL's UIKit orientation hint to
only `LandscapeLeft LandscapeRight`. SDL's
`UIKit_GetSupportedOrientations` intersects that hint with the target's plist
mask. The intersection correctly kept iPhone landscape-only, but it also
reduced iPad to landscape-only despite the four-orientation declaration.

On the current iPadOS 26.5 Simulator, the scene shell could become portrait
while SDL's root controller still reported only landscape. The renderer and
overlay were therefore initialized with landscape bounds inside a portrait
window. This explains the log's landscape size, clipped renderer and missing
right controls together.

## Correction

Commit `2c78c040b` changes only the pre-`SDL_Init` iOS hint:

```text
Portrait PortraitUpsideDown LandscapeLeft LandscapeRight
```

The plist remains the authority for each target. Its intersection keeps iPhone
landscape-only without a platform bridge or duplicated device check; iPad now
retains the four orientations it already declares. The renderer and native
overlay already resize from scene bounds, so no forced geometry request,
manual transform or second layout path was added.

## Dirty-source proof before publication

Both named Simulators were shut down during compilation. Simulator and device
targets built sequentially at nice priority 15 and one Ninja job, repeating the
32 established legacy C warnings and no new Objective-C warning. Candidate
executable hashes were:

```text
iOS Simulator  23edbc1ad3f969ccc0194a9e1f63ea296488acb01b3e0dfbf0ca8ead457180a8
iPhoneOS        790727855a50471b9a2e8defb249bf209f5e5660f10d9e9de6de6a95cc9791cb
```

The dirty app embedded `faa32dc04ffc-dirty`. Only disposable
`CTRPad Import Negatives` was booted and installed. Its portrait cold launch
immediately logged `Window size: 1032x1376`, not the pre-correction landscape
size. The visible legal screen filled the portrait surface and showed both
drift buttons, utilities, steering stick, View, Item, Brake and Gas.

One rotation produced a full landscape scene with complete textured
Crash/trophy/checkered geometry and every control visible. A second rotation
returned to portrait with a centered complete title/menu and all 11 control
accessibility identifiers. Candidate frame hashes were:

```text
portrait cold    b4ff97d0d525617ed8de6ccccb18609c5cb3a89f030de64c66b4addd8f75a07d
landscape        0c9a3a01afbb1180efbe21a01c7f3e1d511a62026c23a5d0735e622b046e79a8
portrait return  24f66465ce3e8a2677f4b1af7653eb52133c01bc64e457167f6994bf16816c8f
```

Those screenshots remained local-only. The app was terminated and the
disposable device shut down before publication.

## Exact clean build

The one-file change was reviewed, committed and pushed before exact rebuild.
Both iOS presets were explicitly reconfigured so SDL and CTRPad embedded clean
commit identity `2c78c040b`:

| Product | Exact result | SHA-256 |
| --- | --- | --- |
| iOS Simulator ARM64 | ARM64 Mach-O; `2c78c040bf2a`; UIKit/GLES link | `87188026542dfee99e7e060f64ef77bff6a81924b477fec704f4ba729ba06b9c` |
| iPhoneOS ARM64 | ARM64 Mach-O; `2c78c040bf2a`; UIKit/GLES link | `3dc6e3c7706e996d59668ffb2c91024473d22b068c0565317fabdc87ccd9ebc1` |

Both report version `0.1.0-beta.7.1` and SDL identity
`SDL-3.4.10-beta-7.1-160-g2c78c040b`. Generated plist inspection confirms the
iPhone mask still contains only both landscape entries while the iPad mask
contains all four. The change is compiled only under `SDL_PLATFORM_IOS`; no
desktop code path changed, so the earlier exact ordinary/sanitizer 22/22 suites
remain the adjacent non-iOS regression boundary rather than being relabeled as
tests of this UIKit policy.

## Exact one-Simulator UI acceptance

Protected `CTRPad Import Validation` remained shut down. The exact Simulator
app installed only on disposable `CTRPad Import Negatives`; container migration
retained canonical data before launch:

```text
BIN   inode 111450682, 605698800 bytes, mtime 1785358888
save  inode 111309627, 6016 bytes, mtime 1785525736,
      SHA-256 6a01b0f5562ed7a279d8f8e51e3b1874ac39a6120f55db4fe3873288950619a3
```

The attached exact console reported version `2c78c040bf2a`, portrait window
`1032x1376`, Apple Software Renderer, GLES 3.0, all PSX/VRAM pipelines, touch
overlay, UIKit display loop and CoreAudio.

Computer Use refreshed state before every action. The exact route was:

1. portrait cold launch: complete legal scene and every control visible;
2. one freshly addressed Rotate action: complete landscape Naughty Dog/title
   scene, no clipping and every control visible; and
3. one fresh rotation shortcut: complete portrait scene and all 11 controls in
   the accessibility tree.

Exact local-only frame evidence is:

| Frame | Dimensions | SHA-256 |
| --- | --- | --- |
| portrait cold | 655 by 903 | `43e62bf243f7f89a9f1e7b61c486ad5773b50b2cbb990e487d09f9c2619a1ec7` |
| landscape | 850 by 708 | `092ccc06911f02f115d32c192535438fef162a713f30405d7a0436ac2bb0852a` |
| portrait return | 655 by 903 | `0953cef917839a96a05c1904d6a8b469dba80952beccc239ae66c555430d478a` |

The frames stay outside Git so retail-derived pixels are not published in the
GPL source repository.

The active file log contained no `[CTR AssetRef]`, visibility-cache exhaustion,
`ERROR`, `FATAL` or unbalanced-appearance line. The attached console did emit
the Simulator runtime's duplicate WebCore/WebKit accessibility-class warning
and one Foundation `NSMapGet(...): map table argument is NULL` diagnostic after
the first rotation. The same single diagnostic occurred in candidate and exact
runs, did not repeat on the return rotation, and did not interrupt rendering,
input accessibility or bounded termination. It is retained as a Simulator-side
diagnostic, not silently reported as fixed.

After exact termination the BIN and save identities above remained unchanged;
the active log itself hashed to
`e18608c228102db26e901efe76c5f6108b50869e4aed2d2363bbff35c3cfa9b6`.
The disposable Simulator was shut down. Final state showed both named devices
shut down.

## Acceptance boundary

The current Simulator landscape-controller-in-portrait clipping defect is
accepted as corrected. iPhone retains landscape policy; iPad cold-launches in
portrait bounds, expands to landscape and returns to portrait without clipping
the renderer or losing controls. Full scene textures also remain visible, so
this does not regress the preceding level-visibility correction.

Physical iPad windowing/rotation feel, Stage Manager or other live window
resizing, cadence, energy and simultaneous human touch remain open. The single
Foundation diagnostic is explicit. M10 and the overall goal remain active.
