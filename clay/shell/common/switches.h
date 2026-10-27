// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <string_view>

#include "clay/common/settings.h"
#include "clay/fml/command_line.h"

#ifndef CLAY_SHELL_COMMON_SWITCHES_H_
#define CLAY_SHELL_COMMON_SWITCHES_H_

namespace clay {

// clang-format off
#ifndef DEF_SWITCHES_START
#define DEF_SWITCHES_START enum class Switch {
#endif
#ifndef DEF_SWITCH
#define DEF_SWITCH(swtch, flag, help) swtch,
#endif
#ifndef DEF_SWITCHES_END
#define DEF_SWITCHES_END Sentinel, } ;
#endif
// clang-format on

DEF_SWITCHES_START
DEF_SWITCH(CacheDirPath, "cache-dir-path",
           "Path to the cache directory. "
           "This is different from the persistent_cache_path in embedder.h, "
           "which is used for Skia shader cache.")
DEF_SWITCH(ICUDataFilePath, "icu-data-file-path", "Path to the ICU data file.")
DEF_SWITCH(ICUSymbolPrefix, "icu-symbol-prefix",
           "Prefix for the symbols representing ICU data linked into the "
           "Flutter library.")
DEF_SWITCH(ICUNativeLibPath, "icu-native-lib-path",
           "Path to the library file that exports the ICU data.")
DEF_SWITCH(EnableSoftwareRendering, "enable-software-rendering",
           "Enable rendering using the Skia software backend. This is useful "
           "when testing Flutter on emulators. By default, Flutter will "
           "attempt to either use OpenGL, Metal, or Vulkan.")
DEF_SWITCH(SkiaDeterministicRendering, "skia-deterministic-rendering",
           "Skips the call to SkGraphics::Init(), thus avoiding swapping out "
           "some Skia function pointers based on available CPU features. This "
           "is used to obtain 100% deterministic behavior in Skia rendering.")
DEF_SWITCH(ClayAssetsDir, "clay-assets-dir",
           "Path to the Clay assets directory.")
DEF_SWITCH(Help, "help", "Display this help text.")
DEF_SWITCH(TraceSkia, "trace-skia",
           "Trace Skia calls. This is useful when debugging the GPU thread."
           "By default, Skia tracing is not enabled to reduce the number of "
           "traced events")
DEF_SWITCH(TraceSkiaAllowlist, "trace-skia-allowlist",
           "Filters out all Skia trace event categories except those that are "
           "specified in this comma separated list.")
DEF_SWITCH(
    TraceAllowlist, "trace-allowlist",
    "Filters out all trace events except those that are specified in this "
    "comma separated list of allowed prefixes.")
DEF_SWITCH(DumpSkpOnShaderCompilation, "dump-skp-on-shader-compilation",
           "Automatically dump the skp that triggers new shader compilations. "
           "This is useful for writing custom ShaderWarmUp to reduce jank. "
           "By default, this is not enabled to reduce the overhead. ")
DEF_SWITCH(CacheSkSL, "cache-sksl",
           "Only cache the shader in SkSL instead of binary or GLSL. This "
           "should only be used during development phases. The generated SkSLs "
           "can later be used in the release build for shader precompilation "
           "at launch in order to eliminate the shader-compile jank.")
DEF_SWITCH(PurgePersistentCache, "purge-persistent-cache",
           "Remove all existing persistent cache. This is mainly for debugging "
           "purposes such as reproducing the shader compilation jank.")
DEF_SWITCH(UseTestFonts, "use-test-fonts",
           "Running tests that layout and measure text will not yield "
           "consistent results across various platforms. Enabling this option "
           "will make font resolution default to the Ahem test font on all "
           "platforms (See https://www.w3.org/Style/CSS/Test/Fonts/Ahem/). "
           "This option is only available on the desktop test shells.")
DEF_SWITCH(DisableAssetFonts, "disable-asset-fonts",
           "Prevents usage of any non-test fonts unless they were explicitly "
           "This option is only available on the desktop test shells.")
DEF_SWITCH(PrefetchedDefaultFontManager, "prefetched-default-font-manager",
           "Indicates whether the embedding started a prefetch of the "
           "default font manager before creating the engine.")
DEF_SWITCH(VerboseLogging, "verbose-logging",
           "By default, only errors are logged. This flag enabled logging at "
           "all severity levels. This is NOT a per shell flag and affect log "
           "levels for all shells in the process.")

DEF_SWITCH(ResourceCacheMaxBytesThreshold, "resource-cache-max-bytes-threshold",
           "The max bytes threshold of resource cache, or 0 for unlimited.")
DEF_SWITCH(EnableSkParagraph, "enable-skparagraph",
           "Selects the SkParagraph implementation of the text layout engine.")
DEF_SWITCH(
    MsaaSamples, "msaa-samples",
    "The minimum number of samples to require for multisampled anti-aliasing.  "
    "Setting this value to 0 or 1 disables MSAA. If it is not 0 or 1, it must "
    "be one of 2, 4, 8, or 16. However, if the GPU does not support the "
    "requested sampling value, MSAA will be disabled.")

DEF_SWITCH(EnableDefaultFocusRing, "enable-default-focus-ring",
           "Whether to use default focus ring. Not used by default.")

DEF_SWITCH(ImageTextureMaxMemoryLimit, "image-texture-max-memory-limit",
           "The max limit of image texture resource cached in clay.")

DEF_SWITCH(LowEndImageTextureMaxMemoryLimit,
           "low-end-image-texture-max-memory-limit",
           "The max limit of image texture resource cached in clay for "
           "low end terminal.")

DEF_SWITCH(EnableRendermodeSync, "enable-rendermode-sync",
           "Enable rendering in Android Render Thread.")

DEF_SWITCH(EnableStencilBuffer, "enable-stencil-buffer",
           "Enable stencil buffer in raster thread")

DEF_SWITCH(GpuResourceCacheMultiplier, "gpu-cache-multiplier",
           "The multiplier for the GPU resource cache size. The actual cache "
           "size is mul * 4 * w * h")

DEF_SWITCHES_END

const std::string_view FlagForSwitch(Switch swtch);

Settings SettingsFromCommandLine(const fml::CommandLine& command_line);

}  // namespace clay

#endif  // CLAY_SHELL_COMMON_SWITCHES_H_
