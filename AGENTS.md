# AI Context & Guidelines

This repository contains a dedicated directory structure for AI agents to better understand the project's specific domain context, architectural decisions, and coding conventions.

## 🤖 For AI Agents

When working in this repository, please consult the contents of the **[`agents/`](./agents)** directory. This folder serves as a knowledge base containing:

- **Domain Knowledge**: Specific business logic and terminology used in this project.
- **Architecture Overview**: High-level design patterns and structural decisions.
- **Coding Standards**: Project-specific conventions that supplement general best practices.

**Action**: Before proposing significant changes or architectural updates, read the relevant documents in `agents/` to align with the project's established patterns.

## Repository Navigation

### Build Artifacts — DO NOT Stage or Commit

This repository builds for multiple platforms (Android, iOS, Web, HarmonyOS, macOS, Windows) and generates large intermediate artifacts. These are already in `.gitignore` but agents must be aware of them:

- `out/`, `build/`, `buildtools/` — GN/Ninja build outputs and toolchain (~2GB+)
- `third_party/{v8,quickjs,libcxx,libcxxabi,llvm,...}` — Habitat-managed dependencies (~800MB+)
- `node_modules/`, `js_libraries/**/dist/` — JS dependencies and outputs
- `core/build/gen/`, `**/build/gen` — Auto-generated code (CSS decoders, feature counters, error codes)
- `platform/android/.gradle/`, `*.cxx/` — Android build cache

### Git Performance

After building, the repo may contain several gigabytes of untracked files. The repo is configured with `core.fsmonitor=true` and `core.untrackedcache=true` to keep `git status` fast. **Do NOT disable these settings.**

When staging changes, **never use `git add -A` or `git add .`** — always add specific files by name to avoid accidentally staging build artifacts.

### Source and Dependency Management (Habitat)

Dependencies are managed by **Habitat** (configured in `.habitat` and `dependencies/DEPS*` files):

```sh
source tools/envsetup.sh    # Initialize PATH and environment
tools/hab sync . -f          # Sync all dependencies via Habitat
```

### Key Development Commands

| Command | Purpose |
|---------|---------|
| `source tools/envsetup.sh` | Initialize PATH and environment variables |
| `tools/hab sync . -f` | Sync dependencies via Habitat |
| `git lynx check` | Run all static analysis checks (coding-style, cpplint, java-lint, commit-message, api-check, etc.) |
| `git lynx check --checkers=<name>` | Run a specific checker (e.g., `api-check`, `coding-style`) |
| `git lynx format` | Auto-format changed files (clang-format, prettier, gn fmt) |

### Commit Conventions

- **PRs must contain exactly ONE commit** — CI will reject multi-commit PRs.
- Format: `[Label][Optional Scope] Title`
- Valid first labels: `Feature`, `BugFix`, `Refactor`, `Optimize`, `Infra`, `Testing`, `Doc`
- Body must explain **why** the change was made, not just what changed.
- Follow [Google's Style Guides](https://google.github.io/styleguide/) for C++, Java, Objective-C, and Python.
- See [`agents/code_review/commit_message_format.md`](./agents/code_review/commit_message_format.md) for details.

### Code Structure Quick Reference

| Directory | Purpose |
|-----------|---------|
| `core/` | C++ core engine (rendering, layout, animation, events, JS runtime) |
| `core/renderer/css/` | CSS parsing and property handling |
| `core/renderer/starlight/` | Starlight layout engine |
| `core/runtime/` | JavaScript runtime bindings (V8, QuickJS) |
| `core/shell/` | Platform shell and bridge layer (65+ subdirs) |
| `clay/` | Rendering engine (graphics, UI elements; Flutter-derived architecture) |
| `platform/{android,darwin,harmony,windows}/` | Platform-specific adaptation layers |
| `js_libraries/` | TypeScript/JS SDK packages |
| `explorer/` | Demo applications for each platform |
| `devtool/` | Developer tools and inspector |
| `tools/` | Build scripts, code generators (`css_generator/`, `error_code/`, `feature_count/`) |
| `testing/` | Test infrastructure and integration tests |
| `agents/` | AI agent context and domain knowledge |
