# AGENTS.md

## Scope

This directory contains Android-specific shell bridges such as Android proxies, Android native facade, Android runtime wrapper, Android layout proxy, and TASM platform invoker glue.

## Edit Rules

- Keep Android bridge behavior here rather than branching through shared shell code.
- Proxy and facade changes often need to remain aligned with Java/JNI-facing expectations and shared shell contracts.

## Common Regression Symptoms

- Android-only shell regressions appear after proxy/facade changes while iOS/Harmony still work.
- TASM or runtime calls are issued on the right shared path but fail at the Android bridge layer.

## Validate

Use `lynx-cpp-test` and start with:

- `shell_unittests_exec`
