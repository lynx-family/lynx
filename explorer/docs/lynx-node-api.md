# Lynx Node-API Addons

Lynx provides experimental runtime integration points that allow a host application to expose Node-API addons to Lynx pages. The open-source Explorer apps include a sample module named `LynxNodeAPIModule` and a sample native loader to demonstrate one host-side integration pattern for this capability.

## Experimental Capability

Lynx itself does not prescribe a single addon loader implementation. In the open-source Explorer sample, the host-side integration is implemented by `LynxNodeAPIModule` together with the shared native loader in `explorer/cpp/LynxNodeAPI.cc`.

What belongs to the Lynx-side integration foundation:

- a host can obtain a runtime-specific `napi_env`
- a host can expose one or more modules to Lynx pages
- a host can choose to wire a Node-API addon loader into that module boundary

What remains host-defined in an integration:

- the page-facing module name, such as `LynxNodeAPI`
- the page-facing method name, such as `requireNodeAddon()`
- how the addon binary is integrated, such as dynamic loading on Android/Harmony/Windows or static registration plus a generated `addon_use.h` reference on Apple platforms
- which exported symbol is used for addon initialization
- where addon exports are published in JS, such as `__lynx_node_addon_exports__`

In this sample implementation:

1. A Lynx page requests an addon by name.
2. The host-side sample loader resolves the addon through the platform integration strategy.
3. Android/Harmony/Windows use dynamic library loading; iOS/macOS link the addon into the host app and include the generated `addon_use.h` header once so `NAPI_USE` retains the addon's static registration entry.
4. The addon is initialized through the standard Node-API registration entry.
5. The sample loader publishes the addon exports on the JS global object under `__lynx_node_addon_exports__`.

Note: the dynamic-loading strategy shown here is for demonstration/experimental use only. In production integrations, avoid relying on default library search paths (especially on Windows) and constrain what can be loaded: validate `addonName` against an allowlist and/or require a fixed base directory; resolve to an absolute, canonical path under that directory; and return actionable diagnostics when loading fails. On Windows, prefer `LoadLibraryExW` with restricted search behavior (e.g., limiting search directories) rather than the default DLL search order.

This capability and the example integration remain experimental. Library naming, packaging rules, and host integration details may continue to evolve. For how to bundle or link the addon into each Explorer app, see the platform-specific Node-API integration documents.

## Integration Prerequisites

To integrate this capability into a host app, use a Lynx 3.9.x SDK baseline.

Platform-specific runtime dependencies:

- Android: integrate a PrimJS 3.9.x runtime and package the matching `libnapi_adapter.so`
- Harmony: integrate a PrimJS 3.9.x runtime and package the matching `libnapi_adapter.so`
- iOS: integrate a PrimJS 3.9.x runtime and add the latest compatible `LynxWeakNodeAPI`

Additional iOS runtime requirement:

- iOS must install the PrimJS to `LynxWeakNodeAPI` bridge once during app startup before the Explorer environment is used

The current open-source Explorer examples implement these prerequisites here:

- Android PrimJS runtime dependency: [platform/android/lynx_android/build.gradle](../../platform/android/lynx_android/build.gradle)
- Harmony PrimJS runtime dependency: [explorer/harmony/oh-package.json5](../harmony/oh-package.json5)
- iOS PrimJS and `LynxWeakNodeAPI` dependencies: [explorer/darwin/ios/lynx_explorer/Podfile](../darwin/ios/lynx_explorer/Podfile)
- iOS runtime bridge installation: [explorer/darwin/ios/lynx_explorer/LynxExplorer/AppDelegate.mm](../darwin/ios/lynx_explorer/LynxExplorer/AppDelegate.mm)

## Explorer Sample Module

Explorer is an example host for this capability. `LynxNodeAPIModule` is sample integration code, not the capability definition itself.

The sample module demonstrates how a host app can:

- accept a page request to load an addon
- bridge the request from the platform UI/runtime layer to native code
- bind a runtime-specific `napi_env`
- invoke the shared addon loader
- expose the resulting exports back to the page JS environment

### Page-side Usage

The sample module exposes a single host method:

- `requireNodeAddon(addonName)`

The `addonName` value should match the library basename without:

- the `lib` prefix on platforms that use it
- the file extension or framework bundle path components

Examples:

- Android shared library: `libsample.so` -> `requireNodeAddon("sample")`
- Harmony shared library: `libsample.so` -> `requireNodeAddon("sample")`
- iOS statically linked addon registered as `sample` -> `requireNodeAddon("sample")`
- macOS statically linked addon registered as `sample` -> `requireNodeAddon("sample")`
- Windows addon binary: `sample.node` or `sample.dll` -> `requireNodeAddon("sample")`

### Enabling It For A Page

In the current Explorer samples, the Node-API example integration is only enabled when the page URL query includes `enable_napi_addon=1` (or `enable_napi_addon=true`).

Example:

- `file://lynx?local://homepage.lynx.bundle?enable_napi_addon=1`

### Shared Loader Files

The shared native loader used by Explorer lives in:

- [explorer/cpp/LynxNodeAPI.h](../cpp/LynxNodeAPI.h)
- [explorer/cpp/LynxNodeAPI.cc](../cpp/LynxNodeAPI.cc)

### Android Example Files

Android registers the sample module and binds the runtime env through these files:

