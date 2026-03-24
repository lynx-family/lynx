# AGENTS.md

## Scope

This directory contains the renderer UI-wrapper aggregation layer, which composes shared prop-bundle helpers, layout wrappers, and painting wrappers.

## Module Map

- `common/`: shared prop-bundle and native-prop helpers.
- `layout/`: layout context and native layout-node wrappers.
- `painting/`: painting context, catalyzer, and platform renderer wrappers.

## Edit Rules

- Keep wrapper contracts thin and focused on bridging renderer data into platform-facing wrapper types.
- Parent `ui_wrapper` should remain a composition layer; concrete logic belongs in `common/`, `layout/`, or `painting/`.

## Common Regression Symptoms

- Layout and painting both regress after shared wrapper contract changes.
- Platform integration compiles but receives incomplete prop bundles or wrapper state.

## Validate

There is no standalone top-level exec here today. Validate through the nearest renderer consumer tests, especially DOM, list, or fragment coverage depending on the touched wrapper path.
