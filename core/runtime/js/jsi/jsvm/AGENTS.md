# AGENTS.md

## Scope

This directory contains the JSVM-backed JSI implementation: context wrappers, runtime wrappers, dynamic loading helpers, host functions/objects, and JSVM-specific exception/util helpers.

## Edit Rules

- Keep JSVM-specific bridge behavior here and preserve parity with the parent JSI contract.
- Dynamic loading, runtime wrappers, and host object/function behavior are closely related in this backend.

## Common Regression Symptoms

- JSVM is the only engine backend that regresses after JSI changes.
- Runtime starts but host functions, exceptions, or dynamic loading behave inconsistently after local edits.

## Validate

Validate through:

- `runtime_tests_exec`
