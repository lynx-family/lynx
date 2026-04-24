// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef GFX_ANIMATION_ANIMATION_KEYFRAME_MODEL_H_
#define GFX_ANIMATION_ANIMATION_KEYFRAME_MODEL_H_

#include <cstdint>
#include <memory>
#include <tuple>
#include <utility>

#include "base/include/fml/time/time_point.h"
#include "gfx/animation/animation_keyframe_curve.h"
#include "gfx/animation/animation_types.h"

namespace lynx {
namespace gfx {

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

  explicit KeyframeModel(std::unique_ptr<AnimationCurve> curve);
  ~KeyframeModel() = default;

  KeyframeModel(const KeyframeModel&) = delete;
  KeyframeModel& operator=(const KeyframeModel&) = delete;

  AnimationCurve* curve() { return curve_.get(); }
  const AnimationCurve* curve() const { return curve_.get(); }

  RunState GetRunState() const { return run_state_; }
  bool is_finished() const { return run_state_ == FINISHED; }

  const fml::TimePoint& start_time() const { return start_time_; }
  const fml::TimePoint& pause_time() const { return pause_time_; }
  const fml::TimeDelta& total_paused_duration() const {
    return total_paused_duration_;
  }

  void set_start_time(fml::TimePoint& monotonic_time) {
    start_time_ = monotonic_time;
  }
  bool has_set_start_time() const { return start_time_ != fml::TimePoint(); }

  double playback_rate() const { return playback_rate_; }
  void set_playback_rate(double playback_rate) {
    playback_rate_ = playback_rate;
  }

  void SetRunState(RunState run_state, fml::TimePoint monotonic_time);

  void SetAnimationData(const AnimationData* data);
  const AnimationData* animation_data() const { return animation_data_; }

  fml::TimeDelta GetRepeatDuration() const;
  Phase CalculatePhase(fml::TimeDelta local_time) const;
  fml::TimeDelta ConvertMonotonicTimeToLocalTime(
      fml::TimePoint monotonic_time) const;
  fml::TimeDelta CalculateActiveTime(fml::TimePoint monotonic_time) const;
  std::tuple<bool, bool> UpdateState(const fml::TimePoint& monotonic_time);
  bool InEffect(fml::TimePoint monotonic_time) const;
  fml::TimeDelta TrimTimeToCurrentIteration(fml::TimePoint monotonic_time,
                                            int& current_iteration_count) const;
  void EnsureFromAndToKeyframe();

 private:
  RunState run_state_;
  const AnimationData* animation_data_{nullptr};
  fml::TimePoint start_time_;
  std::unique_ptr<AnimationCurve> curve_;
  double playback_rate_{1.0};
  fml::TimePoint pause_time_;
  fml::TimeDelta total_paused_duration_{fml::TimeDelta()};
};

}  // namespace gfx
}  // namespace lynx

#endif  // GFX_ANIMATION_ANIMATION_KEYFRAME_MODEL_H_
