// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "gfx/animation/animation_timing.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace lynx {
namespace gfx {

fml::TimeDelta GetRepeatDuration(const AnimationData* data,
                                 fml::TimeDelta duration) {
  if (data == nullptr || data->iteration_count == 0) {
    return fml::TimeDelta::Zero();
  }
  if (data->iteration_count < 0) {
    return fml::TimeDelta::Max();
  }
  if (duration.ToNanoseconds() >=
      (static_cast<double>(std::numeric_limits<int64_t>::max()) /
       static_cast<double>(data->iteration_count))) {
    return fml::TimeDelta::Max();
  }
  return duration * static_cast<double>(data->iteration_count);
}

fml::TimeDelta ConvertMonotonicTimeToLocalTime(
    const AnimationTimingInput& input, fml::TimePoint monotonic_time) {
  fml::TimePoint time = input.is_paused ? input.pause_time : monotonic_time;
  return time - input.start_time - input.total_paused_duration;
}

TimingPhase CalculatePhase(const AnimationTimingInput& input,
                           fml::TimeDelta local_time) {
  if (input.animation_data == nullptr) {
    return TimingPhase::AFTER;
  }
  fml::TimeDelta time_offset =
      fml::TimeDelta::FromMilliseconds(input.animation_data->delay * -1);
  fml::TimeDelta opposite_time_offset = time_offset == fml::TimeDelta::Min()
                                            ? fml::TimeDelta::Max()
                                            : fml::TimeDelta() - time_offset;
  fml::TimeDelta before_active_boundary_time =
      std::max(opposite_time_offset, fml::TimeDelta());
  if (local_time < before_active_boundary_time ||
      (local_time == before_active_boundary_time && input.playback_rate < 0)) {
    return TimingPhase::BEFORE;
  }

  fml::TimeDelta active_duration =
      GetRepeatDuration(input.animation_data, input.duration) /
      std::abs(input.playback_rate);
  fml::TimeDelta active_after_boundary_time =
      input.animation_data->iteration_count >= 0 &&
              opposite_time_offset.ToNanoseconds() <
                  std::numeric_limits<int64_t>::max() -
                      active_duration.ToNanoseconds()
          ? std::max(opposite_time_offset + active_duration, fml::TimeDelta())
          : fml::TimeDelta::Max();
  if (local_time > active_after_boundary_time ||
      (local_time == active_after_boundary_time && input.playback_rate > 0)) {
    return TimingPhase::AFTER;
  }
  return TimingPhase::ACTIVE;
}

TimingPhase CalculatePhase(const AnimationTimingInput& input,
                           fml::TimePoint monotonic_time) {
  return CalculatePhase(input,
                        ConvertMonotonicTimeToLocalTime(input, monotonic_time));
}

fml::TimeDelta CalculateActiveTime(const AnimationTimingInput& input,
                                   fml::TimePoint monotonic_time) {
  if (input.animation_data == nullptr) {
    return fml::TimeDelta::Min();
  }

  fml::TimeDelta time_offset =
      fml::TimeDelta::FromMilliseconds(input.animation_data->delay * -1);
  fml::TimeDelta local_time =
      ConvertMonotonicTimeToLocalTime(input, monotonic_time);

  switch (CalculatePhase(input, local_time)) {
    case TimingPhase::BEFORE:
      if (input.animation_data->fill_mode ==
              AnimationFillModeType::kBackwards ||
          input.animation_data->fill_mode == AnimationFillModeType::kBoth) {
        return std::max(local_time + time_offset, fml::TimeDelta());
      }
      return fml::TimeDelta::Min();
    case TimingPhase::ACTIVE:
      return local_time + time_offset;
    case TimingPhase::AFTER:
      if (input.animation_data->fill_mode == AnimationFillModeType::kForwards ||
          input.animation_data->fill_mode == AnimationFillModeType::kBoth) {
        fml::TimeDelta active_duration =
            GetRepeatDuration(input.animation_data, input.duration) /
            std::abs(input.playback_rate);
        return std::max(std::min(local_time + time_offset, active_duration),
                        fml::TimeDelta());
      }
      return fml::TimeDelta::Min();
  }
  return fml::TimeDelta::Min();
}

fml::TimeDelta CalculateEventTime(const AnimationTimingInput& input,
                                  fml::TimePoint monotonic_time) {
  if (!input.animation_data) {
    return fml::TimeDelta::Min();
  }
  const auto elapsed =
      ConvertMonotonicTimeToLocalTime(input, monotonic_time) -
      fml::TimeDelta::FromMilliseconds(input.animation_data->delay);
  const auto active_duration =
      GetRepeatDuration(input.animation_data, input.duration) /
      std::abs(input.playback_rate);
  return std::max(std::min(elapsed, active_duration), fml::TimeDelta::Zero());
}

AnimationTimingStateUpdate UpdateTimingState(const AnimationTimingInput& input,
                                             fml::TimePoint monotonic_time) {
  AnimationTimingStateUpdate update;
  update.run_state = input.run_state;
  auto phase = CalculatePhase(input, monotonic_time);
  switch (input.run_state) {
    case TimingRunState::STARTING:
      if (phase == TimingPhase::ACTIVE) {
        update.run_state = TimingRunState::RUNNING;
        update.start_event_due = true;
      } else if (phase == TimingPhase::AFTER) {
        update.run_state = TimingRunState::FINISHED;
        update.start_event_due = true;
        update.end_event_due = true;
      }
      break;
    case TimingRunState::RUNNING:
      if (phase == TimingPhase::BEFORE) {
        update.run_state = TimingRunState::STARTING;
        update.end_event_due = true;
      } else if (phase == TimingPhase::AFTER) {
        update.run_state = TimingRunState::FINISHED;
        update.end_event_due = true;
      }
      break;
    case TimingRunState::PAUSED:
      if (phase == TimingPhase::BEFORE) {
        update.run_state = TimingRunState::STARTING;
      } else if (phase == TimingPhase::ACTIVE) {
        update.run_state = TimingRunState::RUNNING;
      } else if (phase == TimingPhase::AFTER) {
        update.run_state = TimingRunState::FINISHED;
      }
      break;
    case TimingRunState::FINISHED:
      if (phase == TimingPhase::BEFORE) {
        update.run_state = TimingRunState::STARTING;
      } else if (phase == TimingPhase::ACTIVE) {
        update.run_state = TimingRunState::RUNNING;
        update.start_event_due = true;
      }
      break;
  }
  return update;
}

bool IsInEffect(const AnimationTimingInput& input,
                fml::TimePoint monotonic_time) {
  return CalculateActiveTime(input, monotonic_time) != fml::TimeDelta::Min();
}

TrimmedAnimationTime TrimTimeToCurrentIteration(
    const AnimationTimingInput& input, fml::TimePoint monotonic_time,
    int current_iteration_count, bool events_only) {
  TrimmedAnimationTime result;
  result.current_iteration_count = current_iteration_count;
  if (input.animation_data == nullptr || input.duration <= fml::TimeDelta()) {
    result.current_iteration_count = 0;
    return result;
  }

  const auto elapsed_time = events_only
                                ? CalculateEventTime(input, monotonic_time)
                                : CalculateActiveTime(input, monotonic_time);
  if (elapsed_time < fml::TimeDelta()) {
    return result;
  }
  if (!input.animation_data->iteration_count) {
    return result;
  }

  fml::TimeDelta repeated_duration =
      GetRepeatDuration(input.animation_data, input.duration);
  fml::TimeDelta active_duration =
      repeated_duration / std::abs(input.playback_rate);

  fml::TimeDelta scaled_active_time;
  if (input.playback_rate < 0) {
    scaled_active_time = (elapsed_time - active_duration) * input.playback_rate;
  } else {
    scaled_active_time = elapsed_time * input.playback_rate;
  }

  fml::TimeDelta iteration_time;
  if (scaled_active_time == repeated_duration &&
      std::fmod(static_cast<double>(input.animation_data->iteration_count),
                1) == 0) {
    iteration_time = input.duration;
  } else {
    iteration_time = scaled_active_time % input.duration;
  }

  int iteration = 0;
  if (scaled_active_time > fml::TimeDelta()) {
    if (iteration_time == input.duration) {
      iteration = static_cast<int>(std::ceil(
          static_cast<double>(input.animation_data->iteration_count) - 1));
    } else {
      iteration = static_cast<int>(scaled_active_time / input.duration);
    }
  }
  result.current_iteration_count = iteration;

  bool reverse =
      input.animation_data->direction == AnimationDirectionType::kReverse ||
      (input.animation_data->direction == AnimationDirectionType::kAlternate &&
       iteration % 2 == 1) ||
      (input.animation_data->direction ==
           AnimationDirectionType::kAlternateReverse &&
       iteration % 2 == 0);
  result.time = reverse ? input.duration - iteration_time : iteration_time;
  return result;
}

int CountIterationEventsDue(int old_iteration_count,
                            int current_iteration_count) {
  if (current_iteration_count <= old_iteration_count) {
    return 0;
  }
  return current_iteration_count - old_iteration_count;
}

fml::TimePoint GetNextAnimationEventTime(const AnimationTimingInput& input,
                                         fml::TimePoint now,
                                         int current_iteration_count,
                                         bool needs_iteration_event) {
  if (!input.animation_data) {
    return fml::TimePoint::Max();
  }
  // A resumed model first needs to leave its held timing state.
  auto update = UpdateTimingState(input, now);
  if (input.is_paused || update.run_state != input.run_state ||
      update.start_event_due || update.end_event_due) {
    return now;
  }
  const auto& data = *input.animation_data;
  // Calculate deadlines without overflowing TimePoint/TimeDelta arithmetic.
  const long double origin =
      static_cast<long double>(
          input.start_time.ToEpochDelta().ToNanoseconds()) +
      input.total_paused_duration.ToNanoseconds();
  auto deadline = [origin, now](long double offset) {
    const long double ticks = origin + offset;
    if (ticks >= std::numeric_limits<int64_t>::max()) {
      return fml::TimePoint::Max();
    }
    return ticks <= now.ToEpochDelta().ToNanoseconds()
               ? now
               : fml::TimePoint::FromTicks(
                     static_cast<int64_t>(std::ceil(ticks)));
  };
  const long double delay = static_cast<long double>(data.delay) * 1000000;
  if (input.run_state == TimingRunState::STARTING) {
    return deadline(std::max(delay, 0.L));
  }
  if (input.run_state == TimingRunState::FINISHED ||
      input.duration <= fml::TimeDelta::Zero() || input.playback_rate == 0) {
    return fml::TimePoint::Max();
  }
  const long double duration =
      input.duration.ToNanoseconds() / std::abs(input.playback_rate);
  auto next =
      data.iteration_count < 0
          ? fml::TimePoint::Max()
          : deadline(std::max(delay + duration * data.iteration_count, 0.L));
  if (needs_iteration_event &&
      (data.iteration_count < 0 ||
       current_iteration_count < data.iteration_count - 1)) {
    next = std::min(
        next,
        deadline(delay +
                 duration *
                     (static_cast<int64_t>(current_iteration_count) + 1)));
  }
  return next;
}

}  // namespace gfx
}  // namespace lynx
