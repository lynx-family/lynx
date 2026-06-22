# Windows Node-API Addons

The open-source Explorer Windows sample includes a `LynxNodeAPI` host module and native loader example for integrating a Node-API addon.

For the experimental capability model, SDK prerequisites, the role of the sample `LynxNodeAPIModule` in Explorer, and addon authoring guidance, see [Lynx Node-API Addons](../docs/lynx-node-api.md). This document only covers Windows-specific packaging.

## How To Integrate The Addon Into The App

On Windows, package the addon as a `.node` or `.dll` binary beside `lynx_explorer.exe`.

> Important (path/encoding/security)
>
> - The sample loader relies on Windows `LoadLibrary` default search behavior and uses the ANSI API (e.g. `LoadLibraryA`). If the executable/addon path contains non-ASCII characters, loading may fail. For the sample, prefer ASCII-only addon filenames and an ASCII-only install path.
> - Do **not** load addons from user-writable or untrusted directories (e.g. Downloads/temp folders) to avoid DLL search-order hijacking risks.
> - For production integrations, prefer a fixed installation directory and load by **absolute path**, using `LoadLibraryExW` (Unicode) and restricting the DLL search path (e.g. `SetDefaultDllDirectories` / `LOAD_LIBRARY_SEARCH_*`) instead of the default search order.

## How To Enable It For A Page

The current Windows Explorer sample only enables the Node-API example integration when the page URL query includes `enable_napi_addon=1` (or `enable_napi_addon=true`).

Example:

- `file://lynx?local://homepage.lynx.bundle?enable_napi_addon=1`

## How It Is Resolved

When JS calls `requireNodeAddon("<addon>")`, the loader tries common Windows candidates such as `<addon>.node` and `<addon>.dll`. `<addon>` should match the binary basename without the file extension.
