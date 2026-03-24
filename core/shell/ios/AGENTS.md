# AGENTS.md

## Scope

This directory contains Darwin/iOS-specific shell bridges such as iOS proxies, runtime facade, native facade, data utilities, and TASM platform invoker glue.

## Edit Rules

- Keep Darwin/iOS bridge behavior here; shared shell semantics stay in the parent shell layer.
- Proxy/facade changes can affect lifecycle, threading, and Objective-C++ bridge behavior together.

## Common Regression Symptoms

- iOS-only shell regressions appear after facade or proxy changes while other platforms remain healthy.
- Data marshaling or lifecycle callbacks drift between shared shell code and Darwin bridge code.

## Validate

Use `lynx-cpp-test` and start with:

- `shell_unittests_exec`
