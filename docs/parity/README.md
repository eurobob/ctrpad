# Retail-Parity Evidence

This directory will contain the deterministic baseline definition, scripted
input fixtures that contain no retail data, comparison-tool documentation, and
per-milestone results.

No full acceptance baseline has passed yet. The 2,200-frame
startup/menu/loader/early-race prefix now matches across ARM64 and i686 on all
six canonical state components; it is narrower than the pending 24,232-frame
golden scenario.

Current evidence and procedures:

- `STATE-DIGEST.md` defines canonical schema 2 and its mutation/address tests.
- `NTSC-U-GOLDEN-RUN.md` defines the full version-4 acceptance procedure.
- `2026-07-29-golden-run-result.md` records the historical version-2 run and
  the input/timing omissions it exposed.
- `2026-07-30-arm64-prefix-parity.md` records the corrected cross-width prefix,
  sanitizer, and audit evidence.

The first task is to evaluate `platform/native_replay_scheduler.c`,
`platform/native_checkpoint.c`, `platform/native_savestate.c`, and
`docs/REPLAYS.md` against a running 32-bit build. A usable gate must:

1. replay identical per-frame inputs;
2. compare game-visible state without embedding host pointer values;
3. cover startup, menus, racing, powerslides, items, transitions, and saves;
4. detect an intentional state mutation;
5. report the first divergent frame and field or region.

Screenshots and playtesting supplement this gate; they do not replace it.
