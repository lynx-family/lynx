# AGENTS.md

## Scope

This directory contains Lynx core animation code: shared animation primitives, CSS keyframe and transition managers, the platform-agnostic `basic_animation/` layer, the Lynx-facing `lynx_basic_animator/` adapter, and small testing and utility helpers.

## Module Map

- Root files in this directory implement shared animation concepts such as `Animation`, animation curves, keyframe models, and CSS animation/transition management.
- `basic_animation/` contains reusable animation abstractions such as timelines, effects, keyframes, and interpolatable property values.
- `lynx_basic_animator/` adapts the basic animation layer to Lynx-facing concepts such as `starlight::AnimationData`, event callbacks, and VSync-backed frame scheduling.
- `testing/` contains mocks used by animation tests. Do not treat these files as production implementations; changes here normally validate through `animation_unittests_exec`.
- `utils/` contains low-level timing helpers such as cubic bezier and timing function logic.

## Key Files And Types

- `animation.*`: root animation lifecycle object for CSS-driven animations. Owns play/pause/stop state, frame requests, event dispatch, and the first-frame dummy tick behavior.
- `css_keyframe_manager.*`: converts CSS keyframe declarations into runtime curves and keyframe models, then ticks all active keyframe animations for an element.
- `css_transition_manager.*`: expands transition properties, constructs per-property transition animations, and decides when transitions should stop or restart.
- `keyframe_effect.*`: element-facing effect container for root animation code. It holds the per-curve `KeyframeModel` instances used by CSS animation and transition flows.
- `keyframe_model.*`: shared root-level model for animation run state and curve progression in the CSS animation path.
- `transform_animation_curve.*` and `keyframed_animation_curve.*`: property-specific curve implementations used by CSS animations and transitions.

## Layer Choice

- This directory contains two similarly named animation stacks. Root types such as `Animation`, `KeyframeEffect`, and `KeyframeModel` belong to the CSS-driven element animation path in `lynx::animation`.
- Types under `basic_animation/` such as `basic::Animation`, `basic::KeyframeEffect`, and `basic::KeyframeModel` belong to the platform-agnostic foundation layer in `lynx::animation::basic`.
- When a change is about CSS property animation, element style updates, transition/keyframe token conversion, or animation events emitted through element infrastructure, prefer the root stack.
- When a change is about generic timing, keyframe progression, interpolation contracts, or reusable frame-callback abstractions, prefer the `basic_animation/` stack.

## Typical Change Patterns

- If you are changing CSS animation parsing or how CSS keyframes become runtime curves, start from `css_keyframe_manager.*`.
- If you are changing transition construction, transition restart/stop behavior, or property expansion such as `all`, `margin`, or `padding`, start from `css_transition_manager.*`.
- If you are changing generic animation lifecycle semantics such as play, pause, stop, dummy-start ticking, or frame scheduling, inspect the root `animation.*` files and compare behavior with `basic_animation/basic_animation.*`.

## Edit Rules

- Put general animation semantics in this directory or `basic_animation/`; keep Lynx-specific adapter logic in `lynx_basic_animator/`.
- Be careful when editing `css_keyframe_manager.*` or `css_transition_manager.*`; these changes affect CSS animation behavior broadly rather than a single caller.
- Keep `utils/` focused on small reusable math and timing helpers. Avoid pulling renderer, shell, or platform glue into it.
- Keep mocks and test doubles under `testing/` only. If production code starts depending on a helper here, that helper likely belongs elsewhere.
- In the root animation layer, watch for lifecycle-sensitive behavior around `Play`, `Pause`, `Stop`, `Destroy`, `RequestNextFrame`, and the dummy start time. These code paths affect event emission and first-frame style application.

## Invariants And Pitfalls

- Root `animation.cc` uses a dummy start time on first play to force an immediate tick. This is intentionally tied to first-frame style correctness and iOS flicker avoidance; do not remove or change it casually.
- `css_keyframe_manager.*` constructs curves and models based on property and curve type. Adding a new animatable property usually requires wiring the right keyframe curve and validating the property can be converted into keyframe values.
- `css_transition_manager.*` expands aggregate transition properties such as `all`, `border-width`, `border-color`, `margin`, and `padding`. A seemingly local change here can fan out into many per-property animations.
- Transition stop conditions depend on start/end value validity and previous-end-value tracking. Regressions here often show up as animations failing to restart or failing to cancel.

## Common Regression Symptoms

- First-frame style flicker or missing initial animated state often points to lifecycle changes around the dummy start time or first tick path in `animation.*`.
- Transitions that stop restarting, refuse to cancel, or only animate part of an aggregate property often point to `css_transition_manager.*`.
- CSS keyframe animations that animate the wrong property set or use the wrong curve type often point to model construction in `css_keyframe_manager.*`.

## Validate

For C++ unit tests in this directory tree, prefer the `lynx-cpp-test` skill. Use that workflow to prepare the environment, generate GN files, build the right target, and run only the relevant unit tests first.

Resolve the exact exec target from this directory's `BUILD.gn`, then use the smallest matching target first.

Common starting points:

- `animation_unittests_exec`
- `basic_animation_unittests_exec`
- `lynx_basic_animator_unittests_exec`
- `animation_utils_unittests_exec`

Expand validation when appropriate:

- If you changed `css_keyframe_manager.*` or `css_transition_manager.*`, use `lynx-cpp-test` to run `animation_unittests_exec` even if the touched helper seems small.
- If you changed shared keyframe/timing/value abstractions, use `lynx-cpp-test` to run both `basic_animation_unittests_exec` and `animation_unittests_exec`.
- If you changed adapter-side callback or VSync behavior, use `lynx-cpp-test` to run `lynx_basic_animator_unittests_exec` and consider whether the shared layer tests should also run.
- If you changed `utils/*`, easing selection, cubic bezier math, or timing-function behavior, use `lynx-cpp-test` to run `animation_utils_unittests_exec`.

## Notes

- Animation tests are not fully isolated from the rest of core. `BUILD.gn` wires animation tests to renderer, runtime bindings, and shell test helpers, so interface changes can have effects outside this directory.
- Prefer adding or extending nearby `*_unittest.cc` files instead of creating distant test coverage.
