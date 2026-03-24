# AGENTS.md

## Scope

This directory contains renderer worklet integration for Lepus-backed components, elements, gestures, RAF handling, and related worklet runtime glue.

## Edit Rules

- Keep worklet bridge behavior here; generic Lepus runtime semantics belong in the runtime layer.
- Gesture, frame-callback, and element bindings often move together. Check the full worklet surface before making narrow fixes.

## Common Regression Symptoms

- Worklet callbacks stop firing, fire on the wrong frame, or expose stale element state after bridge changes.
- Gesture or component worklets regress while the main runtime still works.

## Validate

Use `lynx-cpp-test` and start with:

- `worklet_unittests_exec`
