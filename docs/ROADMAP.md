# CTRPad Roadmap

**Mission:** deliver a native ARM64 Crash Team Racing application for macOS,
iOS, and iPadOS that a user can sideload, supply with their own NTSC-U retail
disc image, and play with controller or touch input at retail physics and
timing.

**Roadmap status:** active

**Current milestone:** M1 — reproducible upstream baseline and parity gate

**Last updated:** 2026-07-29

This is the working source of truth for the port. Milestone status changes only
after its acceptance evidence has been recorded. A successful compile is
evidence, not completion.

## Non-negotiable constraints

- Apple targets are ARM64 and therefore require a correct 64-bit memory model.
  The existing 32-bit configuration gate represents real pointer and serialized
  layout contracts, not an arbitrary build restriction
  (`docs/ctr-native-viability.md:56-69,152-193`).
- The macOS ARM64 build is the diagnostic and parity gate before iOS lifecycle,
  sandbox, signing, and touch work (`docs/ctr-native-viability.md:443-446`).
- Physics and timing drift is a port defect. The fixed-point MIPS/GTE layer is
  already intended to preserve console integer semantics across architectures
  (`docs/ctr-native-viability.md:243-258`).
- Game assets are never committed. Distribution requires the user to supply a
  retail NTSC-U raw MODE2/2352 image.
- Distribution is sideload-only. The application, build instructions, and
  installation information will be published under GPL-3.0; App Store
  submission is out of scope (`docs/ctr-native-viability.md:378-385`).
- Nonobvious technical decisions, failures, and parity results are documented
  under `docs/` as they happen.

## Actual repository inventory

Inventory performed on 2026-07-29 before implementation:

- Git `main` is at `95417c7` and matches `origin/main`. It has three tracked
  files: `.gitignore`, `docs/ctr-native-viability.md`, and `ref/README.md`.
- There is no game source, `CMakeLists.txt`, `game/`, `platform/`, `include/`,
  or vendored SDL tree in this checkout.
- `docs/` contains only the viability report. No prior roadmap, architecture
  decision, build log, or parity result exists.
- Contrary to `ref/README.md:5-14`, the documented `ctr-native`,
  `CTR-ModSDK`, `CTR-in-C`, and `CTR-PC-Port` clones are absent on disk.
- `ref/CTR/` is present and untracked. It contains `CTR.ccd`, `CTR.img`, and
  `CTR.sub`, not the `assets/ctr-u.bin` path expected by ctr-native.
- `CTR.ccd` describes one MODE 2 data track (`ref/CTR/CTR.ccd:6-19,78-79`).
  `CTR.img` is 740,179,104 bytes, exactly 314,702 sectors of 2352 bytes.
  `CTR.sub` is 30,211,392 bytes, exactly 314,702 records of 96 bytes. This
  establishes CloneCD raw-sector structure; NTSC-U identity and loader
  validation remain to be proven.
- `.gitignore:1-8` ignores the four absent reference clone names and
  `.DS_Store`, but does not ignore `ref/CTR/`, disc images, or extracted
  `.BIG`, `.HWL`, `.XA`, and `.STR` assets. Asset ignore protection is
  currently inadequate.
- The development host is Apple Silicon (`arm64`) on macOS 26.5 with Xcode
  26.6, Apple Clang 21.0.0, the macOS/iPhoneOS/iPhoneSimulator 26.5 SDKs,
  CMake 3.27.1, and Ninja 1.13.2.

## Dependency map

```text
M0 repository foundation
  -> M1 reproducible 32-bit baseline and parity harness
  -> M2 64-bit memory-model decisions
  -> M3 mechanical pointer-width and runtime-layout conversion
  -> M4 serialized asset relocation conversion
  -> M5 scratchpad and remaining pinned-layout conversion
  -> M6 correct macOS ARM64 build
  -> M7 shared GLES renderer
  -> M8 iOS/iPadOS application with controller input
  -> M9 sandbox storage and retail-disc import
  -> M10 touch controls
  -> M11 parity, signed packaging, documentation, and publication
```

