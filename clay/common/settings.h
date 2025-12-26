// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_COMMON_SETTINGS_H_
#define CLAY_COMMON_SETTINGS_H_

#include <fcntl.h>

#include <chrono>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "base/include/closure.h"
#include "base/include/fml/time/time_point.h"
#include "base/include/fml/unique_fd.h"
#include "build/build_config.h"
#include "clay/fml/mapping.h"

namespace clay {

// TODO(26783): Deprecate all the "path" struct members in favor of the
// callback that generates the mapping from these paths.
using MappingCallback = std::function<std::unique_ptr<fml::Mapping>(void)>;
using Mappings = std::vector<std::unique_ptr<const fml::Mapping>>;
using MappingsCallback = std::function<Mappings(void)>;

struct Settings {
  Settings();

  Settings(const Settings& other);

  ~Settings();

  std::string temp_directory_path;
  // Isolate settings
  bool trace_skia = false;
  std::vector<std::string> trace_allowlist;
  std::optional<std::vector<std::string>> trace_skia_allowlist;
  bool dump_skp_on_shader_compilation = false;
  bool cache_sksl = false;
  bool purge_persistent_cache = false;

  // Font settings
  bool use_test_fonts = false;

  bool use_asset_fonts = true;

  // Indicates whether the embedding started a prefetch of the default font
  // manager before creating the engine.
  bool prefetched_default_font_manager = false;

  // Selects the SkParagraph implementation of the text layout engine.
  bool enable_skparagraph = false;

  // Data set by platform-specific embedders for use in font initialization.
  uint32_t font_initialization_data = 0;

  // Engine settings
  bool enable_software_rendering = false;
  bool skia_deterministic_rendering_on_cpu = false;
#ifndef NDEBUG
  bool verbose_logging = true;
#else
  bool verbose_logging = false;
#endif

  // The icu_initialization_required setting does not have a corresponding
  // switch because it is intended to be decided during build time, not runtime.
  // Some companies apply source modification here because their build system
  // brings its own ICU data files.
  bool icu_initialization_required = true;
  std::string icu_data_path;
  MappingCallback icu_mapper;

  // Assets settings
  fml::UniqueFD::element_type assets_dir =
      fml::UniqueFD::traits_type::InvalidValue();
  std::string assets_path;

  // This data will be available to the isolate immediately on launch via the
  // PlatformDispatcher.getPersistentIsolateData callback. This is meant for
  // information that the isolate cannot request asynchronously (platform
  // messages can be used for that purpose). This data is held for the lifetime
  // of the shell and is available on isolate restarts in the shell instance.
  // Due to this, the buffer must be as small as possible.
  std::shared_ptr<const fml::Mapping> persistent_isolate_data;

  // Max bytes threshold of resource cache, or 0 for unlimited.
  size_t resource_cache_max_bytes_threshold = 0;

  /// The minimum number of samples to require in multipsampled anti-aliasing.
  ///
  /// Setting this value to 0 or 1 disables MSAA.
  /// If it is not 0 or 1, it must be one of 2, 4, 8, or 16. However, if the
  /// GPU does not support the requested sampling value, MSAA will be disabled.
  uint8_t msaa_samples = 0;

  static bool ShouldEnableStencilBuffer() { return enable_stencil_buffer; }
  static void SetStencilBuffer(bool value) { enable_stencil_buffer = value; }

  static bool ShouldUseSDFTForText() { return use_sdft_for_text; }
  static void SetUseSDFTForText(bool value) { use_sdft_for_text = value; }

  // Whether to use default focus ring.
  bool enable_default_focus_ring = false;

  // Whether to show performance overlay.
  bool enable_performance_overlay = false;

  // Whether create clay ui task runner.
  bool enable_clay_ui_thread = false;

  // clay memory cache related parameters
  int image_texture_cache_max_limit = -1;
  int low_end_image_texture_cache_max_limit = -1;

  // Whether to use synchronous compositor.
  bool enable_sync_compositor = false;
  int gpu_resource_cache_multiplier = 1;

  // Should be in range of [1,3]
  int buffer_size_for_sync_compositor = 2;

 private:
  static bool enable_stencil_buffer;
  static bool use_sdft_for_text;
};

}  // namespace clay

#endif  // CLAY_COMMON_SETTINGS_H_
