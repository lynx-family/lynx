# AGENTS.md

## Scope

This directory contains typed style data structures and transform math used by the renderer and animation layers: transition data, background/filter/shadow data, transform decomposition, timing-function data, and other style-value containers.

## Module Map

- `*_data.*`: typed style payloads such as animation, transition, background, filter, outline, and perspective data.
- `transform/`: matrix, quaternion, and decomposition helpers for transform math.
- `timing_function_data.*`: style-level timing-function representation.
- `style.gni`: source-list definition for the style target.

## Edit Rules

- Keep this directory focused on data representation and math helpers. CSS parsing belongs in the renderer CSS layer, and animation execution belongs in the animation layer.
- Be careful when changing serialization-friendly or copyable style types; many modules treat these structs as cheap value objects.
- Transform math changes are high-risk because small numeric differences can affect layout, animation, and rendering together.

## Common Regression Symptoms

- Transforms visually drift, rotate the wrong way, or decompose incorrectly after math changes in `transform/`.
- Timing or transition data parses correctly upstream but behaves wrong downstream when the style data representation changes.
- Default or copied style values silently diverge when a style struct gains new fields without corresponding initialization behavior.

## Validate

This directory does not define a standalone top-level unit-test exec. Validate through the nearest consumer targets.

Common starting points:

- `css_test_exec` for CSS-to-style conversion and style-object usage
- `animation_unittests_exec` when timing or animation-related style data changes
- `style_object_encoder_testset_exec` when style serialization or template encoding changes

If transform math is touched, make sure the nearest quaternion or transform tests in the owning consumer path still cover the change.
