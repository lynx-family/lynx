# AGENTS.md

## Scope

This directory contains template-code generation helpers such as template/page parsers, meta factories, list parsing, scope handling, and TTML holder logic.

## Module Map

- `template_parser.*`, `template_page_parser.*`, `template_dynamic_component_parser.*`: parser entry points for template structures.
- `meta_factory.*`, `source_generator.*`: generation helpers that turn parsed structures into generated output.
- `list_parser.*`, `template_scope.*`: focused helpers for list and scope handling.
- `ttml_holder.*`, `base_struct.h`: supporting data/model helpers.

## Key Files And Types

- `template_parser.*` and `template_page_parser.*` shape the parsed representation that later generator stages consume.
- `source_generator.*` and `meta_factory.*` are where parser output becomes generated source semantics.
- `template_scope.*` and `list_parser.*` are smaller helpers, but they influence many downstream generated outputs.

## Typical Change Patterns

- If the issue is source generation output shape, inspect `source_generator.*` and `meta_factory.*`.
- If the issue is parser interpretation of page/template structure, inspect the relevant parser entry files first.
- If the issue is about low-level wire format rather than generated source meaning, move back to encoder/decoder ownership.

## Edit Rules

- Keep source-generation and parser-generation logic here; low-level wire-format encoding belongs in encoder/decoder layers.
- Parser and meta-factory changes often affect many generated outputs, so favor clear ownership boundaries.

## Invariants And Pitfalls

- Parser-side changes and generator-side changes form one pipeline. Patching one side in isolation can keep builds green while changing generated semantics.
- Scope and list parsing helpers are small, but they influence many generated artifacts indirectly.

## Common Regression Symptoms

- Generated source compiles but no longer matches template semantics after parser or scope changes.
- Dynamic component or page parsing regresses while raw encoding still looks correct.

## Validate

This directory does not define a standalone exec. Validate through the nearest template codec encoder/decoder tests and the owning generation workflow.

## Notes

- This subtree owns generated-source meaning, not binary transport details. When a bug only appears after encoding/decoding, verify the boundary before expanding generator behavior.
