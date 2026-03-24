# AGENTS.md

## Scope

This directory contains Harmony-specific core-base glue such as Harmony function loading, NAPI environment holding, Harmony property conversion helpers, and Harmony VSync support.

## Module Map

- `harmony_function_loader.*`: Harmony dynamic entry/function loading.
- `harmony_napi_env_holder.*`, `napi_convert_helper.*`, `props_constant.h`: Harmony NAPI environment and property conversion helpers.
- `threading/js_thread_config_getter_harmony.cc`: Harmony-specific JS-thread configuration override.
- `vsync_monitor_harmony.*`: Harmony-specific frame scheduling bridge.

## Key Files And Types

- `harmony_function_loader.*` and `harmony_napi_env_holder.*` are central to platform bring-up and callback plumbing.
- `js_thread_config_getter_harmony.cc` is where Harmony diverges from the shared JS-thread configuration path.
- `vsync_monitor_harmony.*` must stay aligned with the shared VSync contract used above this directory.

## Typical Change Patterns

- If the issue is Harmony-only startup or callback registration, start from the function loader and NAPI env holder.
- If the issue is Harmony-only thread naming or JS runner setup, inspect `js_thread_config_getter_harmony.cc` together with the parent threading layer.
- If the issue is frame cadence or frame callback delivery on Harmony, inspect `vsync_monitor_harmony.*`.

## Edit Rules

- Keep Harmony-specific loaders and NAPI helpers here rather than branching through shared base code.
- Function-loader, NAPI-env, and VSync behavior often interact in platform bring-up and lifecycle paths.

## Invariants And Pitfalls

- Harmony platform glue is sensitive to bring-up ordering; changes that look local can break callback registration or environment availability early in startup.
- Platform-specific VSync behavior should not diverge from the shared callback contract without a strong reason.

## Common Regression Symptoms

- Harmony-only startup or callback regressions appear after loader or NAPI helper changes.
- Harmony frame scheduling drifts from other platforms after local VSync edits.

## Validate

Use `lynx-cpp-test` and start with:

- `lynx_base_unittests_exec`

## Notes

- Local Harmony files are small in count but high in platform impact. Treat them as bring-up-critical adapters, not as miscellaneous helpers.
