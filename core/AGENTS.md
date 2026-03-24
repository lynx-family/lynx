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

## Key Files And Boundaries

- `BUILD.gn`: the top-level composition point for `lynx_native`. If a local-looking change starts breaking unrelated modules, check whether this file pulls the touched target into a wider surface.
- `public/` and `include/`: exported header surfaces. Changes here are rarely isolated to one implementation directory.
- `renderer/`, `runtime/`, and `shell/`: the three main execution planes of core. Many bugs land on a boundary between two of them rather than inside one subtree.
- `shared_data/`: the white-board handshake layer between renderer, runtime, and shell. Bugs that look like "state updated but nobody reacted" often pass through here.

## Typical Cross-Module Paths

- CSS, style, and animation behavior often crosses `renderer/` -> `style/` -> `animation/`.
- Page lifecycle and callback ownership often crosses `shell/` -> `runtime/` -> `renderer/`.
- Bundle loading and execution often crosses `resource/` -> `template_bundle/` -> `runtime/`.
- Shared-session or white-board behavior often crosses `shared_data/` -> `runtime/` and `shared_data/` -> `renderer/`.

## What To Read First

- Read the nearest child `AGENTS.md` before using this root guide to make implementation decisions.
- Read the nearest `BUILD.gn` before adding files or changing target boundaries. Many "small" core edits actually widen through a shared source set.
- Read `public/` or `include/` first when a change looks like a contract issue rather than a local implementation bug.
- For boundary bugs, inspect one producer and one consumer instead of reading only the directory you plan to edit. Renderer-visible symptoms are often rooted in runtime, shell, or shared-data contracts.

## Edit Rules

- Prefer the narrowest module that owns the behavior. Do not push renderer logic into `base/` or runtime semantics into `shell/`.
- Treat `public/` headers as shared API surface. Signature changes there usually require coordinated updates in core and platform code.
- Keep platform-specific implementations in their platform subdirectories; keep shared semantics in the root of each module.
- When a directory already has its own `AGENTS.md`, follow the child document instead of generalizing from this file.

## Common Review Questions

- Is this change modifying a shared contract, or should it stay inside one implementation subtree?
- Does the touched code own the behavior, or is it only adapting state that should really change one layer up or down?
- Does the fix cross a thread, actor, or callback boundary that needs validation in both the caller and callee modules?
- If a value is converted between representations, should the fix live in `value_wrapper/`, `style/`, or the consumer module instead of the current file?

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

Broaden validation when the change crosses a boundary:

- If `public/` or `include/` changes, expect to run the owning consumer tests rather than a single local target.
- If `BUILD.gn` wiring changes, validate both the local module target and at least one top-level consumer target that pulls it in.
- If a fix spans renderer/runtime/shell ownership, do not stop after the first green unit target.

## Coverage Reality

- Most `lynx/core` unit targets are strong at module-local semantics, but weaker at actor boundaries, platform scheduling, and renderer/runtime/shell handoff timing.
- A green local unit target is evidence that the owning module still behaves, not proof that boundary consumers still agree with it.
- The more a change crosses `public/`, `shared_data/`, task-runner ownership, or style/value conversion boundaries, the less you should trust one directory-local target by itself.

## Notes

- `core/BUILD.gn` assembles many modules together under `lynx_native`, so interface changes often surface outside the directory you touched.
- Many subdirectories under `lynx/core/` now provide their own `AGENTS.md`. Prefer the nearest child guide over this root summary whenever one exists.
