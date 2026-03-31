# AGENTS.md

## Scope

This directory contains Lepus-facing binding glue between the Lepus runtime and renderer/module/resource/event surfaces.

## Module Map

- Root files define renderer-function glue.
- `event/`: event context proxy and listener bindings for Lepus.
- `modules/`: Lepus module callback and module manager bindings.
- `resource/`: resource-response handling in Lepus.
- `style/`: CSS/style bridging helpers for runtime consumption.

## Edit Rules

- Keep runtime semantics in the parent Lepus layer and bridge-specific conversions here.
- Event/module/resource bindings may look structurally similar but have different lifetime and ownership assumptions.

## Common Regression Symptoms

- Lepus runtime still executes scripts, but callbacks or resources stop arriving at the right bridge layer.
- CSS/style binding regressions show up only when runtime consumes style fragments.

## Validate

Use `lynx-cpp-test` and start with:

- `lepus_runtime_unittests_exec`
- `response_handler_lepus_unittests_exec` when resource bridging changes

If shared runtime semantics also changed, run `lepus_unittests_exec`.
