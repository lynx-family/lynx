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

- `threading/task_runner_manufactor.h`: defines `ThreadStrategyForRendering` and owns creation of UI, TASM, layout, and JS task runners. Strategy changes here affect thread topology, not just scheduling details.
- `threading/task_runner_manufactor.*`: creates the task-runner topology used by the engine. Changes here can affect multiple threads at once.
- `threading/vsync_monitor.*`: abstract frame-tick source used by higher layers such as animation and shell.
- `threading/thread_merger.*`: governs when thread responsibilities may merge or diverge.
- `threading/vsync_monitor_default.cc`: unittest fallback implementation used when platform VSync is unavailable. It is useful for tests, but it is not the production behavior contract.
- `threading/js_thread_config_getter.*`: chooses JS-thread configuration and naming. Small changes here can alter how runtime threads are provisioned across platforms.
- `UIThread` in `task_runner_manufactor.h`: singleton UI runner bootstrap path. Incorrect initialization order here can surface far away in shell or renderer startup.
- `trace/trace_event_def.h` and `lynx_trace_categories.h`: shared trace categories and event definitions that affect instrumentation across modules.
- `debug/memory_tracer.*`: debug-only memory observation helpers that can change diagnostics and tracing behavior without changing runtime semantics directly.
- `thread/thread_utils.*`: low-level thread utility helpers used broadly by core.
- `memory/memory_pressure_callback.*`: callback fan-out for memory pressure handling.
- `json/json_utils.*`: shared JSON helpers used by multiple modules and tests.

## Platform Split

- Keep shared thread-policy decisions in `threading/`; keep Android, Darwin, and Harmony event-loop or VSync specifics in their platform directories.
- `vsync_monitor_android.*`, `vsync_monitor_darwin.*`, and `vsync_monitor_harmony.*` are platform realizations of the same scheduling role. Shared fixes should usually start in `threading/vsync_monitor.*`, not in one platform file.
- `js_thread_config_getter.*` has both shared and Harmony-specific implementations. When JS-thread naming or grouping differs by platform, verify whether the behavior belongs in the shared getter or the platform override.
## Typical Change Patterns

- If a change is about frame scheduling or display cadence, start from `threading/vsync_monitor.*` or the platform-specific VSync implementation.
- If a change is about task-runner topology, thread ownership, or JS thread setup, start from `task_runner_manufactor.*` and `js_thread_config_getter.*`.
- If a change is about trace categories, instrumentation names, or debug-only memory tracing, keep it in `trace/` or `debug/` instead of mixing it into generic threading helpers.
- If a change is about platform bridge code only, keep it inside `android/`, `darwin/`, or `harmony/` instead of changing shared helpers.

## Invariants And Pitfalls

- `TaskRunnerManufactor` is documented as a UI-thread-created object. Changes to startup or strategy switching should be reviewed with that ownership assumption in mind.
- `VSyncMonitor` supports both a primary callback and secondary callbacks for one requested frame. Regressions here often show up as duplicate or missing frame work rather than crashes.
- `vsync_monitor_default.cc` is a unittest-oriented fallback path, not the production platform implementation. Keep test-only behavior from leaking into shared runtime assumptions.
- `UIThread::Init()` and task-runner bootstrap are part of engine bring-up, so ordering changes here can break startup long before a unit test points back to `base/`.
- `ThreadStrategyForRendering` changes can silently reassign work between UI, TASM, layout, and JS runners. Review them as topology changes, not as small enums.
## Edit Rules

- Keep shared threading semantics in `thread/` or `threading/`; do not hide platform-specific behavior in generic files.
- `json/` and `observer/` are generic utilities. Avoid pulling renderer, shell, or runtime dependencies into them.
- JNI, Harmony NAPI, and Darwin environment helpers are platform glue. Do not make shared code depend on a single platform implementation.

## Common Regression Symptoms

- Frame ticks stop firing, fire twice, or drift across threads after `vsync_monitor` or task-runner changes.
- Deadlocks, tasks running on the wrong thread, or lifetime issues often point to `task_runner_manufactor.*` or `thread_merger.*`.
- Cross-platform breakage that only reproduces on Android, iOS, or Harmony usually points to a platform shim leaking into shared logic.
- Runtime startup or shell bring-up suddenly failing after a "base-only" change often means thread-runner bootstrap or JS-thread configuration changed underneath a higher layer.

## Validate

For C++ unit tests here, prefer the `lynx-cpp-test` skill and use the target declared in this directory's `BUILD.gn`.

Start with:

- `lynx_base_unittests_exec`

This target already covers task-runner, thread-merger, VSync, JSON, and memory-pressure unit coverage in this directory.

If you changed JNI-specific helpers, confirm the affected Android-focused tests in the same target still cover the behavior.
If you changed VSync or task-runner behavior that higher layers depend on, also consider:

- `animation_unittests_exec`
- `shell_unittests_exec`

## Coverage Reality

- `lynx_base_unittests_exec` is strongest on shared thread-topology rules, VSync callback semantics, and utility behavior; it is weaker on full platform loop behavior.
- Platform-specific VSync and environment shims can still require platform validation even when the shared VSync tests stay green.
- If a threading change only fails during renderer startup or shell bring-up, treat that as a boundary validation gap rather than proof that the base tests were wrong.
