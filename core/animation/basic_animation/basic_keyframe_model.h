// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_ANIMATION_BASIC_ANIMATION_BASIC_KEYFRAME_MODEL_H_
#define CORE_ANIMATION_BASIC_ANIMATION_BASIC_KEYFRAME_MODEL_H_

#include <memory>
#include <tuple>
#include <utility>

#include "base/include/fml/time/time_delta.h"
#include "core/animation/basic_animation/animation_effect_timing.h"
#include "core/animation/basic_animation/basic_animation_curve.h"

namespace lynx {
namespace animation {
namespace basic {
class AnimationEffect;
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

 public:
  KeyframeModel() = default;
  ~KeyframeModel() = default;

  KeyframeModel(std::unique_ptr<AnimationCurve> curve,
                basic::AnimationEffect* effect)
      : animation_effect_(effect),
        run_state_(RunState::STARTING),
        curve_(std::move(curve)) {}

  static std::unique_ptr<KeyframeModel> Create(
      std::unique_ptr<AnimationCurve> curve, basic::AnimationEffect* effect);

  const AnimationEffectTiming& timing() const;

  // time point ge/setter start
  const fml::TimePoint& start_time() const { return start_time_; }
  const fml::TimePoint& pause_time() const { return pause_time_; }
  const fml::TimeDelta& total_paused_duration() const {
    return total_paused_duration_;
  }
  void set_start_time(const fml::TimePoint& monotonic_time) {
    start_time_ = monotonic_time;
  }
  bool has_set_start_time() const { return start_time_ != fml::TimePoint(); }
  void SetRunState(RunState run_state, fml::TimePoint monotonic_time);
  RunState GetRunState() { return run_state_; }
  bool is_finished() const { return run_state_ == FINISHED; }
  // time point ge/setter end

  // timing caculate functions start
  fml::TimeDelta GetRepeatDuration() const;

  Phase CalculatePhase(fml::TimeDelta local_time) const;

  // LocalTime is relative time
  fml::TimeDelta ConvertMonotonicTimeToLocalTime(
      fml::TimePoint monotonic_time) const;

  fml::TimeDelta CalculateActiveTime(fml::TimePoint monotonic_time) const;

  fml::TimeDelta TrimTimeToCurrentIteration(fml::TimePoint monotonic_time,
                                            int& current_iteration_count) const;

  bool InEffect(fml::TimePoint monotonic_time) const;

  void UpdateAnimationData(AnimationEffectTiming* data);

  void EnsureFromAndToKeyframe();

  std::tuple<bool, bool> UpdateState(const fml::TimePoint& monotonic_time);
  // timing caculate functions end

  basic::AnimationCurve* curve() { return curve_.get(); }

 protected:
  basic::AnimationEffect* animation_effect_;
  RunState run_state_;
  std::unique_ptr<basic::AnimationCurve> curve_;
  fml::TimePoint start_time_;
  fml::TimePoint pause_time_;
  fml::TimeDelta total_paused_duration_{fml::TimeDelta()};

 private:
  KeyframeModel(const KeyframeModel&) = delete;
  KeyframeModel& operator=(const KeyframeModel&) = delete;
};
}  // namespace basic
}  // namespace animation
}  // namespace lynx

#endif  // CORE_ANIMATION_BASIC_ANIMATION_BASIC_KEYFRAME_MODEL_H_
