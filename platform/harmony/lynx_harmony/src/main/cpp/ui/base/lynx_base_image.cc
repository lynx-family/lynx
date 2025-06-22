// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/ui/base/lynx_base_image.h"

#include <utility>

namespace lynx {
namespace tasm {
namespace harmony {

LynxBaseImage::LynxBaseImage(
    std::vector<std::unique_ptr<LynxPixelMap>> pixelmap_data,
    uint32_t frame_count)
    : pixelmap_data_(std::move(pixelmap_data)), frame_count_(frame_count) {
  if (!pixelmap_data_.empty()) {
    width_ = pixelmap_data_[0]->Width();
    height_ = pixelmap_data_[0]->Height();
  }
  for (const auto& pixelmap : pixelmap_data_) {
    loop_duration_ += pixelmap->Duration();
  }
}

LynxPixelMap* LynxBaseImage::FirstFrame() {
  if (!pixelmap_data_.empty()) {
    return pixelmap_data_[0].get();
  }
  return nullptr;
}

int32_t LynxBaseImage::FrameDuration(int32_t frame_number) {
  if (frame_number < pixelmap_data_.size()) {
    return pixelmap_data_[frame_number]->Duration();
  }
  return 0;
}

LynxPixelMap* LynxBaseImage::FramePixelMap(int32_t frame_number) {
  if (frame_number < pixelmap_data_.size()) {
    return pixelmap_data_[frame_number].get();
  }
  return nullptr;
}

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
