# macOS ARM64 scrapbook STR presentation evidence

**Date:** 2026-07-31

**Implementation source:** `c44ea781039114ef84c9d7fe6043672ea16f8ab5`

**Sanitizer-corrected source:**
`75b09db17d1cabb91f2fece68a43edbf04662992`

**Status:** all 4,424 production frames accepted through decode, macOS OpenGL
presentation, and ASan/UBSan; real-menu cadence and A/V synchronization remain
open

## Question

The earlier `--probe-str-scrapbook` result proved that ARM64 and i686 decode
the first ten retail `TEST.STR` frames to identical RGB555 pixels. It did not
prove that the production upload and presentation path puts those pixels on
screen. This probe asks a narrower, observable question:

> Does the clean ARM64 application decode real retail sectors, upload them
> through PS1 VRAM, draw the selected VRAM display rectangle through the host
> presentation shader, and read back a coherent framebuffer?

## Implemented path

`--probe-str-scrapbook-present FRAME_COUNT OUTPUT.bmp` is parsed before normal
game startup (`main.c:226-396`). After normal asset discovery and retail-media
validation, the probe:

1. initializes the ordinary SDL/OpenGL renderer and verifies that startup
   completed (`platform/native_str.c:955-976`,
   `platform/native_platform.c:271-274`);
2. opens `TEST.STR` through `NativeSTR_StartScrapbook`;
3. clears the 512-by-216 NTSC display region and calls the production
   `NativeSTR_UploadNextFrame` path (`platform/native_str.c:878-898,990-1002`);
4. presents that VRAM rectangle with the same shader entry point used by the
   scrapbook menu (`platform/native_str.c:1005`,
   `platform/native_renderer.c:2124-2145`);
5. reads the actual default framebuffer as portable `GL_RGBA`, restores pack
   alignment, and flips it to top-left image order
   (`platform/native_renderer.c:2147-2186`); and
6. hashes every decoded RGB555 frame and presented 800-by-600 RGBA frame, then
   saves the last framebuffer to BMP outside the repository.

This keeps OpenGL calls inside the renderer boundary. The capture avoids the
desktop-only `GL_BGRA` debug screenshot path so it remains compatible with the
planned GLES renderer.

## Exact clean run

The clean ARM64 binary identified itself as:

```text
CTR Native 0.1.0-beta.7.1 (c44ea7810391)
```

Command:

```sh
./build-macos-arm64/ctr_native \
  --probe-str-scrapbook-present 10 \
  /private/tmp/ctrpad-scrapbook-present-c44ea7810.bmp
```

All decoded hashes match the prior headless ARM64/i686 result:

| Frame | Decoded RGB555 FNV-1a64 | Presented RGBA FNV-1a64 |
|---:|---:|---:|
| 0 | `cc257394fc1aa6bf` | `2bf050200a5e1325` |
| 1 | `c75e60c5237e9883` | `45c7774d600eeaf5` |
| 2 | `1619948468f8d745` | `148309908f2aa8cf` |
| 3 | `9a4c939a61ecb36d` | `70082585f5297934` |
| 4 | `6b4b54295b9aab26` | `45a6560f583bc0c6` |
| 5 | `d18440f9f8499972` | `922326c1006cf522` |
| 6 | `d7af0dc8fe801a69` | `f44fdd5cb6998efc` |
| 7 | `d91d3b4cd26382fa` | `033f47c35bc150cf` |
| 8 | `6a711f4442d7ba8e` | `779b1625210dd584` |
| 9 | `429eaab6a380c28f` | `aacb38c544b080f2` |

```text
headless decoded sequence:  60dcf4c65986a034
presented sequence:         e85a9203c966c801
frame-9 BMP SHA-256:        e7366bc9ff8ea4054d7c45e16f0a0eb8539c3bfb0cb8b1bba1c1bc5b3f1888e3
```

Three initial runs—two pre-commit and one from exact clean commit
`c44ea7810`—produced the same ten decoded hashes, ten presented hashes,
presented sequence hash, and byte-identical frame-9 BMP. The later exact clean
Release and sanitizer reruns at `75b09db17d1c` reproduced all of those values
and the same BMP bytes after correcting the sanitizer finding described
below.

## Visual inspection

Frame 0 is a normal black lead-in and was rejected as insufficient visual
evidence. Frame 9 visibly contains the coherent gray scrapbook cover with the
red-and-black Naughty Dog mark, `Scrapbook`, and `1994-1999`; geometry,
orientation, color channels, and text are intact. This directly answers the
earlier question about whether the screen and textures can be seen.

The BMP and a temporary PNG conversion were inspected locally but are not
committed. They are decoded from the user's retail movie and therefore remain
outside source control. The command and SHA-256 make the observation
repeatable without publishing retail-derived pixels.

## Negative and regression checks

Missing arguments, missing output path, zero count, nonnumeric count, duplicate
present probes, and combining headless and present probes all exited 1 with an
actionable error. The exact clean ARM64 build passed 16/16 existing CTests.
`git ls-files ref/CTR` remained empty.

## Sanitizer chronology

The sanitizer campaign used an ARM64 `RelWithDebInfo` build with:

```text
-fsanitize=address,undefined -fno-omit-frame-pointer
ASAN_OPTIONS=detect_leaks=0:halt_on_error=1:strict_string_checks=1
UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1
```

