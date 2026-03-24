# AGENTS.md

## Scope

This directory contains replay and testbench playback helpers such as replay controller, callback/module testbench suites, layout-tree replay, and embedder replay utilities.

## Module Map

- `replay_controller.*`: top-level replay orchestration.
- `lynx_replay_helper.*`: shared replay utilities.
- `lynx_callback_testbench.*`, `lynx_module_*testbench.*`: testbench helpers for callback and module replay.
- `layout_tree_testbench.*`: layout-tree-specific replay helpers.
- `testbench_test_replay.*`, `testbench_utils_embedder.*`: replay entry and embedder support helpers.

## Key Files And Types

- `replay_controller.*` is the central coordinator for replay flow and should usually be reviewed together with the specific replay helper that changed behavior.
- `lynx_replay_helper.*` is the main shared helper layer that many testbench helpers depend on.
- Testbench files can look leaf-local, but they define determinism expectations that other replay paths rely on.

## Typical Change Patterns

- If the issue is replay lifecycle, sequencing, or controller behavior, start from `replay_controller.*`.
- If the issue is specific to callback, module, or layout replay, inspect the corresponding testbench helper pair.
- If the issue is fixture interpretation or embedder-only replay, inspect `testbench_test_replay.*` or `testbench_utils_embedder.*` before broadening shared helpers.

## Edit Rules

- Preserve deterministic ordering and fixture compatibility. Replay code is sensitive to event sequencing and data shape changes.
- Keep helper utilities generic and controller logic centralized rather than scattering replay state.

## Invariants And Pitfalls

- Replay code can remain build-clean while silently diverging from recorded behavior.
- Shared replay helpers and specialized testbench helpers need to agree on fixture meaning and callback ordering.

## Common Regression Symptoms

- Replays start but diverge from recorded behavior after controller or helper changes.
- Module/callback/layout testbench helpers drift independently after shared replay helper edits.

## Validate

Use `lynx-cpp-test` and start with:

- `replay_unittest_exec`

## Notes

- This subtree already has multiple unit-tested replay helpers, so deterministic behavior changes should usually be accompanied by nearby replay test updates.
