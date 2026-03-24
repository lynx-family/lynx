# AGENTS.md

## Scope

This directory contains Android-specific React/TASM integration details, including mapbuffer-related support.

## Edit Rules

- Keep Android-specific integration here; shared React/TASM contracts belong in the parent directory.
- Mapbuffer and related Android bridge behavior are easy to break with local data-shape changes.

## Common Regression Symptoms

- Android React integration regresses while other TASM paths continue to work.
- Mapbuffer-backed data transport breaks after local schema or wrapper changes.

## Validate

Use `lynx-cpp-test` and start with:

- `mapbuffer_unittests_exec`
