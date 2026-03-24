# AGENTS.md

## Scope

This directory contains React-facing renderer TASM integration and its Android-specific subpaths.

## Module Map

- `android/`: Android-specific React/TASM integration helpers.
- `testing/`: React/TASM test helpers and mocks.

## Edit Rules

- Keep React/TASM integration concerns here rather than pushing them into generic renderer TASM code.
- Test helpers are only for tests and should not become production dependencies.

## Common Regression Symptoms

- React/TASM integration regresses while generic page-config or TASM wiring still works.
- Android-specific React flows diverge from shared expectations after local changes.

## Validate

If Android mapbuffer or React-specific integration changes, use `lynx-cpp-test` and start with:

- `mapbuffer_unittests_exec`

Also run `page_config_unittests_exec` when shared TASM integration is affected.
