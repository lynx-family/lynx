# AGENTS.md

## Scope

This directory contains the low-level NG CSS parser implementation: tokenization, token streams, parser idioms, number parsing, and string-to-number helpers.

## Edit Rules

- Keep lexical/token-stream concerns here; selector construction belongs in `../selector/`.
- Tokenizer and numeric parsing changes are broad-impact and can break many selector/property paths at once.

## Common Regression Symptoms

- Many unrelated selectors or numeric values start failing after tokenizer changes.
- Parsing passes basic inputs but fails edge cases around number formats or token boundaries.

## Validate

Use `lynx-cpp-test` and start with:

- `css_test_exec`
