// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/animation/animator.h"

namespace clay {

void Animator::Pause() {
  if (IsStarted() && !paused_) {
    paused_ = true;
    std::forward_list<AnimatorPauseListener*> listeners = pause_listeners_;
    for (auto listener : listeners) {
      listener->OnAnimationPause(*this);
    }
  }
}

void Animator::Resume() {
  if (paused_) {
    paused_ = false;
    std::forward_list<AnimatorPauseListener*> listeners = pause_listeners_;
    for (auto listener : listeners) {
      listener->OnAnimationResume(*this);
    }
  }
}

int64_t Animator::GetTotalDuration() {
  int64_t duration = GetDuration();
  if (duration == kDurationInfinite) {
    return kDurationInfinite;
  } else {
    return GetStartDelay() + duration;
  }
}

}  // namespace clay
