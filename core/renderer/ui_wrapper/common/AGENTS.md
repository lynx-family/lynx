# AGENTS.md

## Scope

This directory contains shared UI-wrapper prop-bundle helpers and native prop-bundle creation logic.

## Module Map

- `native_prop_bundle.*`: shared native prop-bundle representation and access helpers.
- `prop_bundle_creator_default.*`: default prop-bundle construction path used by wrapper consumers.
- `android/`, `ios/`, `harmony/`: platform-specific prop-bundle and extra-bundle implementations.
- `testing/`: prop-bundle mocks used by tests.

## Key Files And Types

- `native_prop_bundle.*` is the shared representation boundary. Shape changes here can fan out to multiple platform wrappers.
- `prop_bundle_creator_default.*` is where shared bundle construction policy lives before platform-specific conversion takes over.
- Platform `prop_bundle_*` files are adapters; shared bundle semantics should usually not be duplicated across them.

## Typical Change Patterns

- If the issue is shared prop-bundle shape or default creation logic, start from `native_prop_bundle.*` or `prop_bundle_creator_default.*`.
- If the issue reproduces only on one platform, inspect that platform's `prop_bundle_*` or `platform_extra_bundle_*` implementation before changing shared code.

## Edit Rules

- Keep generic prop-bundle construction here; platform-specific interpretation belongs in platform wrapper code.
- Test doubles under `testing/` are only for tests and should not become runtime dependencies.

## Invariants And Pitfalls

- Bundle-layout changes can silently affect multiple wrapper consumers even when only one platform symptom is visible.
- Test mocks under `testing/` should track shared behavior closely enough to stay useful, but they are not production ownership.

## Common Regression Symptoms

- Props silently disappear or serialize differently across wrappers after bundle-creator changes.
- Shared wrapper tests pass but platform rendering regresses because native prop-bundle layout changed.

## Validate

There is no dedicated exec in this directory. Validate through the nearest wrapper consumer tests, especially list, DOM, or fragment paths that consume prop bundles.

## Notes

- The Android subtree already carries prop-bundle-specific tests, so shared changes should be reviewed with platform adapters in mind even when the common layer looks small.
