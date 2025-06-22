// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/ui/base/lynx_pixel_map.h"

#include <native_drawing/drawing_pixel_map.h>

namespace lynx {
namespace tasm {
namespace harmony {

LynxPixelMap::~LynxPixelMap() {
  if (bitmap_) {
    OH_PixelmapNative_Release(bitmap_);
  }
  if (draw_bitmap_) {
    OH_Drawing_PixelMapDissolve(draw_bitmap_);
  }
}

LynxPixelMap::LynxPixelMap(OH_PixelmapNative* bitmap, int32_t duration)
    : bitmap_(bitmap), duration_{duration} {
  OH_Pixelmap_ImageInfo* pixel_map_info = nullptr;
  OH_PixelmapImageInfo_Create(&pixel_map_info);
  OH_PixelmapNative_GetImageInfo(bitmap_, pixel_map_info);
  OH_PixelmapImageInfo_GetWidth(pixel_map_info, &image_width_);
  OH_PixelmapImageInfo_GetHeight(pixel_map_info, &image_height_);
  OH_PixelmapImageInfo_Release(pixel_map_info);
  draw_bitmap_ = OH_Drawing_PixelMapGetFromOhPixelMapNative(bitmap_);
}

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
