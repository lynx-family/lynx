# AGENTS.md

## Scope

This directory currently contains a very small parser utility layer centered on `InputStream`.

## Module Map

- `input_stream.h`: public parser helper interface.
- `input_stream.cc`: implementation of the shared stream-reading helper.
- `BUILD.gn`: target wiring for parser consumers.

## Key Files And Types

- `InputStream` is the only real abstraction here. Keep it generic enough for multiple parser consumers instead of specializing it for one parser pipeline.
- `BUILD.gn` is small, but it still defines the contract surface that parser consumers pull in.

## Typical Change Patterns

- If the change is about generic stream traversal, token reading, or parser helper behavior shared by multiple consumers, this directory is the right place.
- If the change is really CSS-, template-, or domain-specific parsing policy, keep that logic in the owning parser module instead of broadening `InputStream`.

## Edit Rules

- Keep this directory generic and dependency-light. It should remain usable by multiple parser consumers.
- Do not mix runtime, renderer, or CSS-specific behavior into `InputStream`.
- If parser behavior becomes domain-specific, the owning module should usually grow its own helper instead of expanding this directory indiscriminately.

## Common Regression Symptoms

- Multiple parser consumers regress at once after what looked like a tiny `InputStream` fix.
- Parser helpers become harder to reuse because one consumer's assumptions leaked into the generic stream abstraction.

## Validate

There is no dedicated unit-test exec in this directory today. If you change parser behavior here, validate through the nearest consumer tests and consider adding local coverage if the helper grows more complex.

## Notes

- This directory should stay small on purpose. Growth here should mean "more generic parser utility," not "miscellaneous parser logic."
