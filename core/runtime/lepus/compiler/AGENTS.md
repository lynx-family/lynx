# AGENTS.md

## Scope

This directory contains Lepus compiler-specific entry points and compiler-focused unit-test coverage.

## Edit Rules

- Keep compile-only behavior here. Shared parser, scanner, or runtime semantics belong in the parent Lepus layer.
- Compiler output changes should be reviewed for downstream template/bytecode consumers, not just compiler tests.

## Common Regression Symptoms

- Compilation succeeds but generated output is incompatible with runtime expectations.
- Compiler-only tests pass while runtime execution breaks after changing shared compiler/runtime assumptions.

## Validate

Use `lynx-cpp-test` and start with:

- `lepus_compiler_exec`

If compile output semantics changed, also run:

- `lepus_unittests_exec`
