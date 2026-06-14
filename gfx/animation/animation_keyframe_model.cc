// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "gfx/animation/animation_keyframe_model.h"

#include "gfx/animation/animation_timing.h"
#include "gfx/animation/timing_function.h"

namespace lynx {
namespace gfx {

namespace {

TimingRunState ToTimingRunState(KeyframeModel::RunState state) {
  switch (state) {
    case KeyframeModel::STARTING:
      return TimingRunState::STARTING;
    case KeyframeModel::RUNNING:
      return TimingRunState::RUNNING;
    case KeyframeModel::PAUSED:
      return TimingRunState::PAUSED;
    case KeyframeModel::FINISHED:
      return TimingRunState::FINISHED;
  }
  return TimingRunState::STARTING;
}

KeyframeModel::RunState ToKeyframeModelRunState(TimingRunState state) {
  switch (state) {
    case TimingRunState::STARTING:
      return KeyframeModel::STARTING;
    case TimingRunState::RUNNING:
      return KeyframeModel::RUNNING;
    case TimingRunState::PAUSED:
      return KeyframeModel::PAUSED;
    case TimingRunState::FINISHED:
      return KeyframeModel::FINISHED;
  }
  return KeyframeModel::STARTING;
}

KeyframeModel::Phase ToKeyframeModelPhase(TimingPhase phase) {
  switch (phase) {
    case TimingPhase::BEFORE:
      return KeyframeModel::Phase::BEFORE;
    case TimingPhase::ACTIVE:
      return KeyframeModel::Phase::ACTIVE;
    case TimingPhase::AFTER:
      return KeyframeModel::Phase::AFTER;
  }
  return KeyframeModel::Phase::AFTER;
}

}  // namespace

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

AnimationTimingInput KeyframeModel::CreateTimingInput() const {
  AnimationTimingInput input;
  input.animation_data = animation_data_;
  input.duration = curve_ ? curve_->Duration() : fml::TimeDelta();
  input.start_time = start_time_;
  input.pause_time = pause_time_;
  input.total_paused_duration = total_paused_duration_;
  input.is_paused = run_state_ == PAUSED;
  input.run_state = ToTimingRunState(run_state_);
  input.playback_rate = playback_rate_;
  return input;
}

fml::TimeDelta KeyframeModel::GetRepeatDuration() const {
  return gfx::GetRepeatDuration(animation_data_,
                                curve_ ? curve_->Duration() : fml::TimeDelta());
}

KeyframeModel::Phase KeyframeModel::CalculatePhase(
    fml::TimeDelta local_time) const {
  return ToKeyframeModelPhase(
      gfx::CalculatePhase(CreateTimingInput(), local_time));
}

fml::TimeDelta KeyframeModel::ConvertMonotonicTimeToLocalTime(
    fml::TimePoint monotonic_time) const {
  return gfx::ConvertMonotonicTimeToLocalTime(CreateTimingInput(),
                                              monotonic_time);
}

fml::TimeDelta KeyframeModel::CalculateActiveTime(
    fml::TimePoint monotonic_time) const {
  return gfx::CalculateActiveTime(CreateTimingInput(), monotonic_time);
}

std::tuple<bool, bool> KeyframeModel::UpdateState(
    const fml::TimePoint& monotonic_time) {
  auto update = gfx::UpdateTimingState(CreateTimingInput(), monotonic_time);
  auto new_run_state = ToKeyframeModelRunState(update.run_state);
  if (new_run_state != run_state_) {
    SetRunState(new_run_state, monotonic_time);
  }
  return {update.start_event_due, update.end_event_due};
}

bool KeyframeModel::InEffect(fml::TimePoint monotonic_time) const {
  return gfx::IsInEffect(CreateTimingInput(), monotonic_time);
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
  auto trimmed = gfx::TrimTimeToCurrentIteration(
      CreateTimingInput(), monotonic_time, current_iteration_count);
  current_iteration_count = trimmed.current_iteration_count;
  return trimmed.time;
}

void KeyframeModel::EnsureFromAndToKeyframe() {
  if (curve_) {
    curve_->EnsureFromAndToKeyframe();
  }
}

}  // namespace gfx
}  // namespace lynx
