// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/animation/transition_data.h"

namespace clay {

bool TransitionData::operator==(const TransitionData& rhs) const {
  return std::tie(duration, delay, property, timing_func) ==
         std::tie(rhs.duration, rhs.delay, rhs.property, rhs.timing_func);
}

}  // namespace clay
