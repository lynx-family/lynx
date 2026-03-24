# AGENTS.md

## Scope

This directory contains runtime helpers shared across backends, especially console/error reporting and small cross-runtime helper contracts.

## Module Map

- Root files contain shared JS error reporting and console helpers.
- `bindings/` contains shared event, module, and resource bridge contracts.
- `napi/` contains runtime-agnostic NAPI environment and runtime proxy glue.

## Edit Rules

- Keep backend-neutral contracts here. Backend-specific execution should remain in the JS runtime layer, the classic Lepus layer, or the LepusNG layer.
- Error-reporting helpers are shared widely; changes here can fan out to many runtime tests.

## Common Regression Symptoms

- Errors or console output disappear across multiple runtimes after a "small" reporting change.
- Resource or event bridges regress across backends when a shared helper contract shifts.

## Validate

Use `lynx-cpp-test` and start with the nearest targets:

- `runtime_tests_exec`
- `runtime_common_unittests_exec`
- `resource_common_unittests_exec`
