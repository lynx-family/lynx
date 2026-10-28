# @lynx-js/lynx-api-docs Changelog

All notable changes to the public Lynx API documentation bundle.

## 0.3.10 — 2026-11-03

### Documentation

- Prioritize the CSS differences index for CSS authoring, editing, Web migration, and style debugging. Split the reference into seven topic guides while preserving existing constraints and examples.
- Route squeezed or wrapped flex text and numbers directly to the flex-shrinking guidance, including explicit minimum sizes and `flex-shrink` controls.
- Keep visual repairs focused on the diagnosed defect, preserve unrelated code and content, and honor complete-files-only output requests without retrieval notes.
- Correct the supported transform list to exclude `scale3d()` and `rotate3d()`. Clarify that explicit `z-index` values create stacking contexts without `position`, and that fixed positioning accepts definite start or end insets.
- Update README, quick-reference, and migration links to use the CSS differences index and its topic guides.

### Validation

- Check CSS differences topic completeness and local Markdown links and anchors in the public package tests.

## 0.3.9 — 2026-09-21

### Documentation

- Correct attribute-selector support for CSS styling, distinguish it from DOM query matching, and recommend explicit state classes.
- Document the configuration and target SDK conditions for an omitted flex basis, and show explicit shorthand alternatives.
- Distinguish the initial `box-sizing: auto` value from its usual border-box sizing behavior across references and summaries.

## 0.3.8 — 2026-07-16

### Features

- Published the initial functional public documentation bundle.
- Added an installable `using-lynx-api-docs` agent skill with CSS, layout, element, migration, pattern, example, and best-practice references.
- Added project-local skill linking for Claude Code, Codex CLI, Trae, and custom skill directories.

### Packaging

- Added public package metadata, documentation indexes, README, changelog, and Apache-2.0 license contents.
- Excluded configuration metadata and element references that are outside the public Lynx surface.
