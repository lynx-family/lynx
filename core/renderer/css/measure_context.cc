// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/css/measure_context.h"

#include "core/renderer/css/computed_css_style.h"
#include "core/renderer/lynx_env_config.h"

namespace lynx {
namespace tasm {
CssMeasureContext::CssMeasureContext(
    float screen_width, float layouts_unit_per_px,
    float physical_pixels_per_layout_unit, float root_font_size,
    float cur_font_size, const starlight::LayoutUnit& viewport_width,
    const starlight::LayoutUnit& viewport_height)
    : screen_width_(screen_width),
      layouts_unit_per_px_(layouts_unit_per_px),
      physical_pixels_per_layout_unit_(physical_pixels_per_layout_unit),
      root_node_font_size_(root_font_size),
      cur_node_font_size_(cur_font_size),
      viewport_width_(viewport_width),
      viewport_height_(viewport_height) {}

CssMeasureContext::CssMeasureContext(const tasm::LynxEnvConfig& config,
                                     float root_font_size, float cur_font_size)
    : screen_width_(config.ScreenWidth()),
      layouts_unit_per_px_(config.LayoutsUnitPerPx()),
      physical_pixels_per_layout_unit_(config.PhysicalPixelsPerLayoutUnit()),
      root_node_font_size_(root_font_size),
      cur_node_font_size_(cur_font_size),
      font_scale_(config.FontScale()),
      viewport_width_(config.ViewportWidth()),
      viewport_height_(config.ViewportHeight()),
      font_scale_sp_only_(config.FontScaleSpOnly()) {}

}  // namespace tasm
}  // namespace lynx
