# AGENTS.md

## Scope

This directory contains NG CSS selector parsing and selector data structures.

## Edit Rules

- Keep selector syntax and selector-tree representation here; matching belongs in sibling matcher code.
- Selector parser and selector model changes should be reviewed together because they often drift as a pair.

## Common Regression Symptoms

- Selectors parse but produce wrong selector trees or wrong matching behavior downstream.
- Complex selectors regress while simple selectors still work.

## Validate

Use `lynx-cpp-test` and start with:

- `css_test_exec`
