# PerformanceEntry Definitions

## Description

The YAML files in this directory are used to describe [PerformanceEntry](https://lynxjs.org/api/lynx-api/performance-api/performance-entry.html). The `generate_performance_entry.py` script will generate corresponding JAVA/OC/TS/ETS files based on all the YAML files in this directory.

## Usage

If you are modifying an existing `PerformanceEntry`, simply modify the desired yml file. 😊

If you are adding a new `PerformanceEntry`, create it and then place in `lynx/tools/performance/performance_observer/performance_entry_definition_files`. 👷

### Syntax

Your PerformanceEntry file should consist of a new filename, inheritance, and variable definitions. Its basic structure should resemble the code below:

```yaml
# NewEntry.yml
NewEntry:
  allOf:
    - $ref: 'PerformanceEntry.yml#/PerformanceEntry'
    - type: object
      properties:
        propName:
            type: string
```

First, `allOf` indicates that your definition inherits from `PerformanceEntry`. You must ensure that the NewEntry you define has an inheritance relationship with it.

Next, you can define its basic data types as follows:

```yaml
name:
    type: number/string/string/map
```

Or, define more complex object types as follows:

```yaml
name:
    $ref: 'ReferenceObject.yml#/ReferenceObject'
```

#### Extension Tags

To facilitate organizing multi-terminal outputs, we added three descriptors `x-ignore-java`, `x-ignore-oc` and `x-ignore-ts` to control whether to generate code for specific languages. You can use these extension tags as shown in the code below:

```yaml
// NewEntry.yml
NewEntry:
  x-ignore-java: False
  x-ignore-oc: True
  x-ignore-ts: False
  allOf:
    - $ref: 'PerformanceEntry.yml#/PerformanceEntry'
    - type: object
      properties:
        propName:
            type: string
```
