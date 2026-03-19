# AGENTS.md

## Scope

This directory contains Lynx Core’s event system: the event object model (`Event` and its subclasses), event targets (`EventTarget`), listeners and listener maps (`EventListener*`), and the event dispatcher (`EventDispatcher`).

This layer defines platform-agnostic “core event dispatch semantics”. Platform layers (iOS/Android/Harmony, etc.) typically collect input, build the corresponding core events at a higher layer, and then rely on the dispatch logic here to deliver events through capture/target/bubble phases.

## Module Map

- `event.*`: Base event type. Holds event metadata, propagation state, `detail`, and `event_path`.
- `event_dispatcher.*`: Event dispatcher. Fires listeners in Capture → Target → Bubble order (and includes Global).
- `event_target.*`: Event target abstraction. Provides parent/child relationship, listener container, and hooks for building `event_path`.
- `event_listener.*`: Listener abstraction and options (capture/catch/global/once/passive, etc.).
- `event_listener_map.*`: Container that stores listener vectors by event name (add/remove/find).
- `event_dispatch_result.h`: Dispatch result and cancel reasons (`DispatchEventResult` / `EventCancelType`).
- `touch_event.*` / `keyboard_event.*` / `custom_event.*`: Core event subclasses that populate custom detail or handle conflicts/params.
- `*_test.*`: Unit tests for the event system in this directory.

## Key Files And Types

- [Event](file:./event.h)：
  - Propagation phases: `PhaseType::{kCapturingPhase,kAtTarget,kBubblingPhase,kGlobal}`.
  - Propagation control: `is_stop_propagation_` / `is_stop_immediate_propagation_`.
  - Event path: `InitEventPath()` builds `event_path_` upwards via `EventTarget::GetParentTarget()`.
  - Detail payload: `detail_` (`lepus::Value`) is populated by `HandleEventBaseDetail()` and subclass `HandleEventCustomDetail()`.
  - Compatibility/conflicts: `HandleEventConflictAndParam()` may cancel/short-circuit before dispatch (implemented by subclasses).
  
- [EventDispatcher](file:./event_dispatcher.cc)：
  - `DispatchEvent(target, event)` is the entry point; it calls `InitEventPath()` and then `Dispatch()`.
  - `Dispatch()` ordering and cancel rules: conflict/param handling → set target/currentTarget → global → capture → at-target → bubble.
  
- [EventTarget](file:./event_target.h)：
  - Event tree: `GetParentTarget()` defines the parent chain used by `event_path`.
  - Path pruning: `IsEventPathSkip()` and `IsEventPathCatch()` can skip/truncate `event_path` construction.
  - Target metadata: `GetEventTargetInfo()` / `GetEventControlInfo()` provide `target/currentTarget` info for `detail` and additional dispatch-control info.

- [EventListener](file:./event_listener.h)：
  - `Options` encodes listener semantics via bit flags (capture/catch/global, etc.).
  - `IsMatchEvent()` decides whether to fire based on the current event phase and listener options (especially capture/catch/global).

## Dispatch Semantics And Notes

- **Phase semantics**
  - Capture: reverse-iterate from root toward target (`path.rbegin → path.rend`), but does not re-dispatch the target itself in capture.
  - Target: dispatched on the target node; listeners are stably sorted so capture listeners run before bubble listeners (see [event_target.cc](./event_target.cc)).
  - Bubble: iterate from target upward (`path` forward), and likewise does not re-dispatch the target itself.
  - Global: triggered via `target->HandleGlobalEvent()`; global listener matching is controlled by `EventListener::Options::kGlobalBit`.

- **Cancelation and short-circuit**
  - If `stopImmediatePropagation` is set during listener invocation, subsequent listeners on the same target are skipped.
  - If `stopPropagation` is set or the listener options indicate catch, the `DispatchEventResult` is marked as `kCanceledByEventHandler` and `EventDispatcher` stops further propagation.
  - `HandleEventConflictAndParam()` may cancel before dispatch begins (used for event conflicts/parameter adjustments).

- **Weak pointers and lifecycle**
  - `event_path` uses `fml::WeakPtr<EventTarget>`; if a node becomes invalid during dispatch it will be skipped and logged as an error.
  - `EventDispatcher` also holds a weak pointer to the target; if the target is already released it returns `kCanceledBeforeDispatch`.

## Typical Change Patterns

- **Add/extend event types**
  - Extend `Event::EventType` and add a corresponding `Event` subclass (or extend an existing subclass).
  - Implement `HandleEventCustomDetail()` (populate `detail`) and/or `HandleEventConflictAndParam()` (handle conflicts and parameter compatibility) in the subclass.

- **Adjust propagation behavior**
  - Ordering, short-circuit conditions, and global/capture/bubble semantics: start from [event_dispatcher.cc](./event_dispatcher.cc).
  - Pruning/truncating `event_path`: start from `Event::InitEventPath()` and `EventTarget::{IsEventPathSkip,IsEventPathCatch}`.

- **Adjust listener matching/priority**
  - Phase/option matching: start from [event_listener.cc](./event_listener.cc).
  - Stable sorting of listeners at target phase: start from [event_target.cc](./event_target.cc).

## Invariants And Pitfalls

- `EventTarget::DispatchEvent()` copies the listener vector to avoid instability when listeners are added/removed during dispatch; do not break this isolation behavior.
- `TouchEvent::long_press_consumed_` is a static cross-dispatch state (written based on whether `longpress` was consumed). Be careful when changing trigger/cancel logic so this state is not updated incorrectly.

## Validate

- Unit test targets for this directory are in [BUILD.gn](./BUILD.gn):
  - `event_unittests_exec`
  - `event_tests` (group)

- If you changed propagation semantics (short-circuit behavior, phase matching, `event_path` construction), run `event_unittests_exec` first and verify:
  - listener firing order (capture/target/bubble/global)
  - short-circuit behavior for stopPropagation / stopImmediatePropagation / catch
  - robustness when weak pointers in the event path become invalid
