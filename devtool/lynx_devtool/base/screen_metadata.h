// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_LYNX_DEVTOOL_BASE_SCREEN_METADATA_H_
#define DEVTOOL_LYNX_DEVTOOL_BASE_SCREEN_METADATA_H_

#include <string>
#include <vector>

namespace lynx {
namespace devtool {

enum class ScreenShotMode { LYNXVIEW = 0, FULLSCREEN };

struct ScreenMetadata {
  /* data */
  ScreenMetadata()
      : offset_top_(0),
        page_scale_factor_(1),
        device_width_(0),
        device_height_(0),
        scroll_off_set_x_(0),
        scroll_off_set_y_(0),
        timestamp_(0) {}
  float offset_top_;
  float page_scale_factor_;
  float device_width_;
  float device_height_;
  float scroll_off_set_x_;
  float scroll_off_set_y_;
  float timestamp_;
};

enum class ScreenshotType { JPEG = 0, PNG, WEBP, BITMAP };
struct ScreenshotRequest {
  ScreenshotRequest()
      : max_width_(0),
        max_height_(0),
        quality_(100),
        type_(ScreenshotType::JPEG),
        screen_scale_factor_(1.0) {}

  size_t max_width_;
  size_t max_height_;
  int quality_;
  ScreenshotType type_;
  float screen_scale_factor_;
  std::string format_;
  int every_nth_frame_;
};

}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_LYNX_DEVTOOL_BASE_SCREEN_METADATA_H_
