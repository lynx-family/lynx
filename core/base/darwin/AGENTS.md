# AGENTS.md

## Scope

This directory contains Darwin-specific core-base glue such as Darwin environment helpers, trail hub integration, Darwin VSync/message-loop wrappers, and configuration helpers.

## Module Map

- `message_loop_darwin_vsync.*`, `vsync_monitor_darwin.*`: Darwin-specific frame scheduling integration.
- `lynx_env_darwin.*`, `config_darwin.*`: Darwin environment and configuration helpers.
- `lynx_trail_hub_impl_darwin.*`: Darwin-specific trail hub integration.

## Key Files And Types

- `vsync_monitor_darwin.*` and `message_loop_darwin_vsync.*` are the main bridge points between shared base scheduling and Darwin platform loops.
- `lynx_env_darwin.*` and `config_darwin.*` affect platform bring-up and configuration rather than core threading semantics directly.

## Typical Change Patterns

- If the issue is Darwin-only frame scheduling or message-loop behavior, inspect `message_loop_darwin_vsync.*` and `vsync_monitor_darwin.*` together.
- If the issue is startup environment, configuration, or platform service wiring, start from `lynx_env_darwin.*` or `config_darwin.*`.
- If the issue reproduces on multiple platforms, prefer checking the parent `base/` contracts before making Darwin-only fixes.

## Edit Rules

- Keep Darwin-specific environment and VSync bridge code here rather than in shared base files.
- Objective-C++ bridge behavior and frame scheduling are tightly coupled in this subtree.

## Invariants And Pitfalls

- Objective-C++ bridge files here are platform adapters. Avoid pushing shared scheduling policy into them.
- Darwin-only VSync behavior should remain compatible with shared `VSyncMonitor` assumptions used by higher layers.

## Common Regression Symptoms

- iOS/macOS-only lifecycle or VSync regressions appear after shared base changes.
- Environment or trail-hub behavior drifts only on Darwin because bridge files no longer match shared expectations.

## Validate

Use `lynx-cpp-test` and start with:

- `lynx_base_unittests_exec`

## Notes

- The shared base target is still the first test stop here, but some Darwin regressions only appear once Objective-C++ bridge code is exercised in platform flows.
