// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/component/scroller.h"

#include <algorithm>
#include <utility>

#include "clay/fml/logging.h"

namespace clay {

Scroller::Scroller(Delegate* delegate, AnimationHandler* animation_handler,
                   std::unique_ptr<Interpolator> interpolator)
    : interpolator_(std::move(interpolator)), delegate_(delegate) {
  FML_DCHECK(delegate);
  FML_DCHECK(animation_handler);
  animator_ = std::make_unique<internal::ScrollerAnimator>(
      [this](int64_t frame_time) { return OnAnimation(frame_time); },
      animation_handler);
}

bool Scroller::StartScroll(float start, float delta, int duration) {
  if (delta == 0) {
    return false;
  }
  if (duration == 0) {
    ScrollToImmediately(start + delta);
    return false;
  }
  // Assign start_time_ during `OnAnimation`.
  start_time_ = 0;
  duration_ = duration;
  start_offset_ = start;
  delta_ = delta;
  StartAnimator();
  delegate_->OnScrollAnimationStart(start, delta, duration);
  return true;
}

void Scroller::AbortAnimation() { StopAnimator(); }

void Scroller::StartAnimator() {
  if (IsScrolling()) {
    return;
  }
  animator_->Start();
}

void Scroller::StopAnimator() {
  if (!IsScrolling()) {
    return;
  }
  animator_->Stop();
}

bool Scroller::OnAnimation(int64_t frame_time) {
  if (start_time_ == 0) {
    start_time_ = frame_time;
  }
  int elapsed = frame_time - start_time_;
  bool finished = elapsed >= duration_;
  float t = std::clamp<float>(static_cast<float>(elapsed) / duration_, 0, 1);
  float q = interpolator_->Interpolate(t);
  float target = start_offset_ + q * delta_;
  curr_offset_ = target;
  delegate_->OnScrollUpdate(target);

  if (finished) {
    StopAnimator();
    delegate_->OnScrollAnimationEnd();
  }
  return finished;
}

void Scroller::ScrollToImmediately(float offset) {
  StopAnimator();
  if (delegate_) {
    delegate_->OnScrollUpdate(offset);
  }
}

}  // namespace clay
