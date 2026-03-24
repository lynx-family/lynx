# AGENTS.md

## Scope

This directory contains renderer-side TASM support such as page config, TASM config/NAPI glue, sysinfo helpers, React/TASM integration, and test helpers.

## Module Map

- Root files contain shared TASM config and NAPI binding entry points.
- `sysinfo/` contains platform-specific sysinfo providers.
- `react/` contains React/TASM integration support.
- `testing/` contains mocks and test helpers for TASM-facing renderer code.

## Edit Rules

- Keep platform sysinfo behavior in `sysinfo/` and shared config/binding contracts in root files.
- React/TASM integration should stay separate from generic page-config or renderer core logic.
- Test helpers here are not production implementations.

## Common Regression Symptoms

- Page config or TASM wiring regresses even though DOM/CSS logic looks unchanged.
- Platform sysinfo behaves differently across platforms after a shared change.
- Renderer tests start failing because mocks in `testing/` no longer match live contracts.

## Validate

Use `lynx-cpp-test` and start with:

- `page_config_unittests_exec`

If you touched Android mapbuffer integration under `react/android/mapbuffer`, also run:

- `mapbuffer_unittests_exec`
