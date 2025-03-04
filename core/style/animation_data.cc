// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/style/animation_data.h"

#include "core/renderer/starlight/style/default_layout_style.h"
#include "core/style/default_computed_style.h"

namespace lynx {
namespace starlight {
AnimationData::AnimationData()
    : duration(DefaultComputedStyle::DEFAULT_LONG),
      delay(DefaultComputedStyle::DEFAULT_LONG),
      iteration_count(1),
      fill_mode(AnimationFillModeType::kNone),
      direction(AnimationDirectionType::kNormal),
      play_state(AnimationPlayStateType::kRunning) {}

}  // namespace starlight
}  // namespace lynx
