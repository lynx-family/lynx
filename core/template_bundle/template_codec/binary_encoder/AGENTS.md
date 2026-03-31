# AGENTS.md

## Scope

This directory contains binary template encoding, repacking helpers, CSR element writing, and child encoders for CSS and style objects.

## Module Map

- Root files handle template binary writing, repacking, and encoder orchestration.
- `css_encoder/`: CSS token and shared CSS fragment encoding.
- `style_object_encoder/`: style-object parsing and encoding.

## Edit Rules

- Keep generic binary writing/repacking here and domain-specific encoding in the corresponding child directory.
- Encoder changes often need to remain in sync with decoder expectations and versioned codec contracts.

## Common Regression Symptoms

- Encoded output looks structurally valid but the decoder misreads fields after writer changes.
- Repack flows regress independently from full encode flows when helper logic drifts.

## Validate

Use `lynx-cpp-test` and start with:

- `css_encoder_test_exec`
- `style_object_encoder_testset_exec`

If the change affects shared binary writing or repacking, also run:

- `binary_decoder_unittest_exec`
