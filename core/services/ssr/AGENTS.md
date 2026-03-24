# AGENTS.md

## Scope

This directory contains SSR service glue, currently centered on SSR type info and client-side SSR integration sources pulled into the service target.

## Module Map

- `ssr_type_info.h`: SSR-facing type and contract surface.
- `client/`: SSR client utilities, data update management, event utilities, hydrate info, and stylesheet-facing helpers.

## Key Files And Types

- `ssr_type_info.h` is the smallest but broadest contract point in this subtree.
- Client helpers under `client/` form the practical SSR integration path and should be reviewed together when SSR state flow changes.

## Typical Change Patterns

- If the issue is SSR type shape or shared SSR contract meaning, start from `ssr_type_info.h`.
- If the issue is client hydration, data update, or event replay/integration behavior, inspect the matching `client/` helper pair.
- If the issue reproduces outside SSR-only flows, check whether the fix belongs in runtime or renderer instead of broadening this service glue layer.

## Edit Rules

- Keep SSR-specific service contracts here; generic runtime or renderer contracts belong in their owning modules.
- Even small type-info changes can affect SSR client integration and serialization assumptions.

## Invariants And Pitfalls

- SSR-specific helpers can stay green in compile-time checks while still drifting semantically from client expectations.
- This subtree should glue SSR behavior together, not become a second ownership layer for generic rendering logic.

## Common Regression Symptoms

- SSR-specific flows regress while non-SSR runtime/render paths remain healthy.
- Type info compiles but client integration reads it differently after local changes.

## Validate

There is no standalone exec declared in this directory. Validate through the nearest SSR-consuming flows and integration coverage.

## Notes

- SSR bugs are often isolated to this subtree's contracts, so "only SSR broke" is a useful ownership clue rather than a reason to patch shared layers first.
