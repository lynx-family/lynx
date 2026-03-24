# AGENTS.md

## Scope

This directory contains renderer signal primitives such as computations, memos, scopes, and signal contexts.

## Edit Rules

- Keep reactive semantics local to signal primitives; do not mix renderer business logic into these classes.
- Be especially careful with dependency tracking, lifetime, and re-computation order.

## Common Regression Symptoms

- Signals stop updating, update too often, or leak scopes after changes to memo/computation behavior.
- Context or scope teardown regressions show up as stale subscriptions or double execution.

## Validate

Use `lynx-cpp-test` and start with:

- `signal_test_exec`
