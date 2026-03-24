# AGENTS.md

## Scope

This directory contains the LepusNG runtime layer: QuickJS-backed context helpers, callback glue, debug info, and child directories for bindings, compiler pieces, and NAPI integration.

## Edit Rules

- Keep LepusNG-specific runtime behavior here; shared JS or Lepus contracts should remain in their owning layers.
- QuickJS-backed LepusNG helpers and callback plumbing are closely coupled. Check both when changing runtime surfaces.

## Common Regression Symptoms

- LepusNG-specific flows regress while classic Lepus and generic JS runtime still work.
- Promise/callback behavior drifts after QuickJS helper or callback changes.

## Validate

There is no dedicated top-level exec in this directory. Validate through:

- `runtime_tests_exec`

If profiling or NAPI integration is involved, also inspect the nearest profiling or NAPI coverage in the runtime subtree.
