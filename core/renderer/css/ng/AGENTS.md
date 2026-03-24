# AGENTS.md

## Scope

This directory contains the next-generation CSS infrastructure: tokenization, selector parsing/matching, invalidation sets, and rule-set construction.

## Module Map

- `parser/`: tokenizer, token stream, numeric parsing, and low-level parser helpers.
- `selector/`: selector parsing and selector data structures.
- `matcher/`: selector matching logic.
- `invalidation/`: style invalidation sets and rule invalidation logic.
- `style/`: rule-set containers and style-rule data.

## Edit Rules

- Keep tokenizer concerns in `parser/`, selector tree construction in `selector/`, and invalidation behavior in `invalidation/`.
- Invalidation and selector matching changes often look local but affect broad style recomputation behavior.
- Do not couple this directory to DOM mutation or layout code directly; it should stay a CSS engine layer.

## Common Regression Symptoms

- Selectors parse but never match, or match too broadly, after selector or matcher changes.
- Style recomputation gets stuck or becomes too expensive after invalidation-set changes.
- Tokenizer changes break unrelated selectors or number parsing in surprising ways.

## Validate

Use `lynx-cpp-test` and start with:

- `css_test_exec`

If tokenizer or selector parser behavior changed, make sure the NG parser and selector tests in this subtree remain covered by that target.
