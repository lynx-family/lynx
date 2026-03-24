# AGENTS.md

## Scope

This directory contains FSP tracing infrastructure, including FSP tracer configuration and trace generation.

## Module Map

- `fsp_tracer.*`: top-level tracer entry and orchestration.
- `fsp_config.h`: FSP configuration contract.
- `area/` and `axial/`: different tracer implementations for FSP tracing modes.
- `base/`: shared snapshot/tracer base contracts.

## Key Files And Types

- `fsp_tracer.*` is the shared orchestration point that ties config and tracer implementation together.
- `fsp_config.h` defines behavior-shaping inputs rather than just constants.
- Area and axial tracer implementations should stay aligned on observable FSP semantics even when their internal mechanics differ.

## Typical Change Patterns

- If the issue is tracer selection, orchestration, or shared FSP lifecycle, start from `fsp_tracer.*`.
- If the issue is mode-specific tracing behavior, inspect the matching `area/` or `axial/` implementation.
- If the issue is really generic performance reporting rather than FSP-specific semantics, check sibling services before growing this subtree.

## Edit Rules

- Keep FSP tracing as an observational/measurement layer rather than a place to change core renderer/runtime ownership behavior.
- Config and tracer behavior should stay aligned; avoid updating one without the other.

## Invariants And Pitfalls

- FSP config and tracer implementation are one behavior surface from the caller's point of view.
- Measurement layers can regress silently by shifting windows or semantics instead of throwing obvious failures.

## Common Regression Symptoms

- FSP metrics disappear or shift window boundaries after tracer/config changes.
- Performance data remains present but no longer matches the expected FSP semantics.

## Validate

There is no standalone exec declared in this directory. Validate through the nearest performance/FSP-consuming flows.

## Notes

- This subtree is measurement-specific; if a change starts carrying renderer/runtime policy, the ownership is probably drifting.
