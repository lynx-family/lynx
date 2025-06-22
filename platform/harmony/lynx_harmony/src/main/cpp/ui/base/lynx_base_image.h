// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_BASE_LYNX_BASE_IMAGE_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_BASE_LYNX_BASE_IMAGE_H_

#include <memory>
#include <vector>

#include "platform/harmony/lynx_harmony/src/main/cpp/ui/base/lynx_pixel_map.h"

namespace lynx {
namespace tasm {
namespace harmony {
class LynxBaseImage {
 public:
  LynxBaseImage(std::vector<std::unique_ptr<LynxPixelMap>> pixelmap_data,
                uint32_t frame_count);
  uint32_t Width() { return width_; };
  uint32_t Height() { return height_; };
  uint32_t FrameCount() { return frame_count_; };
  int32_t LoopDuration() { return loop_duration_; };
  int32_t FrameDuration(int32_t frame_number);
  bool isAnimImage() { return frame_count_ > 1; };
  LynxPixelMap* FirstFrame();
  LynxPixelMap* FramePixelMap(int32_t frame_number);

  std::vector<std::unique_ptr<LynxPixelMap>> pixelmap_data_;

 private:
  uint32_t frame_count_{0};
  uint32_t width_{0};
  uint32_t height_{0};
  int32_t loop_duration_{0};
};

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_BASE_LYNX_BASE_IMAGE_H_
