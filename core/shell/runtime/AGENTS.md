# AGENTS.md

## Scope

This directory groups shell-side runtime integration submodules such as `bts/` and other runtime bridge helpers used by the shell layer.

## Edit Rules

- Use this directory to choose the correct shell-runtime subtree, then follow the nearest concrete implementation files or child guidance.
- Shared shell actor and lifecycle behavior still belongs in the parent shell layer.

## Common Regression Symptoms

- Shell/runtime integration regresses while generic shell queueing still works.
- BTS or runtime bridge behavior drifts from the surrounding shell actor contracts.

## Validate

Use the parent shell guidance and start with:

- `shell_unittests_exec`
