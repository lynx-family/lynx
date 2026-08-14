# Lynx API Docs - Engine AI Context Bundle

This package installs an agent-oriented Lynx engine documentation set for CSS,
layout, elements, Web migration, patterns, and examples. It is designed for
retrieval by coding agents rather than bulk loading as a traditional API
manual.

The public package documents only the public Lynx surface.

## Versioning

The package version is the documentation baseline. Package 4.3.x describes the
Lynx 4.3.x engine; use the api-docs release selected for the target engine
instead of branching between engine versions inside one installed package.

## Install

```bash
npx @lynx-js/lynx-api-docs install
```

Options:

- `--project <path>`: target another project root
- `--dest <path>`: override the canonical documentation location
- `--dry-run`: preview changes without writing
- `--no-link`: skip additional Claude, Codex, and Trae skill links
- `--link-claude`, `--link-codex`, `--link-trae`: select agent-specific links
- `--link-all`: link all known project-local agent directories
- `--skills-dir <path>`: add a link in a custom skill directory

The canonical skill and all references are installed once at the destination.
The CLI creates the primary `.agents` skill link and by default links the same
canonical tree into supported agent-specific skill directories. It does not
create or modify a project `AGENTS.md`. If symbolic links are unavailable, the
CLI falls back to copying the skill.

## Agent entry points

- [SKILL.md](./skills/using-lynx-api-docs/SKILL.md) defines the retrieval workflow.
- [INDEX.md](./skills/using-lynx-api-docs/INDEX.md) is the generated human-readable topic index.
- [topics.jsonl](./skills/using-lynx-api-docs/topics.jsonl) is the canonical machine-readable routing manifest.

Package installation and maintenance information stays in this README. The
installed agent index contains only retrieval guidance and document routes.

## Development

Regenerate the topic index after changing the manifest:

```bash
node ./generate-index.js
```

Run the package-maintenance routing and critical-claim evaluations:

```bash
npm run evaluate
```

Validate the package, generated index, evaluations, links, and installation
behavior:

```bash
npm test
```
