# CTRPad Decision Log

This log records decisions that constrain later implementation. Detailed
milestone state and acceptance criteria live in `docs/ROADMAP.md`.

## 2026-07-29 — Preserve both project histories

**Decision:** implement on `codex/arm64-apple` in this downstream repository.
Keep `ref/ctr-native` and `ref/ctr-native-android` read-only. Join the existing
CTRPad documentation ancestry with upstream ctr-native's ancestry using a
two-parent merge.

**Why:** the checkout began as a documentation-only repository, while the goal
requires a maintained downstream port and says reference clones must remain
read-only. Commit `268ff6977` preserves provenance for both histories without
copying an unversioned source snapshot.

**Verification:** the merge's second parent is upstream commit `2df55dc5a`.
There is no post-merge source difference from that commit in `CMakeLists.txt`,
`CMakePresets.json`, `main.c`, `game/`, `include/`, `platform/`, or
`externals/`.

## 2026-07-29 — Freeze beta-7.1 as the first evidence baseline

**Decision:** reproduce and measure ctr-native at `2df55dc5a` before changing
its build or memory model.

**Why:** `upstream/master` still matches the viability report exactly. A fixed
baseline makes compiler failures, runtime behavior, and future parity artifacts
attributable.

**Revisit when:** upstream moves or a separable upstream portability fix is
needed. Any update will be an explicit merge or rebase with the baseline
evidence retained.

## 2026-07-29 — Treat retail media as untrusted input

**Decision:** do not rename, link, or depend on the local CloneCD image until
the disc reader validates it. Ignore all common disc and extracted-retail
formats repository-wide.

**Why:** `ref/CTR/CTR.ccd:78-79` and exact sector-size arithmetic establish a
raw MODE2/2352 container, but not NTSC-U identity, revision, completeness, or
loader compatibility.

**Verification:** `.gitignore:8-35` covers the local media directory and known
retail extensions; upstream also ignores `assets/` at `.gitignore:40`.

## 2026-07-29 — Mac ARM64 parity gates iOS

**Decision:** do not debug unresolved pointer-width corruption through an iOS
app shell. A correct, parity-checked Apple Silicon macOS build is required
before the iOS milestone.

**Why:** macOS exercises Apple Clang, ARM64, LP64, and the same core code with
better diagnostics and without signing, lifecycle, and sandbox variables.

**Revisit when:** never as a sequencing shortcut. Renderer research may overlap
the memory work, but iOS acceptance remains gated.
