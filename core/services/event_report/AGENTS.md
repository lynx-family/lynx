# AGENTS.md

## Scope

This directory contains event reporting and tracking infrastructure, including shared event tracker logic and platform-specific tracker implementations.

## Module Map

- `event_tracker.*`: shared tracker logic and event-reporting entry points.
- `event_tracker_platform_impl.*`: generic platform integration surface.
- `event_tracker_nodejs.cc`: NodeJS-specific tracking bridge.
- `android/`, `darwin/`, `embedder/`, `harmony/`: platform-specific tracker implementations.

## Key Files And Types

- `event_tracker.*` is the shared behavior contract that platform implementations should follow.
- `event_tracker_platform_impl.*` is the shared adapter surface; platform subtrees should extend it rather than redefining reporting semantics.
- Platform implementations can diverge in transport details, but not in payload shape or ordering expectations.

## Typical Change Patterns

- If the issue is shared event shape, ordering, or tracker semantics, start from `event_tracker.*`.
- If the issue reproduces only on one platform, inspect that platform's `event_tracker_platform_impl` first.
- If NodeJS-only reporting drifts, inspect `event_tracker_nodejs.cc` without broadening the shared tracker contract unnecessarily.

## Edit Rules

- Keep shared event-tracking semantics in the root files and platform-specific reporting details in platform child directories.
- Event reporting code often looks observational, but ordering and payload shape changes can break downstream analytics flows.

## Invariants And Pitfalls

- Shared tracker changes should preserve platform adapter expectations instead of forcing every backend to reinterpret events.
- Reporting code often fails semantically rather than crashing; payload drift is still a regression.

## Common Regression Symptoms

- Events are emitted with missing fields, wrong ordering, or only fail on one platform after local tracker changes.
- NodeJS or platform-specific trackers drift from the shared tracker contract.

## Validate

There is no standalone exec declared in this directory. Validate through the nearest performance/event-reporting consumers.

## Notes

- Darwin already carries platform-specific unit coverage, but many regressions here still need end-to-end event consumers to confirm payload meaning.
