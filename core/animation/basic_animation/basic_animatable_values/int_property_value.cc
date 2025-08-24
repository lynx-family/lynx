// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/animation/basic_animation/basic_animatable_values/int_property_value.h"

#include <cmath>
#include <memory>

#include "base/include/log/logging.h"

namespace lynx {
namespace animation {
namespace basic {

std::unique_ptr<PropertyValue> IntPropertyValue::Interpolate(
    double progress, const std::unique_ptr<PropertyValue> &end_value) const {
  DCHECK(static_cast<BasicPropertyValueType>(end_value->GetType()) ==
         BasicPropertyValueType::Int);
  int start_int_value = GetIntValue();
  int end_int_value =
      reinterpret_cast<IntPropertyValue *>(end_value.get())->GetIntValue();
  int res_int_value = 0;
  if (start_int_value == end_int_value) {
    res_int_value = start_int_value;
  } else {
    double delta = static_cast<double>(end_int_value - start_int_value);
    if (delta < 0) {
      delta--;
    } else {
      delta++;
    }
    res_int_value =
        start_int_value + static_cast<int>(progress * nextafter(delta, 0));
  }

  return std::make_unique<IntPropertyValue>(res_int_value);
}

}  // namespace basic
}  // namespace animation
}  // namespace lynx
