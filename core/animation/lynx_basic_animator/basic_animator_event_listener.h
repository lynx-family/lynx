// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_ANIMATION_LYNX_BASIC_ANIMATOR_BASIC_ANIMATOR_EVENT_LISTENER_H_
#define CORE_ANIMATION_LYNX_BASIC_ANIMATOR_BASIC_ANIMATOR_EVENT_LISTENER_H_

#include <functional>
#include <memory>

#include "core/animation/basic_animation/animation_event_listener.h"

namespace lynx {
namespace animation {
namespace basic {

class BasicAnimatorEventListener : public AnimationEventListener {
 public:
  using EventCallback = std::function<void()>;

  BasicAnimatorEventListener() = default;
  ~BasicAnimatorEventListener() = default;

  void OnAnimationStart(const Animation&) override;
  void OnAnimationEnd(const Animation&) override;
  void OnAnimationCancel(const Animation&) override;
  void OnAnimationIteration(const Animation&) override;

  void RegisterStartCallback(const EventCallback& cb) { start_callback_ = cb; }

  void RegisterCancelCallback(const EventCallback& cb) {
    cancel_callback_ = cb;
  }

  void RegisterIterationCallback(const EventCallback& cb) {
    iteration_callback_ = cb;
  }

  void RegisterEndCallback(const EventCallback& cb) { end_callback_ = cb; }

 private:
  EventCallback start_callback_;
  EventCallback cancel_callback_;
  EventCallback iteration_callback_;
  EventCallback end_callback_;
};

}  // namespace basic
}  // namespace animation
}  // namespace lynx

#endif  // CORE_ANIMATION_LYNX_BASIC_ANIMATOR_BASIC_ANIMATOR_EVENT_LISTENER_H_
