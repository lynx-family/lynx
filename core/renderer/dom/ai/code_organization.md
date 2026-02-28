# Code Organization Principles

## Keep Implementation and Tests Separate

- **Source files** (`.cc`, `.h`): Stay in their respective component directories (e.g., `fiber/`, `vdom/`, `selector/`)
- **Test files** (`*_unittest.cc`): Always in `testing/`

## Benefits

This separation ensures:

- Clear distinction between production code and test code
- Easier navigation of the codebase
- Simpler build configurations
- Cleaner mental model for developers

## Directory Structure

```
lynx/core/renderer/dom/
├── testing/                    # All test files go here
│   ├── attribute_holder_unittest.cc
│   ├── element_unittest.cc
│   ├── fiber_element_*.cc      # Fiber element tests
│   └── fiber_mock_*.cc         # Test mocks/helpers
├── fiber/                      # Source only - no tests
│   ├── fiber_element.cc
│   └── fiber_element.h
└── BUILD.gn
```

## Best Practices for AI Agents

1. **Always check existing patterns**: Look at existing test files in `testing/` before creating new ones
2. **Use descriptive names**: Test file names should clearly indicate what's being tested
3. **Focus on single categories**: Don't mix unrelated test cases in one file
4. **Update BUILD.gn**: Always add new test files to the build configuration
5. **Verify builds**: Run the unit test build after making changes

## Running Tests

Use the `lynx-cpp-unittest` skill to verify your changes compile and run correctly.
