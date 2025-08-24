// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/animation/basic_animation/basic_animatable_values/float_property_value.h"

#include <memory>

#include "base/include/log/logging.h"

namespace lynx {
namespace animation {
namespace basic {

std::unique_ptr<PropertyValue> FloatPropertyValue::Interpolate(
    double progress, const std::unique_ptr<PropertyValue> &end_value) const {
  DCHECK(static_cast<BasicPropertyValueType>(end_value->GetType()) ==
         BasicPropertyValueType::Float);
  float start_float_value = GetFloatValue();
  float end_float_value =
      reinterpret_cast<FloatPropertyValue *>(end_value.get())->GetFloatValue();
  float res_float_value =
      start_float_value + (end_float_value - start_float_value) * progress;
  return std::make_unique<FloatPropertyValue>(res_float_value);
}

}  // namespace basic
}  // namespace animation
}  // namespace lynx
