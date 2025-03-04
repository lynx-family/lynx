// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_ANIMATION_KEYFRAME_MODEL_H_
#define CORE_ANIMATION_KEYFRAME_MODEL_H_

#include <cmath>
#include <memory>
#include <string>
#include <tuple>

#include "base/include/fml/time/time_point.h"
#include "core/renderer/css/css_value.h"
#include "core/renderer/starlight/style/css_type.h"
#include "core/style/animation_data.h"

namespace lynx {
namespace animation {

class AnimationCurve;

class KeyframeModel {
 public:
  enum RunState {
    STARTING = 0,
    RUNNING,
    PAUSED,
    FINISHED,
  };

  enum class Phase { BEFORE, ACTIVE, AFTER };

  static std::unique_ptr<KeyframeModel> Create(
      std::unique_ptr<AnimationCurve> curve);

  const fml::TimePoint& start_time() const { return start_time_; }
  const fml::TimePoint& pause_time() const { return pause_time_; }
  const fml::TimeDelta& total_paused_duration() const {
    return total_paused_duration_;
  }

  void set_start_time(fml::TimePoint& monotonic_time) {
    start_time_ = monotonic_time;
  }
  bool has_set_start_time() const { return start_time_ != fml::TimePoint(); }

  double playback_rate() { return playback_rate_; }
  void set_playback_rate(double playback_rate) {
    playback_rate_ = playback_rate;
  }

  fml::TimeDelta GetRepeatDuration() const;

  KeyframeModel::Phase CalculatePhase(fml::TimeDelta local_time) const;

  // LocalTime is relative time
  fml::TimeDelta ConvertMonotonicTimeToLocalTime(
      fml::TimePoint monotonic_time) const;

  fml::TimeDelta CalculateActiveTime(fml::TimePoint monotonic_time) const;

  fml::TimeDelta TrimTimeToCurrentIteration(fml::TimePoint monotonic_time,
                                            int& current_iteration_count) const;

  AnimationCurve* curve() { return curve_.get(); }
  const AnimationCurve* curve() const { return curve_.get(); }

  bool InEffect(fml::TimePoint monotonic_time) const;

  void SetRunState(RunState run_state, fml::TimePoint monotonic_time);
  RunState GetRunState() { return run_state_; }
  bool is_finished() const { return run_state_ == FINISHED; }

  void set_animation_data(starlight::AnimationData* data) {
    animation_data_ = data;
  }

  starlight::AnimationData get_animation_data() { return *animation_data_; }

  AnimationCurve* animation_curve() { return curve_.get(); }

  void UpdateAnimationData(starlight::AnimationData* data);

  void EnsureFromAndToKeyframe();

  void NotifyElementSizeUpdated();

  void NotifyUnitValuesUpdatedToAnimation(tasm::CSSValuePattern);

  std::tuple<bool, bool> UpdateState(const fml::TimePoint& monotonic_time);

 public:
  KeyframeModel(std::unique_ptr<AnimationCurve> curve);

 private:
  RunState run_state_;
  starlight::AnimationData* animation_data_;
  fml::TimePoint start_time_;
  std::unique_ptr<AnimationCurve> curve_;
  double playback_rate_;
  fml::TimePoint pause_time_;
  fml::TimeDelta total_paused_duration_{fml::TimeDelta()};
};

}  // namespace animation
}  // namespace lynx

#endif  // CORE_ANIMATION_KEYFRAME_MODEL_H_
