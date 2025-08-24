// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_ANIMATION_BASIC_ANIMATION_BASIC_ANIMATABLE_VALUES_FLOAT_PROPERTY_VALUE_H_
#define CORE_ANIMATION_BASIC_ANIMATION_BASIC_ANIMATABLE_VALUES_FLOAT_PROPERTY_VALUE_H_

#include <memory>

#include "core/animation/basic_animation/property_value.h"

namespace lynx {
namespace animation {
namespace basic {

class FloatPropertyValue : public PropertyValue {
 public:
  explicit FloatPropertyValue(float value) : value_(value) {}

  std::unique_ptr<PropertyValue> Clone() const override {
    return std::make_unique<FloatPropertyValue>(value_);
  }

  ~FloatPropertyValue() override = default;

  float GetFloatValue() const { return value_; }

  void SetFloatValue(float value) { value_ = value; }

  std::unique_ptr<PropertyValue> Interpolate(
      double progress,
      const std::unique_ptr<PropertyValue>& end_value) const override;

  size_t GetType() const override {
    return static_cast<size_t>(BasicPropertyValueType::Float);
  }

 protected:
  float value_ = 1.0f;
};

}  // namespace basic
}  // namespace animation
}  // namespace lynx

#endif  // CORE_ANIMATION_BASIC_ANIMATION_BASIC_ANIMATABLE_VALUES_FLOAT_PROPERTY_VALUE_H_
