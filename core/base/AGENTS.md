# AGENTS.md

## Scope

This directory contains core-level utilities shared across the engine: threading helpers, VSync abstractions, JSON helpers, observer containers, memory pressure hooks, and platform-specific base adapters.

## Module Map

- `thread/` and `threading/`: task runners, thread-merging, VSync-backed scheduling, and JS thread configuration.
- `trace/`: trace-event definitions and shared trace categories consumed across core and platform code.
- `debug/`: memory-tracing and debug-only helpers for diagnostics.
- `memory/`: memory pressure callbacks and related notifications.
- `json/`: lightweight JSON utility helpers used across core.
- `observer/`: generic observer list primitives.
- `android/`, `darwin/`, `harmony/`: platform-specific base shims such as JNI helpers, message-loop/VSync implementations, and platform environment helpers.

## Key Files And Types

- `threading/task_runner_manufactor.*`: creates the task-runner topology used by the engine. Changes here can affect multiple threads at once.
- `threading/vsync_monitor.*`: abstract frame-tick source used by higher layers such as animation and shell.
- `threading/thread_merger.*`: governs when thread responsibilities may merge or diverge.
- `trace/trace_event_def.h` and `lynx_trace_categories.h`: shared trace categories and event definitions that affect instrumentation across modules.
- `thread/thread_utils.*`: low-level thread utility helpers used broadly by core.
- `memory/memory_pressure_callback.*`: callback fan-out for memory pressure handling.
- `json/json_utils.*`: shared JSON helpers used by multiple modules and tests.

## Typical Change Patterns

- If a change is about frame scheduling or display cadence, start from `threading/vsync_monitor.*` or the platform-specific VSync implementation.
- If a change is about task-runner topology, thread ownership, or JS thread setup, start from `task_runner_manufactor.*` and `js_thread_config_getter.*`.
- If a change is about trace categories, instrumentation names, or debug-only memory tracing, keep it in `trace/` or `debug/` instead of mixing it into generic threading helpers.
- If a change is about platform bridge code only, keep it inside `android/`, `darwin/`, or `harmony/` instead of changing shared helpers.

## Edit Rules

- Keep shared threading semantics in `thread/` or `threading/`; do not hide platform-specific behavior in generic files.
- `json/` and `observer/` are generic utilities. Avoid pulling renderer, shell, or runtime dependencies into them.
- JNI, Harmony NAPI, and Darwin environment helpers are platform glue. Do not make shared code depend on a single platform implementation.

## Common Regression Symptoms

- Frame ticks stop firing, fire twice, or drift across threads after `vsync_monitor` or task-runner changes.
- Deadlocks, tasks running on the wrong thread, or lifetime issues often point to `task_runner_manufactor.*` or `thread_merger.*`.
- Cross-platform breakage that only reproduces on Android, iOS, or Harmony usually points to a platform shim leaking into shared logic.

## Validate

For C++ unit tests here, prefer the `lynx-cpp-test` skill and use the target declared in this directory's `BUILD.gn`.

Start with:

- `lynx_base_unittests_exec`

If you changed JNI-specific helpers, confirm the affected Android-focused tests in the same target still cover the behavior.
