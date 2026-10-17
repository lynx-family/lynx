<!-- cspell:ignore libvk -->
# Desktop Skity software rendering

Build with `tools/gn_tools/gn.py --enable-clay --enable-skity --headless --enable-software-rendering` and the usual platform arguments.

SwiftShader uses ANGLE Vulkan on CPU without a hardware fallback and returns top-down premultiplied RGBA8888 through both callbacks. Skia retains native N32.

Ship these runtimes and `vk_swiftshader_icd.json` beside the library containing ANGLE. Preserve the JSON driver path.

| Platform | Loader | Driver |
| --- | --- | --- |
| macOS | `libvulkan.dylib` | `libvk_swiftshader.dylib` |
| Windows | `vulkan-1.dll` | `vk_swiftshader.dll` |
| Linux | `libvulkan.so.1` | `libvk_swiftshader.so` |

Copy pixels during the callback to retain them. V2 reports the format and takes precedence, even when rejecting a frame; unbinding restores the original callback. Bind before creating the view. Zero-initialize Clay configurations and set `struct_size`; the C++ v2 virtual delegates to the original virtual by default.

Build and run `swiftshader_software_unittests` on macOS or Windows with the runtimes beside the binary. Tests cover pixels, resize, callback compatibility and renderer/display lifetimes. macOS Canvas composition is not yet supported by this software backend.
