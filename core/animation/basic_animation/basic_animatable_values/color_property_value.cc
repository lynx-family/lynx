// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/animation/basic_animation/basic_animatable_values/color_property_value.h"

#include <cmath>
#include <memory>

#include "base/include/log/logging.h"

namespace lynx {
namespace animation {
namespace basic {

std::unique_ptr<PropertyValue> ColorPropertyValue::Interpolate(
    double progress, const std::unique_ptr<PropertyValue> &end_value) const {
  DCHECK(static_cast<BasicPropertyValueType>(end_value->GetType()) ==
         BasicPropertyValueType::Color);
  uint32_t start_color_value = GetColorValue();
  uint32_t end_color_value =
      reinterpret_cast<ColorPropertyValue *>(end_value.get())->GetColorValue();

  float startA = ((start_color_value >> 24) & 0xff) / 255.0f;
  float startR = ((start_color_value >> 16) & 0xff) / 255.0f;
  float startG = ((start_color_value >> 8) & 0xff) / 255.0f;
  float startB = ((start_color_value) & 0xff) / 255.0f;

  float endA = ((end_color_value >> 24) & 0xff) / 255.0f;
  float endR = ((end_color_value >> 16) & 0xff) / 255.0f;
  float endG = ((end_color_value >> 8) & 0xff) / 255.0f;
  float endB = ((end_color_value) & 0xff) / 255.0f;

  startR = static_cast<float>(pow(startR, color_space_constant_));
  startG = static_cast<float>(pow(startG, color_space_constant_));
  startB = static_cast<float>(pow(startB, color_space_constant_));

  endR = static_cast<float>(pow(endR, color_space_constant_));
  endG = static_cast<float>(pow(endG, color_space_constant_));
  endB = static_cast<float>(pow(endB, color_space_constant_));

  float a = startA + progress * (endA - startA);
  float b = startB + progress * (endB - startB);
  float r = startR + progress * (endR - startR);
  float g = startG + progress * (endG - startG);

  a = a * 255.0f;
  r = static_cast<float>(pow(r, 1.0 / color_space_constant_)) * 255.0f;
  g = static_cast<float>(pow(g, 1.0 / color_space_constant_)) * 255.0f;
  b = static_cast<float>(pow(b, 1.0 / color_space_constant_)) * 255.0f;
  uint32_t result_color_value = static_cast<uint32_t>(round(a)) << 24 |
                                static_cast<uint32_t>(round(r)) << 16 |
                                static_cast<uint32_t>(round(g)) << 8 |
                                static_cast<uint32_t>(round(b));

  return std::make_unique<ColorPropertyValue>(result_color_value);
}

}  // namespace basic
}  // namespace animation
}  // namespace lynx
