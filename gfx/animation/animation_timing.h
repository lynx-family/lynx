// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef GFX_ANIMATION_ANIMATION_TIMING_H_
#define GFX_ANIMATION_ANIMATION_TIMING_H_

#include "base/include/fml/time/time_point.h"
#include "gfx/animation/animation_types.h"
#include "gfx/gfx_export.h"

namespace lynx {
namespace gfx {

enum class TimingRunState {
  STARTING = 0,
  RUNNING,
  PAUSED,
  FINISHED,
};

enum class TimingPhase { BEFORE, ACTIVE, AFTER };

struct AnimationTimingInput {
  const AnimationData* animation_data{nullptr};
  fml::TimeDelta duration;
  fml::TimePoint start_time;
  fml::TimePoint pause_time;
  fml::TimeDelta total_paused_duration;
  bool is_paused{false};
  TimingRunState run_state{TimingRunState::STARTING};
  double playback_rate{1.0};
};

struct AnimationTimingStateUpdate {
  TimingRunState run_state{TimingRunState::STARTING};
  bool start_event_due{false};
  bool end_event_due{false};
};

struct TrimmedAnimationTime {
  fml::TimeDelta time;
  int current_iteration_count{0};
};

GFX_EXPORT fml::TimeDelta GetRepeatDuration(const AnimationData* data,
                                            fml::TimeDelta duration);
GFX_EXPORT fml::TimeDelta ConvertMonotonicTimeToLocalTime(
    const AnimationTimingInput& input, fml::TimePoint monotonic_time);
GFX_EXPORT TimingPhase CalculatePhase(const AnimationTimingInput& input,
                                      fml::TimeDelta local_time);
GFX_EXPORT TimingPhase CalculatePhase(const AnimationTimingInput& input,
                                      fml::TimePoint monotonic_time);
GFX_EXPORT fml::TimeDelta CalculateActiveTime(const AnimationTimingInput& input,
                                              fml::TimePoint monotonic_time);
GFX_EXPORT AnimationTimingStateUpdate UpdateTimingState(
    const AnimationTimingInput& input, fml::TimePoint monotonic_time);
GFX_EXPORT bool IsInEffect(const AnimationTimingInput& input,
                           fml::TimePoint monotonic_time);
GFX_EXPORT TrimmedAnimationTime TrimTimeToCurrentIteration(
    const AnimationTimingInput& input, fml::TimePoint monotonic_time,
    int current_iteration_count = 0);
GFX_EXPORT int CountIterationEventsDue(int old_iteration_count,
                                       int current_iteration_count);

}  // namespace gfx
}  // namespace lynx

#endif  // GFX_ANIMATION_ANIMATION_TIMING_H_
