# AGENTS.md

## Scope

This directory contains the V8-backed JSI implementation: V8 runtime/isolate wrappers, context wrappers, exceptions, host functions/objects, and inspector hooks.

## Edit Rules

- Keep V8-specific isolate/runtime concerns here rather than in shared JSI code.
- Context, isolate, host object/function, and exception helpers must stay aligned across the backend.

## Common Regression Symptoms

- V8-only runtime regressions appear after shared or local JSI changes.
- Inspector or isolate behavior drifts from runtime behavior after partial backend edits.

## Validate

Validate through:

- `runtime_tests_exec`
