# AGENTS.md

## Scope

This directory contains painting-wrapper objects such as `PaintingContext`, native painting context wrappers, the catalyzer, and platform renderer interfaces.

## Module Map

- `painting_context.*`, `native_painting_context.*`: shared wrapper-facing painting context abstractions.
- `platform_renderer.*`, `platform_renderer_impl.*`: renderer-side platform abstraction and implementation surface.
- `catalyzer.*`: bridge/helper logic around painting pipeline handoff.
- `android/`, `ios/`, `harmony/`, `empty/`: platform-specific painting context, renderer, and UI delegate implementations.

## Key Files And Types

- `PaintingContext` and `NativePaintingContext` are the main shared wrapper contracts.
- `platform_renderer.*` is the cross-platform renderer-facing surface that platform adapters implement.
- Platform-specific `ui_delegate_*` and `platform_renderer_*` files should stay adapters around the shared painting contract.

## Typical Change Patterns

- If the issue is shared painting-context handoff or platform-reference ownership, start from `painting_context.*` or `native_painting_context.*`.
- If the issue reproduces only on one platform renderer, inspect that platform subtree before changing shared wrapper code.
- If the issue is really DOM fragment generation or display-list assembly, the fix likely belongs outside this directory.

## Edit Rules

- Keep painting bridge behavior here; display-list generation or DOM fragment logic belongs elsewhere.
- Be careful with ownership and platform references in native painting contexts.

## Invariants And Pitfalls

- Shared painting-context abstractions should remain platform-neutral even when one platform needs an adapter tweak.
- Platform renderer and UI delegate files are tightly coupled to context handoff; changing one side in isolation can produce stale handle or lifecycle bugs.

## Common Regression Symptoms

- Painting callbacks execute with stale references or missing platform handles.
- Rendering regresses only after paint-context handoff, not during DOM/fragment generation.

## Validate

No standalone exec is defined here. Validate through fragment/display-list and renderer consumer tests.

## Notes

- The iOS subtree already carries painting context unit coverage, but many bugs here still only show up when wrapper consumers exercise the full handoff path.
- On Android, TextService / Textra handoff is not fragment-layer only:
  `NativePaintingContextAndroid` can forward bundles through `PlatformRendererContext`,
  while old `PaintingContextAndroid` may need to bridge text bundles into Java
  extra-data updates for `UIText` / `FlattenUIText`.
