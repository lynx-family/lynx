// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPONENT_SCROLLER_ANIMATOR_H_
#define CLAY_UI_COMPONENT_SCROLLER_ANIMATOR_H_

#include <functional>
#include <utility>

#include "clay/gfx/animation/animation_handler.h"

namespace clay {
namespace internal {

class ScrollerAnimator : public AnimationHandler::AnimationFrameCallback {
 public:
  ScrollerAnimator(const std::function<bool(int64_t frame_time)>& on_animation,
                   AnimationHandler* handler);
  ~ScrollerAnimator() override;

  bool DoAnimationFrame(int64_t frame_time) override;

  void Start();
  void Stop();

  bool Running() const { return running_; }

 private:
  std::function<bool(int64_t frame_time)> on_animation_;
  AnimationHandler* handler_ = nullptr;
  bool running_ = false;
};

}  // namespace internal
}  // namespace clay

#endif  // CLAY_UI_COMPONENT_SCROLLER_ANIMATOR_H_
