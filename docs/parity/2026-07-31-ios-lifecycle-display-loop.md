# iOS lifecycle and display-loop checkpoint — 2026-07-31

## Result

Commit `afb5463cc5115073be9427658df424a5d9c14092` replaces the native
process-exit/iOS endless-loop behavior with a cooperative lifecycle boundary:

- SDL/UIKit owns the iOS application run loop and invokes one retail main-loop
  step from `CADisplayLink` through `SDL_SetiOSAnimationCallback`;
- lifecycle notifications are handled synchronously through an SDL event watch,
  including notifications SDL explicitly does not place on the event queue;
- backgrounding publishes released PSX input, pauses the host audio stream,
  clears stale queued PCM and flushes the log;
- foregrounding clears transport edges, rebases only the host VBlank deadline,
  and resumes audio without resetting the game-visible VBlank count;
- SDL quit/window-close requests are cooperative instead of calling `exit(0)`;
  and
- iOS uses a fully yielding system sleep instead of either the native 200-us
  final spin or the final sub-millisecond spin inside `SDL_DelayPrecise`.

An exact clean ARM64 iPad Simulator build imported the user's ignored NTSC-U
BIN into a temporary, locally ad-hoc-signed package. It completed two real
Home/background/foreground cycles. Each cycle produced the complete ordered
event sequence, suspended/resumed audio, and restored live textured retail
animation without a relaunch or black frame.

This is a lifecycle checkpoint, not M8 acceptance. Simulator presentation is
still far below retail cadence because Apple's Simulator exposes the
`Apple Software Renderer`; a 324-frame diagnostic capture measured 8.335 FPS
and attributed 105.157 ms of its 119.983-ms average frame to the
`renderer_draw_triangles_ms` scope. Physical-device cadence, the repeated
UIKit appearance-transition warning, controller play, save/background
integrity, natural termination, and full nonblocking scheduling remain open.

## Starting boundary

The previous exact clean checkpoint `98ae2c6d86fe` could install and launch a
locally signed Simulator package, initialize GLES/CoreAudio, and render the
retail title menu. It still had these lifecycle defects:

1. `CTR_Main` never returned from its native endless loop.
2. `SDL_EVENT_QUIT` and `SDL_EVENT_WINDOW_CLOSE_REQUESTED` called `exit(0)`.
3. no code handled background, foreground, low-memory, or terminating events;
4. the VBlank deadline stayed based on pre-suspension host time;
5. held keyboard/controller input and queued host PCM crossed suspension
   boundaries unchanged; and
6. the native precision wait contained a final busy spin.

The earlier exact Simulator run also emitted two unbalanced UIKit
appearance-transition warnings. That warning remained an explicit risk rather
than being waived because the app happened to launch.

The source tree was clean at `cbdd58435173d4ba7786599973eea8278a54607c`
before this slice. Retail execution used only
`ref/CTR/CTR - Crash Team Racing (USA).bin`, which is ignored and never staged.

## Investigation

### SDL lifecycle delivery is synchronous

The SDL application-event declarations state that the following mobile events
are not placed on the ordinary event queue:

```text
SDL_EVENT_WILL_ENTER_BACKGROUND
SDL_EVENT_DID_ENTER_BACKGROUND
SDL_EVENT_WILL_ENTER_FOREGROUND
SDL_EVENT_DID_ENTER_FOREGROUND
SDL_EVENT_TERMINATING
SDL_EVENT_LOW_MEMORY
```

Polling them only from the retail frame loop would therefore miss the events.
The implementation installs `SDL_AddEventWatch` immediately after `SDL_Init`
and removes it before `SDL_Quit`. The watch reduces the event into a small
phase/action state machine, then performs only the requested idempotent host
actions (`platform/native_platform.c:124-257,488-495,535-542`).

### Standard-main iOS ownership

SDL's standard-main iOS route recommends installing an animation callback and
returning from `main`, leaving UIKit's run loop in control. The retail native
loop was split without changing the PS1 path:

```text
CTR_MainStep: one existing retail state-loop iteration
CTR_Main:     desktop wrapper that repeats CTR_MainStep
iOS callback: one CTR_MainStep per SDL/UIKit animation callback
```

The split is at `game/MAIN/MainMain.c:58-99,512-527`; UIKit callback ownership
is at `main.c:269-288` and `platform/native_platform.c:800-823`.

