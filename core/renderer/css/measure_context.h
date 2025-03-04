// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_CSS_MEASURE_CONTEXT_H_
#define CORE_RENDERER_CSS_MEASURE_CONTEXT_H_

#include "core/renderer/starlight/layout/layout_global.h"
#include "core/renderer/starlight/types/layout_constraints.h"
#include "core/renderer/tasm/config.h"

// TODO(liting):move to .cc
#include "core/renderer/starlight/types/nlength.h"

namespace lynx {
namespace tasm {
class LynxEnvConfig;
struct PropertiesResolvingStatus;

class CssMeasureContext {
 public:
  CssMeasureContext(const tasm::LynxEnvConfig& config, float root_font_size,
                    float cur_font_size);
  CssMeasureContext(float screen_width, float layouts_unit_per_px,
                    float physical_pixels_per_layout_unit, float root_font_size,
                    float cur_font_size,
                    const starlight::LayoutUnit& viewport_width_,
                    const starlight::LayoutUnit& viewport_height_);
  ~CssMeasureContext() {}
  float screen_width_;
  float layouts_unit_per_px_;
  float physical_pixels_per_layout_unit_;
  float root_node_font_size_;
  float cur_node_font_size_;
  float font_scale_ = tasm::Config::DefaultFontScale();
  starlight::LayoutUnit viewport_width_;
  starlight::LayoutUnit viewport_height_;
  bool font_scale_sp_only_ = false;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_CSS_MEASURE_CONTEXT_H_
