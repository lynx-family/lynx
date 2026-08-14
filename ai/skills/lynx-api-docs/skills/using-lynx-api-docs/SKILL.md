---
name: using-lynx-api-docs
description: Routes Lynx UI authoring, review, Web migration, and styling-debug tasks to engine references for CSS, layout, elements, and installed configuration topics. Applies to Lynx TTML, TSX, styles, and rendering behavior; not to unrelated native engine work.
---

# Using Lynx API Docs

Use retrieval-led reasoning for Lynx UI work. Web knowledge is useful for
comparison, but it is not evidence that Lynx parses, implements, or paints a
feature the same way.

## Retrieval workflow

1. Resolve all documentation paths relative to the directory containing this
   file.
2. Treat this installed skill tree as one coherent documentation baseline. Do
   not mix it with references copied from another package release.
3. Determine the target platform and page compatibility mode when they can
   affect the answer.
4. Search `topics.jsonl` using an exact property, element, symptom, or task
   keyword. If several topics match, prioritize the task operation or symptom
   over an incidental property name, then read only the best match first.
5. Read the matched canonical document. Load a related document only when the
   first reference identifies a dependency or remaining uncertainty.
6. Evaluate the retrieved claim against the package baseline, target platform,
   and compatibility mode. Keep parser acceptance, computed-style
   serialization, layout behavior, and native painting as separate questions;
   evidence for one is not evidence for the others.
7. If a claim is not indexed, search the installed references and then verify
   it against the target engine source. Do not fill the gap from browser
   behavior alone.
8. Apply only the evaluated guidance, and state any unresolved platform or mode
   uncertainty in the answer.

## Progressive-disclosure rules

- Do not load every document in a category.
- Prefer an exact element document for element APIs and an exact compatibility
  topic for CSS behavior.
- Separate authoring guidance from implementation evidence in the answer.
- When the target package baseline, platform, or compatibility mode is unknown,
  state the uncertainty instead of silently combining references from different
  package releases.

## Routing files

- `INDEX.md`: generated topic overview for humans and agents
- `topics.jsonl`: canonical machine-readable routing and applicability data
- `css/supported-properties.md`: property registry, not deep behavior
- `elements/<name>.md`: element-specific APIs and guardrails
