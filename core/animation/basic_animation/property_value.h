// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_ANIMATION_BASIC_ANIMATION_PROPERTY_VALUE_H_
#define CORE_ANIMATION_BASIC_ANIMATION_PROPERTY_VALUE_H_

#include <cstddef>
#include <memory>

namespace lynx {
namespace animation {
namespace basic {

enum class PropertyValueType { Int = 0, Float, Opacity, Color };

class PropertyValue {
 public:
  virtual ~PropertyValue() = default;

  virtual std::unique_ptr<PropertyValue> Interpolate(
      double progress,
      const std::unique_ptr<PropertyValue>& end_value) const = 0;

  virtual size_t GetType() const = 0;
};
}  // namespace basic
}  // namespace animation
}  // namespace lynx

#endif  // CORE_ANIMATION_BASIC_ANIMATION_PROPERTY_VALUE_H_
