// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_ANIMATION_BASIC_ANIMATION_ANIMATION_EVENT_LISTENER_H_
#define CORE_ANIMATION_BASIC_ANIMATION_ANIMATION_EVENT_LISTENER_H_

#include <memory>

#include "core/animation/basic_animation/basic_animation.h"

namespace lynx {
namespace animation {
namespace basic {

class AnimationEventListener
    : public std::enable_shared_from_this<AnimationEventListener> {
 public:
  virtual ~AnimationEventListener() = default;

  void OnAnimationEvent(const Animation& animation, Animation::EventType type) {
    switch (type) {
      case Animation::EventType::Start:
        OnAnimationStart(animation);
        break;
      case Animation::EventType::End:
        OnAnimationEnd(animation);
        break;
      case Animation::EventType::Cancel:
        OnAnimationCancel(animation);
        break;
      case Animation::EventType::Iteration:
        OnAnimationIteration(animation);
        break;
      default:
        LOGE("There is no event of this type");
        break;
    }
  }

  virtual void OnAnimationStart(const Animation&) = 0;
  virtual void OnAnimationEnd(const Animation&) = 0;
  virtual void OnAnimationCancel(const Animation&) = 0;
  virtual void OnAnimationIteration(const Animation&) = 0;
};
}  // namespace basic
}  // namespace animation
}  // namespace lynx

#endif  // CORE_ANIMATION_BASIC_ANIMATION_ANIMATION_EVENT_LISTENER_H_
