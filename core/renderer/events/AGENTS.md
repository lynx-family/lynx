# AGENTS.md

## Scope

This directory contains renderer-side event integration helpers such as touch-event handling, closure listeners, and renderer-facing event wiring.

## Edit Rules

- Keep renderer-specific event adaptation here; shared event semantics belong in the core event layer rather than this renderer bridge.
- Touch and gesture behavior often depends on DOM or platform integration assumptions. Check ownership and propagation expectations before making local fixes.

## Common Regression Symptoms

- Events fire in the renderer but never reach the right DOM consumer.
- Touch handling works for simple taps but fails for gesture or closure-listener flows.

## Validate

Use `lynx-cpp-test` and start with:

- `events_test_exec`

If shared event semantics were touched, also run `event_unittests_exec`.
