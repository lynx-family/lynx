# AGENTS.md

## Scope

This directory contains binary template decoding: template readers, element readers, config decoding, helper generation templates, and parallel parse scheduling.

## Edit Rules

- Keep config decoding, template reading, and parallel parsing responsibilities separated.
- Generated config-decoder templates and YAML inputs are part of the decode contract; do not update one without the others.

## Common Regression Symptoms

- Templates decode but page config or lazy sections are wrong after reader/config changes.
- Parallel parsing introduces ordering or partial-read regressions after scheduler changes.

## Validate

Use `lynx-cpp-test` and start with:

- `binary_decoder_unittest_exec`
