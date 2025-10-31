# JNI Generator

A one-stop tool that automatically generates:
- JNI header files for Java ↔ native interop
- Registration headers and glue code
- A dynamic-library load-time entry file (`JNI_OnLoad`)
- The corresponding GN build rules

...so you never have to hand-write JNI boilerplate again.

---

## Quick Start

```bash
python3 generate_and_register_jni_files.py \
        -root ROOT_DIR \
        -path PATH/TO/jni_configs.yml \
        [--use-base-jni-header]
```

| Flag                  | Purpose                                                                                                 |
| --------------------- | ------------------------------------------------------------------------------------------------------- |
| `-root`               | Repository root, used to resolve all relative paths.                                                    |
| `-path`               | Path to the YAML configuration file that describes what to generate.                                    |
| `--use-base-jni-header` | Use `base/include/platform/android/jni_utils.h` instead of the default `core/base/android/android_jni.h`. |

---

## Configuration File (`jni_configs.yml`)

The YAML configuration file **must** contain three top-level sections:

### 1. `jni_register_configs`

Defines the `JNI_OnLoad` entry file and global registration logic.

```yaml
jni_register_configs:
  output_path: "tools/build_jni/testing/lynx_so_load.cc"
  custom_headers:                                 # [Optional] Extra #include lines
    - core/base/android/android_jni.h
  namespaces:                                     # C++ namespaces for registration functions
    - lynx
  special_cases:                                  # Hand-written registration snippets
    - header: xxx/yyy.h                           # [Optional] Extra #include for this case
      java: platform/android/xxx/Example.java     # [Optional] The Java class for this registration
      method: "lynx::foo::Bar::Register(env);"    # [Optional] A custom registration function call
      macro: OPTIONAL_MACRO                       # [Optional] Guard this registration with an #ifdef
```

### 2. `jni_class_configs`

Lists the Java classes for which JNI bindings should be generated.

```yaml
jni_class_configs:
  output_dir: "tools/build_jni/testing/gen"                 # Output directory for generated JNI headers (relative to -root)
  jni_classes:                                              # List of Java classes to process
    # Import a class list from another YAML file, prepending "lynx/" to their `java` paths
    - !include platform/android/xxx/jni_configs.yml |
        jni_class_configs.jni_classes & java:lynx
    # Process a single Java class
    - java: platform/android/lynx_android/.../Example.java  # Path to the Java source file (relative to -root)
      register_method_name: RegisterJNIForExample           # [Optional] Override the default registration function name
      macro: TEST_MACRO                                     # [Optional] Wrap generated code in the #ifdef and #endif
```

If there is a macro controlling the inclusion of multiple Java classes, you can declare them as follows:

```yaml
jni_class_configs:
  output_dir: "tools/build_jni/testing/gen"                 
  jni_classes:                                             
    # Process two or more Java classes
    - java: 
        - platform/android/lynx_android/.../Example.java
        - platform/android/lynx_android/.../demo.java
      macro: TEST_MACRO
```

### 3. `gn_configs`

Produces a `BUILD.gn` file that bundles the generated source files.

```yaml
gn_configs:
  output_path: "tools/build_jni/testing/BUILD.gn"   # Output path for the BUILD.gn file (relative to -root)
  template_name: lynx_source_set                    # Name of the GN template to use (e.g., source_set)
  custom_headers:                                   # [Optional] Additional .gni files to import (relative to -root)
    - "core/Lynx.gni"
```

---

## Generated Artifacts

The script generates the following files:

| File                         | Description                                                  |
| ---------------------------- | ------------------------------------------------------------ |
| `<ClassName>_jni.h`          | JNI function signatures generated from the Java class.       |
| `<ClassName>_register_jni.h` | Declaration for the registration function, e.g., `void RegisterJNIFor<ClassName>(JNIEnv*);` |
| `*_so_load.cc`               | `JNI_OnLoad` implementation that calls all registration functions. |
| `BUILD.gn`                   | A GN target that bundles all the generated source files.     |

> **Important:** The registration headers only *declare* the registration functions. You must still provide an implementation in your C++ code:

```cpp
#include "gen/Example_jni.h"
#include "gen/Example_register_jni.h"

namespace lynx::jni {
void RegisterJNIForExample(JNIEnv* env) {
  // This function is generated in Example_jni.h
  RegisterNativesImpl(env);
}
}  // namespace lynx::jni
```

---

## Tips

- All `java:` paths in the configuration are **relative to the `-root` directory**.
- If a `macro` is specified for a class, the generated registration code for it will be wrapped in an `#ifdef MACRO ... #endif` block.
- You can specify a custom registration function name for a Java class using the `register_method_name` field. This is useful to avoid naming conflicts.
- Use the `!include` syntax to import and reuse JNI class lists from other YAML files. When including, you can prepend a path prefix to keys from the included file. For example, `& java:lynx` will prepend `lynx/` to all `java:` paths.

---

## Examples

Ready-to-use samples are available in:
- [`testing/`](testing) – A minimal demo.
- [`platform/android/lynx_android/src/main/jni/`](../../../platform/android/lynx_android/src/main/jni) – A full production setup.