# AGENTS.md

## Scope

This directory contains the QuickJS-backed JSI implementation: runtime/context wrappers, exceptions, helper conversions, host functions/objects, and runtime wrapper glue.

## Edit Rules

- Keep QuickJS-specific semantics here; generic JSI contracts belong in the parent JSI layer.
- Runtime, context wrapper, host function/object, and exception handling are tightly coupled. Check all four when changing engine behavior.

## Common Regression Symptoms

- QuickJS is the only engine that regresses after shared JSI changes.
- Host objects/functions compile but behave differently at runtime after helper or context-wrapper edits.

## Validate

Validate through:

- `runtime_tests_exec`

Also inspect the QuickJS unit-test set declared in this directory's `BUILD.gn`.
