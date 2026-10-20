# AGENTS.md

## Scope

This directory contains fiber-specific element implementations and tree-resolution helpers for renderer DOM.

## Module Map

- `*_element.*`: concrete fiber element types such as block, text, image, list, page, and wrapper elements.
- `tree_resolver.*`: resolves fiber tree structure.
- `list_item_scheduler_adapter.*`: list scheduling bridge for fiber paths.
- `platform_layout_function_wrapper.*`: wrapper for platform layout callbacks in fiber flows.

## Edit Rules

- Keep per-element behavior in the owning element type and shared traversal logic in `tree_resolver.*`.
- Control-flow elements such as `if`, `for`, and slots are high fan-out changes because they affect tree shape, scheduling, and selection.
- List-related fiber changes often need to stay aligned with list scheduler adapters and renderer list integrations.
- Frame load callbacks must return through the parallel reduce phase before accessing engine-owned state, then use the pipeline's deferred queue if rendering is active. Keep weak-pointer creation on the engine thread and validate the frame's lifetime and current source when dispatching deferred callbacks.

## Common Regression Symptoms

- Conditional or looped subtrees render incorrectly or retain stale nodes after tree-resolution changes.
- Text, image, or scroll elements behave correctly in simple cases but fail in mixed trees after base fiber contract changes.
- Fiber list items schedule or diff incorrectly after adapter changes.

## Validate

This directory does not define a standalone exec. Validate with:

- `dom_unittest_exec`

If list behavior is involved, also consider:

- `internal_list_container_testset_exec`
