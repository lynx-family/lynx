# @lynx-js/lynx-api-docs Changelog

All notable changes to the public Lynx API documentation bundle.

## 4.3.0 — 2026-08-14

### Documentation

- Updated the documentation baseline to Lynx 4.3.0.
- Verified the open CSS WPT reports against current parser, computed-style,
  layout, native text, background, border, and stacking-context code.
- Documented the supported `filter` subset and the absence of
  `backdrop-filter`, `word-spacing`, `overline`,
  `text-decoration-line`, and `text-decoration-style`.
- Documented the computed-style CSS-text getter allowlist, native
  text-decoration geometry and propagation differences,
  `transform: none` stacking behavior, standard background origin/clip sizing,
  and thick patterned-border geometry.
- Recorded that negative `flex-shrink` longhands are rejected in current code,
  while deferred raw style maps can still lose an earlier valid declaration.
- Corrected cross-file contradictions for 3D transform functions and
  text-decoration longhands.
- Split CSS compatibility details into focused canonical topics and replaced
  duplicated entry-point facts with links.
- Added a machine-readable topic manifest and a generated
  progressive-disclosure index while keeping package version as the only
  documentation baseline selector.
- Added package-maintenance routing and critical-claim evaluations for focused topic
  retrieval, including the background-origin migration diagnosis.

### Packaging

- Install one canonical documentation tree and link agent skill directories to
  that tree instead of copying the full corpus into multiple locations.
- Discover the canonical tree through project-local skill links without
  creating or modifying a project `AGENTS.md`.
- Keep human installation guidance in the package README and remove redundant
  `AGENTS.md`, README, and quick-reference entry points from the installed
  skill tree.

## 0.3.8 — 2026-07-16

### Features

- Published the initial functional public documentation bundle.
- Added an installable `using-lynx-api-docs` agent skill with CSS, layout, element, migration, pattern, example, and best-practice references.
- Added project-local skill linking for Claude Code, Codex CLI, Trae, and custom skill directories.

### Packaging

- Added public package metadata, documentation indexes, README, changelog, and Apache-2.0 license contents.
- Excluded configuration metadata and element references that are outside the public Lynx surface.
