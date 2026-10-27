// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/animation/fling_animator.h"

#include <algorithm>

#include "base/include/fml/time/time_point.h"
#include "clay/fml/logging.h"

namespace clay {
namespace {
constexpr int kNBSamples = 100;
float kSplinePosition[kNBSamples + 1];
constexpr float kStartTension = 0.5f;
constexpr float kEndTension = 1.0f;
const float kDecelerationRate = std::log(0.75) / std::log(0.9);
constexpr float kInflexion = 0.35f;
constexpr float kGravityEarth = 9.80665f;
constexpr float P1 = kStartTension * kInflexion;
constexpr float P2 = 1.0f - kEndTension * (1.0f - kInflexion);
}  // namespace

FlingAnimator::FlingAnimator() {
  SetValueThreshold(GetValueThreshold());
  physical_coeff_ = ComputeDeceleration(friction_);
  InitParams();
}

void FlingAnimator::InitParams() {
  static bool param_setup = false;
  if (param_setup) {
    return;
  }
  float x_min = 0.0f;
  for (int i = 0; i < kNBSamples; i++) {
    const float alpha = static_cast<float>(i) / kNBSamples;
    float x_max = 1.0f;
    float x, tx, coeff;
    while (true) {
      x = x_min + (x_max - x_min) / 2.0f;
      coeff = 3.0f * x * (1.0f - x);
      tx = coeff * ((1.0f - x) * P1 + x * P2) + x * x * x;
      if (std::abs(tx - alpha) < 1E-5) {
        break;
      }
      if (tx > alpha) {
        x_max = x;
      } else {
        x_min = x;
      }
    }
    kSplinePosition[i] = coeff * ((1.0f - x) * kStartTension + x) + x * x * x;
  }
  kSplinePosition[kNBSamples] = 1.0f;
  param_setup = true;
}

void FlingAnimator::SetFriction(float friction) { friction_ = friction; }

float FlingAnimator::GetFriction() { return friction_ / kDefaultFriction; }

float FlingAnimator::GetDistance() { return distance_; }

bool FlingAnimator::UpdateValueAndVelocity(int64_t delta_time) {
  if (!IsRunning()) {
    return false;
  }
  FML_DCHECK(last_frame_time_ - start_time_ >= 0);
  int64_t time_passed = last_frame_time_ - start_time_;
  if (time_passed < duration_) {
    const float t = time_passed / duration_;
    const int index = static_cast<int>(kNBSamples * t);
    float distance_coeff = 1.f;
    float velocity_coeff = 0.f;
    if (index < kNBSamples) {
      const float t_inf = static_cast<float>(index) / kNBSamples;
      const float t_sup = static_cast<float>(index + 1) / kNBSamples;
      const float d_inf = kSplinePosition[index];
      const float d_sup = kSplinePosition[index + 1];
      velocity_coeff = (d_sup - d_inf) / (t_sup - t_inf);
      distance_coeff = d_inf + (t - t_inf) * velocity_coeff;
    }
    velocity_ = velocity_coeff * distance_ / duration_ * 1000.0f;
    value_ = start_value_ + distance_coeff * (final_value_ - start_value_);
    // Pin to mMinX <= mCurrX <= mMaxX
    value_ = std::min(value_, GetMaxValue());
    value_ = std::max(value_, GetMinValue());
    if (IsAtEquilibrium(value_, velocity_)) {
      return true;
    }
  } else {
    value_ = final_value_;
    return true;
  }
  return false;
}
float FlingAnimator::ComputeDeceleration(float friction) {
  return kGravityEarth       // g (m/s^2)
         * 39.37f            // inch/meter
         * density_ * 160.f  // pixels per inch
         * friction;
}

float FlingAnimator::GetSplineFlingDistance(float velocity) {
  const double l = GetSplineDeceleration(velocity);
  return friction_ * physical_coeff_ *
         std::exp(kDecelerationRate / (kDecelerationRate - 1.0) * l);
}

float FlingAnimator::GetSplineDeceleration(float velocity) {
  return std::log(kInflexion * std::abs(velocity) /
                  (friction_ * physical_coeff_));
}

float FlingAnimator::GetSplineFlingDuration(float velocity) {
  const double l = GetSplineDeceleration(velocity);
  return 1000.0 * std::exp(l / (kDecelerationRate - 1.0));
}

float FlingAnimator::GetAcceleration(float value, float velocity) {
  return velocity * friction_;
}

bool FlingAnimator::IsAtEquilibrium(float value, float velocity) {
  return value >= GetMaxValue() || value <= GetMinValue() ||
         std::abs(velocity) < velocity_threshold_;
}

void FlingAnimator::SetValueThreshold(float threshold) {
  velocity_threshold_ = threshold * kVelocityThresholdMultiplier;
}

void FlingAnimator::FlingInitialize() {
  auto velocity = velocity_;
  duration_ = GetSplineFlingDuration(velocity);
  start_time_ = fml::TimePoint::Now().ToEpochDelta().ToMillisecondsF();
  double total_distance = velocity_ > 0 ? GetSplineFlingDistance(velocity)
                                        : -1 * GetSplineFlingDistance(velocity);
  distance_ = total_distance;
  final_value_ = start_value_ + total_distance;
}

}  // namespace clay
