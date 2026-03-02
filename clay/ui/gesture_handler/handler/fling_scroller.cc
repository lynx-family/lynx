// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/gesture_handler/handler/fling_scroller.h"

#include <memory>

#include "base/include/thread/timed_task.h"
#include "clay/gfx/animation/animation_handler.h"
#include "clay/gfx/animation/animator.h"
#include "clay/gfx/animation/cubic_bezier_interpolator.h"
#include "clay/gfx/animation/interpolator.h"
#include "clay/gfx/animation/timing_function_data.h"
#include "clay/gfx/animation/value_animator.h"
#include "clay/gfx/image/image_data.h"
#include "clay/public/style_types.h"
#include "core/base/threading/vsync_monitor.h"

namespace clay {

FlingScroller::FlingScroller(AnimationHandler* animation_handler) {
  last_time_ = 0;
  TimingFunctionData timing_function_data;
  timing_function_data.timing_func = ClayTimingFunctionType::kCubicBezier;
  timing_function_data.x1 = 0.25f;
  timing_function_data.y1 = 0.1f;
  timing_function_data.x2 = 0.25f;
  timing_function_data.y2 = 1.0f;
  animator_ = std::make_unique<ValueAnimator>();
  animator_->SetInterpolator(Interpolator::Create(timing_function_data));
  animator_->SetDuration(1800);
  //  animator_->SetFillMode(ValueAnimator::kForwards);
  animator_->AddListener(this);
  animator_->AddUpdateListener(this);
  animator_->SetAnimationHandler(animation_handler);
}

FlingScroller::~FlingScroller() = default;

void FlingScroller::OnAnimationStart(Animator& animation) {}

void FlingScroller::OnAnimationEnd(Animator& animation) {
  current_fling_state_ = static_cast<uint8_t>(FlingState::IDLE);
  callback_(static_cast<uint8_t>(FlingState::IDLE), 0.f, 0.f);
}

void FlingScroller::OnAnimationCancel(Animator& animation) {
  current_fling_state_ = static_cast<uint8_t>(FlingState::IDLE);
  callback_(static_cast<uint8_t>(FlingState::IDLE), 0.f, 0.f);
}
void FlingScroller::OnAnimationRepeat(Animator& animation) {}

void FlingScroller::OnAnimationUpdate(ValueAnimator& animation) {
  if (last_time_ == 0) {
    last_time_ = fml::TimePoint::Now().ToEpochDelta().ToMilliseconds();
    return;
  }
  auto current_time = fml::TimePoint::Now().ToEpochDelta().ToMilliseconds();
  int64_t delta_time = current_time - last_time_;
  auto progress = animation.GetAnimatedFraction();
  float delta_x = (velocity_x_ * (1.0f - progress) / 1000) * delta_time;
  float delta_y = (velocity_y_ * (1.0f - progress) / 1000) * delta_time;
  current_fling_state_ = static_cast<uint8_t>(FlingState::FLING);
  callback_(static_cast<uint8_t>(FlingState::FLING), -delta_x, -delta_y);
  last_time_ = fml::TimePoint::Now().ToEpochDelta().ToMilliseconds();
}

void FlingScroller::Start(float velocity_x, float velocity_y,
                          FlingCallback callback) {
  if (current_fling_state_ == static_cast<uint8_t>(FlingState::FLING)) {
    Stop();
  }
  last_time_ = 0;
  callback_ = callback;
  velocity_x_ = velocity_x;
  velocity_y_ = velocity_y;
  animator_->Start();
}

bool FlingScroller::IsIdle() {
  return current_fling_state_ == static_cast<uint8_t>(FlingState::IDLE);
}

void FlingScroller::Stop() {
  if (animator_) {
    animator_->End();
  }
  current_fling_state_ = static_cast<uint8_t>(FlingState::IDLE);
}

}  // namespace clay
