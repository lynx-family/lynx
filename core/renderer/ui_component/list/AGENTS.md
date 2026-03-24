# AGENTS.md

## Scope

This directory contains the renderer-owned internal list-container implementation: adapters, layout managers, anchors, children helpers, event management, and animation-aware list container behavior.

## Edit Rules

- Keep renderer-owned list behavior here and do not mix it with the generic decoupled list code in the core list layer.
- Adapter, anchor, and layout-manager changes usually need to stay consistent as a group.
- Animation-aware list flows here should stay aligned with the underlying animation contracts rather than reimplementing them ad hoc.

## Common Regression Symptoms

- Incremental list updates duplicate, drop, or reorder children after adapter or children-helper changes.
- Scroll anchoring or insert/remove positioning regresses after anchor-manager changes.
- Internal list animations stop or produce stale holders after container animation changes.

## Validate

Use `lynx-cpp-test` and start with:

- `internal_list_container_testset_exec`
