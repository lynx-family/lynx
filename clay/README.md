# Clay

`Clay` is a high-performance, feature-rich, and multi-platform consistent rendering runtime.
It implements a complete rendering pipeline, including elements, events, animation and graphics,
accessibility support, plugin architecture, file and network I/O and so on. It is committed to
providing developers in different cross-platform scenarios with rendering capabilities featuring
"high performance, multi-platform consistency, and easy to integrate", helping multi-platform
developers accelerate the innovation and iteration of cross-platform solutions and enabling users
to have a consistent and excellent experience on any device.

## Try Clay

### Environment Setup

The project use [habitat](https://github.com/lynx-family/habitat) to manage dependencies.
You can install habitat by following the instructions on the habitat repository.

Also use [ninja](https://ninja-build.org/) to build project.

### Build and Run
To learn about running example, see [example](example/glfw/README.md)


## Credits

Clay use the following third-party libraries.
We appreciate the efforts of the developers and the open-source community behind these projects.

- [libcxx](https://github.com/llvm/llvm-project/libcxx)
- [libcxxabi](https://github.com/llvm/llvm-project/libcxxabi)
- [abseil-cpp](https://github.com/abseil/abseil-cpp)
- [ANGLE](https://github.com/google/angle)
- [astc-encoder](https://github.com/ARM-software/astc-encoder)
- [wayland](https://wayland.freedesktop.org/)
- [expat](http://sourceforge.net/projects/expat)
- [icu](https://github.com/unicode-org/icu)
- [harfbuzz](https://github.com/harfbuzz/harfbuzz)
- [libjpeg-turbo](https://fuchsia.googlesource.com/third_party/libjpeg-turbo.git)
- [libpng](https://flutter.googlesource.com/third_party/libpng.git)
- [libwebp](https://chromium.googlesource.com/webm/libwebp.git)
- [skia](https://github.com/google/skia)
- [skity](https://github.com/lynx-family/skity)
- [swiftshader](https://swiftshader.googlesource.com/SwiftShader.git)
- [wuffs](https://skia.googlesource.com/external/github.com/google/wuffs.git)
- [VulkanMemoryAllocator](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator)
- [vulkan-deps](https://chromium.googlesource.com/vulkan-deps.git)
- [Vulkan-Headers](https://github.com/KhronosGroup/Vulkan-Headers)
- [Vulkan-Loader](https://github.com/KhronosGroup/Vulkan-Loader)
- [Vulkan-Tools](https://github.com/KhronosGroup/Vulkan-Tools)
- [SPIRV-Tools](https://github.com/KhronosGroup/SPIRV-Tools)
- [SPIRV-Headers](https://github.com/KhronosGroup/SPIRV-Headers)
- [SPIRV-Cross](https://github.com/KhronosGroup/SPIRV-Cross)
- [glslang](https://github.com/KhronosGroup/glslang)
- [zlib](https://chromium.googlesource.com/chromium/src/third_party/zlib)
- [boringssl](https://boringssl.googlesource.com/boringssl.git)
- [GLM](https://github.com/g-truc/glm)
- [FreeType](https://www.freetype.org/)
- [nanopng](https://gitlab.com/TSnake41/nanopng)
- [GLFW](https://www.glfw.org/)
- [gtest-parallel](https://github.com/google/gtest-parallel)
- [googletest](https://github.com/google/googletest)
- [google/benchmark](https://github.com/google/benchmark)

## Referenced API Design and Implementations
The design of some APIs and some implementations in Clay have been inspired by and referenced from the following outstanding projects.
- [flutter engine](https://github.com/flutter/engine)
- [chromium](https://chromium.googlesource.com/chromium/)

## License
Clay is Apache licensed, as found in the [LICENSE](../LICENSE) file.
