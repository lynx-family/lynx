// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/style/layout_animation_data.h"

namespace lynx {
namespace starlight {
BaseLayoutAnimationData::BaseLayoutAnimationData()
    : duration(0), delay(0), property(AnimationPropertyType::kOpacity){};

void BaseLayoutAnimationData::Reset() {
  duration = 0;
  delay = 0;
  property = starlight::AnimationPropertyType::kOpacity;
  timing_function.timing_func = starlight::TimingFunctionType::kLinear;
}

}  // namespace starlight
}  // namespace lynx
