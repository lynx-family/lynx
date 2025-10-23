// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/animation/dynamic_animator.h"

#include <algorithm>

#include "base/include/fml/time/time_point.h"
#include "clay/fml/logging.h"

namespace clay {

DynamicAnimator::~DynamicAnimator() {
  if (running_) {
    GetAnimationHandler()->RemoveCallback(this);
  }
}

void DynamicAnimator::SetStartValue(float start_value) {
  value_ = start_value;
  start_value_ = start_value;
  start_value_is_set_ = true;
}

void DynamicAnimator::SetMinimumVisibleChange(float minimum_visible_change) {
  FML_DCHECK(minimum_visible_change > 0)
      << "Minimum visible change must be positive.";

  min_visible_change_ = minimum_visible_change;
  SetValueThreshold(minimum_visible_change * kThresholdMultiplier);
}

void DynamicAnimator::Start() { StartAnimationInternal(); }

void DynamicAnimator::Cancel() { EndAnimationInternal(true); }

void DynamicAnimator::StartAnimationInternal() {
  if (!running_) {
    running_ = true;
    // Sanity check
    FML_DCHECK(start_value_is_set_)
        << "Starting value need to be set before starting animation.";
    FML_DCHECK(value_ >= min_value_ && value_ <= max_value_)
        << "Starting value need to be in between min value and max value";
    last_frame_time_ = fml::TimePoint::Now().ToEpochDelta().ToMilliseconds();
    GetAnimationHandler()->AddAnimationFrameCallback(this, 0);
  }
}

void DynamicAnimator::EndAnimationInternal(bool canceled) {
  if (!running_) {
    return;
  }
  running_ = false;
  GetAnimationHandler()->RemoveCallback(this);
  last_frame_time_ = 0;
  delta_ = 0;
  start_value_is_set_ = false;
  for (auto l : listeners_) {
    if (l) {
      l->OnDynamicAnimationEnd(*this, canceled, value_, velocity_);
    }
  }
}

bool DynamicAnimator::DoAnimationFrame(int64_t frame_time) {
  FML_DCHECK(last_frame_time_ > 0) << "last_frame_time_ should be set on start";
  int64_t delta_time = frame_time - last_frame_time_;
  last_frame_time_ = frame_time;
  if (start_time_ > last_frame_time_) {
    start_time_ = last_frame_time_;
  }
  float prev_value = value_;
  bool finished = UpdateValueAndVelocity(delta_time);
  value_ = std::clamp(value_, min_value_, max_value_);
  delta_ = value_ - prev_value;
  for (auto l : listeners_) {
    if (l) {
      l->OnDynamicAnimationUpdate(*this, value_, velocity_);
    }
  }
  if (finished) {
    EndAnimationInternal(false);
  }
  CleanupList();
  return finished;
}

void DynamicAnimator::RemoveListener(AnimationListener* listener) {
  // To avoid invalidating iterators, set the item to nullptr instead of
  // remove it.
  auto ret = std::find(listeners_.begin(), listeners_.end(), listener);
  if (ret != listeners_.end()) {
    *ret = nullptr;
    listener_list_dirty_ = true;
  }
}

void DynamicAnimator::RemoveAllListeners() {
  for (auto& l : listeners_) {
    l = nullptr;
  }
  listener_list_dirty_ = true;
}

void DynamicAnimator::CleanupList() {
  if (listener_list_dirty_) {
    listener_list_dirty_ = false;
    listeners_.remove(nullptr);
  }
}

}  // namespace clay
