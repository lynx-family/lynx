# AGENTS.md

## Scope

This directory contains timing and timing-handler infrastructure: timing maps, timing info, timing mediator, and the NG timing-handler path.

## Module Map

- `timing.*`, `timing_info.*`, `timing_map.*`: core timing value objects and lookup/state helpers.
- `timing_handler.*`, `timing_handler_ng.*`: legacy and NG timing handler execution paths.
- `timing_mediator.*`: coordination layer between timing producers and consumers.
- `timing_constants.h`, `timing_constants_deprecated.h`: shared timing constant surfaces.
- `timing_utils.*`: helper utilities around timing data.

## Key Files And Types

- `timing_handler.*` and `timing_handler_ng.*` are the execution paths that consume the timing data objects.
- `timing_info.*` and `timing_map.*` define much of the data meaning that handlers and mediators rely on.
- `timing_mediator.*` is the bridge layer that keeps producer/consumer timing flow consistent.

## Typical Change Patterns

- If the issue is data shape or lookup/aggregation semantics, start from `timing_info.*`, `timing_map.*`, or `timing.*`.
- If the issue is emission timing or handler lifecycle, inspect `timing_handler.*` or `timing_handler_ng.*` together with `timing_mediator.*`.
- If the issue is legacy-vs-NG divergence, inspect both constant surfaces and both handler paths before deciding ownership.

## Edit Rules

- Keep timing value objects and timing mediation consistent; do not patch one side without checking the other.
- Deprecated and NG timing constants coexist here. Be explicit about which path your change targets.

## Invariants And Pitfalls

- Legacy and NG timing paths share concepts but not always the same implementation surface. Be explicit about which contract you are changing.
- Timing regressions often appear as wrong numbers or shifted windows rather than crashes or compile failures.

## Common Regression Symptoms

- Timing metrics exist but are offset, duplicated, or missing after handler or map changes.
- Legacy and NG timing paths diverge unexpectedly after changing shared constants or mediator logic.

## Validate

No standalone exec is defined directly here. Validate through the nearest performance or timing-consuming tests in the surrounding services layer and the renderer layer.

## Notes

- This subtree is one of the easiest places for "small constant fix" changes to create large reporting drift. Review semantics, not just names.
