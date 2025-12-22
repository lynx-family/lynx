# Contributing to @lynx-js/type-config

## Adding a New Configuration

To add a new configuration option, you should not modify the files in this package directly. Instead, follow these steps:

1.  **Update Configuration Definition**
    Modify the source of truth for Lynx configurations:
    `core/template_bundle/template_codec/binary_decoder/lynx_config.yml`

    Add your new configuration item there with appropriate fields (e.g., `valueType`, `defaultValue`, `since`, etc.).

    > **Important**: Please do **NOT** add new `CompilerOptions`. All new configurations should be added to `PageConfig` (exposed as the `Config` interface).

2.  **Generate Type Definitions**
    Run the generation script to update the `d.ts` files and key lists in this package.

    From the `lynx` directory:
    ```bash
    python3 tools/config/check_and_run.py
    ```

    This script will automatically update:
    - `js_libraries/type-config/types/*.d.ts`
    - `js_libraries/type-config/config-keys.js`

## Updating Tests

After generating the new types, you must update the test cases to ensure consistency and correctness.

### 1. Update Runtime Tests (Snapshots)

The file `test/index.test.js` contains snapshots of all configuration keys. If you added a new key, the tests will fail.

Update the snapshot to include your new key, and **update the expected length**:

```javascript
// test/index.test.js

it('should have correct configKeys', () => {
  // Increase the length by 1
  expect(configKeys.length).toBe(123);
  expect(configKeys).toMatchInlineSnapshot(`
    [
      // ...
      "yourNewConfig",
    ]
  `)
})
```

### 2. Add Type Tests

The file `test/index.test-d.ts` ensures that the generated TypeScript definitions are correct.

Add a type check for your new property:
```typescript
// test/index.test-d.ts

expectTypeOf<Config>().toHaveProperty('yourNewConfig').toEqualTypeOf<string | undefined>();
```

## Running Tests

Before submitting your changes, make sure all tests pass.

Run the following command in the `js_libraries/type-config` directory:

```bash
cd js_libraries/type-config
npm test
```

This command executes:
1.  `vitest`: Verifies that the exported keys match the snapshots.
2.  `tsc --noEmit`: Verifies that the type definitions match the expectations in `index.test-d.ts`.