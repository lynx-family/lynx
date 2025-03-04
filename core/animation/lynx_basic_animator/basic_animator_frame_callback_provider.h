// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_ANIMATION_LYNX_BASIC_ANIMATOR_BASIC_ANIMATOR_FRAME_CALLBACK_PROVIDER_H_
#define CORE_ANIMATION_LYNX_BASIC_ANIMATOR_BASIC_ANIMATOR_FRAME_CALLBACK_PROVIDER_H_

#include <memory>

#include "core/animation/basic_animation/animation_frame_callback_provider.h"

namespace lynx {

namespace shell {
class VSyncMonitor;
}

namespace animation {
namespace basic {
class BasicAnimatorFrameCallbackProvider
    : public AnimationFrameCallbackProvider {
 public:
  explicit BasicAnimatorFrameCallbackProvider(
      const std::shared_ptr<shell::VSyncMonitor> &vsync_monitor)
      : vsync_monitor_(vsync_monitor) {}

  ~BasicAnimatorFrameCallbackProvider() override = default;

  void RequestNextFrame(
      base::MoveOnlyClosure<void, const fml::TimePoint &> callback) override;

  std::shared_ptr<shell::VSyncMonitor> GetVSyncMonitor();

 private:
  std::shared_ptr<shell::VSyncMonitor> vsync_monitor_{nullptr};
};
}  // namespace basic
}  // namespace animation
}  // namespace lynx

#endif  // CORE_ANIMATION_LYNX_BASIC_ANIMATOR_BASIC_ANIMATOR_FRAME_CALLBACK_PROVIDER_H_
