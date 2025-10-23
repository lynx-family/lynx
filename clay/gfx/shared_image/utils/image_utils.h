// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_SHARED_IMAGE_UTILS_IMAGE_UTILS_H_
#define CLAY_GFX_SHARED_IMAGE_UTILS_IMAGE_UTILS_H_

#include <cstdint>

namespace clay {

void CopyPlane(const uint8_t* src_y, int src_stride_y, uint8_t* dst_y,
               int dst_stride_y, int width, int height);

}  // namespace clay

#endif  // CLAY_GFX_SHARED_IMAGE_UTILS_IMAGE_UTILS_H_
