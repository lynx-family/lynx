# @lynx-js/lynx-api-docs Changelog

All notable changes to the public Lynx API documentation bundle.

## 0.3.9 — 2026-09-04

### Documentation

- Documented the verified single-layer `background` shorthand forms for an
  image, a color and image in either order, and an inert repeat token followed
  by a color when no image is present.
- Kept other shorthand combinations subject to per-component verification and
  preserved the existing attachment and page-background migration boundaries.

## 0.3.8 — 2026-07-16

### Features

- Published the initial functional public documentation bundle.
- Added an installable `using-lynx-api-docs` agent skill with CSS, layout, element, migration, pattern, example, and best-practice references.
- Added project-local skill linking for Claude Code, Codex CLI, Trae, and custom skill directories.

### Packaging

- Added public package metadata, documentation indexes, README, changelog, and Apache-2.0 license contents.
- Excluded configuration metadata and element references that are outside the public Lynx surface.
