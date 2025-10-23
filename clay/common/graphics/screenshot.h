// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_COMMON_GRAPHICS_SCREENSHOT_H_
#define CLAY_COMMON_GRAPHICS_SCREENSHOT_H_

#include <functional>
#include <memory>
#include <optional>
#include <string>

#include "base/include/fml/task_runner.h"
#include "clay/gfx/rendering_backend.h"

namespace clay {

using ExternalScreenshotCallback = std::function<GrImagePtr()>;

struct ScreenMetadata {
  float device_width_ = 0.f;
  float device_height_ = 0.f;
  float timestamp_ = 0.f;
  float page_scale_factor_ = 1.f;
};

using ScreenshotCallback =
    std::function<void(GrDataPtr, const ScreenMetadata&)>;

enum class ScreenshotType { JPEG = 0, PNG, WEBP, BITMAP };
struct ScreenshotRequest {
  size_t page_width_ = 0;
  size_t page_height_ = 0;
  size_t max_width_ = 0;
  size_t max_height_ = 0;
  int quality_ = 100;
  ScreenshotType type_ = ScreenshotType::BITMAP;
  float screen_scale_factor_ = 1.f;
  // If `is_sync` is false, then the `callback_` is necessary.
  bool is_sync_ = true;
  lynx::fml::RefPtr<lynx::fml::TaskRunner> task_runner_;
  std::optional<ScreenshotCallback> callback_;
  std::string format_;
  int every_nth_frame_;
  uint32_t background_color_ = 0xffffffff;
};

}  // namespace clay

#endif  // CLAY_COMMON_GRAPHICS_SCREENSHOT_H_
