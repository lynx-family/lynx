// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_ANIMATION_ANIMATION_HANDLER_H_
#define CLAY_GFX_ANIMATION_ANIMATION_HANDLER_H_

#include <forward_list>
#include <functional>
#include <unordered_map>
#include <utility>

namespace clay {

class AnimationHandler {
 public:
  /**
   * Callbacks that receives notifications for animation timing.
   */
  class AnimationFrameCallback {
   public:
    /**
     * Run animation based on the frame time.
     * @param frame_time The frame start time
     * @return if the animation has finished.
     */
    virtual bool DoAnimationFrame(int64_t frame_time,
                                  bool update_values = true) = 0;

    /**
     * Return false when this callback should stay registered but should not
     * receive animation ticks in the current visibility state.
     */
    virtual bool ShouldReceiveAnimationFrame(int64_t current_time,
                                             int64_t* next_lifecycle_time) {
      return true;
    }

    virtual ~AnimationFrameCallback() = default;
  };

  /**
   * Return the number of callbacks that have registered for frame callbacks.
   */
  int GetAnimationCount() { return GetCallbackSize(); }

  /**
   * Register to get a callback on the next frame after the delay.
   */
  void AddAnimationFrameCallback(AnimationFrameCallback* callback,
                                 int64_t delay);

  /**
   * Removes the given callback from the list, so it will no longer be called
   * for frame related timing.
   */
  void RemoveCallback(AnimationFrameCallback* callback);

  /**
   * Runs due callbacks for this frame.
   * @return true if there is at least one visible callback that still needs
   * future frame scheduling.
   */
  bool DoAnimationFrame(int64_t frame_time, bool lifecycle_only = false);

  void ScheduleLifecycleCallback(int64_t current_time);

  // Return time in milliseconds
  int64_t GetCurrentAnimationTime() { return last_frame_time_; }

  void SetAnimationCallback(std::function<void(int64_t)> animation_callback) {
    animation_callback_ = std::move(animation_callback);
  }

  void ClearCallbacks() {
    callback_delay_time_map_.clear();
    animation_callbacks_.clear();
    animation_callback_ = nullptr;
    scheduled_lifecycle_time_ = -1;
  }

 private:
  bool IsCallbackDue(AnimationFrameCallback* callback, int64_t current_time);
  void ScheduleLifecycleCallbackAt(int64_t next_lifecycle_time,
                                   int64_t current_time);
  int GetCallbackSize() { return callback_delay_time_map_.size(); }

  std::unordered_map<AnimationFrameCallback*, int64_t> callback_delay_time_map_;
  std::forward_list<AnimationFrameCallback*> animation_callbacks_;
  std::function<void(int64_t)> animation_callback_;
  bool callback_list_dirty_ = false;
  int64_t last_frame_time_ = -1;
  int64_t scheduled_lifecycle_time_ = -1;
};

}  // namespace clay

#endif  // CLAY_GFX_ANIMATION_ANIMATION_HANDLER_H_
