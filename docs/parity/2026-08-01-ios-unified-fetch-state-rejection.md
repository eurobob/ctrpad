# iOS unified framebuffer-fetch state batching: rejected experiment

**Date:** 2026-08-01

**Result:** rejected; no renderer source from either prototype is retained

**Starting revision:** `9a33ae00efb6f47fe10453d09a6ab8e27de651e0`

**Accepted renderer remains:** same-state coherent-fetch batching from
`13f260cb8a0d`

## Purpose and gate

The accepted coherent-fetch renderer still averaged 165.879 ms for the exact
119-split / 78-semitransparent Crash Cove state on the iPad Simulator's Apple
Software Renderer. It issued 66 host draws after conservative adjacent batching.
This experiment asked whether carrying more PS1 state per primitive could join
additional adjacent logical splits without changing their retail order or pixels.

The gate was deliberately stronger than a lower draw-call count:

1. preserve the 20-byte packed `GrVertex` ABI and the established logical trace;
2. preserve every pixel-oracle byte on desktop fallback and enabled iOS GLES;
3. render the real imported NTSC-U route coherently through Crash Cove;
4. improve, or at minimum not materially regress, a matched live frame state;
5. keep both named simulators and the Simulator GUI off for every compile, use
   `nice 15`, one build job, and boot only the disposable simulator for runtime;
6. preserve the imported retail image and memory card exactly.

Both prototypes passed the first three correctness gates. Both failed the matched
performance gate, so their source was restored to the published baseline instead
of being committed.

## Prototype A: one dynamically branched fetch shader

The first prototype reused the final two previously reserved `GrVertex` bytes:
one encoded the texture format and one packed blend mode, textured semitransparency,
sampled STP, and draw-mask state. These renderer-only bytes were populated only
after `NativeGpu_RenderTraceFlush`, keeping the historical logical trace zeroed.
One coherent framebuffer-fetch fragment shader dynamically selected 4-, 8-, or
16-bit PS1 sampling and read all remaining state per primitive. Adjacent eligible
native-VRAM splits could therefore join across texture-format and blend/STP/mask
changes while retaining API primitive order.

The prototype built and passed exact byte checks. Its enabled iOS oracle reduced
the ordered-overlap blend fixture to one draw and the mixed 4/8/16-bit,
opaque/semitransparent/mask fixture to two draws. Logical, blend and presentation
hashes remained:

```text
logical/RGBA      851169f2644a1675
blend             0c0d08324ae06c35
desktop present   a7798c5a6ddee965
iOS present       172d49a34571b64c
```

The unsigned iOS executable hashed to
`3e5e962a268e15b6cf48de94d5f8aa728d19be4285bdc0278600d53469dc5056`.
Its isolated, strict/deep-verified ad-hoc-signed copy hashed to
`146c3600ed5776afab9eeee5101e28d002cd81b8c610de0ec701a1744c1ab9dd`.

The live result was decisively worse. In the exact 119/78 logical state it issued
only 26 calls, but averaged 248.567 ms total and 204.207 ms in renderer triangle
submission. The neighboring 122/79 state averaged 255.827 / 211.549 ms. The
dynamic per-fragment texture-format branch saved API calls while making Apple's
software rasterization substantially more expensive.

Correct-ID termination finalized 1,704 CSV lines at SHA-256
`23a10888ac6ca5c5f3a286cdaa92f78153fea56e54e2953eadff4ddcc728eb07`.
The associated 58-line application log hashed to
`2b1b707cb728964d0af57a403b11da9aa19585800076b28dfa0480cbe3c0f70c`.
This design was rejected before publication.

## Prototype B: three texture-format-specialized fetch shaders

The second prototype removed the dynamic texture-format branch. It left one
reserved vertex byte untouched, packed only PS1 blend/semitransparency/STP/mask
state into the final byte, and compiled separate coherent state-fetch shaders for
4-, 8-, and 16-bit textures. Batching required one texture format but could still
cross blend, opaque/semitransparent, STP, draw-mask and texture-page changes whose
effective values were already carried by the vertices. Unsupported and desktop
paths continued to use the published uniform/two-pass shaders.

The pixel gate again passed exactly. Desktop fallback remained 12 active draws.
The enabled 1032×1376 iOS oracle reported:

```text
fallback-draws=12 active-draws=1 mixed-state-draws=4
hash=851169f2644a1675
blend-hash=0c0d08324ae06c35 blend-oracle=match
present-hash=172d49a34571b64c framebuffer-fetch=enabled
```

With both devices and the GUI off, the macOS ARM64 one-job build took 65.05
seconds, repeated only the established 32 warnings, and all 22 tests passed in
3.45 seconds. The iOS Simulator one-job build took 68.86 seconds with the same
warning set. The unsigned thin-ARM64 iOS executable hashed to
`448eb3146844301c2272bb2ca12f3945fb0810f0696bd8c56ad2193493100c3f`;
the isolated strict/deep-verified signed copy hashed to
`aeccf0e42dcdca729248e8bf420b4c5360015056f15ba40c57f00cfe30a7f7f3`.

Only `CTRPad Import Negatives`
(`26F3DEE8-8840-446D-85FE-C882009C9C06`) booted. The protected
`CTRPad Import Validation` device remained off. The install remapped the data
container to `5574F6D8-8DDF-4D98-A09C-AB0F9E75E171` without replacing the
imported media or save. Computer Use inspected the trophy/title, main menu,
character select, Crash Cove track selection, no-ghost prompt, loading preview,
and starting grid. The checkerboard/menu art, portraits and kart, track preview,
kart, Crash, track geometry and textures, HUD, minimap, lights, banner, sky,
reflections, transparency, and touch overlay were all present. No missing-asset
screen appeared on this bounded route.

