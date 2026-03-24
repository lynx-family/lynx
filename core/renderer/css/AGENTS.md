# AGENTS.md

## Scope

This directory contains Lynx's CSS model and application layer: CSS values, properties, selectors, stylesheet management, dynamic CSS helpers, computed-style helpers, and integration with parser and NG selector infrastructure.

## Module Map

- Root files define CSS value/property/token models, stylesheet management, computed-style helpers, and dynamic CSS handling.
- `parser/` contains property handlers, shorthands, scanners, and string parsing for CSS text input.
- `ng/` contains next-generation selector, tokenizer, invalidation, and rule-set helpers.
- `transforms/` contains CSS transform parsing helpers.

## Edit Rules

- Keep tokenization and property parsing in `parser/` or `ng/`; do not hide parsing logic inside style application code.
- Keep stylesheet, fragment, and computed-style semantics in shared CSS code rather than DOM node classes.
- Selector or invalidation changes can affect large portions of style recomputation. Treat them as high fan-out changes.

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