M7 can be researched against a 32-bit build while M2–M5 are underway, but it
does not bypass M6 as the acceptance gate. M10 does not start until the game
runs on an iPad with a physical controller.

## Milestones

### M0 — Repository and evidence foundation

**Status:** completed 2026-07-29

Result:

- Retail and reference paths are protected by `.gitignore:1-35`; the upstream
  `assets/` exclusion remains at `.gitignore:40`.
- Clean reference clones are recorded in `ref/README.md` at `2df55dc5a` for
  upstream and `34648097d` for the Android GLES branch.
- Commit `268ff6977` joins the CTRPad documentation ancestry with the unchanged
  upstream source ancestry. `git diff upstream/master` reports no difference
  in `CMakeLists.txt`, `CMakePresets.json`, `main.c`, `game/`, `include/`,
  `platform/`, or `externals/`.
- The implementation branch is `codex/arm64-apple`; `upstream` points to
  `https://github.com/CTR-tools/ctr-native.git`.
- `git ls-files` finds no retail-media extension, and representative
  `git check-ignore -v` probes pass for the raw image and extracted formats.

Work:

1. Extend `.gitignore` to cover `ref/CTR/`, all raw/cooked disc image formats,
   and the extracted retail asset extensions and paths.
2. Clone `CTR-tools/ctr-native` read-only into `ref/ctr-native`, initially at
   the viability baseline `2df55dc5a`, then record the current upstream head.
3. Clone `Simon358/ctr-native-android` and its
   `feature/add-android-support` branch into `ref/` before renderer work.
4. Correct `ref/README.md` to reflect what is actually cloned, including exact
   origins, branches, and commits.
5. Add `CTR-tools/ctr-native` as the upstream Git remote. Create the downstream
   `codex/arm64-apple` work branch and incorporate upstream history into this
   repository while preserving the existing CTRPad documentation commit. The
   reference clone remains read-only; implementation happens in the downstream
   working tree.
6. Add a decision log and a results area under `docs/`.

Acceptance:

- `git check-ignore -v` proves that `.bin`, `.img`, `.iso`, `.cue`, `.ccd`,
  `.sub`, `.BIG`, `.HWL`, `.XA`, and `.STR` retail material cannot be added by
  an ordinary `git add`.
- `git status` does not enumerate any retail files.
- Reference origins and exact commits in `ref/README.md` match `git remote` and
  `git rev-parse` output.
- The downstream work branch contains both the CTRPad documentation history
  and unmodified upstream source history.
- No reference clone or retail asset is tracked.

### M1 — Reproducible upstream baseline and parity gate

**Status:** in progress; M0 completed

Work:

1. Reproduce upstream's supported 32-bit Linux build in a pinned container or
   CI environment and run it with the retail image.
2. Read the checkpoint, replay scheduler, savestate, and CTest paths in full.
3. Determine the strongest deterministic comparison available: per-frame pad
   snapshots, checkpoints, serialized state hashes, render-independent game
   state, or a combination.
4. Create a repeatable golden run covering startup, menus, a race, powerslide
   boosts, items, lap completion, and save/load.
5. Record exact commands, compiler versions, inputs, hashes, outputs, known
   nondeterminism, and failures in `docs/parity/`.
6. Attempt an unchanged Apple ARM64 configure to preserve the expected
   64-bit-gate and `-msse` failure evidence.

Acceptance:

- The supported 32-bit build and golden run are reproducible from documented
  commands.
- The parity gate fails when a known game-state byte is perturbed.
- The gate distinguishes gameplay state from allowed host-only differences
  such as window handles, file paths, clocks, and native pointer values.
- If existing tooling cannot provide a trustworthy gate, its exact limitation
  and the replacement harness are documented before 64-bit structural work.

### M2 — 64-bit memory-model design and layout census

**Status:** pending; depends on M1

Work:

1. Recount pointer narrowing, integer-pointer fields, pointer-bearing
   size-pinned structs, scratchpad `Ptr32` slots, and in-place asset relocation
   sites against the incorporated upstream commit.
