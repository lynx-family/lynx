# AGENTS.md

## Scope

This directory contains style-object parsing and encoding for template bundle generation.

## Edit Rules

- Keep style-object parsing concerns here rather than mixing them into generic binary-writer code.
- This directory sits between style representation and codec output, so small shape changes can ripple widely.

## Common Regression Symptoms

- Style objects encode without crashing but decode with missing or out-of-order fields.
- Parser behavior diverges from style-object expectations after local schema changes.

## Validate

Use `lynx-cpp-test` and start with:

- `style_object_encoder_testset_exec`
