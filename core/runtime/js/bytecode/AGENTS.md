# AGENTS.md

## Scope

This directory contains JS bytecode and cache-management helpers such as cache manager, cache tracker, metadata, and bytecode backend glue.

## Edit Rules

- Keep bytecode/cache policy here rather than leaking it into generic runtime manager logic.
- Cache metadata and tracker changes are high-risk because they affect reuse, invalidation, and persistence together.

## Common Regression Symptoms

- Cached scripts stop being reused, reuse stale bytecode, or invalidate too often after tracker changes.
- Bytecode behavior diverges between engines or platforms after metadata changes.

## Validate

There is no local exec target in this directory. Validate through:

- `runtime_tests_exec`

If cache-specific behavior changed, inspect the nearby bytecode unit-test set in this subtree as part of the runtime target.