2. Classify every affected field as:
   - serialized/file-layout guest value;
   - scratchpad/retail-layout guest value;
   - resident executable-map guest value;
   - runtime-only host pointer;
   - opaque retail address label never dereferenced.
3. Prototype the two load-bearing approaches against one real asset:
   centralized guest offsets/handles with boundary translation, and a bounded
   guest arena. Do not depend on a low-address host allocation that Apple
   platforms cannot guarantee.
4. Write an architecture decision record with measured tradeoffs and
   migration rules.

Acceptance:

- Every pointer-bearing pinned field has an owner and migration decision.
- A real relocated asset can be loaded and traversed without truncating a host
  pointer.
- Corrupt/out-of-range guest references fail deterministically with a useful
  diagnostic.
- The chosen representation preserves on-disc and retail scratchpad byte
  layouts.

### M3 — Mechanical pointer-width and runtime-layout conversion

**Status:** pending; depends on M2

Work:

- Replace pointer-narrowing round trips with `uintptr_t`, native pointers, or
  the selected guest-reference type according to the census.
- Convert runtime-only integer pointer fields and parameters.
- Relax or split layout assertions only where a structure is proven
  runtime-only; retain guest-layout assertions.
- Make SSE flags architecture-conditional.
- Add `-fno-strict-aliasing` and `-fwrapv` for compilers that support them.
- Lift the global 32-bit gate only when targeted assertions and diagnostics
  replace it.

Acceptance:

- Apple Clang ARM64 compiles the converted mechanical surface without
  pointer-to-int or int-to-pointer truncation warnings.
- A checked/sanitized build reports no new pointer overflow, alignment, or
  bounds failures in the golden run.
- The 32-bit golden baseline remains byte-identical in game-visible state.

Expected commit sequence: build portability flags; guest-reference primitives;
runtime-only field conversions by subsystem; parameter/cast cleanup; assertion
split; gate replacement.

### M4 — Serialized asset relocation

**Status:** pending; depends on M2 and M3

Work:

- Replace `LOAD_RunPtrMap` host-pointer writes into 32-bit asset slots with the
  selected guest-reference representation.
- Convert every relocated model, level, string-table, and command-list
  consumer through checked translation boundaries.
- Integrate the checkpoint pointer-slot registry where useful instead of
  creating a second unsynchronized relocation inventory.
- Add malformed-map, overflow, duplicate-patch, and out-of-range tests.

Acceptance:

- The full retail asset set validates and loads on 32-bit and ARM64 hosts.
- No loaded file image contains a truncated native pointer.
- Golden races spanning multiple tracks, models, menus, and credits match the
  32-bit baseline.
- Asset translation failures name the asset, offset, and violated range.

### M5 — Scratchpad and remaining pinned layouts

**Status:** pending; depends on M3 and M4

Work:

- Convert DrawLevel, RenderBucket, Torch, and other `Ptr32` scratchpad slots
  without changing retail offsets.
- Finish resident executable-map and MEMPACK pointer-field conversion.
- Convert checkpoint/savestate formats that currently serialize host addresses.
- Version changed developer-tool formats and retain explicit compatibility
  handling.

Acceptance:

- All scratchpad size and offset assertions still pass.
- Renderer-heavy scenes, split-screen paths, effects, and allocation churn pass
  sanitizer runs.
- Save/checkpoint/replay round trips are deterministic across process image
  bases.
- The repository-wide census has no unexplained native-pointer narrowing.

### M6 — Correct macOS ARM64 desktop build

**Status:** pending; depends on M1–M5

Work:

- Add a documented macOS ARM64 CMake preset and bundle/run workflow.
- Validate audio, desktop renderer, keyboard, MFi/Bluetooth controller,
  memcards, replays, savestates, XA audio, and STR video.
- Run sanitizers and the full parity gate under Apple Clang.
- Measure frame cadence against the retail 30 Hz logic / approximately
  59.817 Hz VBlank model.

Acceptance:

- A clean checkout configures, builds, and launches natively on Apple Silicon.
- The golden suite matches game-visible 32-bit state exactly.
- At least one complete Adventure-mode progression segment and representative
  Arcade races play without corruption, crash, or timing drift.
