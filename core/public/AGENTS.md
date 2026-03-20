# AGENTS.md

## Scope

This directory contains stable core-facing interface headers shared across shell, renderer, runtime, resource loading, list handling, devtool integration, and native module bindings.

## Module Map

- `lynx_engine_proxy.h`, `lynx_layout_proxy.h`, `lynx_runtime_proxy.h`: shell/runtime/layout proxy interfaces.
- `list_container_proxy.h`, `list_engine_proxy.h`, `list_data.h`: list-facing contracts shared with shell and renderer.
- `lynx_resource_loader.h`: public resource-loading contract.
- `perf_controller_proxy.h`, `runtime_lifecycle_observer.h`, `vsync_*`: shared lifecycle and performance-facing interfaces.
- `jsb/`: native module and extension module interfaces.
- `devtool/`: devtool proxy and inspector ownership headers.

## Edit Rules

- Treat these headers as shared API surface. Signature, ownership, or type changes usually require coordinated updates in shell, runtime, resource-loading, and platform implementations.
- Keep this directory header-only and dependency-light. Do not move implementation logic here.
- Prefer additive changes over breaking changes when possible.

## Common Regression Symptoms

- Build failures across multiple modules after a seemingly small header edit usually mean a shared contract changed here.
- Proxy calls compile but break at runtime when ownership, callback lifetime, or thread assumptions changed without updating implementations.
- Native module, resource loader, or list integration regressions often originate from a contract mismatch in this directory.

## Validate

This directory does not define its own unit-test exec. Validate through the owning implementation modules:

- `shell_unittests_exec` for proxy or lifecycle contracts
- `list_container_testset_exec` for list-facing contracts
- `lazy_bundle_test_exec` or runtime resource tests for resource-loader contracts
- `runtime_tests_exec` or module-binding tests for `jsb/` contracts
