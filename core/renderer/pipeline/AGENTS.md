# AGENTS.md

## Scope

This directory contains the renderer pipeline coordination layer of Lynx: versioned pipeline context management, lifecycle state transitions, observer delivery, and scope-based wiring between `TemplateAssembler` and pixel-pipeline execution.

## Key Files And Types

- `BUILD.gn`: defines the `pipeline` source set and the `pipeline_test_exec` unit test target.
- `pipeline_context.*`: owns one pipeline run's options, version, lifecycle state, and observer callback data snapshots.
- `pipeline_context_manager.*`: owned by `TemplateAssembler`; creates, stores, looks up, and removes contexts, and is the single owner of TemplateAssembler-level observer registration and notification dispatch.
- `pipeline_lifecycle.*`: defines legal lifecycle transitions and guards state ordering.
- `pipeline_scope.*`: RAII helper that creates or updates the current context on entry and triggers `RunPixelPipeline()` on exit.
- `pipeline_lifecycle_observer.h`: observer contract for lifecycle callbacks routed through `TemplateAssembler`.
- `pipeline_version.h`: version identity for context lookup and ordering.
- `README.md`: design overview, lifecycle diagram, and observer usage guidance for this directory.

## Typical Change Patterns

- For lifecycle ordering, stage gating, or `is_state_executed` semantics, start in `pipeline_lifecycle.*` and `pipeline_context.*`.
- For context ownership, cross-context observer persistence, or current-context tracking, start in `pipeline_context_manager.*`.
- For scope entry/exit semantics, start in `pipeline_scope.*` and verify against `TemplateAssembler::RunPixelPipeline()` and `TemplateAssembler::OnLayoutAfter()`.
- If the change is about actual resolve, layout, or flush implementation, this directory is usually not the owning layer; follow the call path into `tasm/`, `dom/`, `starlight/`, or `ui_wrapper/`.

## Edit Rules

- Keep `pipeline/` focused on sequencing, state, and observer coordination. Do not move DOM mutation, style-resolution logic, or layout implementation into this directory.
- Preserve lifecycle validity. If you add or loosen transitions, update the lifecycle rules, related tests, and `README.md` together.
- Treat `PipelineOptions` and `PipelineVersion` ownership carefully. `PipelineOptions::version` is a borrowed pointer to versioned context state and must stay aligned with context creation and removal.
- Observer lifetime is aligned with the `TemplateAssembler` instance. Do not reintroduce public per-context observer registration from external callers.
- When observer callbacks can re-enter, preserve snapshot semantics for the observer list for the current notification. Callback data is already built as a per-transition value snapshot before dispatch.
- Be explicit about the difference between resetting `current_pipeline_context_` and removing a versioned context from the manager; these happen at different points in async layout flows.

## Common Regression Symptoms

- Missing or duplicate lifecycle callbacks usually point to manager-level notification, snapshot iteration, or deduplication drift.
- Incorrect `prev_state`, `cur_state`, or `is_state_executed` values usually point to lifecycle gating or callback snapshot regressions.
- Context lookup failures or stale `PipelineOptions::version` pointers after layout usually point to removal/reset sequencing bugs.
- Pipelines that stop too early, skip flush, or never reach `kStopped` usually point to `RunPixelPipeline()` and `OnLayoutAfter()` coordination drift.
- Async layout regressions where the current context is cleared too early or too late usually point to reset timing around `RequestLayout()` and `OnLayoutAfter()`.

## Validate

For C++ unit tests here, prefer the `lynx-cpp-test` skill and use the smallest relevant target from the nearest `BUILD.gn`.

Common starting point:

- `pipeline_test_exec`

Expand validation when pipeline changes cross directory boundaries:

- If the change touches `TemplateAssembler` integration or layout completion sequencing, consider the nearest broader renderer test target in addition to `pipeline_test_exec`.
