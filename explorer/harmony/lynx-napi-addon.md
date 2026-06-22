# Harmony Node-API Addons

The open-source Explorer Harmony sample includes a `LynxNodeAPI` host module and native loader example for integrating a Node-API addon.

For the experimental capability model, SDK prerequisites, the role of the sample `LynxNodeAPIModule` in Explorer, and addon authoring guidance, see [Lynx Node-API Addons](../docs/lynx-node-api.md). This document only covers Harmony-specific packaging and build integration.

## How To Integrate The Addon Into The App

On Harmony, integrate the addon through a `har` package that carries the addon native shared library into the app package. For local Explorer development, addon shared libraries can also be placed under `explorer/harmony/lynx_explorer/src/main/cpp/napi_addons/<abi>/`, and the Explorer CMake build will copy them into the native library output directory.

Before building the local Explorer app, install OHPM dependencies for the Harmony workspace, app, and native type package:

```bash
ohpm install --all
cd lynx_explorer && ohpm install --all
cd src/main/cpp/types/liblynx_napi_addon_loader && ohpm install --all
```

## How To Enable It For A Page

The current Harmony Explorer sample only enables the Node-API example integration when the page URL query includes `enable_napi_addon=1` (or `enable_napi_addon=true`).

Example:

- `file://lynx?local://main.lynx.bundle?enable_napi_addon=1`

## How It Is Resolved

When JS calls `requireNodeAddon("<addon>")`, the loader will try common candidates such as `lib<addon>.so` from the process default library search paths. `<addon>` should match the library basename without the `lib` prefix and `.so` suffix.