When inactive, the step pumps host events and returns without advancing game
logic. When the callback observes a cooperative quit, it stops the display
callback and runs platform shutdown.

This returns control to UIKit between retail frames. It does not yet turn all
retail waits into continuations: a frame can still sleep synchronously inside
`VSync`. The checkpoint is therefore described as a hybrid display-driven
outer loop, not a completed nonblocking scheduler.

### Audio boundary

Background entry now:

1. pauses the SDL audio-stream device;
2. locks the native output queue;
3. discards PCM rendered before the suspension boundary; and
4. leaves emulated SPU/XA state authoritative.

Foreground entry clears the stale host queue again, then resumes the same
stream. Calls are safe before audio initialization and idempotence is owned by
the lifecycle reducer, so paired `will`/`did` events do not pause or resume
twice (`platform/native_audio.c:2436-2472`).

### Input boundary

Background entry clears the quick-key latch and name-entry transport key, then
publishes released active-low PSX pad snapshots. This prevents a key, button,
trigger, or analog axis held when UIKit resigns active from sticking in the
guest pad bus. Foreground entry clears only transient transport edges; the next
ordinary input update resamples current SDL keyboard/gamepad state
(`platform/native_input.c:958-992`).

### VBlank rebase

Foreground entry clears `s_nextVBlankCounter` and the division remainder, but
preserves:

- `s_nativeVBlankCount`;
- registered VBlank callbacks;
- RCNT1/game timer state; and
- all retail state.

The next `VSync` establishes a fresh absolute host target. This prevents a
background-duration burst while retaining the exact NTSC rational and normal
catch-up rules during active execution (`platform/native_platform.c:830-945`).

### Removing the hidden spin

The first implementation set the explicit iOS final spin window from 200 us to
zero. Source inspection then showed that `SDL_DelayPrecise` itself sleeps until
the final sub-millisecond interval and busy-spins the remainder. The iOS path
was changed to `SDL_DelayNS`, which yields for the complete wait. Desktop keeps
the already accepted `SDL_DelayPrecise` plus bounded-spin behavior unchanged
(`platform/native_platform.c:840-849,896-945`).

An absolute target still controls each synthetic NTSC VBlank. A late iOS wake
does not accumulate drift; ordinary catch-up and the existing pathological
stall limiter decide how elapsed VBlanks are emitted.

## Deterministic lifecycle test

CTest 19 adds `ctr_native_lifecycle`. Its reducer test covers:

- paired `will`/`did` background events;
- duplicate background idempotence;
- paired `will`/`did` foreground events;
- duplicate foreground idempotence;
- direct `did-background` and `did-foreground` recovery;
- low-memory log flush without audio transition;
- final terminating state and cooperative quit;
- no resume after termination; and
- VBlank-deadline rebase while preserving a synthetic game-visible count.

The exact marker is:

```text
[CTR Lifecycle] self-test passed: background=idempotent foreground=rebase quit=cooperative audio=paired low-memory=flush
```

The marker passed in the desktop-GL, GLES-configured, combined ASan/UBSan, and
optimized i686 configurations. The reducer cases are source-owned at
`platform/native_platform.c:1108-1192`.

## Live Simulator process

### Diagnostic runs

The first dirty build ran the same eventual source diff with build identity
`cbdd58435173-dirty`. Two complete Home/resume cycles proved the event-watch
route before the wait was changed. A second yielding build completed one full
cycle and one intentionally rapid cycle. In the rapid cycle UIKit delivered
`will-enter-background` followed directly by `did-enter-foreground`; the
reducer recovered and resumed once, matching its direct-transition unit case.

A desktop dirty launch also received terminal Ctrl-C as an SDL quit request,
returned through the cooperative loop, closed the platform log, and exited 0.
That diagnostic established the replacement for direct `exit(0)`; exact clean
coverage is retained by the lifecycle test.

### Exact clean package

The committed Simulator product was copied to:

```text
/private/tmp/ctrpad-ios-lifecycle-exact-cNF1CU/CTRPad.app
```

The ignored NTSC-U BIN was cloned into that temporary package as
`assets/ctr-u.bin`. The package received a local ad-hoc signature, passed
strict deep verification, installed on Simulator
`D80E9862-C29A-4D69-B8E5-D81D396C17D5`, and launched as
`io.github.chrissotraidis.ctrpad`.

Launch identity and runtime state were:

