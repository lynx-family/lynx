# AGENTS.md

## Scope

This directory contains LepusNG NAPI worklet bindings for Lepus components, elements, gestures, frame callbacks, and UI loader glue.

## Module Map

- `*.idl`: IDL declarations for worklet-exposed Lepus surfaces such as component, element, gesture, and Lynx host objects.
- `napi_lepus_*.*`: NAPI wrapper implementations for the IDL-defined worklet surfaces.
- `napi_frame_callback.*`, `napi_func_callback.*`: callback binding helpers used by worklet-facing code.
- `napi_loader_ui.*`: worklet/UI loader bridge.

## Key Files And Types

- The IDL files are part of the contract, not just documentation. Generated expectations and hand-written NAPI wrappers need to stay aligned.
- `napi_frame_callback.*` and `napi_func_callback.*` are central to callback lifecycle and worklet invocation behavior.
- `napi_loader_ui.*` sits on the bridge between worklet exposure and UI loader behavior.

## Typical Change Patterns

- If the issue is on one worklet-exposed object shape, inspect the matching `.idl` and `napi_lepus_*.*` pair together.
- If the issue is callback delivery, lifecycle, or frame-hook behavior, inspect `napi_frame_callback.*` or `napi_func_callback.*`.
- If the issue is generic LepusNG NAPI infrastructure rather than worklet surfaces, move up to the parent `napi/` layer.

## Edit Rules

- Keep worklet-facing NAPI glue here; generic LepusNG NAPI infrastructure belongs in the parent directory.
- IDL files and generated/bound implementation code must stay aligned.

## Invariants And Pitfalls

- IDL-backed surfaces and NAPI wrappers can compile while still disagreeing on runtime-visible behavior.
- Callback helper changes can silently break worklet scheduling or frame delivery even when object creation still works.

## Common Regression Symptoms

- Worklet objects exist but callbacks or frame hooks stop working after binding changes.
- A single worklet surface regresses because generated IDL-backed expectations drifted from implementation.

## Validate

No standalone exec is declared directly here. Validate through the nearest worklet/runtime consumers that exercise these bindings.

## Notes

- This subtree is adapter-heavy. When in doubt, compare the local binding change against the parent LepusNG NAPI contract before expanding behavior here.
