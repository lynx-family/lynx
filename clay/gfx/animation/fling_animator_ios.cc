// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <algorithm>
#include <cmath>

#include "base/include/fml/time/time_point.h"
#include "clay/fml/logging.h"
#include "clay/gfx/animation/fling_animator.h"

namespace clay {
namespace {
// Flutter's normal iOS scroll simulation uses drag = 0.135 per second,
// approximating UIScrollView's normal deceleration rate (0.998 per ms).
const double kDragLog = std::log(0.135);
}  // namespace

FlingAnimator::FlingAnimator() {
  friction_ = 1.f;
  SetValueThreshold(GetValueThreshold());
}

void FlingAnimator::InitParams() {}

void FlingAnimator::SetDevicePixelRatio(float pixel_ratio) {
  SetMinimumVisibleChange(1.f / pixel_ratio);
}

void FlingAnimator::SetFriction(float friction) { friction_ = friction; }

float FlingAnimator::GetFriction() { return friction_; }

float FlingAnimator::GetDistance() { return distance_; }

bool FlingAnimator::UpdateValueAndVelocity(int64_t delta_time) {
  if (!IsRunning()) {
    return false;
  }
  FML_DCHECK(last_frame_time_ >= start_time_);
  const double drag_log = kDragLog * friction_;
  const double exponent =
      drag_log * ((last_frame_time_ - start_time_) / 1000.0);
  velocity_ = start_velocity_ * std::exp(exponent);
  value_ = start_value_ + start_velocity_ * std::expm1(exponent) / drag_log;
  value_ = std::clamp(value_, GetMinValue(), GetMaxValue());
  // Stop at the sampled position, without jumping to the asymptotic endpoint.
  // Preserve the residual velocity for the existing boundary/bounce handoff.
  return IsAtEquilibrium(value_, velocity_);
}

float FlingAnimator::GetAcceleration(float value, float velocity) {
  return velocity * kDragLog * friction_;
}

bool FlingAnimator::IsAtEquilibrium(float value, float velocity) {
  return value >= GetMaxValue() || value <= GetMinValue() ||
         std::abs(velocity) < velocity_threshold_;
}

void FlingAnimator::SetValueThreshold(float threshold) {
  // Flutter allows one visible pixel of movement over 50 ms at rest.
  velocity_threshold_ = threshold / kThresholdMultiplier / 0.050f;
}

void FlingAnimator::FlingInitialize() {
  FML_DCHECK(friction_ > 0.f);
  start_velocity_ = velocity_;
  start_time_ = fml::TimePoint::Now().ToEpochDelta().ToMilliseconds();
  distance_ = -start_velocity_ / (kDragLog * friction_);
  final_value_ = start_value_ + distance_;
}

}  // namespace clay
