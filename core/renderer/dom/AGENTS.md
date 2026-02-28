# AI Guidelines for DOM Module

Quick reference for AI agents working on the DOM (Document Object Model) module.

## Quick Rules

1. **All test files MUST be in `testing/` directory** - never in source directories
2. **Test naming**: `{component}_{category}_unittest.cc` (e.g., `fiber_element_animation_variable_unittest.cc`)
3. **One category per test file** - don't mix unrelated features
4. **Update BUILD.gn** - add `testing/` prefix when adding test files

## When to Read Detailed Documentation

| Scenario | Read This |
|----------|-----------|
| Creating new test files | [`ai/test_file_naming.md`](ai/test_file_naming.md) |
| Updating build configuration | [`ai/build_configuration.md`](ai/build_configuration.md) |
| Understanding project structure | [`ai/code_organization.md`](ai/code_organization.md) |

## Directory Structure

```
lynx/core/renderer/dom/
├── testing/           # All test files (*.cc) and test helpers
├── fiber/            # Source only - Fiber element implementation
├── vdom/             # Source only - Virtual DOM implementation
├── selector/         # Source only - CSS selector implementation
├── fragment/         # Source only - Fragment implementation
└── ai/               # Detailed AI documentation
```
