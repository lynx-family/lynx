# AGENTS.md

## Scope

This directory contains the core renderer DOM layer: element ownership, attributes, style resolution, element managers, layout-node coordination, and specialized subtrees such as air, fiber, selector, fragment, and vdom/radon.

## Module Map

- Root files define generic `Element` ownership, attributes, managers, and style resolution.
- `air/` contains Air-specific event/style store integration.
- `fiber/` contains fiber element types and tree resolution helpers.
- `fragment/` contains display-list and fragment behavior. Follow its child `AGENTS.md` when working there.
- `selector/` contains element selection logic.
- `vdom/` contains Radon VDOM structures and diff/select helpers.

## Edit Rules

- Keep generic element ownership and mutation semantics in the root DOM layer; specialized flows should stay in their own subdirectories.
- DOM changes often couple attributes, style resolution, and layout-node management together. Check all three before landing a "local" fix.
- Avoid moving renderer-wide helpers into `Element` or manager classes.
- **Adopted StyleSheets**: StyleSheet-related features and bug fixes must also account for `adopted_stylesheets_` in `element_manager.h`. These are CSS stylesheets loaded at runtime via DOM API (`AdoptStyleSheet`/`GetAdoptedStyleSheets`) and carry the highest cascade priority. Any change to stylesheet resolution, keyframes lookup, or style fragment iteration that only considers statically-bundled stylesheets will miss styles injected through this path.
- **CSSFragmentDecorator Integration**: When wiring adopted stylesheets into DOM elements, pass `element_manager()` to `CSSFragmentDecorator`'s constructor so the decorator can access adopted sheets. After this, style-related queries on the element (keyframes, font faces, pseudo rules) should go through the `CSSFragment*` returned by `GetRelatedCSSFragment()` rather than manually iterating `element_manager_->GetAdoptedStyleSheets()`.

## Common Regression Symptoms

- Nodes stop appearing, appear in the wrong order, or keep stale styles after manager or ownership changes.
- Selection, fragment, or fiber regressions often start from a shared `Element` contract mismatch.
- Layout-node churn or stale layout state often points to manager/style-resolution coupling rather than a single method bug.

## Validate

Prefer `lynx-cpp-test` and start with:

- `dom_unittest_exec`
- `element_selector_unittests_exec` when selector behavior changes
- `fragment_test_exec` when fragment/display-list behavior changes
