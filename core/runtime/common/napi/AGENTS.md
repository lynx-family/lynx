# AGENTS.md

## Scope

This directory contains shared NAPI environment and runtime-proxy glue for multiple runtime backends.

## Edit Rules

- Keep environment and runtime-proxy logic backend-agnostic where possible; backend-specific proxy specializations should stay in their dedicated files.
- Be especially careful with lifetime, runtime ownership, and cross-engine capability differences.

## Common Regression Symptoms

- NAPI-backed modules work in one engine but fail in another after proxy changes.
- Environment creation/teardown regresses only in integration flows, not in isolated call sites.

## Validate

There is no standalone exec declared directly here. Validate through:

- `runtime_tests_exec`

If the NAPI environment contract changed, also inspect nearby runtime common and value-wrapper coverage.
