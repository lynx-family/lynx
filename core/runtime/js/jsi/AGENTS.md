# AGENTS.md

## Scope

This directory contains the JSI abstraction layer used by the JS runtime, plus engine-specific wrappers such as QuickJS, JSC, JSVM, and V8 integrations.

## Edit Rules

- Keep the generic JSI contract in shared files and engine-specific behavior in the corresponding subdirectories.
- Argument conversion and object wrapper semantics are shared foundations; small mistakes here propagate widely.

## Common Regression Symptoms

- Multiple engine backends regress after a shared JSI contract change.
- One engine behaves differently from the rest when a backend wrapper drifts from the shared abstraction.

## Validate

Validate with:

- `runtime_tests_exec`
- `js_runtime_unittests_exec`

If QuickJS-specific wrappers changed, also inspect the QuickJS tests in the `quickjs/` subtree.
