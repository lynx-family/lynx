# AGENTS.md

## Scope

This directory contains Lynx's runtime stack: shared runtime helpers, JS runtime glue, Lepus and LepusNG execution stacks, runtime bindings, resource/module bridges, and profiling support.

## Module Map

- `common/`: cross-runtime reporting, shared bindings, and NAPI glue.
- `js/`: JS runtime manager, JS bundle handling, JSI integration, JS bindings, and bytecode helpers.
- `lepus/`: Lepus runtime, parser/compiler pieces, bindings, and execution helpers.
- `lepusng/`: LepusNG compiler/bindings/NAPI integration.
- `profile/`: runtime profiling helpers for QuickJS, V8, and LepusNG.
- `trace/`: runtime trace definitions.

## Layer Choice

- Use `common/` for cross-runtime contracts or reporting helpers shared by more than one runtime backend.
- Use `js/` for JS runtime lifecycle, JS bundle loading, JSI objects, and JS-side bindings.
- Use `lepus/` for the classic Lepus interpreter/compiler path and its bindings.
- Use `lepusng/` only when the change is specific to LepusNG semantics or its compiler/binding path.
- Use `profile/` for profiling and measurement helpers rather than execution semantics.

## Key Files And Types

- `BUILD.gn`: the `runtime` target shows how JS, common bindings, profiles, and loader pieces are composed.
- `js/runtime_manager.*`: JS runtime lifecycle and coordination.
- `js/js_bundle.*`: JS bundle ownership and loading surface.
- `lepus/context.*`, `lepus/parser.*`, `lepus/*compiler*`: Lepus execution and compilation pipeline.
- `common/bindings/*`: shared module, event, and resource binding points.

## Edit Rules

- Keep backend-specific behavior in the owning backend directory; avoid leaking Lepus-only assumptions into shared runtime code.
- Binding layers often sit between runtime and renderer/shell. Be careful with ownership, callback lifetime, and thread assumptions.
- Profile helpers should observe runtime behavior, not silently change it.

## Common Regression Symptoms

- Script loads but module/event/resource bindings stop working usually points to `common/bindings` or `js/bindings`.
- Lepus template execution, parsing, or bytecode regressions usually point to `lepus/` compiler or context changes.
- Runtime lifecycle regressions such as stale contexts or broken teardown often point to `js/runtime_manager.*` or shared observer/binding code.

## Validate

For C++ unit tests here, prefer the `lynx-cpp-test` skill and use the nearest target from `BUILD.gn`.

Common starting points:

- `runtime_tests_exec`
- `js_runtime_unittests_exec`
- `lepus_unittests_exec`
- `lepus_runtime_unittests_exec`
- `runtime_common_unittests_exec`
- `resource_common_unittests_exec`
- `response_handler_js_unittests_exec`
- `response_handler_lepus_unittests_exec`
- `task_unittests_exec`
- `lepus_compiler_exec`
- `profile_unittests_exec`, `quickjs_profile_unittests_exec`, `v8_profile_unittests_exec`, `lepusng_profile_unittests_exec`

Expand validation when changes affect renderer or shell integration.
