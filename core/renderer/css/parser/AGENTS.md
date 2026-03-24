# AGENTS.md

## Scope

This directory contains CSS string parsing and property-handler logic: scanners, token streams, and per-property or shorthand handlers that convert CSS text into typed style values.

## Edit Rules

- Put property-specific parsing rules in the relevant `*_handler.*` file and keep shared scanner/token utilities generic.
- Shorthand handlers are high-risk because they fan out into multiple longhand properties. Check defaults and expansion rules carefully.
- Keep parsing concerns here; downstream style interpretation belongs in CSS model or style layers.

## Common Regression Symptoms

- A single property starts parsing wrong units, enums, or defaults after changing its handler.
- Shorthands expand incompletely or overwrite unrelated longhands.
- The parser accepts invalid input or rejects valid input when scanner/token-stream changes drift.

## Validate

Prefer `lynx-cpp-test` and start with:

- `css_parser_test_exec`

If the parsed output is consumed by broader CSS logic, also run:

- `css_test_exec`