- Saves persist across relaunch.
- No unresolved sanitizer finding or pointer-width warning remains.

M6 is a hard gate. iOS platform work does not proceed on an incorrect macOS
build.

### M7 — Shared OpenGL ES 3 renderer

**Status:** pending; research may overlap M2–M5; acceptance depends on M6

Work:

- Diff Simon358's GLES branch against the exact upstream baseline and port only
  understood changes.
- Add an SDL GLES 3 context path, ES 3 shaders, RGBA readback/repack, and
  platform guards for unsupported debug-only desktop GL features.
- Keep the existing 24-bit GPU-link bridge unchanged unless a failing test
  demonstrates a defect.
- Validate desktop GL and GLES output using frame captures and game-state
  parity.

Acceptance:

- macOS can run the shared GLES path and complete the golden suite.
- Representative frame captures have no missing primitives, incorrect CLUT
  colors, mask-bit failures, transparency regressions, or framebuffer-feedback
  artifacts.
- Renderer choice does not alter game-visible state or frame cadence.

### M8 — iOS/iPadOS application with controller input

**Status:** pending; depends on M6 and M7

Work:

- Add device and simulator CMake presets, app-bundle metadata, orientations,
  launch assets, and signing configuration that does not embed personal team
  credentials in source.
- Adopt SDL's iOS-owned entry point and lifecycle.
- Replace direct process exit with suspend/resume-safe state transitions.
- Replace battery-hostile spin pacing with an iOS-appropriate display-driven
  mechanism while retaining retail timing.
- Bring up audio, GLES, and MFi/Bluetooth controllers on a real iPad.

Acceptance:

- A development-signed build installs and launches on a real ARM64 iPad.
- With a controller, the user can navigate menus, complete races, hear XA/audio,
  view STR video, suspend/resume, and relaunch.
- Backgrounding does not corrupt saves, audio, renderer state, or timing.
- Controller gameplay matches the macOS parity run.

### M9 — Sandbox storage and retail-disc import

**Status:** pending; depends on M8

Work:

- Separate immutable bundle resources, imported retail media, user-visible
  documents, and private preference/save paths.
- Use an iOS document picker / Files integration to import or securely reference
  the user's image.
- Validate raw MODE2/2352 structure and NTSC-U identity with clear, actionable
  errors; accept the current CloneCD image only after loader validation.
- Persist memcards, settings, logs, and crash diagnostics in appropriate
  sandbox locations.

Acceptance:

- A fresh install with no asset presents an import flow, not a crash or terminal
  log.
- Importing a valid user-supplied image reaches the game without extraction.
- Cooked ISO, wrong-region, truncated, and inaccessible files receive distinct
  errors.
- Saves persist across launch, backgrounding, app updates, and asset
  re-selection.
- No retail byte is included in the application bundle or Git history.

### M10 — Touch-first controls

**Status:** pending; depends on M8 and M9

Work:

- Add touch as a peer in the platform input composition path so it can coexist
  with a connected controller.
- Preserve true analog steering and multi-touch holds.
- Iterate on layouts for steering + held drift + three boost taps, the core
  simultaneity problem identified in the viability report
  (`docs/ctr-native-viability.md:319-358`).
- Cover menu navigation, accelerate, brake/reverse, hop/drift, fire/aim,
  camera, pause, and race-start skip without obscuring critical play space.
- Add safe-area, hand-size, handedness, opacity, scale, and remapping options as
  testing justifies them.

Acceptance:

- Touch-only users can start the app, select content, race, pause, and save.
- Steering remains continuously analog while accelerate and drift are held.
- A tester can intentionally execute repeated three-boost drift chains in both
  turn directions without grip changes or missed simultaneous contacts.
- Controls work across supported iPad aspect ratios, orientations, and safe
  areas and meet practical touch-target/contrast accessibility requirements.
- Connecting a controller does not require disabling touch and causes no stuck
  inputs.

### M11 — Retail parity, signing, and GPL-compliant release

