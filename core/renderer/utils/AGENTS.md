# AGENTS.md

## Scope

This directory contains renderer-specific utility helpers such as renderer env/tracing helpers, diff utilities, devtool lifecycle state, and prop-bundle style writing helpers.

## Edit Rules

- Keep this directory dependency-light and utility-oriented. Do not move core DOM or pipeline ownership here.
- Diff and prop-bundle helpers are reused broadly; small changes can cause many downstream regressions.

## Common Regression Symptoms

- Devtool lifecycle state drifts from renderer behavior after helper changes.
- Diff or prop-bundle utilities regress multiple unrelated renderer tests at once.

## Validate

Use `lynx-cpp-test` and start with:

- `renderer_utils_unittests_exec`
