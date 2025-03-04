// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/lynx_env_config.h"

#include "base/trace/native/trace_event.h"
#include "core/base/lynx_trace_categories.h"

namespace lynx {
namespace tasm {

LynxEnvConfig::LynxEnvConfig(float width, float height,
                             float layouts_unit_per_px,
                             double physical_pixels_per_layout_unit) {
  screen_width_ = width;
  screen_height_ = height;
  layouts_unit_per_px_ = layouts_unit_per_px;
  physical_pixels_per_layout_unit_ = physical_pixels_per_layout_unit;
}

void LynxEnvConfig::UpdateScreenSize(float width, float height) {
  screen_width_ = width;
  screen_height_ = height;
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "UpdateScreenSize",
              [width, height](lynx::perfetto::EventContext ctx) {
                ctx.event()->add_debug_annotations("screen_width",
                                                   std::to_string(width));
                ctx.event()->add_debug_annotations("screen_height",
                                                   std::to_string(height));
              });
}

}  // namespace tasm
}  // namespace lynx
