# AGENTS.md

## Scope

This directory contains Starlight's shared layout types: units, constraints, directions, measure-function contracts, layout results, and generic length/value helpers.

## Edit Rules

- Treat these files as shared foundational types used by both Starlight layout algorithms and style-to-layout translation.
- Small type or enum changes here can have broad compile-time and runtime impact.

## Common Regression Symptoms

- Multiple layout algorithms regress at once after a seemingly harmless type change.
- Numeric or unit interpretation drifts after `nlength` or layout-unit changes.

## Validate

Use `lynx-cpp-test` and start with:

- `starlight_unittest_exec`
