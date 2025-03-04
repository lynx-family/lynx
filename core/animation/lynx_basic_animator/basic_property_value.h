// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_ANIMATION_LYNX_BASIC_ANIMATOR_BASIC_PROPERTY_VALUE_H_
#define CORE_ANIMATION_LYNX_BASIC_ANIMATOR_BASIC_PROPERTY_VALUE_H_

#include <memory>

#include "core/animation/basic_animation/property_value.h"

namespace lynx {
namespace animation {

class BasicFloatPropertyValue : public basic::PropertyValue {
 public:
  explicit BasicFloatPropertyValue(float value) : float_value_(value) {}

  std::unique_ptr<PropertyValue> Interpolate(
      double progress,
      const std::unique_ptr<PropertyValue>& end_value) const override;

  size_t GetType() const override {
    return static_cast<size_t>(basic::PropertyValueType::Float);
  }

  float GetFloatValue() const { return float_value_; }

 private:
  float float_value_{1.0};
};

}  // namespace animation
}  // namespace lynx

#endif  // CORE_ANIMATION_LYNX_BASIC_ANIMATOR_BASIC_PROPERTY_VALUE_H_
