// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_ANIMATION_TRANSITION_DATA_H_
#define CLAY_GFX_ANIMATION_TRANSITION_DATA_H_

#include "clay/gfx/animation/timing_function_data.h"

namespace clay {

struct TransitionData {
  int duration = 0;
  int delay = 0;
  ClayAnimationPropertyType property = ClayAnimationPropertyType::kNone;
  TimingFunctionData timing_func;
  bool operator==(const TransitionData& rhs) const;
};

}  // namespace clay

#endif  // CLAY_GFX_ANIMATION_TRANSITION_DATA_H_
