# AGENTS.md

## Scope

This directory contains the JS runtime layer: runtime manager/lifecycle, JS bundle ownership, execution wrappers, JS utilities, and child directories for bindings, bytecode, and JSI.

## Module Map

- Root files manage JS bundle, runtime manager, executor, and lifecycle helpers.
- `bindings/`: JS-facing event/module/resource/global bindings.
- `bytecode/`: JS cache and bytecode helpers.
- `jsi/`: JSI abstraction and concrete engine wrappers.

## Edit Rules

- Keep engine-neutral JS runtime lifecycle in the root directory and engine-specific object semantics in `jsi/`.
- Changes to runtime manager or bundle ownership often affect shell integration and teardown order.

## Common Regression Symptoms

- JS runtime starts but lifecycle callbacks or teardown behave incorrectly.
- Bundles load but exposed APIs or task adapters stop behaving consistently.

## Validate

Use `lynx-cpp-test` and start with:

- `runtime_tests_exec`

Expand to the nearest child targets when working under `bindings/` or `jsi/`.
