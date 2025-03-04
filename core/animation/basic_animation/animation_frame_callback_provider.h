// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_ANIMATION_BASIC_ANIMATION_ANIMATION_FRAME_CALLBACK_PROVIDER_H_
#define CORE_ANIMATION_BASIC_ANIMATION_ANIMATION_FRAME_CALLBACK_PROVIDER_H_

#include "base/include/closure.h"
#include "base/include/fml/time/time_point.h"

namespace lynx {
namespace animation {
namespace basic {
class AnimationFrameCallbackProvider {
 public:
  virtual ~AnimationFrameCallbackProvider() = default;

  // It must invoke callback async, otherwise it will encounter deadlock.
  virtual void RequestNextFrame(
      base::MoveOnlyClosure<void, const fml::TimePoint&> callback) = 0;
};
}  // namespace basic
}  // namespace animation
}  // namespace lynx

#endif  // CORE_ANIMATION_BASIC_ANIMATION_ANIMATION_FRAME_CALLBACK_PROVIDER_H_
