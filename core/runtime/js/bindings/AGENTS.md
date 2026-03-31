# AGENTS.md

## Scope

This directory contains JS runtime bindings such as global objects, JS app glue, task adapters, JS error helpers, and child directories for event/module/resource bindings.

## Module Map

- Root files define shared JS binding objects like `Lynx`, `JSApp`, task adapters, and codec helpers.
- `event/`: JS event listener and context proxy integration.
- `modules/`: JS module manager and module callback/binding infrastructure.
- `resource/`: resource response handling on the JS side.

## Edit Rules

- Keep shared JS binding objects in the root and backend-neutral shared contracts in the common runtime binding layer.
- Module, event, and resource bindings have different lifecycle assumptions; avoid patching them all through one generic fix without checking ownership.

## Common Regression Symptoms

- JS globals exist but callbacks fail, module invocations disappear, or resources never resolve.
- Event listeners behave differently from resource/module callbacks after shared binding changes.

## Validate

Use `lynx-cpp-test` and start with:

- `runtime_tests_exec`
- `js_runtime_unittests_exec`
- `response_handler_js_unittests_exec` when resource bindings change
