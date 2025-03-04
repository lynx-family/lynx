// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_LYNX_ENV_CONFIG_H_
#define CORE_RENDERER_LYNX_ENV_CONFIG_H_

#include <memory>
#include <string>

#include "core/renderer/starlight/layout/layout_global.h"
#include "core/renderer/starlight/types/layout_unit.h"
#include "core/renderer/tasm/config.h"

namespace lynx {
namespace tasm {

class LynxEnvConfig {
 public:
  BASE_EXPORT LynxEnvConfig(float width, float height,
                            float layouts_unit_per_px,
                            double physical_pixels_per_layout_unit);
  BASE_EXPORT ~LynxEnvConfig() = default;

  float ScreenWidth() const { return screen_width_; }
  float ScreenHeight() const { return screen_height_; }
  const starlight::LayoutUnit& ViewportWidth() const { return viewport_width_; }
  const starlight::LayoutUnit& ViewportHeight() const {
    return viewport_height_;
  }
  void UpdateViewport(float width, SLMeasureMode width_mode_, float height,
                      SLMeasureMode height_mode) {
    viewport_width_ = width_mode_ == SLMeasureModeDefinite
                          ? starlight::LayoutUnit(width)
                          : starlight::LayoutUnit();
    viewport_height_ = height_mode == SLMeasureModeDefinite
                           ? starlight::LayoutUnit(height)
                           : starlight::LayoutUnit();
  }
  void UpdateScreenSize(float width, float height);
  float DefaultFontSize() const {
    return layouts_unit_per_px_ * DEFAULT_FONT_SIZE_DP;
  }
  float FontScale() const { return font_scale_; }
  bool FontScaleSpOnly() const { return font_scale_sp_only_; }
  void SetFontScale(float font_scale) { font_scale_ = font_scale; }
  void SetFontScaleSpOnly(bool font_scale_sp_only) {
    font_scale_sp_only_ = font_scale_sp_only;
  }

  float PageDefaultFontSize() const {
    return (font_scale_sp_only_ ? 1.f : FontScale()) * DefaultFontSize();
  }

  double PhysicalPixelsPerLayoutUnit() const {
    return physical_pixels_per_layout_unit_;
  }

  float LayoutsUnitPerPx() const { return layouts_unit_per_px_; }

 private:
  // The unit of this two values is layout unit.
  float screen_width_;
  float screen_height_;
  starlight::LayoutUnit viewport_width_;
  starlight::LayoutUnit viewport_height_;
  float font_scale_ = 1.f;
  bool font_scale_sp_only_ = false;
  // Currently, layout unit is equal to default unit used by platform
  // On iOS one layout unit equals to one ios point
  // On Android one layout unit equals to one physical pixel
  float layouts_unit_per_px_;
  double physical_pixels_per_layout_unit_;
};

}  // namespace tasm
};  // namespace lynx

#endif  // CORE_RENDERER_LYNX_ENV_CONFIG_H_
