// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_GESTURE_HANDLER_HANDLER_FLING_SCROLLER_H_
#define CLAY_UI_GESTURE_HANDLER_HANDLER_FLING_SCROLLER_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

#include "clay/gfx/animation/animation_handler.h"
#include "clay/gfx/animation/animator_listener.h"
#include "clay/gfx/animation/value_animator.h"
#include "clay/public/value.h"
#include "clay/ui/gesture_handler/handler/base_gesture_handler.h"

namespace clay {

class GestureDetector;
struct GestureCallback;

class LynxContext;
class GestureArenaMember;

using FlingCallback = std::function<void(uint8_t, float, float)>;

class FlingScroller : public std::enable_shared_from_this<FlingScroller>,
                      public AnimatorListener,
                      public AnimatorUpdateListener {
 public:
  // Enum class for representing different types of gestures.
  enum class FlingState : uint8_t {
    IDLE = 0,
    FLING = 1,
  };
  explicit FlingScroller(AnimationHandler* animation_handler);

  ~FlingScroller();

  void Start(float velocity_x, float velocity_y, FlingCallback callback);

  void Stop();

  bool IsIdle();

  void OnAnimationStart(Animator& animation) override;
  void OnAnimationEnd(Animator& animation) override;
  void OnAnimationCancel(Animator& animation) override;
  void OnAnimationRepeat(Animator& animation) override;
  void OnAnimationUpdate(ValueAnimator& animation) override;

 private:
  // std::shared_ptr<animation::basic::LynxBasicAnimator>
  // fling_scroller_animator_;
  int64_t last_time_;
  uint8_t current_fling_state_{0};
  std::unique_ptr<ValueAnimator> animator_;
  FlingCallback callback_;
  float velocity_x_;
  float velocity_y_;
};

}  // namespace clay

#endif  // CLAY_UI_GESTURE_HANDLER_HANDLER_FLING_SCROLLER_H_
