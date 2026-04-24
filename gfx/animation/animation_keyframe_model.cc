// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "gfx/animation/animation_keyframe_model.h"

#include <algorithm>
#include <cmath>
#include <limits>

#include "gfx/animation/timing_function.h"

namespace lynx {
namespace gfx {

std::unique_ptr<KeyframeModel> KeyframeModel::Create(
    std::unique_ptr<AnimationCurve> curve) {
  return std::make_unique<KeyframeModel>(std::move(curve));
}

KeyframeModel::KeyframeModel(std::unique_ptr<AnimationCurve> curve)
    : run_state_(RunState::STARTING), curve_(std::move(curve)) {}

void KeyframeModel::SetAnimationData(const AnimationData* data) {
  animation_data_ = data;
  if (!animation_data_ || !curve_) {
    return;
  }

  curve_->SetTimingFunction(
      lynx::gfx::CreateTimingFunction(animation_data_->timing_func));
  curve_->set_scaled_duration(animation_data_->duration / 1000.0);
}

fml::TimeDelta KeyframeModel::GetRepeatDuration() const {
  if (!animation_data_ || animation_data_->iteration_count == 0 || !curve_) {
    return fml::TimeDelta::Zero();
  }

  if (curve_->Duration().ToNanoseconds() >=
      (static_cast<double>(std::numeric_limits<int64_t>::max()) /
       static_cast<double>(animation_data_->iteration_count))) {
    return fml::TimeDelta::Max();
  }

  return curve_->Duration() *
         static_cast<double>(animation_data_->iteration_count);
}

KeyframeModel::Phase KeyframeModel::CalculatePhase(
    fml::TimeDelta local_time) const {
  if (!animation_data_) {
    return Phase::AFTER;
  }
  fml::TimeDelta time_offset =
      fml::TimeDelta::FromMilliseconds(animation_data_->delay * -1);
  fml::TimeDelta opposite_time_offset = time_offset == fml::TimeDelta::Min()
                                            ? fml::TimeDelta::Max()
                                            : fml::TimeDelta() - time_offset;
  fml::TimeDelta before_active_boundary_time =
      std::max(opposite_time_offset, fml::TimeDelta());
  if (local_time < before_active_boundary_time ||
      (local_time == before_active_boundary_time && playback_rate_ < 0)) {
    return Phase::BEFORE;
  }

  fml::TimeDelta active_duration =
      GetRepeatDuration() / std::abs(playback_rate_);

  fml::TimeDelta active_after_boundary_time =
      animation_data_->iteration_count >= 0 &&
              (opposite_time_offset.ToNanoseconds() <
               std::numeric_limits<int64_t>::max() -
                   active_duration.ToNanoseconds())
          ? std::max(opposite_time_offset + active_duration, fml::TimeDelta())
          : fml::TimeDelta::Max();
  if (local_time > active_after_boundary_time ||
      (local_time == active_after_boundary_time && playback_rate_ > 0)) {
    return Phase::AFTER;
  }
  return Phase::ACTIVE;
}

fml::TimeDelta KeyframeModel::ConvertMonotonicTimeToLocalTime(
    fml::TimePoint monotonic_time) const {
  fml::TimePoint time = (run_state_ == PAUSED) ? pause_time_ : monotonic_time;
  return time - start_time_ - total_paused_duration_;
}

fml::TimeDelta KeyframeModel::CalculateActiveTime(
    fml::TimePoint monotonic_time) const {
  if (!animation_data_) {
    return fml::TimeDelta::Min();
  }

  fml::TimeDelta time_offset =
      fml::TimeDelta::FromMilliseconds(animation_data_->delay * -1);
  fml::TimeDelta local_time = ConvertMonotonicTimeToLocalTime(monotonic_time);
  Phase phase = CalculatePhase(local_time);

  switch (phase) {
    case Phase::BEFORE:
      if (animation_data_->fill_mode == AnimationFillModeType::kBackwards ||
          animation_data_->fill_mode == AnimationFillModeType::kBoth) {
        return std::max(local_time + time_offset, fml::TimeDelta());
      }
      return fml::TimeDelta::Min();
    case Phase::ACTIVE:
      return local_time + time_offset;
    case Phase::AFTER:
      if (animation_data_->fill_mode == AnimationFillModeType::kForwards ||
          animation_data_->fill_mode == AnimationFillModeType::kBoth) {
        fml::TimeDelta active_duration =
            GetRepeatDuration() / std::abs(playback_rate_);
        return std::max(std::min(local_time + time_offset, active_duration),
                        fml::TimeDelta());
      }
      return fml::TimeDelta::Min();
    default:
      return fml::TimeDelta::Min();
  }
}

std::tuple<bool, bool> KeyframeModel::UpdateState(
    const fml::TimePoint& monotonic_time) {
  bool start_event_due = false;
  bool end_event_due = false;
  fml::TimeDelta local_time = ConvertMonotonicTimeToLocalTime(monotonic_time);
  Phase phase = CalculatePhase(local_time);
  switch (run_state_) {
    case STARTING:
      if (phase == Phase::ACTIVE) {
        SetRunState(RUNNING, monotonic_time);
        start_event_due = true;
      } else if (phase == Phase::AFTER) {
        SetRunState(FINISHED, monotonic_time);
        start_event_due = true;
        end_event_due = true;
      }
      break;
    case RUNNING:
      if (phase == Phase::BEFORE) {
        SetRunState(STARTING, monotonic_time);
        end_event_due = true;
      } else if (phase == Phase::AFTER) {
        SetRunState(FINISHED, monotonic_time);
        end_event_due = true;
      }
      break;
    case PAUSED:
      if (phase == Phase::BEFORE) {
        SetRunState(STARTING, monotonic_time);
      } else if (phase == Phase::ACTIVE) {
        SetRunState(RUNNING, monotonic_time);
      } else if (phase == Phase::AFTER) {
        SetRunState(FINISHED, monotonic_time);
      }
      break;
    case FINISHED:
      if (phase == Phase::BEFORE) {
        SetRunState(STARTING, monotonic_time);
      } else if (phase == Phase::ACTIVE) {
        SetRunState(RUNNING, monotonic_time);
        start_event_due = true;
      }
      break;
  }
  return {start_event_due, end_event_due};
}

bool KeyframeModel::InEffect(fml::TimePoint monotonic_time) const {
  return CalculateActiveTime(monotonic_time) != fml::TimeDelta::Min();
}

void KeyframeModel::SetRunState(RunState run_state,
                                fml::TimePoint monotonic_time) {
  if ((run_state == STARTING || run_state == RUNNING ||
       run_state == FINISHED) &&
      run_state_ == PAUSED) {
    total_paused_duration_ =
        total_paused_duration_ + (monotonic_time - pause_time_);
  } else if (run_state == PAUSED) {
    pause_time_ = monotonic_time;
  }
  run_state_ = run_state;
}

fml::TimeDelta KeyframeModel::TrimTimeToCurrentIteration(
    fml::TimePoint monotonic_time, int& current_iteration_count) const {
  if (!animation_data_ || !curve_) {
    current_iteration_count = 0;
    return fml::TimeDelta();
  }

  fml::TimeDelta active_time = CalculateActiveTime(monotonic_time);

  fml::TimeDelta start_offset = fml::TimeDelta();
  if (active_time < fml::TimeDelta()) {
    return start_offset;
  }
  if (!animation_data_->iteration_count) {
    return fml::TimeDelta();
  }
  if (curve_->Duration() <= fml::TimeDelta()) {
    return fml::TimeDelta();
  }

  fml::TimeDelta repeated_duration = GetRepeatDuration();
  fml::TimeDelta active_duration = repeated_duration / std::abs(playback_rate_);

  fml::TimeDelta scaled_active_time;
  if (playback_rate_ < 0) {
    scaled_active_time =
        ((active_time - active_duration) * playback_rate_) + start_offset;
  } else {
    scaled_active_time = (active_time * playback_rate_) + start_offset;
  }

  fml::TimeDelta iteration_time;
  if (scaled_active_time - start_offset == repeated_duration &&
      std::fmod(static_cast<double>(animation_data_->iteration_count), 1) ==
          0) {
    iteration_time = curve_->Duration();
  } else {
    iteration_time = scaled_active_time % curve_->Duration();
  }

  int iteration;
  if (scaled_active_time <= fml::TimeDelta()) {
    iteration = 0;
  } else if (iteration_time == curve_->Duration()) {
    iteration =
        std::ceil(static_cast<double>(animation_data_->iteration_count) - 1);
  } else {
    iteration = static_cast<int>(scaled_active_time / curve_->Duration());
  }
  current_iteration_count = iteration;

  bool reverse =
      (animation_data_->direction == AnimationDirectionType::kReverse) ||
      (animation_data_->direction == AnimationDirectionType::kAlternate &&
       iteration % 2 == 1) ||
      (animation_data_->direction ==
           AnimationDirectionType::kAlternateReverse &&
       iteration % 2 == 0);
  if (reverse) {
    iteration_time = curve_->Duration() - iteration_time;
  }
  return iteration_time;
}

void KeyframeModel::EnsureFromAndToKeyframe() {
  if (curve_) {
    curve_->EnsureFromAndToKeyframe();
  }
}

}  // namespace gfx
}  // namespace lynx
