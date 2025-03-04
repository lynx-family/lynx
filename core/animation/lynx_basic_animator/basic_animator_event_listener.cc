// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/animation/lynx_basic_animator/basic_animator_event_listener.h"

#include <utility>

namespace lynx {
namespace animation {
namespace basic {

void BasicAnimatorEventListener::OnAnimationStart(const Animation& animation) {
  LOGI("Basic Animation Start. ");
  if (start_callback_) {
    return start_callback_();
  } else {
    LOGI("LynxBasicAnimator has no start callback! Please check!")
  }
}

void BasicAnimatorEventListener::OnAnimationEnd(const Animation& animation) {
  LOGI("Basic Animation End. ");
  if (end_callback_) {
    return end_callback_();
  } else {
    LOGI("LynxBasicAnimator has no start callback! Please check!")
  }
}

void BasicAnimatorEventListener::OnAnimationCancel(const Animation& animation) {
  LOGI("Basic Animation Cancel. ");
  if (cancel_callback_) {
    return cancel_callback_();
  } else {
    LOGI("LynxBasicAnimator has no start callback! Please check!")
  }
}

void BasicAnimatorEventListener::OnAnimationIteration(
    const Animation& animation) {
  LOGI("Basic Animation Iteration. ");
  if (iteration_callback_) {
    return iteration_callback_();
  } else {
    LOGI("LynxBasicAnimator has no start callback! Please check!")
  }
}

}  // namespace basic
}  // namespace animation
}  // namespace lynx
