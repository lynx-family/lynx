# AGENTS.md

## Scope

This directory groups renderer-owned UI component implementations. Today the primary concrete subtree here is `list/`.

## Module Map

- `list/`: list container adapters, layout managers, event helpers, animation helpers, and list-specific unit tests.

## Key Files And Boundaries

- This directory is mostly a routing layer. Real implementation ownership currently lives in the `list/` subtree.
- If another renderer UI component grows here, it should get its own child `AGENTS.md` rather than overloading the list guidance.

## Typical Change Patterns

- If the change is list-specific, go straight to `list/` and follow that child guide.
- If the change is renderer-generic rather than component-specific, it probably belongs elsewhere in `renderer/` instead of under `ui_component/`.

## Edit Rules

- Use this directory to choose the right component subtree, then follow the nearest child `AGENTS.md`.
- Do not treat this directory itself as a place to add generic renderer utilities.

## Common Regression Symptoms

- A fix lands in `ui_component/` even though the real ownership was in generic renderer or DOM code.
- New component logic gets mixed into the list subtree because there is no directory-level routing guidance.

## Validate

Follow the child component's validation guidance. For list-related changes, start with:

- `internal_list_container_testset_exec`

## Notes

- This directory should stay mostly organizational until multiple top-level UI components live here.
