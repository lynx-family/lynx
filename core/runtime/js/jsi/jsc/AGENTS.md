# AGENTS.md

## Scope

This directory contains the JavaScriptCore-backed JSI implementation: context wrappers, exception helpers, host functions/objects, and JSC runtime wrappers.

## Edit Rules

- Keep JSC-specific semantics here; shared JSI contracts belong in the parent JSI layer.
- Context wrappers, runtime wrappers, and host object/function behavior should stay consistent as a set.

## Common Regression Symptoms

- Only JSC-backed runtime flows regress after shared JSI changes.
- Host objects/functions compile but behave differently in JSC after local helper or wrapper changes.

## Validate

Validate through:

- `runtime_tests_exec`

If the change is engine-specific, inspect the nearest JSC-backed runtime coverage in the JS runtime test flow.
