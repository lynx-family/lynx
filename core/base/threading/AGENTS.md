# AGENTS.md

## Scope

This directory contains core threading and scheduling primitives such as task-runner creation, thread merging, VSync monitor abstraction, and JS thread configuration.

## Module Map

- `task_runner_manufactor.*`, `task_runner_vsync.*`: task-runner creation and VSync-aligned runner behavior.
- `thread_merger.*`: thread merge/split coordination.
- `vsync_monitor.*`, `vsync_monitor_default.cc`: shared VSync abstraction and unittest fallback behavior.
- `js_thread_config_getter.*`: JS-thread grouping and configuration.

## Key Files And Types

- `TaskRunnerManufactor` owns thread topology decisions across UI, TASM, layout, and JS runners.
- `VSyncMonitor` is the shared frame-callback contract consumed by animation and shell-facing code.
- `thread_merger.*` affects when cross-thread ownership can collapse or separate.

## Typical Change Patterns

- If the issue is runner topology or thread ownership, start from `task_runner_manufactor.*`.
- If the issue is frame scheduling, duplicate callbacks, or missing frame work, inspect `vsync_monitor.*` and `task_runner_vsync.*` together.
- If the issue is JS-thread grouping or runner reuse, inspect `js_thread_config_getter.*`.

## Edit Rules

- Treat these files as shared scheduling infrastructure used by animation, shell, and runtime.
- Be careful with ownership, cross-thread callbacks, and fallback/default VSync behavior.

## Invariants And Pitfalls

- `vsync_monitor_default.cc` is a unittest fallback and should not quietly become the de facto production behavior.
- Thread-topology edits often break higher layers indirectly, so they need broader validation than a local helper change.

## Common Regression Symptoms

- Tasks execute on the wrong thread or stop executing after scheduler changes.
- Frame callbacks drift, duplicate, or disappear after VSync monitor changes.

## Validate

Use `lynx-cpp-test` and start with:

- `lynx_base_unittests_exec`

## Notes

- This directory is the most shared part of `base/`. Small edits here often deserve parent `base/AGENTS.md` review before landing.
