# ref/

Reference implementations and upstream repositories. **Read-only.** Nothing in here is modified; these are clones kept for source reading, diffing, and provenance checks. If work begins, fork properly rather than editing in place.

Cloned 2026-07-29.

| Directory | Origin | HEAD at clone | License | Why it is here |
|---|---|---|---|---|
| `ctr-native/` | https://github.com/CTR-tools/ctr-native | `2df55dc5a` (2026-07-26), version `0.1.0-beta.7.1` | GPL-3.0 since 2026-06-25 | The port under assessment. 6,629 commits, 255 game `.c` files, ~190k lines across `game/`, `platform/`, `include/`. |
| `CTR-ModSDK/` | https://github.com/CTR-tools/CTR-ModSDK | `e3f19686` (2026-07-05) | GPL-3.0 since 2026-06-25 | The decompilation ctr-native is built on. 5,480 commits. Externals are submodules, not vendored, so they are absent from this clone. |
| `CTR-in-C/` | https://github.com/CTR-tools/CTR-in-C | `ab59a2b4` (2026-07-24) | **No LICENSE file** | Disassembly and decompilation research. Contains the byte-matching track. Treat as all-rights-reserved. |
| `CTR-PC-Port/` | https://github.com/NyperYuhgard/CTR-PC-Port | `970f381` (2026-07-28) | **No LICENSE file** | A detached copy of ctr-native (not a GitHub fork) adding 8-player UDP netplay. Predates the MSVC/CMakePresets refactor. Redistributes GPLv3 code without license text, so it is not a viable base to build on. Kept for the netplay implementation only. |

## Not cloned, but relevant

- https://github.com/Simon358/ctr-native-android: branch `feature/add-android-support` carries a working OpenGL ES 3 renderer path for this exact codebase. The single most useful artifact for a GLES retarget. Clone this before starting renderer work.
- https://github.com/OpenDriver2/PsyCross: MIT-licensed origin of ctr-native's platform layer, PsyQ facade, GTE core, and renderer. Not vendored by ctr-native, which owns modified derivatives instead.
- https://github.com/libsdl-org/SDL: already vendored inside `ctr-native/externals/SDL` at version 3.4.10, full source including the iOS backends.

## Assets

None of these repositories ship game assets, and none should. ctr-native requires a user-supplied NTSC-U disc image at `assets/ctr-u.bin` in raw MODE2/2352 format. Do not commit game data to this repository.

## See also

[`docs/ctr-native-viability.md`](../docs/ctr-native-viability.md) for the full assessment these clones were read for.