```text
build ID:          afb5463cc511
window:            1376x1032 points, 1376x1032 pixels
presentation:      framebuffer=1, renderbuffer=1
adapter:           Apple Software Renderer, Apple Inc.
API:               OpenGL ES 3.0 APPLE-23.1.1
shader language:   OpenGL ES GLSL ES 3.00
PSX shaders:       4-bit, 8-bit, 16-bit, RGBA ready
VRAM pipelines:    ready
audio:             CoreAudio, 44100 Hz, stereo, 1024 sample frames
```

Computer Use drove the Simulator Home button and the CTRPad SpringBoard icon.
The exact log contains two complete repetitions:

```text
[CTR Lifecycle] event=will-enter-background phase=will-background audio=suspended quit=0
[CTR Lifecycle] event=did-enter-background phase=background audio=suspended quit=0
[CTR Lifecycle] event=will-enter-foreground phase=will-foreground audio=suspended quit=0
[CTR Lifecycle] event=did-enter-foreground phase=active audio=active quit=0
```

After the first resume, visual inspection showed the retail intro with Crash
above N. Tropy and other karts. After the second resume, it showed Crash and
the trophy over the animated checkered title background. Geometry, colors,
alpha, text and textures remained coherent; neither resume produced a black
or stale SpringBoard frame.

The local-only exact log is 27 lines and has SHA-256
`30f5b5ec516ffae64f05dc23e4570b438cdadb15858189439749572c7d021ec3`.
It remains outside Git. The package executable after local signing has SHA-256
`bffddcaf98a990c50d67f0b97457af15b271e69f73d3ab660c0b32a13114bb23`.
No retail-derived screenshot or package is tracked.

The final process was boundedly terminated through `simctl`; this does not
claim a natural `SDL_EVENT_TERMINATING` delivery or save-on-termination proof.

## Simulator cadence diagnosis

The iOS internal FPS window was reduced from 2,000 to 120 rendered frames so a
short device lifecycle run exposes cadence. Exact windows around the two
lifecycle cycles were 9.08, 7.04, 8.37, 9.56 and 7.41 FPS. They vary with the
active retail intro/title scene and are far below the 29.909-Hz target.

To separate pacing from renderer cost, the yielding dirty build launched with
the existing `--perf` instrumentation. Ctrl-C ended the console-bound process
while row 324 was being written, so that partial row and the absent shutdown
summary were rejected. The remaining 324 complete CSV rows were analyzed:

```text
complete rows:                         324
average total:                         119.983 ms (8.335 FPS)
average work:                          109.354 ms
average renderer_draw_triangles_ms:    105.157 ms
average VBlank wait:                    10.629 ms
average swap:                            3.364 ms
average framebuffer store:               4.857 ms
maximum total:                         519.623 ms
```

The CSV has SHA-256
`d0f1e10eb90de0696e74def5cf0cf789a49276147c0430b247ca04958d2fdd7a`
and remains local-only. GLES disables this renderer's GPU timer extension, so
the GPU CSV contained only its header. The CPU scope shows that the Simulator's
software `glDrawArrays` path, not the 10.629-ms average VBlank wait, dominates
the missed budget. This is a diagnosis, not a waiver: device hardware must be
measured before cadence can be accepted.

## Rejected and bounded diagnostics

- Attaching LLDB to the Simulator process stalled during attach and suspended
  the app. The orphaned debugger was terminated and the process resumed. No
  state or cadence conclusion was drawn from that attempt.
- Two `sample` attempts similarly remained blocked without a usable report;
  those diagnostic processes were terminated. No profile claim is based on
  them.
- The partial performance row and missing shutdown summary were excluded as
  described above; only complete CSV records were aggregated.
- The two launch-time `SDL_uikitviewcontroller` unbalanced
  appearance-transition warnings repeated after the display-loop change.
  Therefore the change did not fix them, and rotation/view-controller lifecycle
  remains open.
- A duplicate WebCore/WebKit accessibility-bundle class message is recorded as
  an iOS 26.5 Simulator runtime warning, not attributed to CTRPad.

## Exact clean cross-target matrix

All producers below embed `afb5463cc511` and were explicitly reconfigured
after the commit so the build ID is not inherited from a dirty producer.

