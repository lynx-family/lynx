# Building Lynx Explorer for Windows
This document provides instructions for building the Lynx Explorer Windows app from source.

## System Requirements
- 100GB or more of disk space
- Git/Python3(>=3.9) installed
- Node.js(>=18) installed

## Install Dependencies
The following dependencies are needed:

- Visual Studio(2022 recommended)

## Environment Setup
You must set the following environment variables:
```shell
DEPOT_TOOLS_WIN_TOOLCHAIN=0
GYP_MSVS_OVERRIDE_PATH="C:\Program Files (x86)\Microsoft Visual Studio\2022\Community" # (or your location for Visual Studio)
WINDOWSSDKDIR="C:\Program Files (x86)\Windows Kits\10" # (or your location for Windows Kits)
```

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
.\tools\envsetup.ps1
.\tools\hab.ps1 sync . --target clay
```

## Build Lynx Explorer

Run the following commands from the root of the repository to build the LynxExplorer app.

```shell
$PSNativeCommandArgumentPassing = 'Legacy' # Compatibility with PowerShell 7 if you are using it

.\buildtools\gn\gn.exe gen out\Default --args='desktop_enable_embedder_layer = true enable_clay_standalone = true disable_visibility_hidden = true use_ndk_static_cxx = false  enable_linker_map = false enable_clay = true is_headless = true skia_enable_flutter_defines = true  skia_use_dng_sdk = false skia_use_sfntly = false skia_enable_pdf = false skia_enable_svg = true enable_svg = true skia_enable_skottie = true skia_use_x11 = false skia_use_wuffs = true skia_use_expat = true skia_use_fontconfig = false clay_enable_skshaper = true skia_use_icu = true allow_deprecated_api_calls = true stripped_symbols = true is_official_build = true enable_lto = false is_clang = true enable_lepusng_worklet = true enable_napi_binding = true is_debug = false enable_inspector = true jsengine_type = \"quickjs\"' --ide=vs
.\buildtools\ninja\ninja.exe -C out\Default explorer
```
Or, you can run the following command to build the `LynxSDK` if you need.
This will generate `lynx_sdk_windows_${target_cpu}.zip` in `out\Default` directory.
```shell
.\buildtools\ninja\ninja.exe -C out\Default platform\windows:package_sdk
```

## Run and Debug

After the build is completed, the `lynx_explorer.exe` is located in the out\Default\lynx_explorer directory. Double-click to run it.

### Debug with VS

Run the `gn gen` command with param `--ide=vs`, `all.sln` will be generated in dictionary `out\Default`.
1. Open `all.sln` by Visual Studio
2. Select `lynx_explorer` as Startup Project.
3. Run and Debug.
