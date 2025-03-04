// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SERVICES_STARLIGHT_STANDALONE_CORE_INCLUDE_STARLIGHT_CONFIG_H_
#define CORE_SERVICES_STARLIGHT_STANDALONE_CORE_INCLUDE_STARLIGHT_CONFIG_H_

#include "core/services/starlight_standalone/core/include/starlight.h"
#include "core/services/starlight_standalone/core/include/starlight_value.h"

namespace starlight {

class LayoutConfig {
 public:
  LayoutConfig(){};
  LayoutConfig(int32_t screen_width, int32_t screen_height, float scale = 1.f,
               float density = 1.f)
      : screen_width_(screen_width),
        screen_height_(screen_height),
        scale_(scale),
        density_(density) {}
  LayoutConfig(int32_t screen_width, int32_t screen_height,
               float viewport_width, float viewport_height,
               StarlightMeasureMode viewport_width_mode,
               StarlightMeasureMode viewport_height_mode, float scale,
               float density)
      : screen_width_(screen_width),
        screen_height_(screen_height),
        viewport_width_(viewport_width),
        viewport_height_(viewport_height),
        viewport_width_mode_(viewport_width_mode),
        viewport_height_mode_(viewport_height_mode),
        scale_(scale),
        density_(density) {}

  void UpdateViewport(float width, StarlightMeasureMode width_mode,
                      float height, StarlightMeasureMode height_mode) {
    viewport_width_ = width;
    viewport_width_mode_ = width_mode;
    viewport_height_ = height;
    viewport_height_mode_ = height_mode;
  }

  int32_t ScreenWidth() const { return screen_width_; }
  int32_t ScreenHeight() const { return screen_height_; }
  float ViewportWidth() const { return viewport_width_; }
  float ViewportHeight() const { return viewport_height_; }
  StarlightMeasureMode ViewportWidthMode() const {
    return viewport_width_mode_;
  }
  StarlightMeasureMode ViewportHeightMode() const {
    return viewport_height_mode_;
  }
  float Scale() const { return scale_; }
  float Density() const { return density_; }

 private:
  int32_t screen_width_;
  int32_t screen_height_;
  float viewport_width_;
  float viewport_height_;
  StarlightMeasureMode viewport_width_mode_;
  StarlightMeasureMode viewport_height_mode_;
  float scale_;
  float density_;
};

}  // namespace starlight

#endif  // CORE_SERVICES_STARLIGHT_STANDALONE_CORE_INCLUDE_STARLIGHT_CONFIG_H_
