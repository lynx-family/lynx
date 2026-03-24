# AGENTS.md

## Scope

This directory contains Starlight layout algorithms and layout-object infrastructure: flex, grid, linear, relative, staggered-grid, positioning, caches, and property resolution.

## Edit Rules

- Keep algorithm-specific behavior in the corresponding algorithm file and shared geometry/property logic in shared helpers.
- Cache behavior and property resolution are high-risk because they affect many algorithms at once.
- Avoid folding style-data interpretation into layout-object state without checking the sibling Starlight style layer.

## Common Regression Symptoms

- Only one layout mode breaks after algorithm changes, or multiple modes regress after shared helper changes.
- Incremental layout is wrong while full layout is correct, often pointing to cache-manager or layout-object updates.

## Validate

Use `lynx-cpp-test` and start with:

- `starlight_unittest_exec`