**Status:** pending; depends on all prior milestones

Work:

- Run the complete cross-architecture parity suite and prolonged playtesting.
- Produce reproducible macOS and iOS/iPadOS release builds.
- Document Xcode, direct-device, and AltStore-style sideload installation.
- Publish complete corresponding source, build scripts, notices, modifications,
  and Installation Information; exclude all game assets and signing secrets.
- Record known limitations honestly.

Acceptance:

- A clean machine can build from the published source and instructions.
- A person can sign, install, import their own valid NTSC-U image, play with
  touch, and retain saves.
- Physics, input timing, replay/checkpoint state, and frame pacing satisfy the
  parity gate on macOS ARM64 and iPad hardware.
- The released archive contains no retail assets, credentials, or undocumented
  generated dependencies.
- License texts, third-party notices, source offer, and modification/install
  information are complete.

## Verification strategy

Every structural change is checked at four levels:

1. **Compile-time layout:** static size/offset assertions remain for retail and
   serialized representations.
2. **Runtime safety:** bounds-checked guest translation plus ASan/UBSan where
   supported.
3. **Determinism:** identical scripted inputs produce identical game-visible
   checkpoint/state hashes against the 32-bit golden run.
4. **Product behavior:** controller and touch playtesting, frame captures,
   audio/video checks, save/relaunch, and device lifecycle.

Visual similarity or successful play alone cannot clear a parity milestone.
Conversely, host pointer values, OS paths, renderer object names, and wall-clock
timestamps are explicitly excluded from game-visible deterministic state.

## Open risks

| Risk | Current evidence | Mitigation / exit criterion |
|---|---|---|
| Guest-reference design expands into a full arena rewrite | Asset relocation writes host bases into 32-bit file slots; about 75 pinned pointer-bearing structs were estimated | M2 prototype on real assets before broad edits; preserve guest layout and centralize translation |
| Existing replay/checkpoint tooling is not a sufficient parity oracle | Prior report found infrastructure but did not run or assess coverage (`docs/ctr-native-viability.md:471-474`) | Prove mutation sensitivity in M1 or build a state-hash harness |
| Current retail image is not the required NTSC-U revision | Container format is proven MODE2/2352, game identity is not | Validate through loader and disc metadata before relying on it |
| Upstream has moved since `2df55dc5a` | Viability report is commit-specific | Freeze a reproducible baseline, inspect current head, then rebase intentionally |
| Reference documentation is stale | `ref/README.md` claims four clones that are absent | Derive documentation from actual remote/commit checks |
| Retail assets can be committed accidentally | Current `.gitignore` does not cover them | Complete M0 ignore checks before any source import commit |
| Apple lifecycle/pacing changes perturb timing | Current code blocks and spin-waits around synthetic VBlank | Compare game-state timing and measured cadence before/after display integration |
| Touch boost chains are ergonomically poor despite correct input injection | Three boosts require steering + drift hold + repeated taps | Device prototypes and repeated triple-boost usability criterion |
| Signing/publishing depends on credentials and external accounts | Not yet inventoried | Keep configuration credential-free; verify available team/device before release milestone |
| GPL provenance and commercial-IP exposure need legal judgment | Facts are documented but not legal advice (`docs/ctr-native-viability.md:363-400`) | Sideload-only, publish corresponding source, and obtain counsel review before public release |

## Decision log

- **2026-07-29 — Start from evidence, not the prior “wait” verdict.** The goal
  authorizes proceeding. Estimates and unknowns in the viability report will be
  replaced with build results.
- **2026-07-29 — Preserve upstream history in the downstream repository.**
  `ref/ctr-native` remains read-only. Implementation will occur on a dedicated
  downstream branch whose history incorporates upstream and the existing
  CTRPad documentation.
- **2026-07-29 — Treat the present CloneCD image as unvalidated retail input.**
  MODE2/2352 structure is proven; region/revision and loader compatibility are
  not.
- **2026-07-29 — Mac ARM64 retail parity is the iOS gate.** iOS complexity will
  not be used to debug unresolved 64-bit state corruption.
