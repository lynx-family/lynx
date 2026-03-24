# AGENTS.md

## Scope

This directory contains DOM element selection logic for normal and fiber trees.

## Edit Rules

- Keep selector traversal and match-result logic here; do not push it into generic DOM element classes.
- Changes to selection result shape or ordering can affect multiple consumers even when tests are small.

## Common Regression Symptoms

- Element lookup works on one tree type but not another after `fiber_element_selector` or shared result changes.
- Selection order, match shape, or filtering subtly drifts after `select_result` or selector-item changes.

## Validate

Use `lynx-cpp-test` and start with:

- `element_selector_unittests_exec`

If shared DOM contracts changed, also run `dom_unittest_exec`.
