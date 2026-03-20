# AGENTS.md

## Scope

This directory contains core inspector-facing interfaces: runtime inspector management, inspector observers, console-message plumbing, and stylesheet-facing inspection data.

## Module Map

- `runtime_inspector_manager.h`: top-level runtime inspection coordination.
- `observer/`: observer interfaces for common, element, Lepus, and runtime inspection hooks.
- `console_message_postman.h`: console-message forwarding contract.
- `style_sheet.h`: stylesheet-facing inspector data surface.

## Edit Rules

- Keep this directory as an interface and coordination layer. Heavy renderer or runtime logic should stay in the owning module.
- Observer interfaces here are shared contracts. Be careful when changing method shapes or event ordering.
- If a change is only about a concrete runtime implementation, prefer editing the implementation side outside this directory and keep the inspector interface stable.

## Common Regression Symptoms

- Inspector sessions connect but never receive updates when observer contracts drift.
- Console messages stop showing up or duplicate after changes to message-posting interfaces.
- Runtime inspection works for one backend but not another when shared inspector contracts become runtime-specific.

## Validate

For C++ unit tests here, prefer the `lynx-cpp-test` skill and start with:

- `inspector_test_exec`

If you changed runtime-facing contracts, also consider the owning runtime tests in the runtime layer.
