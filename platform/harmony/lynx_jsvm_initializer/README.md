# Lynx JSVM Initializer

`@lynx/lynx_jsvm_initializer` provides `liblynx_jsvm_initializer.so`, a shared
initialization entry point for Harmony JSVM users in the same process.

## Dependency

The Lynx Harmony package declares this module as a dependency. Applications that
depend on Lynx receive the initializer transitively and do not need an ArkTS
initialization call.

Within the Lynx Harmony workspace, a module can declare the dependency using the
shared version parameter:

```json5
{
  "dependencies": {
    "@lynx/lynx_jsvm_initializer": "@param:dependencies.lynx_version"
  }
}
```

For local development, the root project's `overrides` maps this package to the
local module directory. Outside that workspace, use a concrete package version
compatible with the Lynx package in the application.

## Native Initialization

Native callers link against `liblynx_jsvm_initializer.so` and use the existing C
entry point:

```cpp
JSVM_Status Lynx_JSVM_Common_Init(const JSVM_InitOptions* options);
```

The source declaration is in `src/main/cpp/jsvm_initializer.h`. The HAR ships the
shared library and an empty ArkTS entry point; it does not ship C/C++ source files
or headers, and importing the ArkTS entry point does not initialize JSVM.

- All participating callers must use the same loaded initializer library to
  share its initialization state. Do not compile the implementation into each
  caller's library.
- The caller that performs initialization supplies the options. Concurrent
  callers wait for that attempt and receive its cached result; later options
  do not reconfigure JSVM.
- The library dynamically resolves `OH_JSVM_Init` and calls it at most once.
  Resolution failures and initialization failures are cached without retry.
- Direct calls to `OH_JSVM_Init` outside this library are not covered by this
  guarantee.

## License

This module is licensed under the [Apache License, Version 2.0](LICENSE).
