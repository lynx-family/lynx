# AGENTS.md

## Scope

This directory contains Lynx's shared-data white-board layer used to exchange structured state between runtime, TASM/renderer, and shell-side delegates.

## Module Map

- `lynx_white_board.*`: the underlying white-board data structure.
- `white_board_delegate.*`: shared delegate-facing adapter.
- `white_board_runtime_delegate.*`: runtime-facing white-board bridge.
- `white_board_tasm_delegate.*`: TASM/renderer-facing white-board bridge.
- `white_board_inspector.h`: inspector-facing white-board contract.

## Key Files And Types

- `lynx_white_board.*`: owns the shared key/value store, listener maps, storage-type separation, and inspector access surface.
- `WhiteBoardStorageType`: distinguishes Lepus, JS, and client-facing listener/storage channels. Changes here affect subscription routing rather than just storage layout.
- `WhiteBoardListener`: packages trigger and removal callbacks together. Subscription and unsubscribe semantics need to stay aligned with this shape.
- `shared_data_trace_event_def.h`: trace labels for shared-data operations. Keep tracing changes aligned with the actual mutation and callback paths.
- `white_board_delegate.*`: common delegate facade that turns white-board updates into Lepus callbacks, JS API callbacks, platform callbacks, and event-listener wiring.
- `white_board_runtime_delegate.*`: runtime-facing bridge that needs both runtime actor wiring and runtime-facade actor wiring.
- `white_board_tasm_delegate.*`: TASM-facing bridge that routes updates back through `TemplateAssembler`.
- `white_board_inspector.h`: inspector-facing read surface for white-board contents. Debug-tool changes should usually stay here instead of leaking into mutation logic.

## Adjacent Layers

- Runtime-side event hookup enters through `runtime/common/bindings/event/context_proxy.h` and the runtime delegate. Bugs that look like "JS subscription did nothing" often sit on that boundary.
- Renderer and TASM consumption flows back through `TemplateAssembler`, so renderer-visible stale state can still be caused by delegate routing here.
- Platform callback delivery crosses into shell-facing callback holders, so callback-removal mismatches may surface as shell integration issues rather than white-board crashes.

## Typical Change Patterns

- If the change is about storage semantics, listener routing, or inspector visibility, start from `lynx_white_board.*`.
- If the change is about how JS callbacks, Lepus callbacks, or platform callbacks are invoked or removed, start from `white_board_delegate.*`.
- If the change is about runtime-thread ownership or actor handoff, inspect `white_board_runtime_delegate.*` together with the shell/runtime actors it talks to.
- If the change is about renderer or TASM-side consumption of shared state, inspect `white_board_tasm_delegate.*` before touching the underlying board implementation.

## Edit Rules

- Keep the white-board data structure generic and keep module-specific behavior inside the corresponding delegate layer.
- Shared-data changes are often cross-thread and cross-module by nature. Be careful with lifetime, mutation order, and ownership assumptions.
- If a change is only about runtime or renderer interpretation of shared data, prefer changing the corresponding delegate instead of the shared board itself.

## Invariants And Pitfalls

- `lynx_white_board.h` still documents the white board as TASM-thread-owned for normal operation. Do not assume the presence of internal locks means arbitrary cross-thread mutation is automatically safe.
- `white_board_delegate.*` spans three callback worlds at once: Lepus, JS, and platform callbacks. A subscription-path change should be reviewed together with the corresponding removal path.
- `AddEventListeners()` is a second-step initialization hook for runtime event wiring. Runtime-delegate changes can look correct locally but still fail if actor wiring and event wiring drift apart.
- `WhiteBoardRuntimeDelegate` needs both `runtime_actor_` and `runtime_facade_actor_` for its full bridge behavior. Wiring only one side often creates partial-success bugs that look like missing callbacks on one thread.
- `WhiteBoardStorageType` is routing metadata, not just storage tagging. Adding or reordering types affects listener lookup, callback delivery, and removal behavior together.

## Common Regression Symptoms

- Runtime and renderer observe different values or stale values when delegate wiring drifts.
- Shared state appears to update but consumers stop reacting when delegate callbacks no longer match the board mutation path.
- Integration tests fail outside this directory because shell, runtime, and renderer each depend on the same white-board contract.

## Validate

For C++ unit tests here, prefer the `lynx-cpp-test` skill and start with:

- `shared_data_test_exec`

This target already pulls renderer DOM, Lepus bindings, and shell testing dependencies, so it is the first place to validate cross-layer white-board behavior.

If you changed delegate contracts or shared-data threading behavior, also consider nearby shell and renderer integration tests.
If the change affects callback routing or runtime-facade actor wiring, also consider:

- `runtime_tests_exec`
- `shell_unittests_exec`

## Coverage Reality

- `shared_data_test_exec` gives useful coverage for white-board storage rules and basic delegate call paths, but much of the delegate coverage is smoke-test level rather than deep sequencing coverage.
- Subscription, unsubscription, and callback-removal bugs can still hide until runtime, renderer, or shell integration paths exercise the full actor wiring.
- If your change touches `AddEventListeners()`, actor setup, or callback removal behavior, assume local tests are necessary but not sufficient.
