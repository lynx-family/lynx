# macOS Node-API Addons

The open-source Explorer macOS sample includes a `LynxNodeAPI` host module and native loader example for integrating a Node-API addon.

For the experimental capability model, SDK prerequisites, the role of the sample `LynxNodeAPIModule` in Explorer, and addon authoring guidance, see [Lynx Node-API Addons](../../docs/lynx-node-api.md). This document only covers macOS-specific packaging.

## How To Integrate The Addon Into The App

Apple platforms prefer static integration. On macOS, link the addon static library into the host app and include the generated `addon_use.h` header from one host translation unit. The use header expands to `NAPI_USE(<addon_symbol>)`, which keeps the addon's static registration symbol alive during the final app link.

For local Explorer development, pass both the addon static library and the generated use header to GN, for example:

```bash
buildtools/gn/gn gen out/Default --args='... use_weak_suffix_napi = true explorer_node_api_addon_static_library = "/absolute/path/to/dist/macos/macosx/lib<addon>.a" explorer_node_api_addon_use_header = "/absolute/path/to/dist/macos/macosx/include/addon_use.h"'
```

The Explorer sample includes the use header from `AppDelegate.mm` when `explorer_node_api_addon_use_header` is set. Do not use `-force_load` for this integration path.

## How To Enable It For A Page

The current macOS Explorer sample only enables the Node-API example integration when the page URL query includes `enable_napi_addon=1` (or `enable_napi_addon=true`).

Example:

- `file://lynx?local://homepage/main.lynx.bundle?enable_napi_addon=1`

When verifying from the command line, pass the URL as an app argument, for example `LynxExplorer.app/Contents/MacOS/LynxExplorer --url=file://lynx?local://homepage/main.lynx.bundle?enable_napi_addon=1`.

## How It Is Resolved

When JS calls `requireNodeAddon("<addon>")`, the loader resolves the addon from the host's static addon registry. The addon name passed from JS must match the generated addon name.
