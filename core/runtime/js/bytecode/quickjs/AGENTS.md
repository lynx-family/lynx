# AGENTS.md

## Scope

This directory contains QuickJS-specific JS bytecode/cache integration under the JS bytecode layer.

## Edit Rules

- Keep QuickJS-specific cache or bytecode behavior here; shared cache policy belongs in the parent `bytecode/` directory.
- Be careful when changing cached data shape or QuickJS-specific serialization assumptions.

## Common Regression Symptoms

- QuickJS cached execution regresses while other engines keep working.
- Cache hits start producing stale or incompatible QuickJS bytecode after local changes.

## Validate

Validate through:

- `runtime_tests_exec`
