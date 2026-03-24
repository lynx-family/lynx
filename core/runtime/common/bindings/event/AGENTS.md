# AGENTS.md

## Scope

This directory contains shared runtime event-bridge contracts such as `ContextProxy`, message events, and runtime event constants.

## Edit Rules

- Keep these event contracts backend-neutral so both JS and Lepus bindings can reuse them.
- Changes to message-event payload or context-proxy semantics often break multiple runtimes at once.

## Common Regression Symptoms

- Event bridges regress in both JS and Lepus after a shared context/message change.
- Event payloads compile but lose fields or semantics during cross-runtime handoff.

## Validate

Use `lynx-cpp-test` and start with:

- `runtime_common_unittests_exec`
