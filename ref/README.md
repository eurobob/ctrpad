# ref/

Reference implementations, upstream repositories, and local retail media.
**Read-only.** Repository clones in this directory are kept for source reading,
diffing, and provenance checks. Implementation happens in the CTRPad working
tree, never by modifying a reference clone.

Cloned 2026-07-29.

| Directory | Origin | HEAD at clone | License | Why it is here |
|---|---|---|---|---|
| `ctr-native/` | https://github.com/CTR-tools/ctr-native | `2df55dc5a` (2026-07-26), version `0.1.0-beta.7.1` | GPL-3.0 since 2026-06-25 | The port under assessment. 6,629 commits, 255 game `.c` files, ~190k lines across `game/`, `platform/`, `include/`. |
| `ctr-native-android/` | https://github.com/Simon358/ctr-native-android | `34648097d` on `feature/add-android-support` (2026-07-10) | GPL-3.0 | The Android fork's OpenGL ES 3 renderer and mobile build changes. It remains a 32-bit Android port and is renderer reference material only. |

The reference clones were re-verified after cloning with clean worktrees and
the exact origins and commits above. Upstream `ctr-native` remained at the
viability report's `2df55dc5a` baseline on 2026-07-29.

## Relevant but not currently cloned

- https://github.com/CTR-tools/CTR-ModSDK at the previously assessed
  `e3f19686`: the decompilation ctr-native is built on.
- https://github.com/CTR-tools/CTR-in-C at the previously assessed
  `ab59a2b4`: disassembly and decompilation research with no license file.
- https://github.com/NyperYuhgard/CTR-PC-Port at the previously assessed
  `970f381`: detached netplay copy with no license file; not a viable base.
- https://github.com/OpenDriver2/PsyCross: MIT-licensed origin of ctr-native's platform layer, PsyQ facade, GTE core, and renderer. Not vendored by ctr-native, which owns modified derivatives instead.
- https://github.com/libsdl-org/SDL: already vendored inside `ctr-native/externals/SDL` at version 3.4.10, full source including the iOS backends.

## Assets

None of the reference repositories ship game assets, and none should.
ctr-native requires a user-supplied NTSC-U disc image at `assets/ctr-u.bin` in
raw MODE2/2352 format.

The local, gitignored `CTR/` directory currently contains a user-supplied
CloneCD `CTR.ccd` / `CTR.img` / `CTR.sub` triplet. `CTR.ccd` declares MODE 2;
`CTR.img` is exactly 314,702 2352-byte sectors and `CTR.sub` is exactly 314,702
96-byte subchannel records. These measurements establish the container layout,
not the disc region or revision. Loader validation must prove that the image is
the required NTSC-U retail data before it becomes a test baseline.

Never commit, copy into tracked paths, or distribute retail media or extracted
game files.

## See also

[`docs/ctr-native-viability.md`](../docs/ctr-native-viability.md) for the full assessment these clones were read for.
