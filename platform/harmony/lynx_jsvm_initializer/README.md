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
- The caller that performs initialization supplies the options, with the three
  flags described below overridden if a configuration has been saved. Concurrent
  callers wait for that attempt and receive its cached result; later options
  do not reconfigure JSVM.
- The library dynamically resolves `OH_JSVM_Init` and calls it at most once.
  Resolution failures and initialization failures are cached without retry.
- Direct calls to `OH_JSVM_Init` outside this library are not covered by this
  guarantee.

## Configuring JSVM from ArkTS

Applications can set three GC flags through `@lynx/lynx` before any participating
component starts JSVM initialization:

```ts
import { LynxEnv } from '@lynx/lynx';

const accepted = LynxEnv.setJSVMInitOptions({
  incrementalMarkingHardTrigger: 40,
  minSemiSpaceSize: 1,
  maxSemiSpaceSize: 4,
});
// Check accepted before creating JSVM runtimes.
LynxEnv.initialize(context);
```

All three fields are required:

| Field | Flag | Accepted values |
| --- | --- | --- |
| `incrementalMarkingHardTrigger` | `--incremental-marking-hard-trigger` | Integer from 0 to 100; 0 uses the engine's default trigger policy |
| `minSemiSpaceSize` | `--min-semi-space-size` | Positive int32 size in MiB, no greater than `maxSemiSpaceSize` |
| `maxSemiSpaceSize` | `--max-semi-space-size` | Positive int32 size in MiB |

The setter only saves a copy of the configuration; it does not initialize JSVM.
It returns `false` for invalid values or once shared initialization has started,
including after a failed attempt. Before initialization, the last successful
setter call replaces the complete configuration. Invalid calls leave it unchanged.

Without a successful setter call, the shared initializer forwards the first
caller's options unchanged. Lynx's default path adds none of these three flags;
no preset is applied automatically. With a saved configuration, these three
flags override matching caller flags; other options and external references are
preserved. This applies even when another component initializes through the
shared library first. It does not cover components that call `OH_JSVM_Init`
directly. `LynxEnv.initialize()` does not itself initialize JSVM, but setting the
options before it is recommended to avoid racing runtime creation.

Native callers can save the same configuration using
`Lynx_JSVM_SetInitOptions(int32_t, int32_t, int32_t)` in the order shown above.
All callers must link to the same initializer shared library.

## License

This module is licensed under the [Apache License, Version 2.0](LICENSE).
