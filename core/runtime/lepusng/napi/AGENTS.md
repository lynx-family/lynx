# AGENTS.md

## Scope

This directory contains LepusNG NAPI integration, including generated test scaffolding and worklet-facing NAPI bindings.

## Module Map

- `test/`: generated or test-focused NAPI module/context scaffolding.
- `worklet/`: NAPI worklet bindings for Lepus components, elements, gestures, and frame callbacks.

## Edit Rules

- Keep test scaffolding in `test/` and production worklet glue in `worklet/`.
- IDL- or generated-binding changes can have wide impact even if the implementation diff is small.

## Common Regression Symptoms

- Worklet bindings compile but generated NAPI surfaces drift from runtime expectations.
- Test-only NAPI scaffolding passes while real worklet bindings fail after interface changes.

## Validate

No standalone exec is declared at this level. Validate through the owning runtime/worklet consumers and the nearest generated test targets in this subtree.