Rejected observations were retained rather than silently retried:

1. The first CTest launch requested `detect_leaks=1`. Apple's ASan runtime
   reported that leak detection is unsupported and aborted all 16 processes
   with exit 134. This was a test-environment error, not an accepted suite.
2. With supported options, all 16 CTests and the ten-frame headless probe
   passed. The first presentation probe then stopped at
   `platform/native_renderer.c:1254`: forming
   `&((GrVertex *)NULL)->field` for an OpenGL buffer offset invoked undefined
   null-member access.
3. Commit `75b09db17` replaced all four expressions with defined
   `offsetof(GrVertex, field)` offsets
   (`platform/native_renderer.c:1254-1261`) and pinned the packed vertex size
   and four offsets with static assertions
   (`include/platform/native_renderer_types.h:37-41`).
4. A presentation rerun with ordinary HID enumeration reached an ASan
   heap-buffer-overflow inside Apple's CoreGraphics `pdf_lexer_scan`, through
   CoreUI/SwiftUI/AppKit while SDL enumerated HID devices. Renderer
   initialization had completed, but the stack had not entered the STR
   decode/upload/presentation loop. That run is rejected and is not
   represented as a game-code fix.
5. The renderer-specific probe was rerun with
   `SDL_JOYSTICK_HIDAPI=0`, avoiding that unrelated Apple framework path.
   The separate sanitized `ctr_native_input` CTest still ran and passed with
   the ordinary suite.

The exact clean sanitizer binary identified itself as:

```text
CTR Native 0.1.0-beta.7.1 (75b09db17d1c)
```

It passed 16/16 CTests, the ten-frame headless sequence
`60dcf4c65986a034`, and the ten-frame presented sequence
`e85a9203c966c801` without an ASan/UBSan report. Its output BMP SHA-256 was
again
`e7366bc9ff8ea4054d7c45e16f0a0eb8539c3bfb0cb8b1bba1c1bc5b3f1888e3`
and was byte-identical to the exact clean Release output.

The sanitizer tree was created at 02:39:24 CDT. The final exact-clean
sanitizer screenshot was written at 03:33:43 CDT, an observed campaign wall
interval of 54 minutes, 19 seconds that includes compilation, rejected runs,
diagnosis, correction, normal-build validation, and the clean rebuild. The
source correction was committed at 03:26:12 CDT; the final accepted screenshot
followed 7 minutes, 31 seconds later.

## Complete movie coverage

The exact clean `75b09db17d1c` Release and sanitizer binaries were then run
across the declared 4,424-frame scrapbook length. All four commands ran
serially at host nice level 15 so the preserved i686 parity verifier retained
priority:

```text
Release   --probe-str-scrapbook 4424
Sanitize  --probe-str-scrapbook 4424
Release   --probe-str-scrapbook-present 4424 OUTPUT.bmp
Sanitize  --probe-str-scrapbook-present 4424 OUTPUT.bmp
```

Results:

| Path | Release wall | Sanitizer wall | Sequence FNV-1a64 |
|---|---:|---:|---:|
| RGB555 decode | 61.35 s | 68.69 s | `4b193011f608bc90` |
| VRAM/presentation/readback | 64.85 s | 78.75 s | `e7e81ffeedaa7b2c` |

Every headless record reported 512 by 208 pixels. Independent continuity
checks proved that both headless builds and both presentation builds emitted
source/probe indices 0 through 4,423 exactly once. The 4,424 filtered
per-frame lines had these compact SHA-256 manifests:

```text
decoded records:    6f70283316cf03bc786e3b96847cf04ace34a12dea33a5e3d6a630f7cb63c7e5
presented records:  96d3254f21c70e004e2319886ef72080c5ca3d413f920c4e04a591238e1f480e
```

Release and sanitizer produced the same manifest in each case; strict
per-frame comparisons exited 0. The final frame is part of the movie's black
tail, so its BMP is not used as new visual evidence. It nevertheless compared
byte-for-byte equal across builds with SHA-256
`85b43f1be768060c8abf89f7f9dfc1052cd78249947910302f5a5a19657677aa`.
The earlier coherent frame 9 remains the inspected visual artifact.

The four measured process times total 273.64 seconds. No ASan/UBSan diagnostic
appeared. Logs, BMPs, and retail-derived pixels remain in `/private/tmp` and
outside Git.

## Acceptance boundary

Accepted:

- real NTSC-U disc-image lookup and all-4,424-frame decode;
- exact continuity with the accepted headless RGB555 hashes;
- production `LoadImage` plus host VRAM texture upload;
- macOS ARM64 direct-VRAM presentation shader execution;
- actual framebuffer readback and a coherent, repeatable visual artifact;
- defined and compile-time-pinned OpenGL vertex attribute offsets; and
- the complete headless/presentation path under ASan/UBSan, with the
  renderer-only HID isolation described above.

Not accepted:

- retail 15-fps presentation cadence during the real menu loop;
- interleaved XA audio/video synchronization;
- normal menu entry, skip, and teardown under broad manual play; or
- GLES/iOS presentation.

The Apple framework HID-enumeration fault under ASan remains a documented
environment boundary. Normal Release HID initialization and the sanitized
input CTest pass, but this narrow result is not broad full-game sanitizer
acceptance.

This is a concrete M6 improvement, not completion of M6 or the iPad objective.
