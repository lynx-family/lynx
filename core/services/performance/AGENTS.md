# AGENTS.md

## Scope

This directory contains performance service orchestration such as performance controller, mediator, and event sending.

## Module Map

- `performance_controller.*`: central controller for performance service flow.
- `performance_mediator.*`: coordination layer between signal producers and reporting consumers.
- `performance_event_sender.h`: event emission interface.
- `js_blocking_monitor/`, `memory_monitor/`: focused monitoring helpers.
- `android/`, `darwin/`, `harmony/`: platform-specific controller implementations.
- `fsp_tracing/`: performance-owned FSP tracing helpers used by this service layer.

## Key Files And Types

- `performance_controller.*` and `performance_mediator.*` form the main orchestration pair and should usually be reviewed together.
- `performance_event_sender.h` is part of the reporting contract rather than a passive helper.
- `js_blocking_monitor/` and `memory_monitor/` are specialized producers that still need to align with the service's reporting lifecycle.

## Typical Change Patterns

- If the issue is lifecycle ownership, timing of emission, or controller orchestration, start from `performance_controller.*` and `performance_mediator.*`.
- If the issue is metric production from JS blocking or memory monitoring, inspect the focused monitor subtree first.
- If the issue is platform-only controller behavior, inspect the relevant platform subtree before changing shared orchestration.

## Edit Rules

- Keep measurement and reporting orchestration here; do not move renderer or runtime ownership logic into the performance service.
- Controller and mediator changes often affect reporting order and lifecycle together.

## Invariants And Pitfalls

- Performance code often breaks by shifting timing windows or mediator ordering while still emitting data.
- Platform controllers should stay aligned with shared event semantics rather than inventing platform-only metric meaning.

## Common Regression Symptoms

- Performance metrics are emitted late, duplicated, or with missing fields after mediator/controller changes.
- Numbers are present but mapped to the wrong lifecycle window.

## Validate

No standalone exec is defined directly here. Validate through the nearest renderer/runtime/shell consumers that emit performance data.

## Notes

- This subtree is service orchestration, not a dumping ground for measurement logic that really belongs in renderer, runtime, or shell.
