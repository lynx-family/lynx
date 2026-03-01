# AI Agent Guide

Lynx is an open-source cross-platform UI framework for building performant native UIs
with a web-like developer experience. The core runtime is written in C++; app UIs are
written in ReactLynx (TypeScript) and compiled to Lynx bundles.

## Repository Layout

| Directory       | What's in it                                                   |
|-----------------|----------------------------------------------------------------|
| `core/`         | Main Lynx runtime (C++)                                        |
| `base/`         | Utilities: logging, threading, tracing                         |
| `clay/`         | High-performance rendering engine (Skia/Skity, Flutter-based)  |
| `platform/`     | Platform adapters: Android, iOS, Harmony, Windows, macOS       |
| `explorer/`     | Official demo/test app — native shell + ReactLynx UI           |
| `js_libraries/` | TypeScript/JS libraries                                        |
| `tools_shared/` | Dev tooling: `git lynx check/format/build` CLI                 |
| `buildtools/`   | Pre-compiled toolchain (clang-format, gn, ninja, node, …)      |
| `testing/`      | Testing framework and unit test guides                         |

## Task → Where to Read

| You are working on…                              | Read                                                                                                                                                      |
|--------------------------------------------------|-----------------------------------------------------------------------------------------------------------------------------------------------------------|
| Submitting a PR (commit format, CI workflow)     | [CONTRIBUTING.md](./CONTRIBUTING.md)                                                                                                                      |
| Formatting code / running CI checks locally      | [CONTRIBUTING.md § Static Code Analysis](./CONTRIBUTING.md#static-code-analysis-tasks) + [agents/code_review/coding_style.md](./agents/code_review/coding_style.md) for AI-specific pitfalls |
| Explorer JS UI (homepage / showcase screens)     | [explorer/README.md](./explorer/README.md)                                                                                                                |
| Building Android native app                      | [explorer/android/README.md](./explorer/android/README.md)                                                                                                |
| Building iOS native app                          | [explorer/darwin/ios/README.md](./explorer/darwin/ios/README.md)                                                                                          |
| Understanding `git lynx` / build tooling         | [tools_shared/README.md](./tools_shared/README.md)                                                                                                        |
| Running unit tests                               | [testing/README_UT.md](./testing/README_UT.md)                                                                                                            |

## Critical Gotchas

- **Source `tools/envsetup.sh` before any build or format command.** `git lynx format/check`
  and `clang-format` rely on `buildtools/` being in `PATH`. Without sourcing, the commands
  will either fail or silently use the wrong system-installed version.

- **Each PR must contain exactly one commit.** CI rejects PRs with more than one commit.
  Squash all fixups before pushing. See [CONTRIBUTING.md](./CONTRIBUTING.md) for the
  required commit message format.
