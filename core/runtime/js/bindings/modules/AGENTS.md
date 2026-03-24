# AGENTS.md

## Scope

This directory contains JS module binding infrastructure: module managers, JSI module bindings/callbacks, module timing, interception, and native-module factory integration.

## Edit Rules

- Keep module registration and invocation semantics here; shared native-module contracts live in the common runtime binding layer.
- Be careful with callback lifetime, interception order, and timing collection, because they often interact in one invocation path.

## Common Regression Symptoms

- Modules register successfully but callbacks never arrive or arrive with wrong timing/interception behavior.
- Native-module factories still build, but runtime invocation order or callback lifetime regresses.

## Validate

Use `lynx-cpp-test` and start with:

- `runtime_tests_exec`

If event or resource bridges changed alongside modules, expand to the corresponding JS binding targets.
