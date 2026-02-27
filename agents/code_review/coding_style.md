# Coding Style & Formatting

CI runs `coding-style` checks on every PR. All changed files must pass `clang-format` or `prettier` formatting before merge.

## Formatting Tools by File Type

| File Extension | Formatter | Config |
|---|---|---|
| `.java`, `.h`, `.hpp`, `.c`, `.cc`, `.cpp`, `.m`, `.mm` | `clang-format` | `.clang-format` (`BasedOnStyle: Google`) |
| `.ts`, `.tsx`, `.yml`, `.yaml` | `prettier@2.2.1` | `.prettierrc` |
| `.gn`, `.gni` | `gn format` | — |

## How to Format

Run **before committing**:

```bash
# Format all changed files (requires tools/envsetup.sh to be sourced first)
source tools/envsetup.sh
python3 tools_shared/git_lynx.py format

# Or format a single file manually:
# C++/ObjC/Java:
buildtools/llvm/bin/clang-format -i <file>
# TypeScript/YAML:
npx prettier@2.2.1 -w <file>
```

## Key Details

- **Java files use `clang-format` with Google style**, not a Java-specific formatter. Pay attention to:
  - Ternary operator line breaks: `? value\n: fallback` (each on its own line)
  - Method chain wrapping: assignment on one line, call indented on the next
  - 100-character line limit (Google style default)

- **TypeScript/TSX files use `prettier@2.2.1`** (pinned version). The `.prettierrc` config enforces:
  - `printWidth: 80`, `singleQuote: true`, `tabWidth: 2`, `trailingComma: "es5"`

- **`.d.ts` files are excluded** from prettier checks (see `.prettierignore`)

## CI Checks

CI runs these static checks (see `.github/workflows/ci.yml`):

| Check | What it does |
|---|---|
| `coding-style` | Verifies `clang-format` / `prettier` formatting compliance |
| `cpplint` | C++ lint (Google style) |
| `java-lint` | PMD-based Java static analysis |
| `android-check-style` | Checkstyle (UnusedImports, etc.) |
| `commit-message` | Validates commit message format (see `commit_message_format.md`) |

## Running All Checks Locally

```bash
source tools/envsetup.sh
python3 tools_shared/git_lynx.py check
```

Or run specific checkers:

```bash
python3 tools_shared/git_lynx.py check --checkers coding-style
python3 tools_shared/git_lynx.py check --checkers java-lint
```

## Common Pitfalls

1. **Forgetting to format Java files**: Java uses `clang-format` (Google style), which has different conventions from standard Java IDEs. Always run `clang-format -i` on Java files.
2. **Wrong prettier version**: CI uses `prettier@2.2.1` specifically. Using a different version may produce different output.
3. **Missing trailing newline**: The format script ensures files end with a newline. Some editors strip trailing newlines.
