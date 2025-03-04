// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_ANIMATION_BASIC_ANIMATION_ANIMATION_FRAME_CALLBACK_H_
#define CORE_ANIMATION_BASIC_ANIMATION_ANIMATION_FRAME_CALLBACK_H_

#include <memory>

#include "base/include/fml/time/time_point.h"

namespace lynx {
namespace animation {
namespace basic {
class AnimationFrameCallback
    : public std::enable_shared_from_this<AnimationFrameCallback> {
 public:
  virtual ~AnimationFrameCallback() = default;

  virtual void DoAnimationFrame(const fml::TimePoint& frame_time) = 0;
};
}  // namespace basic
}  // namespace animation
}  // namespace lynx

#endif  // CORE_ANIMATION_BASIC_ANIMATION_ANIMATION_FRAME_CALLBACK_H_
