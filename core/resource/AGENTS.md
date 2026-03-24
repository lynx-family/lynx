# AGENTS.md

## Scope

This directory contains resource-loading infrastructure for Lynx, including generic resource-loader entry points, platform-specific resource-loader implementations, and lazy-bundle loading support.

## Module Map

- `lynx_resource_loader*`: shared resource-loader entry points and platform implementations.
- `external_resource/`: external resource loader helpers.
- `lazy_bundle/`: lazy-bundle request, lifecycle, loader, and utility logic.
- `trace/`: resource trace-event definitions.

## Key Files And Types

- `lynx_resource_loader.cc`: shared resource-loader entry point used by platform-specific implementations.
- `lynx_resource_loader_android.*`, `lynx_resource_loader_darwin.*`, `lynx_resource_loader_harmony.*`: platform-specific loading and reporting behavior.
- `lazy_bundle/lazy_bundle_loader.*`: lazy-bundle orchestration.
- `lazy_bundle/lazy_bundle_lifecycle_option.*`: lazy-bundle lifecycle behavior controls.

## Edit Rules

- Keep shared resource semantics in platform-agnostic files and platform-specific behavior in the corresponding platform files.
- Lazy-bundle logic is more than transport; be careful with lifecycle, request deduplication, and bundle state transitions.
- Resource loader changes often affect runtime resource bindings or renderer-side bundle consumption even if the touched file is local.

## Common Regression Symptoms

- Bundles never load, load twice, or resolve in the wrong lifecycle state after lazy-bundle changes.
- Only one platform regresses after a shared loader change when platform-specific fallbacks are bypassed incorrectly.
- Resource timing or reporting becomes inconsistent when loader and tracing/reporting helpers drift apart.

## Validate

For C++ unit tests here, prefer the `lynx-cpp-test` skill and start with:

- `lazy_bundle_test_exec`

If you changed shared resource request/response flow, also consider the nearest runtime resource-binding tests in the shared runtime bindings, JS runtime bindings, or Lepus runtime bindings.
