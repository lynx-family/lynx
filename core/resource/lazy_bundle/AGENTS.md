# AGENTS.md

## Scope

This directory contains lazy-bundle loading support: request description, lifecycle options, loader orchestration, and lazy-bundle utilities.

## Edit Rules

- Keep lifecycle policy in `lazy_bundle_lifecycle_option.*` and I/O orchestration in `lazy_bundle_loader.*`.
- Lazy-bundle changes are integration-heavy; preserve request/state transitions carefully.

## Common Regression Symptoms

- Lazy bundles never load, load twice, or transition through the wrong lifecycle state.
- Request shape changes break downstream consumers even when the loader still compiles.

## Validate

Use `lynx-cpp-test` and start with:

- `lazy_bundle_test_exec`
