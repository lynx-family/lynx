# AGENTS.md

## Scope

This directory contains the classic Lepus runtime stack: parser/scanner, context/value model, bytecode generation, builtins/APIs, and execution helpers.

## Module Map

- Root files define Lepus runtime objects such as context, parser, scanner, values, builtin APIs, binary readers/writers, and runtime helpers.
- `bindings/`: renderer/module/event/resource bridges for Lepus.
- `compiler/`: compiler-specific test entry and encoding helpers.
- `tasks/`: task helpers around Lepus execution.

## Edit Rules

- Keep language/runtime semantics here and binding glue in `bindings/`.
- Parser, scanner, bytecode, and context changes are high-risk because they affect both compilation and execution paths.

## Common Regression Symptoms

- Lepus code parses but executes incorrectly after parser or context changes.
- Runtime values serialize, decode, or compare incorrectly after value/binary helper changes.
- Bindings appear to work but specific events/modules/resources stop bridging correctly after a shared Lepus runtime change.

## Validate

Use `lynx-cpp-test` and start with:

- `lepus_unittests_exec`
- `task_unittests_exec` when task behavior changes
- `lepus_runtime_unittests_exec` or `response_handler_lepus_unittests_exec` when bindings are involved
