# AGENTS.md

## Scope

This directory contains lazy-bundle loading support: request description,
lifecycle options, unified engine/runtime resource loading, shared bundle
management, and lazy-bundle utilities.

## Edit Rules

- Keep lifecycle policy in `lazy_bundle_lifecycle_option.*` and I/O orchestration in `lazy_bundle_loader.*`.
- Keep cross-thread template-bundle state in `bundle_manager.*`; loader
  instances that serve different actors must share the same manager.
- Lazy-bundle changes are integration-heavy; preserve request/state transitions carefully.

## Common Regression Symptoms

- Lazy bundles never load, load twice, or transition through the wrong lifecycle state.
- Request shape changes break downstream consumers even when the loader still compiles.

## Validate

Use `lynx-cpp-test` and start with:

- `lazy_bundle_test_exec`
