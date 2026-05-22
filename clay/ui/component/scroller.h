// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPONENT_SCROLLER_H_
#define CLAY_UI_COMPONENT_SCROLLER_H_

#include <memory>

#include "clay/gfx/animation/interpolator.h"
#include "clay/ui/component/scroller_animator.h"

namespace clay {

class Scroller {
 public:
  class Delegate {
   public:
    virtual void OnScrollUpdate(float scroll_offset) {}
    virtual void OnScrollAnimationStart(float start, float delta,
                                        int duration) {}
    virtual void OnScrollAnimationEnd() {}
  };

  static constexpr int DEFAULT_DURATION = 250;

  Scroller(Delegate* delegate, AnimationHandler* animation_handler,
           std::unique_ptr<Interpolator> interpolator);

  // If return false, it means there is no animation started.
  bool StartScroll(float start, float delta, int duration = DEFAULT_DURATION);
  void ScrollToImmediately(float offset);
  void AbortAnimation();

  bool IsScrolling() const { return animator_->Running(); }
  float GetCurrentOffset() const { return curr_offset_; }
  float GetStartOffset() const { return start_offset_; }
  float GetDuration() const { return duration_; }

 private:
  void StartAnimator();
  void StopAnimator();
  bool OnAnimation(int64_t frame_time);

  std::unique_ptr<Interpolator> interpolator_;
  std::unique_ptr<internal::ScrollerAnimator> animator_;
  Delegate* delegate_;

  int duration_ = 0;
  int64_t start_time_ = 0;
  float curr_offset_ = 0.f;
  float start_offset_ = 0.f;
  float delta_ = 0.f;
};

}  // namespace clay

#endif  // CLAY_UI_COMPONENT_SCROLLER_H_
