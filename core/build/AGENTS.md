# AGENTS.md

## Scope

This directory is a very small build-support layer for generated core build artifacts. Today it mainly exposes generated sub-error-code sources through the `build` target.

## Key Files And Boundaries

- `BUILD.gn`: the only meaningful ownership file here. Treat it as target wiring for generated build artifacts rather than as a place for engine behavior.
- Generated sub-error-code sources that flow through this target are downstream artifacts. If their contents are wrong, the source definition or generator is usually the real owner.

## Typical Change Patterns

- If you are only exposing or wiring generated sources into a consumer target, this directory is the right place.
- If you are changing how error codes are defined, named, or generated, stop here and inspect the upstream generator or definition source instead of patching the generated build layer.

## Edit Rules

- Treat files here as generated or generation-adjacent build artifacts, not hand-authored business logic.
- If a change is really about error-code definitions or generation rules, the owning generator or upstream definition is usually the better place to edit.
- Avoid introducing unrelated core logic into this directory.

## Common Regression Symptoms

- Build wiring succeeds locally but generated error-code symbols disappear from downstream targets.
- A build-only edit appears to fix one consumer while silently disconnecting another shared consumer of the generated artifact.

## Validate

This directory does not define a standalone unit-test exec. Validate through the nearest consumer build path if you change generated artifact wiring.

## Notes

- A correct change here is usually small. Large edits in this directory are a signal that the real ownership may be elsewhere.
