# Android Node-API Addons

The open-source Explorer Android sample includes a `LynxNodeAPI` host module and native loader example for integrating a Node-API addon.

For the experimental capability model, SDK prerequisites, the role of the sample `LynxNodeAPIModule` in Explorer, and addon authoring guidance, see [Lynx Node-API Addons](../docs/lynx-node-api.md). This document only covers Android-specific packaging.

## How To Integrate The Addon Into The App

On Android, package the addon as `lib<addon>.so` so it ends up in the app's native library directory.

- Release integration: publish the addon through an Android `aar` that contains `jni/<abi>/lib<addon>.so`. Gradle will package the native library into the app automatically.
- Local integration: copy `lib<addon>.so` into a directory listed in `jniLibs.srcDirs`. In the open-source Explorer sample, the default manual-drop location is `explorer/android/lynx_explorer/src/main/jniLibs/<abi>/`.

`<abi>` is one of: `arm64-v8a`, `armeabi-v7a`, `x86_64`, etc.

If you only have addon binaries for a subset of ABIs, configure the Android build ABI list in:

- `explorer/android/gradle.properties`

Example:

```gradle
abiList=arm64-v8a
```

## How To Enable It For A Page

The current Android Explorer sample only enables the Node-API example integration when the page URL query includes `enable_napi_addon=1` (or `enable_napi_addon=true`).

Example:

- `file://lynx?local://homepage.lynx.bundle?enable_napi_addon=1`

## How It Is Resolved

When JS calls `requireNodeAddon("<addon>")`, the loader will try common candidates such as `lib<addon>.so` from the process default library search paths. `<addon>` should match the library basename without the `lib` prefix and `.so` suffix.
