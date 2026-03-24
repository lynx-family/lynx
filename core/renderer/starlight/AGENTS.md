# AGENTS.md

## Scope

This directory contains the Starlight layout/style engine used by the renderer: layout algorithms, style data, layout types, and layout events.

## Module Map

- `layout/`: layout objects, algorithms, caches, and property resolving.
- `style/`: layout-facing style data and computed-layout style containers.
- `types/`: shared layout units, constraints, and result types.
- `event/`: layout event types and handlers.

## Edit Rules

- Keep layout algorithms in `layout/` and data representation in `style/`; avoid mixing algorithmic fixes into style holders.
- Starlight changes often affect layout correctness and performance together. Be cautious with caches and property resolution.
- Child docs for `layout/` and `style/` take precedence when working there.

## Common Regression Symptoms

- Layout trees build but compute wrong sizes/positions after algorithm or property-resolution changes.
- Stale cache behavior leads to wrong layout only after incremental updates.
- Style fields look right but map into wrong layout behavior after style-data changes.

## Validate

Use `lynx-cpp-test` and start with:

- `starlight_unittest_exec`
