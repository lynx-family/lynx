# AGENTS.md

## Scope

This directory contains the Radon VDOM implementation: Radon nodes/components/pages/slots, list diff logic, reuse pools, selection helpers, and factory/dispatch options.

## Edit Rules

- Keep Radon-specific diffing, reuse, and selection behavior here rather than parent DOM code.
- With new styling enabled, pseudo-state invalidation must mark the Element's style dirty and defer resolution. Eager legacy refresh overwrites the committed style before the new pipeline can diff it or sample transitions. Let style resolution determine whether layout is needed. Preserve the existing eager refresh and batching behavior when new styling is disabled.
- List diff nodes, reuse pools, and factory behavior are tightly coupled. Be careful with incremental-update assumptions.

## Common Regression Symptoms

- Incremental updates create duplicate or stale Radon nodes after diff or factory changes.
- List reuse regresses only after multiple updates, often pointing to reuse-pool or diff-list behavior.

## Validate

There is no standalone Radon exec. Validate through:

- `dom_unittest_exec`

If list-heavy VDOM behavior changed, also inspect nearby list coverage.
