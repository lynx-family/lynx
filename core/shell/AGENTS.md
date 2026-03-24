# AGENTS.md

## Scope

This directory contains Lynx's multi-threaded shell layer: actor ownership, shell lifecycle, cross-thread mediators, proxy interfaces, operation queues, BTS runtime glue, and platform-specific native-facade bridges.

## Module Map

- `lynx_shell.*`, `lynx_shell_builder.*`: top-level shell lifecycle and construction.
- `lynx_engine.*`, `tasm_mediator.*`, `layout_mediator.*`: actor-owned engine and mediator layers.
- `lynx_*_proxy_impl.*`, `native_facade*`: proxy and platform-bridge implementations.
- `*_operation_queue*`, `dynamic_ui_operation_queue.*`: cross-thread queueing and UI/TASM operation ordering.
- `runtime/bts/`: BTS runtime coordination.
- `android/`, `ios/`, `harmony/`: platform-specific shell and facade bridges.
- `testing/`: mock shell delegates and shell unit-test setup.

## Key Files And Types

- `lynx_shell.*`: main shell entry point coordinating actors and platform access.
- `lynx_engine.*`: TASM-side actor ownership surface.
- `tasm_mediator.*` and `layout_mediator.*`: cross-thread mediator layers between actors.
- `native_facade.h`: shell-facing platform abstraction.
- `vsync_observer_impl.*`: shell-side frame-tick observer bridge.

## Edit Rules

- Follow the actor ownership model described in `README.md`: let a `LynxActor` own lifecycle and use `Act()` or delegate/proxy paths for cross-thread communication.
- Prefer stack ownership or `std::unique_ptr` for actor-owned objects; avoid introducing `std::shared_ptr`-style lifecycle management in shell flows.
- Keep platform-specific behavior in `android/`, `ios/`, or `harmony/` instead of branching through shared shell code.
- Queueing and mediator changes often affect ordering guarantees. Inspect both producer and consumer sides before making "local" fixes.

## Common Regression Symptoms

- Work executes on the wrong thread, arrives too early, or never arrives after queue/mediator changes.
- Shell construction or teardown leaks, double-destroys, or races after ownership changes.
- Platform-only regressions often come from proxy/facade drift between shared shell code and platform-specific bridge files.

## Validate

For C++ unit tests here, prefer the `lynx-cpp-test` skill and use targets declared in `testing/BUILD.gn`.

Start with:

- `shell_unittests_exec`

If you changed shell helpers used by the shared-data layer, the renderer layer, or list proxies, consider those consumer tests as well.
