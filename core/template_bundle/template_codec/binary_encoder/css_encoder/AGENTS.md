# AGENTS.md

## Scope

This directory contains CSS encoding for template bundles: CSS parser/token encoding, font-face and keyframe token encoding, and shared CSS fragment encoding.

## Edit Rules

- Keep encoding-specific logic here; general CSS parsing and model behavior still belongs in the renderer CSS layer.
- Changes to token encoding should be reviewed for decoder compatibility, not just local encode output.

## Common Regression Symptoms

- CSS parses correctly upstream but encoded bundles lose keyframes, font faces, or fragment data.
- Decoder and encoder disagree about token layout after local parser/token changes.

## Validate

Use `lynx-cpp-test` and start with:

- `css_encoder_test_exec`
