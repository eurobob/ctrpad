# Retail-Parity Evidence

This directory will contain the deterministic baseline definition, scripted
input fixtures that contain no retail data, comparison-tool documentation, and
per-milestone results.

No formal same-commit full acceptance pair has passed yet. Both cross-width
emitter defects are corrected: clean ARM64 report `ctr-215303` matches the
earlier immutable i686 trajectory on all eight state/transport components for
24,232 frames, and the clean current i686 regeneration is still running.
Fresh current-build version-4 report `ctr-223221` structurally advances a lap.

Current evidence and procedures:

- `STATE-DIGEST.md` defines canonical schema 2 and its mutation/address tests.
- `NTSC-U-GOLDEN-RUN.md` defines the full version-4 acceptance procedure.
- `2026-07-29-golden-run-result.md` records the historical version-2 run and
  the input/timing omissions it exposed.
- `2026-07-30-arm64-prefix-parity.md` records the corrected cross-width prefix,
  sanitizer, and audit evidence.
- `2026-07-30-arm64-full-regeneration.md` records the complete ARM64 capture
  and the rejected pre-correction i686 candidate.
- `2026-07-30-full-cross-width-result.md` records the finalized corrected
  i686 rejection, both first-divergence diagnoses and corrections, rejected
  tracing/extension routes, and accepted current-format lap coverage.
- `2026-07-30-macos-arm64-cadence.md` records the complete transport audit and
  direct checkpoint-local wall measurement against the exact NTSC VBlank
  model.
- `2026-07-30-macos-arm64-save-relaunch.md` records the game-driven save,
  retail CRC validation, and exact-producer second-process reload.
- `2026-07-30-macos-arm64-audio-output.md` records the exact-producer
  CoreAudio open, non-silent PCM capture, zero short-run transport faults,
  and direct initial XA-sector decode.

The first task is to evaluate `platform/native_replay_scheduler.c`,
`platform/native_checkpoint.c`, `platform/native_savestate.c`, and
`docs/REPLAYS.md` against a running 32-bit build. A usable gate must:

1. replay identical per-frame inputs;
2. compare game-visible state without embedding host pointer values;
3. cover startup, menus, racing, powerslides, items, transitions, and saves;
4. detect an intentional state mutation;
5. report the first divergent frame and field or region.

Screenshots and playtesting supplement this gate; they do not replace it.
