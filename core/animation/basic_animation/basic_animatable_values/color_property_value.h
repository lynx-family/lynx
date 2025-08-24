// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_ANIMATION_BASIC_ANIMATION_BASIC_ANIMATABLE_VALUES_COLOR_PROPERTY_VALUE_H_
#define CORE_ANIMATION_BASIC_ANIMATION_BASIC_ANIMATABLE_VALUES_COLOR_PROPERTY_VALUE_H_

#include <memory>

#include "core/animation/basic_animation/property_value.h"

namespace lynx {
namespace animation {
namespace basic {

class ColorPropertyValue : public PropertyValue {
 public:
  explicit ColorPropertyValue(uint32_t value) : value_(value) {}

  std::unique_ptr<PropertyValue> Clone() const override {
    return std::make_unique<ColorPropertyValue>(value_);
  }

  ~ColorPropertyValue() override = default;

  uint32_t GetColorValue() const { return value_; }

  void SetColorValue(uint32_t value) { value_ = value; }

  void SetColorSRGBSpace() { color_space_constant_ = 2.2; }

  void SetColorLinearSpace() { color_space_constant_ = 1.0; }

  std::unique_ptr<PropertyValue> Interpolate(
      double progress,
      const std::unique_ptr<PropertyValue>& end_value) const override;

  size_t GetType() const override {
    return static_cast<size_t>(BasicPropertyValueType::Color);
  }

 protected:
  double color_space_constant_ = 1.0;
  uint32_t value_ = 0x0;
};

}  // namespace basic
}  // namespace animation
}  // namespace lynx

#endif  // CORE_ANIMATION_BASIC_ANIMATION_BASIC_ANIMATABLE_VALUES_COLOR_PROPERTY_VALUE_H_
