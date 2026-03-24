# AGENTS.md

## Scope

This directory contains recorder infrastructure for Lynx initialization, native module activity, template assembler recording, and base testbench recording.

## Module Map

- `recorder_controller.*`: top-level recorder orchestration.
- `lynxview_init_recorder.*`, `native_module_recorder.*`, `template_assembler_recorder.*`: focused recorder implementations for major recording domains.
- `testbench_base_recorder.*`: shared testbench recording support.
- `recorder_constants.h`: recorder-wide constants and shared labels.
- `ios/`: Darwin-specific recorder implementation glue.

## Key Files And Types

- `recorder_controller.*` coordinates the recorder lifecycle and should usually be reviewed together with the domain-specific recorder it is orchestrating.
- `template_assembler_recorder.*` and `testbench_base_recorder.*` are the strongest local anchors for deterministic recording behavior.
- `recorder_constants.h` can look harmless, but constant drift often changes fixture compatibility.

## Typical Change Patterns

- If the issue is recorder lifecycle or enable/disable behavior, start from `recorder_controller.*`.
- If the issue is about what gets recorded for one domain, inspect the matching recorder implementation first.
- If the issue is replay compatibility or fixture shape, inspect constants and testbench recorders together rather than patching one local recorder in isolation.

## Edit Rules

- Keep deterministic recording behavior here; do not entangle recorder state with unrelated service ownership.
- Recorder constants, controller flow, and base recorder behavior usually need to stay aligned.

## Invariants And Pitfalls

- Recording code often regresses by subtly changing ordering or fixture shape while still "capturing something."
- Platform-specific recorder glue should stay compatible with the shared recorder contract instead of reinterpreting event meaning.

## Common Regression Symptoms

- Recordings become incomplete, non-deterministic, or incompatible with replay after controller or format changes.
- Only part of the initialization or module activity is recorded after narrowing a "local" change.

## Validate

Use `lynx-cpp-test` and start with:

- `record_unittest_exec`

## Notes

- This subtree already has dedicated recorder unit coverage, so format or determinism fixes should usually be paired with nearby tests instead of relying only on service-level consumers.