```text
macOS ARM64 desktop-GL app
  CTest:       19/19 in 1.07 seconds
  signature:   strict deep ad-hoc verification passed
  architecture: Mach-O 64-bit arm64
  SHA-256:     0804b67d0e25233115f65035f434aa9b557465c29e01e3fba543063b56b697ec

macOS ARM64 GLES configuration
  CTest:       19/19 in 0.65 seconds
  runtime:     expected diagnostic exit 1; Cocoa ANGLE/EGL unavailable
  architecture: Mach-O 64-bit arm64
  SHA-256:     bf60b6668515aa9e77fff6998f2d71db6e03b780838706311caf1743559edcf6

combined ASan/UBSan ARM64
  CTest:       19/19 in 6.28 seconds
  ASan:        detect_leaks=0, halt_on_error=1, abort_on_error=1
  UBSan:       halt_on_error=1, print_stacktrace=1
  finding:     none
  architecture: Mach-O 64-bit arm64
  SHA-256:     6efa286e64c0a2ff998d04fb412441546ea608b5058c114d365b4cd302a77401

iOS Simulator ARM64 before local package signing
  platform:    IOSSIMULATOR, iOS 15.0 floor, SDK 26.5
  SHA-256:     b1161f25d842fac4a9a56f59b0cf58b7344999681fd9b3ed54bc6b0ce5176c84

iOS device ARM64, unsigned and not run
  platform:    IOS, iOS 15.0 floor, SDK 26.5
  SHA-256:     8364ec5c2935e6bd18582f1e8c4f476ab5334a96c55c54e1bdc98d28213b1546

optimized Linux i686
  CTest:       19/19 in 10.34 seconds
  architecture: ELF 32-bit LSB PIE, Intel 80386, GNU/Linux 3.2.0
  GNU Build ID: d032b695e7957142bc16a754a8af8a2946dab2b5
  SHA-256:     5f1f8b06ceacbd4d4bd80e2c0e62f056faaddac0e1d81a48c67f6c9d80abdc65
```

The normal Apple/iOS compiles repeated the established 32 warnings. The
sanitizer compile repeated the established 59 warnings, and the i686 compile
repeated four established warnings. No new warning was accepted for this
implementation.

## Acceptance boundary

Accepted at this checkpoint:

- SDL lifecycle events are observed through their required synchronous route;
- repeated background/foreground host transitions are idempotent;
- input, output audio and the host VBlank deadline have explicit suspension
  boundaries;
- the iOS outer loop returns to UIKit between retail steps;
- direct process exit is removed from normal SDL quit/window-close handling;
- the iOS wait contains no project spin and no `SDL_DelayPrecise` final spin;
- exact clean Simulator GLES rendering/audio survives two complete Home/resume
  cycles; and
- the media-free lifecycle contract passes across the tested host widths and
  sanitizer configuration.

Still open:

- the repeated UIKit appearance-transition warning and rotation lifecycle;
- real iPad hardware cadence and energy behavior;
- a fully nonblocking display scheduler rather than a synchronous retail step
  inside each display callback;
- MFi/Bluetooth controller play and hardware audio/XA/STR behavior;
- background/save atomicity and relaunch persistence in the iOS sandbox;
- natural termination and low-memory delivery on device;
- document-picker retail import; and
- touch controls and complete races.

Neither M7 nor M8 is marked complete by this checkpoint.

## Elapsed time and concurrent verifier

The previous documented checkpoint ended at 150,887 goal seconds. The exact
validation reading for this slice was 154,273 seconds, or 1 day, 18 hours,
51 minutes, 13 seconds cumulative. That is an interval of 3,386 seconds
(56 minutes, 26 seconds). The goal timer measures cumulative product-task time,
not labor effort or a performance benchmark.

The final pre-publication documentation/keyboard/i686 audit reading was
155,756 seconds, or 1 day, 19 hours, 15 minutes, 56 seconds cumulative. The
follow-up audit added 1,483 seconds (24 minutes, 43 seconds); the full interval
from the preceding 150,887-second checkpoint was therefore 4,869 seconds
(1 hour, 21 minutes, 9 seconds).

The protected historical i686 alternate-loader verifier was not rebuilt,
paused, restarted or terminated. At this checkpoint Docker reported it
running, unpaused and not OOM-killed. Its machine-owned exit-status file was
still zero bytes. `playback-2.log` had crossed nine 2,000-frame FPS windows;
the most recent recorded windows were 0.61 and 0.99 FPS. Completion, process
exit, alternate raw-layout separation and deliberate mutation remain
unaccepted until the script writes its final status.
