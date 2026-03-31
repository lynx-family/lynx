# AGENTS.md

## Scope

This directory contains the core template codec layer: shared codec constants, magic numbers, versioning, Lepus command glue, and the top-level encoder/decoder composition.

## Module Map

- Root files define wire-format constants, magic numbers, compile options, and Lepus command integration.
- `binary_decoder/`: binary template decoding.
- `binary_encoder/`: binary template encoding and repacking.
- `generator/`: source/template parsing and code generation helpers.

## Edit Rules

- Treat root codec files as wire-format contracts. Seemingly small changes can have broad backward-compatibility impact.
- Keep shared constants/versioning here and push concrete read/write behavior into encoder or decoder subdirectories.

## Common Regression Symptoms

- Encoders and decoders drift apart after changing shared constants or magic/version behavior.
- Template bundles decode on one side but fail or misread metadata on another.

## Validate

Use `lynx-cpp-test` and start with the nearest codec target:

- `binary_decoder_unittest_exec`
- `css_encoder_test_exec`
- `style_object_encoder_testset_exec`
