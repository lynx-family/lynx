// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/animation/animation_data.h"

namespace clay {

bool AnimationData::operator==(const AnimationData& rhs) const {
  return std::tie(name, timing_func, iteration_count, fill_mode, duration,
                  delay, direction, play_state) ==
         std::tie(rhs.name, rhs.timing_func, rhs.iteration_count, rhs.fill_mode,
                  rhs.duration, rhs.delay, rhs.direction, rhs.play_state);
}

}  // namespace clay
