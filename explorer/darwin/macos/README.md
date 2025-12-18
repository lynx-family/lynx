# Building Lynx Explorer for macOS
This document provides instructions for building the Lynx Explorer macOS app from source.

## System Requirements
- 100GB or more of disk space
- Git/Python3(>=3.9) installed
- Node.js(>=18) installed

## Install Dependencies
The following dependencies are needed:

- Xcode(>=15.0)

### Xcode

Lynx requires Xcode 15.0 or later. It is recommended to keep Xcode up to date. You can install or update it on the [App Store](https://developer.apple.com/xcode/).

- Open Xcode->Settings->Locations, to make sure the `Command Line Tools` are configured
- You can run `xcode-select -p` in the terminal, and if it prints a correct path, it's configure successfully.

## Get the Source Code

### Pull the Repository

Pull the code from the Github repository.

```shell
git clone https://github.com/lynx-family/lynx.git
```

### Install Third-party Library

Run the following commands from the root of the repository to install the dependencies.

```shell
cd lynx
source tools/envsetup.sh
tools/hab sync . --target clay
```

## Build Lynx Explorer

Run the following commands from the root of the repository to build the LynxExplorer app.

```shell
buildtools/gn/gn gen out/Default --args='desktop_enable_embedder_layer = true enable_clay_standalone = true disable_visibility_hidden = true use_ndk_static_cxx = false enable_linker_map = false enable_clay = true is_headless = true skia_enable_flutter_defines = true skia_use_dng_sdk = false skia_use_sfntly = false skia_enable_pdf = false skia_enable_svg = true enable_svg = true skia_enable_skottie = true skia_use_x11 = false skia_use_wuffs = true skia_use_expat = true skia_use_fontconfig = false clay_enable_skshaper = true skia_use_icu = true skia_gl_standard = "" skia_use_metal = true shell_enable_metal = true allow_deprecated_api_calls = true stripped_symbols = true is_official_build = true use_clang_static_analyzer = false enable_lto = false enable_lepusng_worklet = true enable_napi_binding = true enable_inspector = true jsengine_type="quickjs" use_flutter_cxx = false is_debug = false' --ide=xcode
buildtools/ninja/ninja -C out/Default explorer
```
Or, you can run the following command to build the `LynxSDK` if you need.
This will generate `lynx_sdk_macos_${target_cpu}.zip` in `out/Default` directory.
```shell
buildtools/ninja/ninja -C out/Default platform/darwin/macos:package_sdk
```

## Run and Debug

After the build is completed, LynxExplorer.app is located in the out/Default directory. Double-click to run it.

### Debug with Xcode

Run the `gn gen` command with param `--ide=xcode`, `all.xcodeproj` will be generated in dictionary `out/Default`.
1. Open `all.xcodeproj` by Xcode
2. Select `lynx_explorer` to build and run.
