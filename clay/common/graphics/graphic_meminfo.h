// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_COMMON_GRAPHICS_GRAPHIC_MEMINFO_H_
#define CLAY_COMMON_GRAPHICS_GRAPHIC_MEMINFO_H_

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "clay/gfx/rendering_backend.h"

namespace clay {
struct RasterCacheInfo {
  RasterCacheInfo(int64_t single_cache_size, int64_t single_cache_height,
                  int64_t single_cache_width,
                  std::string single_cache_color_type, int64_t layer_address,
                  int64_t cache_address, clay::GrImagePtr image)
      : single_cache_size(single_cache_size),
        single_cache_height(single_cache_height),
        single_cache_width(single_cache_width),
        single_cache_color_type(single_cache_color_type),
        layer_address(layer_address),
        cache_address(cache_address),
        image(image) {}
  int64_t single_cache_size;
  int64_t single_cache_height;
  int64_t single_cache_width;
  std::string single_cache_color_type;
  int64_t layer_address;
  int64_t cache_address;
  clay::GrImagePtr image;
};

}  // namespace clay

#endif  // CLAY_COMMON_GRAPHICS_GRAPHIC_MEMINFO_H_
