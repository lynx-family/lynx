// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/component/scroller_animator.h"

#include <functional>

#include "clay/fml/logging.h"

namespace clay {
namespace internal {

ScrollerAnimator::ScrollerAnimator(
    const std::function<bool(int64_t frame_time)>& on_animation,
    AnimationHandler* handler)
    : on_animation_(on_animation), handler_(handler) {
  FML_DCHECK(on_animation_);
  FML_DCHECK(handler_);
}

ScrollerAnimator::~ScrollerAnimator() { Stop(); }

bool ScrollerAnimator::DoAnimationFrame(int64_t frame_time) {
  return on_animation_(frame_time);
}

void ScrollerAnimator::Start() {
  if (running_) {
    return;
  }
  running_ = true;
  handler_->AddAnimationFrameCallback(this, 0);
}

void ScrollerAnimator::Stop() {
  if (!running_) {
    return;
  }
  running_ = false;
  handler_->RemoveCallback(this);
}

}  // namespace internal
}  // namespace clay
