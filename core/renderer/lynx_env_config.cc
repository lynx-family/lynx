// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/lynx_env_config.h"

#include "base/include/log/logging.h"
#include "base/trace/native/trace_event.h"
#include "core/renderer/trace/renderer_trace_event_def.h"

namespace lynx {
namespace tasm {

LynxEnvConfig::LynxEnvConfig(float width, float height,
                             float layouts_unit_per_px,
                             double physical_pixels_per_layout_unit) {
  DCHECK(layouts_unit_per_px == 1.0 || physical_pixels_per_layout_unit == 1.0);
  DCHECK(layouts_unit_per_px != 0 && physical_pixels_per_layout_unit != 0);
  screen_width_ = width;
  screen_height_ = height;
  layouts_unit_per_px_ = layouts_unit_per_px;
  physical_pixels_per_layout_unit_ = physical_pixels_per_layout_unit;
  vwbase_for_font_size_to_align_with_legacy_bug_ =
      viewport_width_.IsDefinite() ? viewport_width_
                                   : starlight::LayoutUnit(screen_width_);
  vhbase_for_font_size_to_align_with_legacy_bug_ =
      viewport_height_.IsDefinite() ? viewport_height_
                                    : starlight::LayoutUnit(screen_height_);
}

void LynxEnvConfig::UpdateScreenSize(float width, float height) {
  screen_width_ = width;
  screen_height_ = height;
  TRACE_EVENT(LYNX_TRACE_CATEGORY, LYNX_ENV_CONFIG_UPDATE_SCREEN_SIZE,
              [width, height](lynx::perfetto::EventContext ctx) {
                ctx.event()->add_debug_annotations("screen_width",
                                                   std::to_string(width));
                ctx.event()->add_debug_annotations("screen_height",
                                                   std::to_string(height));
              });
  vwbase_for_font_size_to_align_with_legacy_bug_ =
      viewport_width_.IsDefinite() ? viewport_width_
                                   : starlight::LayoutUnit(screen_width_);
  vhbase_for_font_size_to_align_with_legacy_bug_ =
      viewport_height_.IsDefinite() ? viewport_height_
                                    : starlight::LayoutUnit(screen_height_);
}

bool LynxEnvConfig::UpdateDevicePixelRatio(float device_pixel_ratio) {
  if (DevicePixelRatio() == device_pixel_ratio) {
    return false;
  }
  if (layouts_unit_per_px_ == 1.0f) {
    physical_pixels_per_layout_unit_ = device_pixel_ratio;
  } else {
    layouts_unit_per_px_ = device_pixel_ratio;
  }
  return true;
}

}  // namespace tasm
}  // namespace lynx
