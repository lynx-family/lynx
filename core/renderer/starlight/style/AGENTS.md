# AGENTS.md

## Scope

This directory contains Starlight's layout-facing style data structures and computed layout-style helpers.

## Edit Rules

- Keep this directory data-centric. Algorithmic interpretation should stay in the sibling Starlight layout layer.
- Many structs here are copied around as value objects. Be careful when adding fields or changing defaults.

## Common Regression Symptoms

- Layout behavior regresses without parser changes because style data no longer matches layout expectations.
- Incremental or copied style state diverges after changing data ownership or initialization behavior.

## Validate

Use `lynx-cpp-test` and start with:

- `starlight_unittest_exec`
