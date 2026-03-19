# AGENTS.md

## Scope

This directory adapts the generic `basic_animation/` layer to Lynx-facing animator behavior. It bridges shared animation primitives with `starlight::AnimationData`, VSync-backed frame callbacks, and user-visible animator event hooks.

## Module Map

- `basic_animator.*` is the main Lynx animator entry point and `AnimatorTarget` implementation.
- `basic_animator_event_listener.*` translates low-level animation events into externally registered callbacks.
- `basic_animator_frame_callback_provider.*` connects animation frame requests to `base::VSyncMonitor`.
- `basic_property_value.*` contains Lynx-specific property-value adapters built on top of `basic_animation/property_value.h`.

## Key Files And Types

- `basic_animator.*`: defines `LynxBasicAnimator`, the adapter that builds a shared basic animation from `starlight::AnimationData`, starts and stops it, and converts animated progress back into Lynx-visible callbacks.
- `basic_animator_event_listener.*`: receives shared animation lifecycle events and maps them to externally registered start/cancel/iteration/end callbacks.
- `basic_animator_frame_callback_provider.*`: requests frames from `base::VSyncMonitor`, including creating and binding a thread-local monitor when one is not injected.
- `basic_property_value.*`: adapter-side `PropertyValue` implementation for float progress interpolation used by `LynxBasicAnimator`.

## Layer Choice

- This directory is the Lynx-facing adapter layer on top of `basic_animation/`; it is not the place for generic timing, interpolation, or reusable keyframe machinery.
- If the issue is specific to `starlight::AnimationData`, callback wiring, adapter-visible progress values, or VSync-backed frame hookup, this directory is the right place.
- If the issue is about generic timing semantics, iteration trimming, shared frame-callback abstractions, or core interpolation contracts, inspect `../basic_animation/` instead.
- If you are unsure, inspect the parent `animation/AGENTS.md` first and then compare this directory with `../basic_animation/`.

## Typical Change Patterns

- If the change is about how a Lynx basic animator is initialized from `starlight::AnimationData`, inspect `basic_animator.cc`.
- If the change is about user callback timing or which animation event maps to which callback, inspect `basic_animator_event_listener.*` and `basic_animator.cc` together.
- If the change is about frame scheduling, thread affinity, or VSync hookup, inspect `basic_animator_frame_callback_provider.*`.
- If the change is about the value passed back through `UpdateAnimatedStyle`, inspect `basic_property_value.*` and the `"BASIC_TYPE_FLOAT"` path in `basic_animator.cc`.

## Edit Rules

- Keep generic animation semantics in `../basic_animation/`. This directory should stay focused on Lynx-facing adaptation and integration.
- Changes to `basic_animator.*` can affect start/stop lifecycle, callback timing, and value propagation. Validate event and frame-scheduling behavior together rather than in isolation.
- Keep `basic_animator_frame_callback_provider.*` aligned with VSync-driven execution. Avoid introducing alternative scheduling semantics here without a strong reason.
- If you add new adapter-side property value behavior, confirm it still matches the `PropertyValue` type contract defined by the shared animation layer.
- Treat callback and logging-only branches carefully. Several paths currently log when callbacks are absent; a refactor here can silently change observable event behavior.

## Invariants And Pitfalls

- `basic_animator.cc` hardcodes a two-keyframe `"BASIC_TYPE_FLOAT"` animation from `0.0` to `1.0`, then converts progress back through `UpdateAnimatedStyle`. If you change that contract, update both keyframe construction and value consumption together.
- `InitializeAnimator()` wires together effect timing, keyframes, frame provider, and event listener in one place. Partial refactors here often break either frame callbacks or event delivery.
- `basic_animator_frame_callback_provider.cc` creates and binds a thread-local `VSyncMonitor` when one is not injected. Changes here can alter thread behavior in tests and in production.
- `basic_property_value.cc` assumes float-to-float interpolation. If the adapter starts supporting more value kinds, expand the type contract deliberately rather than overloading the existing float path.

## Common Regression Symptoms

- Progress callbacks not firing, firing only once, or returning the wrong value usually point to `basic_animator.cc` and `basic_property_value.*`.
- Start/cancel/iteration/end callbacks missing or arriving in the wrong order usually point to `basic_animator_event_listener.*` or how it is wired in `InitializeAnimator()`.
- Animations that appear to initialize correctly but never advance frame-by-frame usually point to `basic_animator_frame_callback_provider.*` and its `VSyncMonitor` path.

## Validate

Use the `lynx-cpp-test` skill for environment setup, GN generation, target build, single-test execution, and coverage workflows.

Resolve the exact exec target from this directory's `BUILD.gn`.

- Start with `lynx_basic_animator_unittests_exec`.
- If your change crosses the shared adapter boundary, also run `basic_animation_unittests_exec`.

Expand validation when appropriate:

- If you touched event callback mapping, confirm start, cancel, iteration, and end behavior in the adapter test coverage.
- If you touched VSync or frame provider behavior, review whether the change should also be exercised through the shared animation scheduling path.

## Notes

- This adapter depends on shell/base integration points, so seemingly local lifecycle or callback changes may surface as integration issues rather than pure animation failures.
- `GetStyle` and `UpdateAnimatedStyle` form the adapter boundary between shared animation values and Lynx-visible state updates; review both sides when changing property handling.
