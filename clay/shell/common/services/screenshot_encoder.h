// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_COMMON_SERVICES_SCREENSHOT_ENCODER_H_
#define CLAY_SHELL_COMMON_SERVICES_SCREENSHOT_ENCODER_H_

#include <memory>

#include "clay/common/graphics/screenshot.h"
#include "clay/gfx/rendering_backend.h"
#include "clay/shell/common/screenshot_utils.h"

namespace clay {

struct ScreenshotEncodeResult {
  GrDataPtr data = nullptr;
  clay::ScreenMetadata metadata;
};

class ScreenshotEncoder {
 public:
  static ScreenshotEncodeResult ScaleAndEncode(
      ScreenshotData screenshot, const clay::ScreenshotRequest& request);

#ifndef ENABLE_SKITY
  static ScreenshotEncodeResult Encode(const SkPixmap& pixmap,
                                       const clay::ScreenshotRequest& request);
#else
  static ScreenshotEncodeResult Encode(std::shared_ptr<skity::Pixmap> pixmap,
                                       const clay::ScreenshotRequest& request);
#endif

 private:
#ifndef ENABLE_SKITY
  static SkPixmap ScaleImage(ScreenshotData screenshot, SkBitmap& scaled_bitmap,
                             const clay::ScreenshotRequest& request);
#else
  static std::shared_ptr<skity::Pixmap> ScaleImage(
      ScreenshotData screenshot, const clay::ScreenshotRequest& request);
#endif
};

}  // namespace clay

#endif  // CLAY_SHELL_COMMON_SERVICES_SCREENSHOT_ENCODER_H_
