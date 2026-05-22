// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_COMMON_SERVICES_ANIMATOR_INFO_SERVICE_H_
#define CLAY_SHELL_COMMON_SERVICES_ANIMATOR_INFO_SERVICE_H_

#include <atomic>
#include <memory>

#include "base/include/fml/time/time_point.h"
#include "clay/common/service/service.h"

namespace clay {

class AnimatorInfoService
    : public clay::Service<AnimatorInfoService, clay::Owner::kUI,
                           clay::ServiceFlags::kMultiThread> {
 public:
  static std::shared_ptr<AnimatorInfoService> Create();

  /// Target time for the latest frame. See also `Shell::OnAnimatorBeginFrame`
  /// for when this time gets updated.
  fml::TimePoint GetLatestFrameTargetTime() const;

  void SetLatestFrameTargetTime(fml::TimePoint frame_target_time);

 private:
  std::atomic<fml::TimePoint> latest_frame_target_time_;
};

}  // namespace clay

#endif  // CLAY_SHELL_COMMON_SERVICES_ANIMATOR_INFO_SERVICE_H_
