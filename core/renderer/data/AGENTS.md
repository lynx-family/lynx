# AGENTS.md

## Scope

This directory contains renderer data holders such as template data, platform data, and view-data management.

## Edit Rules

- Keep this directory as a data and ownership layer, not a place for DOM mutation or CSS logic.
- Shared data structures here are consumed by multiple renderer paths; favor additive, backward-compatible changes.
- Platform-specific data differences belong in platform subdirectories or platform wrappers, not in generic fields without clear ownership.

## Common Regression Symptoms

- Page or template data arrives but downstream renderer behavior looks stale or incomplete.
- A small field change breaks multiple renderer consumers because data contracts here are shared widely.

## Validate

This directory does not define a standalone unit-test exec. Validate through the nearest renderer integration targets, especially `page_config_unittests_exec` or `dom_unittest_exec` when data shape changes affect page assembly or DOM behavior.
