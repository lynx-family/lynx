// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/animation/lynx_basic_animator/basic_property_value.h"

#include <memory>

#include "base/include/log/logging.h"

namespace lynx {
namespace animation {

std::unique_ptr<basic::PropertyValue> BasicFloatPropertyValue::Interpolate(
    double progress, const std::unique_ptr<PropertyValue>& end_value) const {
  DCHECK(static_cast<basic::PropertyValueType>(end_value->GetType()) ==
         basic::PropertyValueType::Float);
  return std::make_unique<BasicFloatPropertyValue>(
      float_value_ +
      (reinterpret_cast<BasicFloatPropertyValue*>(end_value.get())
           ->GetFloatValue() -
       float_value_) *
          progress);
}

}  // namespace animation
}  // namespace lynx
