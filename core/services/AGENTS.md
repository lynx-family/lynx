# AGENTS.md

## Scope

This directory contains cross-cutting engine services such as event reporting, feature counting, fluency/performance tracing, replay/recorder support, timing handlers, watch-dog helpers, and SSR- or starlight-related service glue.

## Module Map

- `event_report/`: event tracking and platform report integration.
- `feature_count/`: feature usage counting.
- `fluency/`, `long_task_timing/`, `performance/`, `fsp_tracing/`, `timing_handler/`: runtime and renderer performance/timing instrumentation.
- `recorder/` and `replay/`: testbench, recorder, and replay flows.
- `watch_dog/`: watchdog-related interfaces.
- `trace/`: service trace-event definitions.
- `ssr/`, `starlight_standalone/`: smaller service adapters for specific runtime modes.

## Edit Rules

- Keep each service focused on observation, reporting, or replay responsibilities. Do not turn services into hidden ownership layers for renderer or runtime state.
- Platform-specific reporting code belongs in platform child directories when present.
- Recorder/replay changes should preserve determinism and testbench assumptions; seemingly small format or event-order changes often have wide effects.

## Common Regression Symptoms

- Missing or duplicated telemetry events often point to `event_report/`, `performance/`, or `timing_handler/`.
- Numbers exist but are obviously wrong when timing start/stop ownership drifts across services.
- Replay and recorder regressions usually show up as mismatched event order, incomplete snapshots, or unstable testbench playback.

## Validate

For C++ unit tests here, prefer the `lynx-cpp-test` skill and use the nearest service target.

Common starting points:

- `record_unittest_exec`
- `replay_unittest_exec`

Many service directories do not define standalone execs. When that happens, validate through the closest owning consumer tests in the renderer, runtime, or shell layers.
