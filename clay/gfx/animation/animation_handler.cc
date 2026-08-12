// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/animation/animation_handler.h"

#include <algorithm>

#include "base/include/fml/time/time_point.h"

namespace clay {

int64_t AnimationHandler::GetCurrentAnimationTime() const {
  return fml::TimePoint::Now().ToEpochDelta().ToMilliseconds();
}

/**
 * Register to get a callback on the next frame after the delay.
 */
void AnimationHandler::AddAnimationFrameCallback(
    AnimationFrameCallback* callback, int64_t delay) {
  if (callback_delay_time_map_.find(callback) ==
      callback_delay_time_map_.end()) {
    callback_delay_time_map_[callback] = CallbackSchedule{delay, -1};
    animation_callbacks_.push_front(callback);
    const bool should_receive_frame =
        callback->ShouldReceiveAnimationFrame(0, nullptr);
    if (animation_callback_ && should_receive_frame) {
      animation_callback_(-1);
    }
  }
}

/**
 * Removes the given callback from the list, so it will no longer be called for
 * frame related timing.
 */
void AnimationHandler::RemoveCallback(AnimationFrameCallback* callback) {
  // To avoid invalidating iterators, set the item to nullptr instead of
  // remove it.
  auto ret = std::find(animation_callbacks_.begin(), animation_callbacks_.end(),
                       callback);
  if (ret != animation_callbacks_.end()) {
    *ret = nullptr;
    callback_list_dirty_ = true;
  }

  const bool callback_removed = callback_delay_time_map_.erase(callback) > 0;
  if (callback_removed && scheduled_lifecycle_time_ >= 0) {
    RescheduleLifecycleCallback(GetCurrentAnimationTime());
  }
}

bool AnimationHandler::DoAnimationFrame(int64_t frame_time,
                                        bool lifecycle_only) {
  if (lifecycle_only || (scheduled_lifecycle_time_ >= 0 &&
                         frame_time >= scheduled_lifecycle_time_)) {
    InvalidateLifecycleSchedule();
  }
  const int64_t current_time = frame_time;
  last_frame_time_ = frame_time;
  bool has_visible_callback = false;
  int64_t next_lifecycle_time_to_schedule = -1;
  for (AnimationFrameCallback*& callback : animation_callbacks_) {
    if (!callback) {
      continue;
    }
    auto* current_callback = callback;
    int64_t next_lifecycle_time = -1;
    const bool should_receive_frame =
        current_callback->ShouldReceiveAnimationFrame(current_time,
                                                      &next_lifecycle_time);
    if (!IsCallbackDue(current_callback, current_time)) {
      has_visible_callback |= should_receive_frame;
      if (!should_receive_frame && next_lifecycle_time >= 0 &&
          (next_lifecycle_time_to_schedule < 0 ||
           next_lifecycle_time < next_lifecycle_time_to_schedule)) {
        next_lifecycle_time_to_schedule = next_lifecycle_time;
      }
      continue;
    }
    if (should_receive_frame) {
      if (lifecycle_only) {
        // A lifecycle task may wake after the callback becomes visible. Leave
        // value updates to the next normal animation frame.
        has_visible_callback = true;
        continue;
      }
      const bool finished = current_callback->DoAnimationFrame(frame_time);
      // DoAnimationFrame may remove itself, which nulls this list slot, or
      // change its next update from a frame to a lifecycle wake-up.
      if (callback != nullptr && !finished) {
        int64_t next_time = -1;
        const bool needs_next_frame =
            current_callback->ShouldReceiveAnimationFrame(current_time,
                                                          &next_time);
        has_visible_callback |= needs_next_frame;
        if (!needs_next_frame && next_time >= 0 &&
            (next_lifecycle_time_to_schedule < 0 ||
             next_time < next_lifecycle_time_to_schedule)) {
          next_lifecycle_time_to_schedule = next_time;
        }
      }
      continue;
    }

    if (next_lifecycle_time < 0 || next_lifecycle_time > current_time) {
      if (next_lifecycle_time >= 0 &&
          (next_lifecycle_time_to_schedule < 0 ||
           next_lifecycle_time < next_lifecycle_time_to_schedule)) {
        next_lifecycle_time_to_schedule = next_lifecycle_time;
      }
      continue;
    }
    current_callback->DoAnimationFrame(frame_time, false);
    if (callback) {
      int64_t next_time = -1;
      if (!current_callback->ShouldReceiveAnimationFrame(current_time,
                                                         &next_time) &&
          next_time >= 0 &&
          (next_lifecycle_time_to_schedule < 0 ||
           next_time < next_lifecycle_time_to_schedule)) {
        next_lifecycle_time_to_schedule = next_time;
      }
    }
  }
  // clean up removed callbacks
  if (callback_list_dirty_) {
    callback_list_dirty_ = false;
    animation_callbacks_.remove(nullptr);
  }
  ScheduleLifecycleCallbackAt(next_lifecycle_time_to_schedule, current_time);
  return has_visible_callback;
}

void AnimationHandler::ScheduleLifecycleCallback(int64_t current_time) {
  if (!animation_callback_) {
    return;
  }

  int64_t next_lifecycle_time = -1;
  for (auto* callback : animation_callbacks_) {
    if (!callback) {
      continue;
    }
    int64_t callback_time = -1;
    if (callback->ShouldReceiveAnimationFrame(current_time, &callback_time)) {
      continue;
    }
    if (callback_time < 0) {
      continue;
    }
    if (next_lifecycle_time < 0 || callback_time < next_lifecycle_time) {
      next_lifecycle_time = callback_time;
    }
  }

  ScheduleLifecycleCallbackAt(next_lifecycle_time, current_time);
}

void AnimationHandler::ScheduleLifecycleCallbackAt(int64_t next_lifecycle_time,
                                                   int64_t current_time) {
  if (!animation_callback_) {
    return;
  }

  if (next_lifecycle_time < 0) {
    if (scheduled_lifecycle_time_ >= 0) {
      InvalidateLifecycleSchedule();
    }
    return;
  }

  if (scheduled_lifecycle_time_ >= 0 &&
      scheduled_lifecycle_time_ <= next_lifecycle_time) {
    return;
  }

  InvalidateLifecycleSchedule();
  scheduled_lifecycle_time_ = next_lifecycle_time;
  animation_callback_(
      std::max(static_cast<int64_t>(0), next_lifecycle_time - current_time));
}

/**
 * Remove the callbacks from callback_delay_time_map_ once they have passed the
 * initial delay so that they can start getting frame callbacks.
 *
 * @return true if they have passed the initial delay or have no delay, false
 * otherwise.
 */
bool AnimationHandler::IsCallbackDue(AnimationFrameCallback* callback,
                                     int64_t current_time) {
  auto it = callback_delay_time_map_.find(callback);
  if (it == callback_delay_time_map_.end()) {
    return false;
  }
  CallbackSchedule& schedule = it->second;
  if (schedule.delay <= 0) {
    return true;
  }
  if (schedule.start_time < 0) {
    schedule.start_time = current_time + schedule.delay;
  }
  return current_time >= schedule.start_time;
}

}  // namespace clay
