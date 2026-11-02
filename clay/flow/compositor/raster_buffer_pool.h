// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_FLOW_COMPOSITOR_RASTER_BUFFER_POOL_H_
#define CLAY_FLOW_COMPOSITOR_RASTER_BUFFER_POOL_H_

#include <algorithm>
#include <cstdint>

#include "clay/flow/compositor/frame_types.h"

namespace clay::display {
struct RasterBufferSize {
  uint32_t width = 0, height = 0;
  bool operator==(const RasterBufferSize& b) const {
    return width == b.width && height == b.height;
  }
};

// Bound both alignment overhead and retained capacity as a segment shrinks.
// One additional bucket avoids reallocating when a scroll crosses a boundary.
inline constexpr uint32_t kRasterBufferHeightAlignment = 256;
inline constexpr uint32_t kMaxRasterBufferDimension = 16384;

inline RasterBufferSize RasterBufferAllocationSize(RasterBufferSize logical,
                                                   uint32_t maximum_height) {
  if (!logical.width || !logical.height || !maximum_height ||
      logical.width > kMaxRasterBufferDimension ||
      logical.height > maximum_height ||
      maximum_height > kMaxRasterBufferDimension)
    return {};
  const auto height = (logical.height + kRasterBufferHeightAlignment - 1) /
                      kRasterBufferHeightAlignment *
                      kRasterBufferHeightAlignment;
  return {logical.width, std::min(maximum_height, height)};
}

inline bool CanUseRasterBuffer(RasterBufferSize capacity,
                               RasterBufferSize logical,
                               uint32_t maximum_height) {
  const auto requested = RasterBufferAllocationSize(logical, maximum_height);
  return requested.width && capacity.width == logical.width &&
         capacity.height >= logical.height &&
         capacity.height <=
             std::min(maximum_height,
                      requested.height + kRasterBufferHeightAlignment);
}

inline PixelRect RasterBufferValidRect(RasterBufferSize capacity,
                                       RasterBufferSize logical,
                                       ImageOrigin origin) {
  if (!logical.width || !logical.height ||
      capacity.width > kMaxRasterBufferDimension ||
      capacity.height > kMaxRasterBufferDimension ||
      logical.width > capacity.width || logical.height > capacity.height)
    return {};
  // Raster draws from the canvas's top edge. A bottom-left image exposes that
  // content at the high end of its physical Y range; Display's existing flip
  // maps this source rectangle back to logical Y=0 without moving the segment.
  return {0,
          origin == ImageOrigin::kBottomLeft
              ? static_cast<int32_t>(capacity.height - logical.height)
              : 0,
          logical.width, logical.height};
}
}  // namespace clay::display
#endif  // CLAY_FLOW_COMPOSITOR_RASTER_BUFFER_POOL_H_
