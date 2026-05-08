# AGENTS.md

## Scope

This directory contains Lynx's CSS model and application layer: CSS values, properties, selectors, stylesheet management, dynamic CSS helpers, computed-style helpers, and integration with parser and NG selector infrastructure.
New CSS feature work should target the NG path by default; non-NG code in this area is primarily compatibility and maintenance surface.

## Module Map

- Root files define CSS value/property/token models, stylesheet management, computed-style helpers, and dynamic CSS handling.
- `parser/` contains property handlers, shorthands, scanners, and string parsing for CSS text input.
- `ng/` contains next-generation selector, tokenizer, invalidation, and rule-set helpers.
- `transforms/` contains CSS transform parsing helpers.

## Edit Rules

- Keep tokenization and property parsing in `parser/` or `ng/`; do not hide parsing logic inside style application code.
- Keep stylesheet, fragment, and computed-style semantics in shared CSS code rather than DOM node classes.
- New CSS features should land in the `ng/` path by default. Extend non-NG behavior only when the task is explicitly about compatibility or maintenance.
- Selector or invalidation changes can affect large portions of style recomputation. Treat them as high fan-out changes.
- **Adopted StyleSheets**: StyleSheet-related features and bug fixes must also account for `adopted_stylesheets_` in `core/renderer/dom/element_manager.h`. These are CSS stylesheets loaded at runtime via DOM API (`AdoptStyleSheet`/`GetAdoptedStyleSheets`) and carry the highest cascade priority. Any change to stylesheet resolution, keyframes lookup, or fragment iteration that only considers statically-bundled stylesheets will miss styles injected through this path.

## CSSFragmentDecorator and Adopted Sheets

- `CSSFragmentDecorator` is the single integration point for adopted stylesheets. It combines `intrinsic_style_sheets_` (statically bundled) with `adopted_stylesheets_` (runtime-injected via `ElementManager`).
- All callers should obtain a `CSSFragment*` via `GetRelatedCSSFragment()` or `GetCSSFragment()`, which returns a `CSSFragmentDecorator` when adopted sheets are present. Do **not** query `ElementManager::GetAdoptedStyleSheets()` directly from style resolution code.
- The decorator uses `ForEachAdoptedFragment(visitor, reverse)` to iterate adopted sheets under `ElementManager`'s shared lock. Iteration order matters: reverse (newest first) for keyframes lookup, forward for font faces and invalidation.
- When adding new virtual extension points to `CSSFragment` for adopted-sheet behavior (e.g., `ForEachXxxMap`), always provide a base implementation that delegates to the intrinsic fragment only, and override it in `CSSFragmentDecorator` to include adopted sheets.
- Thread safety: `ElementManager` guards `adopted_stylesheets_` with a `std::shared_mutex`. Readers use `ForEachAdoptedStyleSheet()` which acquires a `shared_lock`. Do not copy the vector outside the lock.

## Common Regression Symptoms

- CSS parses but applies the wrong property/value, usually pointing to parser handlers or property metadata.
- Dynamic or shared styles stop updating when stylesheet-manager or fragment wiring drifts.
- Selector matches become too broad or too narrow after `ng/` invalidation or selector changes.

## Validate

For C++ tests here, prefer `lynx-cpp-test` and use the targets in this subtree's `BUILD.gn` files.

Start with:

- `css_test_exec`
- `css_parser_test_exec`

If you changed keyframes or animation-related CSS tokens, also consider `animation_unittests_exec`.
