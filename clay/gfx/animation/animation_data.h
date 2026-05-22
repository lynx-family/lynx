// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_ANIMATION_ANIMATION_DATA_H_
#define CLAY_GFX_ANIMATION_ANIMATION_DATA_H_

#include <string>

#include "clay/gfx/animation/timing_function_data.h"
#include "clay/public/clay.h"

namespace clay {

struct AnimationParams {
  ClayEventType event_type;
  const char* animation_name;
};

struct AnimationData {
  AnimationData() = default;
  ~AnimationData() = default;

  std::string name;
  int duration = 0;
  int delay = 0;
  TimingFunctionData timing_func;
  int iteration_count = 0;
  ClayAnimationFillModeType fill_mode = ClayAnimationFillModeType::kNone;
  ClayAnimationDirectionType direction = ClayAnimationDirectionType::kNormal;
  ClayAnimationPlayStateType play_state = ClayAnimationPlayStateType::kRunning;

  bool operator==(const AnimationData& rhs) const;
};

}  // namespace clay

#endif  // CLAY_GFX_ANIMATION_ANIMATION_DATA_H_
