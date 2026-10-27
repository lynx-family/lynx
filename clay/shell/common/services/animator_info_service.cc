// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/shell/common/services/animator_info_service.h"

namespace clay {

std::shared_ptr<AnimatorInfoService> AnimatorInfoService::Create() {
  return std::make_shared<AnimatorInfoService>();
}

/// Target time for the latest frame. See also `Shell::OnAnimatorBeginFrame`
/// for when this time gets updated.
fml::TimePoint AnimatorInfoService::GetLatestFrameTargetTime() const {
  return latest_frame_target_time_.load();
}

void AnimatorInfoService::SetLatestFrameTargetTime(
    fml::TimePoint frame_target_time) {
  latest_frame_target_time_.store(frame_target_time);
}

}  // namespace clay