- [explorer/android/lynx_explorer/src/main/java/com/lynx/explorer/modules/LynxNodeAPIModule.java](../android/lynx_explorer/src/main/java/com/lynx/explorer/modules/LynxNodeAPIModule.java)
- [explorer/android/lynx_explorer/src/main/java/com/lynx/explorer/modules/LynxModuleAdapter.java](../android/lynx_explorer/src/main/java/com/lynx/explorer/modules/LynxModuleAdapter.java)
- [explorer/android/lynx_explorer/src/main/java/com/lynx/explorer/LynxViewShellActivity.java](../android/lynx_explorer/src/main/java/com/lynx/explorer/LynxViewShellActivity.java)

### iOS Example Files

iOS registers the sample module, installs the runtime bridge, and binds `napi_env` through these files:

- [explorer/darwin/ios/lynx_explorer/LynxExplorer/AppDelegate.mm](../darwin/ios/lynx_explorer/LynxExplorer/AppDelegate.mm)
- [explorer/darwin/ios/lynx_explorer/LynxExplorer/modules/LynxNodeAPIModule.h](../darwin/ios/lynx_explorer/LynxExplorer/modules/LynxNodeAPIModule.h)
- [explorer/darwin/ios/lynx_explorer/LynxExplorer/modules/LynxNodeAPIModule.mm](../darwin/ios/lynx_explorer/LynxExplorer/modules/LynxNodeAPIModule.mm)
- [explorer/darwin/ios/lynx_explorer/LynxExplorer/modules/LynxNodeAPILifecycleListener.mm](../darwin/ios/lynx_explorer/LynxExplorer/modules/LynxNodeAPILifecycleListener.mm)
- [explorer/darwin/ios/lynx_explorer/LynxExplorer/LynxViewShellViewController.m](../darwin/ios/lynx_explorer/LynxExplorer/LynxViewShellViewController.m)

### Harmony Example Files

Harmony registers the sample module, manages the sendable token, and bridges into native code through these files:

- [explorer/harmony/lynx_explorer/src/main/ets/module/LynxNodeAPIModule.ets](../harmony/lynx_explorer/src/main/ets/module/LynxNodeAPIModule.ets)
- [explorer/harmony/lynx_explorer/src/main/ets/pages/Lynx.ets](../harmony/lynx_explorer/src/main/ets/pages/Lynx.ets)
- [explorer/harmony/lynx_explorer/src/main/cpp/lynx_node_api_napi.cpp](../harmony/lynx_explorer/src/main/cpp/lynx_node_api_napi.cpp)
- [explorer/harmony/lynx_explorer/src/main/cpp/CMakeLists.txt](../harmony/lynx_explorer/src/main/cpp/CMakeLists.txt)

### macOS Example Files

macOS registers the sample module in the embedder builder, binds `napi_env` through a runtime lifecycle observer, and reuses the shared native loader through these files:

- [explorer/darwin/macos/lynx_explorer/LynxExplorer/ViewController.mm](../darwin/macos/lynx_explorer/LynxExplorer/ViewController.mm)
- [explorer/darwin/macos/lynx_explorer/LynxExplorer/module/LynxNodeAPIModule.h](../darwin/macos/lynx_explorer/LynxExplorer/module/LynxNodeAPIModule.h)
- [explorer/darwin/macos/lynx_explorer/LynxExplorer/module/LynxNodeAPIModule.mm](../darwin/macos/lynx_explorer/LynxExplorer/module/LynxNodeAPIModule.mm)
- [explorer/darwin/macos/lynx_explorer/LynxExplorer/runtime/ExampleLynxRuntimeLifecycleObserver.mm](../darwin/macos/lynx_explorer/LynxExplorer/runtime/ExampleLynxRuntimeLifecycleObserver.mm)

### Windows Example Files

Windows registers the sample module in the embedder builder, binds `napi_env` through a runtime lifecycle observer, and reuses the shared native loader through these files:

- [explorer/windows/lynx_explorer/lynx_window.cc](../windows/lynx_explorer/lynx_window.cc)
- [explorer/windows/lynx_explorer/module/lynx_node_api_module.h](../windows/lynx_explorer/module/lynx_node_api_module.h)
- [explorer/windows/lynx_explorer/module/lynx_node_api_module.cc](../windows/lynx_explorer/module/lynx_node_api_module.cc)
- [explorer/windows/lynx_explorer/runtime/example_lynx_runtime_lifecycle_observer.cc](../windows/lynx_explorer/runtime/example_lynx_runtime_lifecycle_observer.cc)

## Platform Integration

The sample loader expects the addon to be integrated according to the platform packaging model. Apple platforms prefer static integration, while Android/Harmony/Windows keep using dynamic addon binaries. Use the platform guides for the exact integration steps:

- Android packaging and ABI notes: [explorer/android/lynx-napi-addon.md](../android/lynx-napi-addon.md)
- iOS static-library `podspec` and `xcframework` integration: [explorer/darwin/ios/lynx-napi-addon.md](../darwin/ios/lynx-napi-addon.md)
- Harmony `har` integration: [explorer/harmony/lynx-napi-addon.md](../harmony/lynx-napi-addon.md)
- macOS static-library integration: [explorer/darwin/macos/lynx-napi-addon.md](../darwin/macos/lynx-napi-addon.md)
- Windows app-local packaging: [explorer/windows/lynx-napi-addon.md](../windows/lynx-napi-addon.md)

## Building Addons

If you are authoring a Node-API addon for Lynx, use the latest `@lynx-js/weak-node-api` and follow its documentation for headers, registration macros, exported symbols, and build setup.

The open-source Explorer documentation only covers how to integrate a built addon into each host app. It is not the canonical guide for authoring the addon binary itself.
