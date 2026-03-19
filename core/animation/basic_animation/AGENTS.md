# AGENTS.md

## Scope

This directory contains the platform-agnostic foundation of Lynx animations: timelines, effects, keyframes, keyframe models, frame callback abstractions, and basic interpolatable property values.

## Module Map

- `basic_animation.*`, `animation_effect.*`, `animation_timeline.*`, and `animation_effect_timing.*` define the core animation lifecycle and timing model.
- `basic_keyframe_effect.*`, `basic_keyframe_model.*`, and `keyframe.*` implement generic keyframe-based animation behavior.
- `basic_animatable_values/` contains concrete `PropertyValue` implementations used for interpolation.
- `thread_local_animation_handler.*` manages frame callback dispatch for animations on the current thread.

## Key Files And Types

- `basic_animation.*`: the shared `basic::Animation` runtime. It coordinates state transitions, requests frames, ticks effects, and emits animation events.
- `animation_effect.*`: base effect abstraction that owns timing data and the map of per-property keyframe models.
- `animation_effect_timing.*`: central timing object for duration, delay, fill mode, playback direction, easing, and iteration count.
- `basic_keyframe_effect.*`: turns generic keyframe tokens into per-property curves and pushes interpolated values back to an `AnimatorTarget`.
- `basic_keyframe_model.*`: core timing/state machine for a single property animation. It handles phase calculation, pause accounting, active time, iteration trimming, and reverse playback.
- `property_value.h`: base interpolation contract for value types used by curves and keyframes.
- `basic_animatable_values/*`: concrete implementations for float, int, and color interpolation.
- `thread_local_animation_handler.*`: thread-local scheduler used when an animation relies on the shared fallback frame-callback path instead of an injected provider.

## Layer Choice

- The types in this directory live in `lynx::animation::basic` and are not the same as the root `lynx::animation` classes with similar names.
- If the change is about CSS keyframes, CSS transitions, or element-facing animation events, you are probably in the wrong layer and should inspect the parent `animation/` directory first.
- If the change is about generic timing, interpolation, run state, or reusable keyframe machinery that should not depend on element/CSS infrastructure, this directory is the right place.
- If you are unsure which layer owns the behavior, inspect the parent `animation/AGENTS.md` first and choose the layer before editing.

## Typical Change Patterns

- If the change is about state transitions, event emission, or first-frame scheduling, inspect `basic_animation.*`.
- If the change is about keyframe normalization, per-property keyframe model creation, or target style updates, inspect `basic_keyframe_effect.*`.
- If the change is about timing math, fill behavior, playback direction, pause accounting, or iteration boundaries, inspect `basic_keyframe_model.*`.
- If the change is about value interpolation or supported primitive value kinds, inspect `property_value.h` and `basic_animatable_values/`.

## Edit Rules

- Keep this layer reusable and platform-agnostic. Prefer not to introduce Lynx shell, renderer, or product-specific behavior here unless the abstraction genuinely belongs in the shared animation core.
- Add new interpolatable value types under `basic_animatable_values/` only when they fit the `PropertyValue` contract cleanly: clone, interpolate, and stable type identification.
- Treat `thread_local_animation_handler.*` changes as lifecycle-sensitive. Small edits here can affect callback ordering, repeated frame requests, and cleanup behavior.
- If you add new source files, register them in `BUILD.gn` and keep tests close to the implementation they cover.
- Keep target interaction abstract. `AnimatorTarget` and frame callback provider interfaces are the intended bridge points; avoid sneaking higher-level environment assumptions into core types.

## Invariants And Pitfalls

- `basic_animation.cc` intentionally uses a dummy start time on first play, then switches to the real frame time on first tick. Changes here can alter first-frame behavior and event ordering.
- `basic_keyframe_effect.cc` normalizes missing or out-of-order offsets before constructing models. Offset cleanup and interpolation are part of correctness, not just preprocessing.
- `basic_keyframe_model.cc` owns tricky timing rules: pause duration accounting, fill-mode handling, reverse and alternate playback, and iteration trimming. Changes here tend to have wide behavioral impact.
- `thread_local_animation_handler.cc` clears the pending-request flag before invoking callbacks so callbacks can request another frame safely. Preserve that ordering unless you are deliberately redesigning the scheduler.

## Common Regression Symptoms

- Wrong iteration counts, incorrect reverse playback, or fill-mode surprises usually point to timing logic in `basic_keyframe_model.*`.
- Pause/resume bugs or animations that stop requesting frames after resuming often point to `basic_animation.*` or `thread_local_animation_handler.*`.
- Value interpolation glitches that affect only one primitive type usually point to `basic_animatable_values/*` rather than the higher-level timing stack.

## Validate

Use the `lynx-cpp-test` skill for environment setup, GN generation, target build, single-test execution, and coverage workflows.

Resolve the exact exec target from this directory's `BUILD.gn`.

- Start with `basic_animation_unittests_exec`.
- If you change public behavior consumed by the parent animation layer, also run `animation_unittests_exec`.

## Notes

- The current dedicated unit coverage in this directory focuses on `basic_animatable_values/`, so broader timing or scheduler changes may need focused tests added near the affected classes.
- This directory depends only on animation utilities at build time; keep that narrow dependency surface when possible.
