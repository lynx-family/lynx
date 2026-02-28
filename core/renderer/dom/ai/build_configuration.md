# BUILD.gn Configuration

## Adding New Test Files

When adding new test files, update the `BUILD.gn` file in the dom directory:

1. Add the test file path to the `sources` list in the `dom_testset` target
2. Use the `testing/` prefix for all test file paths

## Example

```gn
unittest_set("dom_testset") {
  sources = [
    "testing/attribute_holder_unittest.cc",
    "testing/element_unittest.cc",
    "testing/fiber_element_animation_variable_unittest.cc",
    # ... other test files
  ]
}
```

## Important Notes

- Always use `testing/` prefix for test files
- Test support files (mocks, helpers) also go in the sources list
- Non-test files (headers, implementation) should not be in the unittest sources
