# AGENTS.md

## Scope

This directory contains the Radon VDOM implementation: Radon nodes/components/pages/slots, list diff logic, reuse pools, selection helpers, and factory/dispatch options.

## Edit Rules

- Keep Radon-specific diffing, reuse, and selection behavior here rather than parent DOM code.
- List diff nodes, reuse pools, and factory behavior are tightly coupled. Be careful with incremental-update assumptions.

## Common Regression Symptoms

- Incremental updates create duplicate or stale Radon nodes after diff or factory changes.
- List reuse regresses only after multiple updates, often pointing to reuse-pool or diff-list behavior.

## Validate

There is no standalone Radon exec. Validate through:

- `dom_unittest_exec`

If list-heavy VDOM behavior changed, also inspect nearby list coverage.
