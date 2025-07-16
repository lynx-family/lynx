# PerformanceEntry Definitions

This document provides instructions on how to define and use `PerformanceEntry` objects within the Lynx framework. The YAML files in this directory serve as the basis for generating corresponding code in Java, Objective-C, ArkTS, and TypeScript.

## Description

The `generate_performance_entry.py` script utilizes the YAML files in this directory to generate `PerformanceEntry` definitions for multiple platforms. These definitions are essential for performance monitoring and analysis across different parts of the Lynx framework.

## Usage

### How to Modify a PerformanceEntry?

To modify an existing `PerformanceEntry`, simply locate the corresponding YAML file and apply your changes directly. The script will automatically pick up the modifications during the next generation cycle.

### How to Add a New PerformanceEntry?

1.  Create a new YAML file in the `tools/performance/performance_observer/definition_yaml_files` directory.
2.  Add the new file to `tools/performance/performance_observer/performance_entry_definition_files`.
3.  Define the structure of your new `PerformanceEntry` in the YAML file using the syntax described below.

### How to Verify Your Modifications?

You can generate the `PerformanceEntry` files by running the following commands:

```shell
cd /path/to/lynx
source ./tools/envsetup.sh
./hab sync
```

After generation, verify the output in the following locations:

*   **TypeScript**: `js_libraries/types/types/background-thread`
*   **Java**: `platform/android/lynx_android/src/main/java/com/lynx/tasm/performance/performanceobserver`
*   **Objective-C**: `platform/darwin/common/lynx/performance/performance_observer` (implementation) and `platform/darwin/common/lynx/public/performance/performance_observer` (headers)

## Example

Here is an example of how to define a `TestCaseEntry` with a nested `TestCaseResult` object:

```typescript
export interface TestCaseResult {
    status: number;
    isSuccess: boolean;
    errorMessage: string;
}

export interface TestCaseEntry extends PerformanceEntry {
    testStart: number;
    testEnd: number;
    testInfo: Record<string, number>;
    testResult: TestCaseResult;
}
```

To create this, add a `TestCaseEntry.yml` file with the following content:

```yaml
TestCaseResult:
  type: object
  properties:
    status:
      type: number
    isSuccess:
      type: boolean
    errorMessage:
      type: string

TestCaseEntry:
  allOf:
    - $ref: 'PerformanceEntry.yml#/PerformanceEntry'
    - type: object
      properties:
        testStart:
          type: number
        testEnd:
          type: number
        testInfo:
          type: map
          keyType: string
          valueType: number
        testResult:
          $ref: '#/TestCaseResult'
```

## YAML Syntax

A `PerformanceEntry` definition is structured as a YAML object with a class name, inheritance rules, and property definitions.

### Variable Definitions

Variables are defined under the `properties` key. Each variable must have a name and a type, which can be specified using either `type` or `$ref`.

### Supported Types

The following data types are supported:

*   `number`
*   `integer`
*   `string`
*   `boolean`
*   `timestamp`
*   `map` (requires `keyType` and `valueType`)
*   `$ref` (for referencing other objects)

### Extension Tags

We use the following extension tags to manage multi-platform code generation:

*   `x-type`: Specifies the entry type (e.g., `init`, `pipeline`, `resource`, `memory`, `metric`).
*   `x-name`: Assigns a specific name to the entry.
*   `x-lang`: A list of languages for which the definition should be generated (e.g., `java`, `objc`, `ts`, `ArkTS`). If this field is not specified, it defaults to all supported languages.
*   `ts-discriminated-union`: Creates a union of multiple types, as demonstrated in `HostPlatformTiming.yml`. This should be used in conjunction with `ts-literal-type` to create a discriminated union.
*   `ts-literal-type`: Defines a literal type in TypeScript (e.g., `'Android'`, `'iOS'`). This is used to define the discriminant property for each type within a `ts-discriminated-union`.