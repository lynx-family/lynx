# AGENTS.md

## Scope

This directory contains the simple-style-object representation and decoding helpers used by renderer styling flows.

## Edit Rules

- Keep this directory focused on style-object representation and decoding, not full CSS parsing or layout semantics.
- Because this layer bridges style data into runtime/renderer consumers, be careful with field ordering and default values.

## Common Regression Symptoms

- Style objects decode but lose fields or defaults after parser changes.
- Consumers read style objects successfully but apply visibly wrong style because representation drifted.

## Validate

Use `lynx-cpp-test` and start with:

- `style_object_unittest_exec`
