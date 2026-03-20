# AGENTS.md

## Scope

This directory contains exported standalone headers for `starlight_standalone`.

## Module Map

- `starlight_standalone/starlight.h`: umbrella include for standalone consumers.
- `starlight_standalone/starlight_config.h`: standalone configuration-facing definitions.
- `starlight_standalone/starlight_enums.h`: exported enum surface used by standalone callers.
- `starlight_standalone/starlight_standalone.h`: higher-level standalone entry header.
- `starlight_standalone/starlight_value.h`: exported value/container-facing standalone types.

## Key Files And Types

- These headers are exported surface, not internal convenience wrappers. Changing one can affect downstream standalone consumers that are not otherwise visible from `lynx/core`.
- `starlight.h` and `starlight_standalone.h` are aggregation points. Dependency creep or include-order surprises often surface here first.

## Typical Change Patterns

- If a change is about standalone-facing type shape, config names, or exported enums, this directory is the right surface.
- If a change only matters to one implementation module, prefer editing that module and keep these headers as thin contracts.

## Edit Rules

- Treat these headers as public include surface. Keep them stable and dependency-light.
- Header changes here can affect external or standalone consumers even when internal core code still compiles.
- If a change is implementation-specific rather than interface-specific, prefer editing the owning standalone implementation module or other source directories instead.

## Common Regression Symptoms

- Standalone builds compile differently from in-tree core builds after a header-only change here.
- Enums or value types still compile but downstream callers interpret them differently because the exported contract drifted.

## Validate

This directory does not define standalone unit tests. Validate through the owning standalone/starlight consumers when header contracts change.

## Notes

- Favor additive edits and dependency minimization here. This directory is a contract surface, not an implementation layer.
