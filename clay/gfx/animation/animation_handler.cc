// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/animation/animation_handler.h"

#include <algorithm>

#include "base/include/fml/time/time_point.h"

namespace clay {

/**
 * Register to get a callback on the next frame after the delay.
 */
void AnimationHandler::AddAnimationFrameCallback(
    AnimationFrameCallback* callback, int64_t delay) {
  if (callback_delay_time_map_.find(callback) ==
      callback_delay_time_map_.end()) {
    if (delay > 0) {
      fml::TimePoint delayed_start_time =
          fml::TimePoint::Now() + fml::TimeDelta::FromMilliseconds(delay);
      delay = delayed_start_time.ToEpochDelta().ToMilliseconds();
    }
    callback_delay_time_map_[callback] = delay;
    animation_callbacks_.push_front(callback);
    if (on_new_animation_callback_) {
      on_new_animation_callback_();
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

  auto it = callback_delay_time_map_.find(callback);
  if (it != callback_delay_time_map_.end()) {
    callback_delay_time_map_.erase(it);
  }
}

void AnimationHandler::DoAnimationFrame(int64_t frame_time) {
  int64_t current_time = fml::TimePoint::Now().ToEpochDelta().ToMilliseconds();
  last_frame_time_ = frame_time;
  for (auto& callback : animation_callbacks_) {
    if (callback && IsCallbackDue(callback, current_time)) {
      callback->DoAnimationFrame(frame_time);
    }
  }
  // clean up removed callbacks
  if (callback_list_dirty_) {
    callback_list_dirty_ = false;
    animation_callbacks_.remove(nullptr);
  }
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
  int64_t start_time = it->second;
  if (start_time <= 0) {
    return true;
  }
  return start_time < current_time;
}

}  // namespace clay
