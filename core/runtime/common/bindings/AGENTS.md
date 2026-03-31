# AGENTS.md

## Scope

This directory contains cross-runtime binding contracts shared by multiple runtime backends: event context proxies, native module management, and resource-response promises.

## Module Map

- `event/`: shared message-event and context-proxy contracts.
- `modules/`: shared native module manager surface.
- `resource/`: shared response-promise/resource-response abstractions.

## Edit Rules

- Keep these contracts backend-neutral. Backend-specific wrappers belong in the JS binding layer or the Lepus binding layer.
- Changes to event/module/resource contracts can easily break multiple runtimes at once.

## Common Regression Symptoms

- JS and Lepus backends break the same way after a supposedly local binding change.
- Shared event context proxies or response promises drift from backend implementations and start failing only at integration time.

## Validate

Use `lynx-cpp-test` and start with:

- `runtime_common_unittests_exec`
- `resource_common_unittests_exec`

If native module management changed, also run:

- `runtime_tests_exec`
