# AGENTS.md

## Scope

This directory contains layout-wrapper objects such as `LayoutContext`, layout nodes, and list-node wrappers that bridge renderer layout state into wrapper consumers.

## Module Map

- `layout_context.*`, `layout_context_data.h`: wrapper-facing layout context abstraction and shared context data.
- `layout_node.*`, `list_node.*`: wrapper layout node surfaces used by consumers such as list and platform wrappers.
- `android/`, `ios/`, `harmony/`, `textra/`, `empty/`: platform- or backend-specific layout context and text-layout implementations.

## Key Files And Types

- `LayoutContext` is the top-level wrapper bridge for layout state.
- `layout_node.*` is the generic wrapper node surface; `list_node.*` is the list-specific specialization.
- Backend-specific text layout files under `android/`, `ios/`, and `textra/` are adapters around the shared wrapper contract rather than places for generic layout policy.

## Typical Change Patterns

- If the issue is wrapper-level node exposure or context handoff, start from `layout_context.*` or `layout_node.*`.
- If the issue is list-specific wrapper layout behavior, inspect `list_node.*` together with list consumers.
- If the issue is text measurement or platform-only layout bridging, inspect the relevant platform/backend subtree first.

## Edit Rules

- Keep wrapper-level layout translation here; core layout algorithms belong in DOM or Starlight layers.
- Changes to `LayoutContext` and `layout_node.*` often affect many wrapper consumers simultaneously.

## Invariants And Pitfalls

- This directory should translate layout state, not own layout algorithms.
- Platform text-layout adapters can drift from the shared wrapper surface if one backend gets patched in isolation.

## Common Regression Symptoms

- Layout wrappers build but propagate stale or incomplete node information.
- List-specific wrapper behavior diverges from generic node behavior after local changes.

## Validate

No standalone exec is defined here. Validate through the nearest renderer consumers, especially DOM, list, or fragment tests.

## Notes

- Wrapper-layer fixes here often need a quick check against both generic nodes and list-node consumers before they are truly safe.
