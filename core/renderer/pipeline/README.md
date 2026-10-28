# Pipeline Design & Observer Guide

## Purpose
`lynx/core/renderer/pipeline` defines a versioned pixel pipeline context system.

Core goals:
- Manage each pipeline run with explicit `PipelineContext`.
- Control execution order with `PipelineLifecycle` state machine.
- Support cross-stage decisions via `PipelineOptions` flags.
- Provide stable observer extension points for business features.

## Main Components
- `PipelineVersion`
  - Tracks `{major, minor}` version.
  - New context is generated from previous version by major/minor bump.
- `PipelineLifecycle`
  - Defines allowed state transitions.
  - Prevents invalid stage ordering.
- `PipelineContext`
  - Holds one pipeline run's options, version, lifecycle, hash, and observer callback data source.
- `PipelineContextManager`
  - Owned by `TemplateAssembler`.
  - Creates, stores, removes contexts by version.
  - Owns instance-level observer registry and lifecycle notification dispatch.
- `PipelineScope`
  - RAII helper: create/update context on enter, trigger `RunPixelPipeline()` on exit.

## Lifecycle States
Canonical path:
1. `kInactive`
2. `kInStyleResolve`
3. `kAfterStyleResolve`
4. `kInPerformLayout`
5. `kAfterPerformLayout`
6. `kUIOpFlush`
7. `kStopped`

Notes:
- `kStopped` is terminal.
- `is_state_executed` is calculated per state from options flags (`resolve/layout/flush`).

### State Diagram
```text
                       +----------------+
                       | kUninitialized |
                       +----------------+
                               |
                               v
                         +-----------+
                         | kInactive | ----------------------------------+
                         +-----------+                                   |
                               |                                         |
                               v                                         |
                       +-----------------+                               |
           +---------> | kInStyleResolve | ---------------------+        |
           |           +-----------------+                      |        |
           |                   |                                |        |
           |                   v                                |        |
           |         +--------------------+                     |        |
           |         | kAfterStyleResolve | --------------+     |        |
           |         +--------------------+               |     |        |
           |                   |                          |     |        |
           |                   v                          |     |        |
           |         +-------------------+                v     v        v
           |         | kInPerformLayout | --------> +----------+
           |         +-------------------+            | kStopped |
           |                   |                      +----------+
           |                   v                          ^     ^        ^
           |        +---------------------+               |     |        |
           |        | kAfterPerformLayout | --------------+     |        |
           |        +---------------------+                     |        |
           |                   |                                |        |
           |                   v                                |        |
           |             +------------+                         |        |
           +------------ | kUIOpFlush | ------------------------+        |
                         +------------+                                  |
                                                                         |
                                                                         |
```

## Execution Flow
1. Caller enters pipeline scope or creates context through manager.
2. Manager assigns version and binds `PipelineOptions` to context.
3. `TemplateAssembler::RunPixelPipeline()` advances lifecycle and conditionally executes stages:
   - Resolve
   - Layout
   - UI flush
4. Context reaches `kStopped`.
5. Manager removes context by version.

## Observer Integration
Observer lifecycle aligns with `TemplateAssembler` instance, not single context.

Public entry points:
- `TemplateAssembler::AddPipelineObserver(PipelineLifecycleObserver*)`
- `TemplateAssembler::RemovePipelineObserver(PipelineLifecycleObserver*)`

Routing:
- `TemplateAssembler` forwards to `PipelineContextManager`.
- Manager keeps global observer list for this tasm instance.
- Manager dispatches lifecycle notifications directly from context snapshots.

### Minimal Usage Pattern
```cpp
class MyPipelineObserver : public PipelineLifecycleObserver {
 public:
  void OnLifecycleChanged(const Data& data) override {
    // handle state transitions
  }
};

auto observer = std::make_unique<MyPipelineObserver>();
tasm->AddPipelineObserver(observer.get());

// ... pipeline runs ...

// before observer owner destruction
tasm->RemovePipelineObserver(observer.get());
```

## Observer Callback Data
`PipelineLifecycleObserver::Data` important fields:
- `prev_state`, `cur_state`: lifecycle transition.
- `is_state_executed`: whether this stage actually executed.
- `pipeline_version`: context version.
- `pipeline_id`, `pipeline_origin`: request-level identity.
- `timestamp_us`: creation timestamp in microseconds.

## Observer Behavior Contract
- Register once, receive callbacks for all later pipelines in same `TemplateAssembler`.
- No callbacks for historical pipelines before registration.
- Repeated add of same observer is deduplicated.
- Repeated remove is safe (idempotent).
- Removing observers during callback is supported.
- Removing another observer during callback does not cancel that observer's current notification.
- Adding an observer during callback does not make it join the current notification.
- Registration does not replay last state.

## Safety Guarantees
- Re-entrant callback safety:
  - `PipelineContextManager::NotifyLifecycleChanged` iterates over a copied observer snapshot.
  - Callback data is already a per-transition value snapshot built before dispatch.
  - Removing observers inside callbacks does not invalidate current iteration.
- Expired weak observer cleanup:
  - Invalid weak pointers are erased during manager add/remove/notify paths.

## Common Mistakes
- Registering observer for every pipeline run (not needed).
- Forgetting to remove observer before owner destruction.
- Assuming observer registration replays historical states.

## Test Recommendations
When adding or changing observer behavior, cover:
1. Cross-context persistence.
2. Data deduplication after context already exists.
3. Remove stops future callbacks.
4. Self-remove / remove-other during callback.
5. Add/remove during callback preserve current-round snapshot semantics.
6. Expired observer cleanup.

Related tests:
- `pipeline_context_manager_unittest.cc`
- `pipeline_context_unittest.cc`
- `pipeline_scope_unittest.cc`
