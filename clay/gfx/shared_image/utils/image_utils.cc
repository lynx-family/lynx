// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/shared_image/utils/image_utils.h"

#include <cstddef>
#include <cstring>
#include <limits>

namespace clay {

void CopyPlane(const uint8_t* src_y, int src_stride_y, uint8_t* dst_y,
               int dst_stride_y, int width, int height) {
  int y;
  if (!src_y || !dst_y || width <= 0 || height == 0 ||
      height == std::numeric_limits<int>::min()) {
    return;
  }
  const int rows = height < 0 ? -height : height;
  const auto stride_fits = [rows](int stride) {
    const int64_t signed_stride = stride;
    const uint64_t magnitude =
        signed_stride < 0 ? -signed_stride : signed_stride;
    return rows <= 1 ||
           magnitude <=
               static_cast<uint64_t>(std::numeric_limits<ptrdiff_t>::max()) /
                   static_cast<uint64_t>(rows - 1);
  };
  if (!stride_fits(src_stride_y) || !stride_fits(dst_stride_y)) {
    return;
  }
  // Negative height means invert the image.
  if (height < 0) {
    height = -height;
    if (dst_stride_y == std::numeric_limits<int>::min()) {
      return;
    }
    dst_y = dst_y + static_cast<ptrdiff_t>(height - 1) * dst_stride_y;
    dst_stride_y = -dst_stride_y;
  }
  // Coalesce rows.
  size_t row_bytes = static_cast<size_t>(width);
  if (src_stride_y == width && dst_stride_y == width &&
      static_cast<size_t>(height) <=
          std::numeric_limits<size_t>::max() / row_bytes) {
    row_bytes *= static_cast<size_t>(height);
    height = 1;
    src_stride_y = dst_stride_y = 0;
  }
  // Nothing to do.
  if (src_y == dst_y && src_stride_y == dst_stride_y) {
    return;
  }

  // Copy plane
  for (y = 0; y < height; ++y) {
    std::memcpy(static_cast<void*>(dst_y), static_cast<const void*>(src_y),
                row_bytes);
    if (y + 1 < height) {
      src_y += src_stride_y;
      dst_y += dst_stride_y;
    }
  }
}
}  // namespace clay
