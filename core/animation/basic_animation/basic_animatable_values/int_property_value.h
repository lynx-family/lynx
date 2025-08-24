// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_ANIMATION_BASIC_ANIMATION_BASIC_ANIMATABLE_VALUES_INT_PROPERTY_VALUE_H_
#define CORE_ANIMATION_BASIC_ANIMATION_BASIC_ANIMATABLE_VALUES_INT_PROPERTY_VALUE_H_

#include <memory>

#include "core/animation/basic_animation/property_value.h"

namespace lynx {
namespace animation {
namespace basic {

class IntPropertyValue : public PropertyValue {
 public:
  explicit IntPropertyValue(int value) : value_(value) {}

  std::unique_ptr<PropertyValue> Clone() const override {
    return std::make_unique<IntPropertyValue>(value_);
  }

  ~IntPropertyValue() override = default;
  int GetIntValue() const { return value_; }

  void SetIntValue(int value) { value_ = value; }

  std::unique_ptr<PropertyValue> Interpolate(
      double progress,
      const std::unique_ptr<PropertyValue>& end_value) const override;

  size_t GetType() const override {
    return static_cast<size_t>(BasicPropertyValueType::Int);
  }

 protected:
  int value_ = 1;
};

}  // namespace basic
}  // namespace animation
}  // namespace lynx

#endif  // CORE_ANIMATION_BASIC_ANIMATION_BASIC_ANIMATABLE_VALUES_INT_PROPERTY_VALUE_H_
