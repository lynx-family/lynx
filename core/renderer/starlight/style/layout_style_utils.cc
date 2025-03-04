// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/starlight/style/layout_style_utils.h"

#include <cmath>

namespace lynx {
namespace starlight {

float LayoutStyleUtils::RoundValueToPixelGrid(
    const float value, const float physical_pixels_per_layout_unit_) {
  return std::roundf(value * physical_pixels_per_layout_unit_) /
         physical_pixels_per_layout_unit_;
}
}  // namespace starlight
}  // namespace lynx