Correct-ID termination finalized the second profile:

```text
frame CSV       1,072,472 bytes / 3,216 lines including header
frame CSV SHA   994a1f7917923eea39b669aaed6399c4bf6c57c84f174ce53aafe3e9aad21c2f
app log         7,867 bytes / 69 lines
app log SHA     ce135e14c9738c66b3ad18ef610b421c4c7cbbb5471eb1603c6a8a7ed1a52810
targeted faults 0
```

## Matched performance decision

The comparison selects the same logical 119-split / 78-semitransparent frame
state. It does not compare unrelated track positions.

| Renderer | Frames | Calls | Merged splits | Total ms | Triangle ms | Reciprocal FPS |
|---|---:|---:|---:|---:|---:|---:|
| published same-state batching | 145 | 66 | 56 | 165.879 | 136.514 | 6.03 |
| dynamic unified shader | 78 | 26 | 96 | 248.567 | 204.207 | 4.02 |
| format-specialized state shaders | 165 | 52 | 70 | 236.441 | 190.911 | 4.23 |

The specialized design removes another 14 calls from the published baseline
(-21.21%), but increases total frame cost by 70.562 ms (+42.54%) and triangle
submission by 54.397 ms (+39.85%). It is somewhat faster than the dynamic
prototype, which confirms that the per-fragment format branch was costly, but
it remains far slower than the published renderer. Lower call count is not a
product win when the actual frame becomes slower.

The accepted decision is therefore to retain `13f260cb8a0d`'s conservative
same-state batching and reject both unified-state variants. The next renderer
optimization must target the measured Apple Software Renderer cost with a
different mechanism and must pass this same matched-state gate.

## Preservation and cleanup

After the final profile, the authoritative imported data remained:

```text
NTSC-U BIN inode 111450682, 605,698,800 bytes
SHA-256          f780bf2331476aabfc00772fa758b12dd95ebfbc907968132cbd3cdd4e2c07c0
memory card      inode 111309627, 6,016 bytes
SHA-256          6a01b0f5562ed7a279d8f8e51e3b1874ac39a6120f55db4fe3873288950619a3
```

Correct-ID app termination closed the attached console. The disposable device
was shut down, the protected device was confirmed shut down, and `super+q`
closed the Simulator GUI. The two task-owned signed-copy directories were
removed only after their executable hashes and strict verification were recorded.
The installed app, simulator data, builds and all profile/log evidence remain.

The four candidate source files were restored through `apply_patch` to the
published revision. A low-priority one-job macOS rebuild then repeated the 32
known warnings, and all 22 tests passed in 3.18 seconds. The independent verbose
pixel test passed in 1.27 seconds with desktop 12/12 draws and the established
logical, blend and presentation hashes. `git diff --check` and final status found
no retained renderer change.

## Rejected commands and recoveries

The historical record keeps the unsuccessful paths rather than erasing them:

- Three early source patches had whitespace/macro context mismatches and applied
  nothing; smaller literal-context patches produced the tested prototypes.
- Read-only searches named absent `src/`, `tests/`, `scripts/`, or one obsolete
  platform path and emitted harmless path warnings.
- During prototype A, Computer Use briefly returned `noWindowsAvailable` after
  input had landed. A later CLI diagnostic accidentally included
  `--terminate-running-process`, restarting the exact bundle. The short first
  profile was discarded; the restarted, correctly terminated 1,704-line run is
  the only prototype-A profile cited above.
- During prototype B, the same transient Computer Use window error recovered by
  a fresh state request without restarting the application.
- The first retail/save hash command assumed an obsolete `retail/CTR.bin` path
  and found nothing. A read-only file census located the authoritative imported
  BIN under `Documents/CTRPad/assets/ctr-u.bin`; its inode and canonical hash
  matched the prior boundary.
- The first quit call used an unsupported `modifiers` argument and did not close
  Simulator. The documented xdotool-style `super+q` form succeeded, then direct
  process and device checks proved the GUI and both devices off.
- The first attempt to restore source fed a raw Git reverse diff to `apply_patch`
  and was rejected because the required patch wrapper was absent. A translator
  then missed reverse-diff `b/` prefixes, and its next form retained unsupported
  numeric hunk ranges; both applied nothing. The final wrapper emitted explicit
  update sections with context-only hunk headers and restored all four files.
- A verification loop temporarily named its local variable `path`, shadowing
  zsh's `PATH` only inside that process and causing `stat`/`find` command-not-found
  messages. Renaming it `artifact_dir` and using absolute tool paths verified the
  signed copies. An attempted `rm -rf` was policy-rejected before execution;
  explicit `rm -r` removed only the two verified `/tmp/ctrpad-*-dirty.*` paths.
- A broad fault scan matched the retail title `Crash Team Racing` inside two log
  path fields. The corrected severity/failure/asset-pattern scan returned zero.

None of these rejected routes changed retail media, save data, a protected
simulator, a Git commit, or a remote branch.

## Time boundary and remaining work

The rejection/restoration reading was 254,820 seconds: 2 days, 22 hours,
47 minutes cumulative, 2,945 seconds (49 minutes, 5 seconds) after the prior
251,875-second interactive handoff. Goal time includes the user's inspection,
two implementations, builds, signing, byte oracles, one-Simulator navigation,
slow live profiling, source restoration, cleanup, testing and documentation; it
is not a build benchmark or person-hour estimate.

The overall goal remains active. Simulator cadence, broader scene/effect churn,
full touch-race ergonomics, exact post-publication replay, Apple signing, the
physical-iPad gate and the final sideloadable package remain open.
