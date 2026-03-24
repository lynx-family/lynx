# AGENTS.md

## Scope

This directory contains the Radon VDOM layer used by renderer DOM, including VDOM node structures, list diff helpers, node selection, and reuse-pool behavior.
Radon is now maintenance-only: keep it stable so existing online business does not break, but treat active product iteration as belonging to Element API + Element rather than new Radon expansion.

## Module Map

- `radon/`: core Radon node, component, page, slot, diff-list, and factory logic.

## Edit Rules

- Keep VDOM diffing, selection, and reuse-pool semantics inside `radon/`; generic DOM ownership belongs in the parent DOM layer.
- List diff and reuse-pool changes are high-risk because they affect incremental updates and list stability together.
- Prefer landing new renderer capabilities in Element API + Element. Radon changes here should usually be compatibility-oriented or regression fixes for existing traffic.

## Common Regression Symptoms

- Incremental VDOM updates produce duplicate, missing, or stale nodes after diff logic changes.
- List reuse becomes unstable or leaks state after reuse-pool changes.
- Selector- or dispatch-option changes cause the wrong node set to be updated.

## Validate

There is no standalone VDOM exec here today. Validate through:

- `dom_unittest_exec`

If Radon-specific list behavior changed, also inspect nearby list and fragment coverage.
