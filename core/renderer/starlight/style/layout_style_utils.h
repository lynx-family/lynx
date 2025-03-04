// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_STARLIGHT_STYLE_LAYOUT_STYLE_UTILS_H_
#define CORE_RENDERER_STARLIGHT_STYLE_LAYOUT_STYLE_UTILS_H_

#include "core/renderer/starlight/types/layout_attribute.h"

namespace lynx {
namespace starlight {

class LayoutStyleUtils {
 public:
  static float RoundValueToPixelGrid(
      const float value, const float physical_pixels_per_layout_unit);

  static bool ListComponentTypeIsRow(ListComponentType type) {
    return type == ListComponentType::HEADER ||
           type == ListComponentType::FOOTER ||
           type == ListComponentType::LIST_ROW;
  }
};
}  // namespace starlight
}  // namespace lynx

#endif  // CORE_RENDERER_STARLIGHT_STYLE_LAYOUT_STYLE_UTILS_H_
