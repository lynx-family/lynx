# AGENTS.md

## Scope

This directory contains runtime profiling infrastructure and per-engine profiling support for QuickJS, V8, and LepusNG.

## Edit Rules

- Keep profiling observational. Avoid changing runtime behavior while touching profiler code unless that is the explicit goal.
- Cross-engine profiling APIs should stay consistent even when backend implementations differ.

## Common Regression Symptoms

- Profiling output disappears or misattributes runtime work after manager changes.
- One engine reports profiling data differently from others after backend-specific edits.

## Validate

Use `lynx-cpp-test` and start with:

- `profile_unittests_exec`
- `quickjs_profile_unittests_exec`
- `v8_profile_unittests_exec`
- `lepusng_profile_unittests_exec`
