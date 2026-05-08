# AGENTS.md

## Scope

This directory contains the renderer stack of Lynx: CSS parsing and style application, DOM and fragment structures, TASM/page assembly, data flow, layout/pipeline coordination, starlight layout/style machinery, signals, worklets, and renderer utilities.

## Module Map

- `css/`: CSS values, selectors, parsing, stylesheet management, and computed-style helpers.
- `dom/`: element tree, attributes, layout-node management, selectors, fragments, and VDOM/radon pieces.
- `data/`: template/page data and view-data managers.
- `events/`: renderer-side event helpers and integration glue.
- `pipeline/`: render/update pipeline coordination.
- `signal/`: signal graph used by renderer subsystems.
- `starlight/`: layout/style execution machinery and starlight data structures.
- `tasm/`: page-assembly, React/TASM integration, and renderer-side TASM helpers.
- `utils/`: renderer-specific helpers and environment utilities.
- `worklet/`: renderer worklet support.
- `simple_styling/`, `ui_component/`, `ui_wrapper/`: specialized style or UI integration layers.

## Key Files And Types

- `BUILD.gn`: the `tasm_group` and `tasm` targets show how renderer code fans out into animation, runtime, shared data, shell, and services.
- `dom/element.*` and related managers: core DOM node ownership and mutation path.
- `css/css_*`: CSS token/value/model definitions and stylesheet application path.
- `pipeline/*` and `starlight/*`: update and layout pipelines that propagate renderer state changes.
- `signal/*`: reactive coordination primitives used by renderer subsystems.

## Typical Change Patterns

- For CSS parsing, style tokenization, or stylesheet behavior, start in `css/`.
- For element tree, fragment generation, selectors, or layout-node ownership, start in `dom/`.
- For render/update sequencing issues, start in `pipeline/` and `starlight/`.
- For page configuration or TASM-facing renderer assembly, start from `BUILD.gn` and `tasm/`.
- If you are inside `dom/fragment/`, follow the child `AGENTS.md` there instead of this file.

## Edit Rules

- Keep CSS parsing and value semantics in `css/`; do not hide them inside DOM mutation code.
- Keep DOM ownership and element-tree behavior in `dom/`; do not move renderer-wide utilities into node classes.
- Renderer code depends on the runtime, shared-data, and shell layers in several paths. Local renderer edits can still be cross-module changes.
- Prefer pure helpers in `utils/` and keep platform glue out of shared renderer code.
- **Adopted Stylesheet Pattern**: The renderer uses `CSSFragmentDecorator` (in `css/`) as the bridge between `ElementManager`'s runtime-adopted stylesheets (`dom/`) and CSS resolution (`style_resolver.cc`). This is a cross-module pattern: `ElementManager` owns the storage + lock, `CSSFragmentDecorator` owns the iteration logic, and `StyleResolver` + DOM elements consume the unified `CSSFragment*` interface. When changing any piece of this triangle, verify the other two still compile and behave correctly.

## Common Regression Symptoms

- Styles parse correctly but do not apply usually points to `css/` to `dom/` handoff issues.
- Nodes appear in the wrong order, disappear, or fail to update after mutations when `dom/` ownership or pipeline invalidation drifts.
- Layout or display-list regressions often point to `starlight/`, `pipeline/`, or `dom/fragment/`.
- Page config or TASM assembly regressions usually point to `tasm/` or renderer target wiring in `BUILD.gn`.

## Validate

For C++ unit tests here, prefer the `lynx-cpp-test` skill and use the smallest relevant target from the nearest `BUILD.gn`.

Common starting points:

- `page_config_unittests_exec`
- `css_test_exec`
- `css_parser_test_exec`
- `dom_unittest_exec`
- `fragment_test_exec`
- `element_selector_unittests_exec`
- `pipeline_test_exec`
- `signal_test_exec`
- `starlight_unittest_exec`
- `worklet_unittests_exec`
- `renderer_utils_unittests_exec`

Expand validation when renderer changes cross into animation, runtime, or shell integration.
