# AGENTS.md

## Scope

This directory contains Lynx's decoupled list-container abstractions: adapters, layout managers, anchor management, event helpers, and optional list animation integration.

## Module Map

- `decoupled_list_container_*`: list-container implementation and default fallback behavior.
- `decoupled_*layout_manager*`: grid, linear, staggered-grid, and shared layout logic.
- `decoupled_*adapter*`: adapter and item-holder coordination.
- `decoupled_list_anchor_manager.*` and `decoupled_list_children_helper.*`: anchor and child bookkeeping.
- `list_animation_manager*`: list animation coordination, with implementation selected by build flags.
- `testing/`: mock list elements and test helpers.

## Key Files And Types

- `decoupled_list_container_impl.*`: main implementation path when platform list implementation is disabled.
- `decoupled_list_layout_manager.*`: core layout bookkeeping shared by list layouts.
- `decoupled_list_event_manager.*`: list event dispatch coordination.
- `list_animation_manager.*`: abstraction for animated list operations; implementation changes can affect adapter, layout, and animation coupling together.

## Edit Rules

- Keep this directory platform-agnostic. Platform UI specifics belong in renderer or platform layers, not here.
- Layout-manager changes often affect anchor handling and child bookkeeping together; inspect related helpers before making local fixes.
- Animation wiring here depends on the Lynx basic animator adapter in the animation layer. Do not re-implement generic animation logic in list code.
- `testing/` is test-only support code and should not become production dependency.

## Common Regression Symptoms

- List items jump, anchor to the wrong place, or lose scroll position after layout-manager changes.
- Insert/remove operations produce duplicate or missing child bookkeeping when adapter or children-helper logic drifts.
- List animations stop running or run with the wrong ordering when animation-manager changes break adapter coordination.

## Validate

For C++ unit tests here, prefer the `lynx-cpp-test` skill and use the smallest matching target from this directory's `BUILD.gn`.

Start with:

- `list_container_testset_exec`

If you changed behavior shared with renderer-side list integration, also consider:

- `internal_list_container_testset_exec`
