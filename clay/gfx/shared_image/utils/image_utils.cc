// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/shared_image/utils/image_utils.h"

#include <cstring>

namespace clay {

void CopyPlane(const uint8_t* src_y, int src_stride_y, uint8_t* dst_y,
               int dst_stride_y, int width, int height) {
  int y;
  if (width <= 0 || height == 0) {
    return;
  }
  // Negative height means invert the image.
  if (height < 0) {
    height = -height;
    dst_y = dst_y + (height - 1) * dst_stride_y;
    dst_stride_y = -dst_stride_y;
  }
  // Coalesce rows.
  if (src_stride_y == width && dst_stride_y == width) {
    width *= height;
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
                width);
    src_y += src_stride_y;
    dst_y += dst_stride_y;
  }
}
}  // namespace clay
