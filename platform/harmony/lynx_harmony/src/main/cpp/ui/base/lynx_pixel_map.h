// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_BASE_LYNX_PIXEL_MAP_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_BASE_LYNX_PIXEL_MAP_H_

#include <multimedia/image_framework/image/pixelmap_native.h>
#include <native_drawing/drawing_types.h>

#include <cstdint>

namespace lynx {
namespace tasm {
namespace harmony {

class LynxPixelMap {
 public:
  LynxPixelMap(OH_PixelmapNative* bitmap, int32_t duration);
  ~LynxPixelMap();
  uint32_t Width() { return image_width_; };
  uint32_t Height() { return image_height_; };
  int32_t Duration() { return duration_; };
  OH_Drawing_PixelMap* DrawBitmap() { return draw_bitmap_; }
  OH_PixelmapNative* Bitmap() { return bitmap_; }

 private:
  OH_PixelmapNative* bitmap_{nullptr};
  OH_Drawing_PixelMap* draw_bitmap_{nullptr};
  uint32_t image_width_{0};
  uint32_t image_height_{0};
  int32_t duration_{0};
};

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_BASE_LYNX_PIXEL_MAP_H_
