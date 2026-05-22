/*
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Copyright 2021 The Lynx Authors. All rights reserved.
 * Licensed under the Apache License Version 2.0 that can be found in the
 * LICENSE file in the root directory of this source tree.
 */

#ifndef CLAY_GFX_ANIMATION_ANIMATOR_H_
#define CLAY_GFX_ANIMATION_ANIMATOR_H_

#include <forward_list>
#include <memory>
#include <string>
#include <utility>

#include "clay/gfx/animation/animator_listener.h"

namespace clay {

class Interpolator;

/**
 * This is the superclass for classes which provide basic support for
 * animations which can be started, ended, and have AnimatorListeners
 * added to them.
 */
class Animator {
 public:
  /**
   * The value used to indicate infinite duration (e.g. when Animators repeat
   * infinitely).
   */
  static constexpr int kDurationInfinite = -1;

  virtual ~Animator() = default;

  /**
   * Starts this animation. If the animation has a nonzero startDelay, the
   * animation will start running after that delay elapses. A non-delayed
   * animation will have its initial value(s) set immediately, followed by
   * calls to AnimatorListener#OnAnimationStart(Animator) for any listeners
   * of this animator.
   */
  virtual void Start() = 0;

  /**
   * Cancels the animation. Unlike End(), Cancel() causes the animation to
   * stop in its tracks, sending an
   * AnimatorListener#OnAnimationCancel(Animator) to its listeners, followed
   * by an AnimatorListener#OnAnimationEnd(Animator) message.
   */
  virtual void Cancel() = 0;

  /**
   * Ends the animation. This causes the animation to assign the end value of
   * the property being animated, then calling the
   * AnimatorListener#OnAnimationEnd(Animator) method on its listeners.
   */
  virtual void End() = 0;

  /**
   * Pauses a running animation. This method should only be called on the
   * same thread on which the animation was started. If the animation has
   * not yet been IsStarted() or has since ended, then the call is ignored.
   * Paused animations can be resumed by calling Resume().
   */
  virtual void Pause();

  /**
   * Resumes a paused animation, causing the animator to pick up where it
   * left off when it was paused. Calls to Resume() on an animator that is
   * not currently paused will be ignored.
   */
  virtual void Resume();

  /**
   * The amount of time, in milliseconds, to delay processing the animation
   * after Start() is called.
   *
   * @return the number of milliseconds to delay running the animation
   */
  virtual int64_t GetStartDelay() = 0;

  /**
   * The amount of time, in milliseconds, to delay processing the animation
   * after Start() is called.

   * @param startDelay The amount of the delay, in milliseconds
   */
  virtual void SetStartDelay(int64_t start_delay) = 0;

  /**
   * Sets the duration of the animation.
   *
   * @param duration The length of the animation, in milliseconds.
   */
  virtual void SetDuration(int64_t duration) = 0;

  /**
   * Gets the duration of the animation.
   *
   * @return The length of the animation, in milliseconds.
   */
  virtual int64_t GetDuration() = 0;

  /**
   * Gets the total duration of the animation, accounting for animation
   * sequences, start delay, and repeating.
   *
   * @return  Total time an animation takes to finish, starting from the time
   *          Start() is called. kDurationInfinite will be returned if the
   *          animation or any child animation repeats infinite times.
   */
  virtual int64_t GetTotalDuration();

  /**
   * The time interpolator used in calculating the elapsed fraction of the
   * animation. The interpolator determines whether the animation runs with
   * linear or non-linear motion, such as acceleration and deceleration.
   *
   * @param value the interpolator to be used by this animation
   */
  virtual void SetInterpolator(std::unique_ptr<Interpolator> value) = 0;

  virtual Interpolator* GetInterpolator() { return nullptr; }

  /**
   * Returns whether this Animator is currently running (having been started
   * and gone past any initial startDelay period and not yet ended).
   */
  virtual bool IsRunning() = 0;

  /**
   * Whether the Animator has been started and not yet ended.
   */
  virtual bool IsStarted() {
    // Default method returns value for IsRunning(). Subclasses should
    // override to return a real value.
    return IsRunning();
  }

  virtual bool IsInitialized() { return true; }

  bool IsPaused() { return paused_; }

  void AddListener(AnimatorListener* listener) {
    listeners_.push_front(listener);
  }

  void RemoveListener(AnimatorListener* listener) {
    listeners_.remove(listener);
  }

  std::forward_list<AnimatorListener*> GetListeners() { return listeners_; }

  void AddPauseListener(AnimatorPauseListener* listener) {
    pause_listeners_.push_front(listener);
  }

  void RemovePauseListener(AnimatorPauseListener* listener) {
    pause_listeners_.remove(listener);
  }

  void RemoveAllListeners() {
    listeners_.clear();
    pause_listeners_.clear();
  }

  void SetAnimationName(std::string animation_name) {
    animation_name_ = std::move(animation_name);
  }

  const std::string& GetAnimationName() const { return animation_name_; }

 protected:
  virtual bool CanReverse() { return false; }

  virtual void Reverse() {}

  // Pulse an animation frame into the animation.
  virtual bool PulseAnimationFrame(int64_t frame_time) { return false; }

  /**
   * Internal use only.
   * This call starts the animation in regular or reverse direction without
   * requiring them to register frame callbacks. The caller will be
   * responsible for all the subsequent animation pulses. Specifically,
   * the caller needs to call doAnimationFrame(...) for the animation on
   * every frame.
   *
   * @param in_reverse whether the animation should play in reverse direction
   */
  virtual void StartWithoutPulsing(bool in_reverse) {
    if (in_reverse) {
      Reverse();
    } else {
      Start();
    }
  }

  /**
   * Internal use only.
   * Skips the animation value to end/start, depending on whether the play
   * direction is forward or backward.
   *
   * @param in_reverse whether the end value is based on a reverse direction.
   *                   If yes, this is equivalent to skip to start value in
   *                   a forward playing direction.
   */
  virtual void SkipToEndValue(bool in_reverse) {}

  /**
   * Internal use only.
   */
  virtual void AnimateBasedOnPlayTime(int64_t current_play_time,
                                      int64_t last_play_time, bool in_reverse) {
  }

  /**
   * The set of listeners to be sent events through the life of an animation.
   */
  std::forward_list<AnimatorListener*> listeners_;

  /**
   * The set of listeners to be sent pause/resume events through the life
   * of an animation.
   */
  std::forward_list<AnimatorPauseListener*> pause_listeners_;

  /**
   * Whether this animator is currently in a paused state.
   */
  bool paused_ = false;

  // bind with a specific animation_name
  std::string animation_name_;
};

}  // namespace clay

#endif  // CLAY_GFX_ANIMATION_ANIMATOR_H_
