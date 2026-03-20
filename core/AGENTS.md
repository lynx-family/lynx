# AGENTS.md

## Scope

This directory contains the native engine core of Lynx: animation, event dispatch, renderer, runtime, shell orchestration, shared data, services, style data types, resource loading, template bundle handling, and supporting base utilities.

Child `AGENTS.md` files take precedence when they exist. Use this file to choose the right layer before editing.

## Module Map

- `animation/`: CSS animation primitives, transition/keyframe management, and the shared basic-animation foundation layer.
- `base/`: core-level threading, VSync, JSON, observer, and platform utility helpers used across the engine.
- `event/`: platform-agnostic event model, listener matching, and dispatch semantics.
- `inspector/`: inspector-facing observer and runtime inspection interfaces.
- `list/`: decoupled list container abstractions and list animation helpers.
- `parser/`: small parser utilities such as `InputStream`.
- `public/`: stable proxy and interface headers consumed across core and platform layers.
- `renderer/`: TASM, CSS, DOM, layout/pipeline, starlight, worklets, and renderer utilities.
- `resource/`: resource loading and lazy-bundle support.
- `runtime/`: JS runtime glue, Lepus/LepusNG execution stacks, bindings, and profiling.
- `services/`: performance, event report, replay, recorder, timing, and other cross-cutting services.
- `shared_data/`: white-board style shared state between runtime, renderer, and shell.
- `shell/`: multi-threaded shell orchestration, actor/proxy boundaries, and platform bridges.
- `style/`: typed style data objects and transform math.
- `template_bundle/`: template bundle wrappers and binary codec layers.
- `value_wrapper/`: conversions between public values, Lepus values, NAPI values, and platform representations.
- `build/`, `include/`: supporting build or umbrella-header directories. These usually do not need directory-local guidance.

## Layer Choice

- If a change is about CSS tokens, DOM nodes, layout invalidation, or page assembly, start in `renderer/`.
- If a change is about JS execution, native module binding, Lepus bytecode, or runtime lifecycle, start in `runtime/`.
- If a change is about thread ownership, cross-thread `Act()` calls, native-platform bridges, or queueing work between shell actors, start in `shell/`.
- If a change is about shared contracts rather than implementations, check `public/` before changing concrete classes.
- If a change is only a data holder or math helper for style values, start in `style/` rather than `renderer/` or `animation/`.

## Edit Rules

- Prefer the narrowest module that owns the behavior. Do not push renderer logic into `base/` or runtime semantics into `shell/`.
- Treat `public/` headers as shared API surface. Signature changes there usually require coordinated updates in core and platform code.
- Keep platform-specific implementations in their platform subdirectories; keep shared semantics in the root of each module.
- When a directory already has its own `AGENTS.md`, follow the child document instead of generalizing from this file.

## Validate

For C++ unit tests under `lynx/core`, prefer the `lynx-cpp-test` skill. Resolve the exact exec target from the nearest `BUILD.gn`, then run the smallest relevant target first.

Common starting points by area:

- `animation_unittests_exec`, `basic_animation_unittests_exec`, `lynx_basic_animator_unittests_exec`
- `event_unittests_exec`
- `lynx_base_unittests_exec`
- `dom_unittest_exec`, `css_test_exec`, `pipeline_test_exec`, `starlight_unittest_exec`
- `runtime_tests_exec`, `lepus_unittests_exec`, `js_runtime_unittests_exec`
- `shell_unittests_exec`
- `shared_data_test_exec`
- `lazy_bundle_test_exec`
- `value_wrapper_unittest_exec`

## Notes

- `core/BUILD.gn` assembles many modules together under `lynx_native`, so interface changes often surface outside the directory you touched.
- Many subdirectories under `lynx/core/` now provide their own `AGENTS.md`. Prefer the nearest child guide over this root summary whenever one exists.
