# Test File Naming Guidelines

## Pattern

Test files should follow this naming pattern:

```
{component}_{category}_unittest.cc
```

Where:

- `{component}`: The name of the component being tested (e.g., `element`, `fiber_element`, `attribute_holder`)
- `{category}`: The specific category/aspect being tested (optional for general tests)

## Examples

### Good Examples

- `element_unittest.cc` - General element tests
- `element_container_unittest.cc` - Tests for element container functionality
- `fiber_element_animation_variable_unittest.cc` - Tests for animation variables in fiber elements
- `fiber_element_css_selector_unittest.cc` - Tests for CSS selector handling in fiber elements
- `fiber_element_dynamic_style_unittest.cc` - Tests for dynamic styling in fiber elements

### Bad Examples

- `fiber_element_all_tests.cc` - Mixing multiple unrelated features
- `test_file_1.cc`, `test_file_2.cc` - Non-descriptive names

## One Category Per Test File

Each test file should focus on **one specific category or feature**. This makes:

- Tests easier to understand and maintain
- Failures easier to diagnose
- Parallel test execution more effective
- AI context cleaner when working on specific features

## Test Support Files

Support files for tests (mocks, helpers, base classes) should be placed in `testing/` and named descriptively:

- `fiber_mock_painting_context.cc/.h` - Mock painting context for fiber tests
- `fiber_mock_text_layout.cc/.h` - Mock text layout for fiber tests
- `fiber_element_test.cc/.h` - Base test class for fiber element tests
