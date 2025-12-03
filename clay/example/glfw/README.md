# Clay engine GLFW Example
## Description
This is an example of how to use Clay Engine on MacOSX or Windows environment.

In this example we are demonstrating rendering UI inside of the GUI library
[GLFW](https://www.glfw.org/).

## Running Instructions
This example was tested on MacOSX and Windows.

The example depends on GLFW:
* [GLFW](https://www.glfw.org/) - This can be installed with [Homebrew](https://brew.sh/) - `brew install glfw`

In order to **build** and **run** the example you should be able to go into root directory and run:
* MacOSX
```shell
tools/hab sync . -f --target clay
buildtools/gn/gn gen out/Default --args='enable_clay_standalone = true disable_visibility_hidden = true use_ndk_static_cxx = false enable_linker_map = false enable_clay = true is_headless = true skia_enable_flutter_defines = true skia_use_dng_sdk = false skia_use_sfntly = false skia_enable_pdf = false skia_enable_svg = true enable_svg = true skia_enable_skottie = true skia_use_x11 = false skia_use_wuffs = true skia_use_expat = true skia_use_fontconfig = false clay_enable_skshaper = true skia_use_icu = true skia_gl_standard = "" allow_deprecated_api_calls = true stripped_symbols = true is_official_build = true is_debug = false use_clang_static_analyzer = false enable_lto = false' --export-compile-commands
buildtools/ninja/ninja -C out/Default clay/example/glfw
./out/Default/clay_glfw
```

* Windows
```shell
tools\hab.ps1 sync . -f --target clay
buildtools\gn\gn.exe gen out\Default --args="enable_clay_standalone = true disable_visibility_hidden = true use_ndk_static_cxx = false  enable_linker_map = false enable_clay = true is_headless = true skia_enable_flutter_defines = true  skia_use_dng_sdk = false skia_use_sfntly = false skia_enable_pdf = false skia_enable_svg = true enable_svg = true skia_enable_skottie = true skia_use_x11 = false skia_use_wuffs = true skia_use_expat = true skia_use_fontconfig = false clay_enable_skshaper = true skia_use_icu = true allow_deprecated_api_calls = true stripped_symbols = true is_official_build = true is_debug = false use_clang_static_analyzer = false enable_lto = false enable_libcpp_abi_namespace_cr = true use_flutter_cxx = true enable_desktop_embeddings = true is_clang = true"
buildtools\ninja\ninja.exe -C out\Default\ clay\example\glfw
.\out\Default\clay_glfw.exe
```
