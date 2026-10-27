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
 *
 */

#ifndef CLAY_GFX_ANIMATION_VALUE_ANIMATOR_H_
#define CLAY_GFX_ANIMATION_VALUE_ANIMATOR_H_

#include <forward_list>
#include <memory>

#include "clay/gfx/animation/animation_data.h"
#include "clay/gfx/animation/animation_handler.h"
#include "clay/gfx/animation/animator.h"
#include "clay/gfx/animation/interpolator.h"
#include "third_party/googletest/googletest/include/gtest/gtest_prod.h"  // nogncheck

namespace clay {

/**
 * This class provides a simple timing engine for running animations
 * which calculate animated values and set them on target objects.
 *
 * There is a single timing pulse that all animations use.
 */
class ValueAnimator : public Animator,
                      public AnimationHandler::AnimationFrameCallback {
 public:
  ~ValueAnimator() override;
  ValueAnimator() = default;
  explicit ValueAnimator(const AnimationData& animation_data);

  /**
   * Starts this animation. If the animation has a nonzero startDelay, the
   * animation will start running after that delay elapses. A non-delayed
   * animation will have its initial value(s) set immediately, followed by
   * calls to AnimatorListener#OnAnimationStart(Animator) for any listeners
   * of this animator.
   */
  void Start() override { Start(false); }

  /**
   * Cancels the animation. Unlike End(), Cancel() causes the animation to
   * stop in its tracks, sending an
   * AnimatorListener#OnAnimationCancel(Animator) to its listeners, followed
   * by an AnimatorListener#OnAnimationEnd(Animator) message.
   */
  void Cancel() override;

  /**
   * Ends the animation. This causes the animation to assign the end value of
   * the property being animated, then calling the
   * AnimatorListener#OnAnimationEnd(Animator) method on its listeners.
   */
  void End() override;

  /**
   * Pauses a running animation. This method should only be called on the
   * same thread on which the animation was started. If the animation has
   * not yet been IsStarted() or has since ended, then the call is ignored.
   * Paused animations can be resumed by calling Resume().
   */
  void Pause() override;

  /**
   * Resumes a paused animation, causing the animator to pick up where it
   * left off when it was paused. Calls to Resume() on an animator that is
   * not currently paused will be ignored.
   */
  void Resume() override;

  /**
   * The amount of time, in milliseconds, to delay processing the animation
   * after Start() is called.
   *
   * @return the number of milliseconds to delay running the animation
   */
  int64_t GetStartDelay() override { return start_delay_; }

  /**
   * The amount of time, in milliseconds, to delay processing the animation
   * after Start() is called.

   * @param startDelay The amount of the delay, in milliseconds
   */
  void SetStartDelay(int64_t start_delay) override;

  /**
   * Sets the duration of the animation.
   *
   * @param duration The length of the animation, in milliseconds.
   */
  void SetDuration(int64_t duration) override { duration_ = duration; }

  /**
   * Gets the duration of the animation.
   *
   * @return The length of the animation, in milliseconds.
   */
  int64_t GetDuration() override { return duration_; }

  /**
   * Gets the total duration of the animation, accounting for animation
   * sequences, start delay, and repeating.
   *
   * @return  Total time an animation takes to finish, starting from the time
   *          Start() is called. kDurationInfinite will be returned if the
   *          animation or any child animation repeats infinite times.
   */
  int64_t GetTotalDuration() override;

  /**
   * The time interpolator used in calculating the elapsed fraction of the
   * animation. The interpolator determines whether the animation runs with
   * linear or non-linear motion, such as acceleration and deceleration.
   *
   * @param value the interpolator to be used by this animation
   */
  void SetInterpolator(std::unique_ptr<Interpolator> value) override;

  Interpolator* GetInterpolator() override { return interpolator_.get(); }

  /**
   * Returns whether this Animator is currently running (having been started
   * and gone past any initial startDelay period and not yet ended).
   */
  bool IsRunning() override;

  /**
   * Returns whether this Animator is currently running reversed.
   */
  bool IsReversing() { return IsRunning() && reversing_; }

  /**
   * Whether the Animator has been started and not yet ended.
   */
  bool IsStarted() override;

  bool IsInitialized() override { return initialized_; }

  bool DoAnimationFrame(int64_t frame_time) override;

  void RestartAnimationIfNeeded(int64_t current_time);

  /**
   * Adds a listener to the set of listeners that are sent update events through
   * the life of an animation. This method is called on all listeners for every
   * frame of the animation, after the values for the animation have been
   * calculated.
   *
   * @param listener the listener to be added to the current set of listeners
   * for this animation.
   */
  void AddUpdateListener(AnimatorUpdateListener* listener) {
    update_listeners_.push_front(listener);
  }

  void RemoveAllUpdateListeners() { update_listeners_.clear(); }

  /**
   * Removes a listener from the set listening to frame updates for this
   * animation.
   *
   * @param listener the listener to be removed from the current set of update
   * listeners for this animation.
   */
  void RemoveUpdateListener(AnimatorUpdateListener* listener) {
    update_listeners_.remove(listener);
  }

  /**
   * Sets how many times the animation should be repeated. If the repeat
   * count is 0, the animation is never repeated. If the repeat count is
   * greater than 0 or {@link #kInfinite}, the repeat mode will be taken
   * into account. The repeat count is 0 by default.
   *
   * @param value the number of times the animation should be repeated
   */
  void SetRepeatCount(int value) { repeat_count_ = value; }
  /**
   * Defines how many times the animation should repeat. The default value
   * is 0.
   *
   * @return the number of times the animation should repeat, or {@link
   * #kInfinite}
   */
  int GetRepeatCount() { return repeat_count_; }

  enum RepeatMode {
    /**
     * The animation plays forwards each cycle. In other words, each time the
     * animation cycles, the animation will reset to the beginning state and
     * start over again. This is the default value.
     */
    kNormal = 1,
    /**
     * The animation plays backwards each cycle. In other words, each time the
     * animation cycles, the animation will reset to the end state and start
     * over again. Animation steps are performed backwards, and timing functions
     * are also reversed.
     */
    kReverse = 2,
    /**
     * The animation reverses direction each cycle, with the first iteration
     * being played forwards. The count to determine if a cycle is even or odd
     * starts at one.
     */
    kAlternate = 3,
    /**
     * The animation reverses direction each cycle, with the first iteration
     * being played backwards. The count to determine if a cycle is even or
     * odd starts at one.
     */
    kAlternateReverse = 4,
    /**
     * This value used with the setRepeatCount(int) property to repeat
     * the animation indefinitely.
     */
    kInfinite = -1
  };

  enum FillMode {
    /**
     * The animation will not apply any styles to the target when it's not
     * executing. The element will instead be displayed using any other CSS
     * rules applied to it. This is the default value.
     */
    kNone = 0,
    /**
     * The target will retain the computed values set by the last keyframe
     * encountered during execution. The last keyframe depends on the value of
     * animation-direction and animation-iteration-count.
     */
    kForwards = 1 << 0,
    /**
     * The animation will apply the values defined in the first relevant
     * keyframe as soon as it is applied to the target, and retain this during
     * the animation-delay period. The first relevant keyframe depends on the
     * value of animation-direction.
     */
    kBackward = 1 << 1,
    /**
     * The animation will follow the rules for both forwards and backwards, thus
     * extending the animation properties in both directions.
     */
    kBoth = kForwards | kBackward,
  };

  /**
   * Defines what this animation should do when it reaches the end. This
   * setting is applied only when the repeat count is either greater than
   * 0 or kInfinite. Defaults to kNormal.
   */
  void SetRepeatMode(RepeatMode value) { repeat_mode_ = value; }

  RepeatMode GetRepeatMode() { return repeat_mode_; }

  void SetFillMode(FillMode value) { fill_mode_ = value; }

  FillMode GetFillMode() const { return fill_mode_; }

  /**
   * Returns the current animation fraction, which is the elapsed/interpolated
   * fraction used in the most recent frame update on the animation.
   *
   * @return Elapsed/interpolated fraction of the animation.
   */
  float GetAnimatedFraction();

  void SetCurrentPlayTime(int64_t play_time);
  void SetCurrentFraction(float fraction);

  // Return the number of animations currently running.
  int GetCurrentAnimationsCount();

  // Return the AnimationHandler that will be used to schedule updates for
  // this animator.
  AnimationHandler* GetAnimationHandler();

  // Sets the animation handler used to schedule updates for this animator
  // or null to use the default handler.
  void SetAnimationHandler(AnimationHandler* animation_handler);

  /**
   * Overrides the global duration scale by a custom value.
   *
   * @param duration_scale The duration scale to set;
   *                      or -1.f to use the global duration scale.
   */
  void OverrideDurationScale(float duration_scale) {
    duration_scale_ = duration_scale;
  }

  static void SetDurationScale(float duration_scale) {
    duration_scale_s_ = duration_scale;
  }

  static float GetDurationScale() { return duration_scale_s_; }

  // Returns whether animators are currently enabled, system-wide.
  static bool AreAnimatorsEnabled() { return !(duration_scale_s_ == 0); }

  /**
   * Plays the ValueAnimator in reverse. If the animation is already running,
   * it will stop itself and play backwards from the point reached when reverse
   * was called. If the animation is not currently running, then it will start
   * from the end and play backwards. This behavior is only set for the current
   * animation; future playing of the animation will use the default behavior of
   * playing forward.
   */
  void Reverse() override;

  // return A ValueAnimator with `start_listeners_called_` is false while other
  // properties remains.
  virtual std::unique_ptr<ValueAnimator> Clone() const;

  bool StartListenersCalled() const { return start_listeners_called_; }

  void SetStartListenersCalled(bool start_listeners_called) {
    start_listeners_called_ = start_listeners_called;
  }

  void SetAnimationData(AnimationData animation_data);

  void AddAnimationCallback(int64_t delay);

  // TODO(wangyanyi) Need to consider situations such as pausing、 iteration
  int64_t GetActivatedTime() { return last_frame_time_ - start_time_; }

 protected:
  bool CanReverse() override;

  // Pulse an animation frame into the animation.
  bool PulseAnimationFrame(int64_t frame_time) override;

  void StartWithoutPulsing(bool in_reverse) override;

  void SkipToEndValue(bool in_reverse) override;

  void AnimateBasedOnPlayTime(int64_t current_play_time, int64_t last_play_time,
                              bool in_reverse) override;

 private:
  void InitAnimation();
  void Reset();
  int GetCurrentIteration(float fraction);
  float GetCurrentIterationFraction(float fraction, bool in_reverse);
  float ClampFraction(float fraction);
  bool ShouldPlayBackward(int iteration, bool in_reverse);
  int64_t GetCurrentPlayTime();

  void NotifyStartListeners();
  void Start(bool play_backwards);

  // the remove param is used to solved to keyframe animation stop when
  // fill-mode=forward
  void EndAnimation(bool remove = true);
  void StartAnimation();
  bool IsPulsingInternal();
  bool AnimateBasedOnTime(int64_t current_time);

  void AnimateValue(float fraction);

  void AddOneShotCommitCallback();
  void RemoveAnimationCallback();

  float ResolveDurationScale() {
    return duration_scale_ >= 0.f ? duration_scale_ : duration_scale_s_;
  }

  int64_t GetScaledDuration() {
    return (int64_t)(duration_ * ResolveDurationScale());
  }

  FRIEND_TEST(KeyFrameTest, UpdateAnimation);

  /**
   * The first time that the animation's animateFrame() method is called. This
   * time is used to determine elapsed time (and therefore the elapsed fraction)
   * in subsequent calls to animateFrame().
   *
   * Whenever start_time_ is set, you must also update start_time_committed_.
   */
  int64_t start_time_ = -1;

  /**
   * When true, the start time has been firmly committed as a chosen reference
   * point in time by which the progress of the animation will be evaluated.
   * When false, the start time may be updated when the first animation frame is
   * committed so as to compensate for jank that may have occurred between when
   * the start time was initialized and when the frame was actually drawn.
   *
   * This flag is generally set to false during the first frame of the animation
   * when the animation playing state transitions from STOPPED to RUNNING or
   * resumes after having been paused.  This flag is set to true when the start
   * time is firmly committed and should not be further compensated for jank.
   */
  bool start_time_committed_;

  /**
   * Set when setCurrentPlayTime() is called. If negative, animation is not
   * currently seeked to a value.
   */
  float seek_fraction_ = -1;

  /**
   * Set on the next frame after pause() is called, used to calculate a new
   * startTime or delayStartTime which allows the animator to continue from the
   * point at which it was paused. If negative, has not yet been set.
   */
  int64_t pause_time_;

  /**
   * Set when an animator is resumed. This triggers logic in the next frame
   * which actually resumes the animator.
   */
  bool resumed_ = false;

  /**
   * Flag to indicate whether this animator is playing in reverse mode,
   * specifically by being started or interrupted by a call to reverse(). This
   * flag is different than mPlayingBackwards, which indicates merely whether
   * the current iteration of the animator is playing in reverse. It is used in
   * corner cases to determine proper end behavior.
   */
  bool reversing_;

  /**
   * Tracks the overall fraction of the animation, ranging from 0 to
   * repeat_count_ + 1
   */
  float overall_fraction_ = 0.f;

  /**
   * Tracks current elapsed/eased fraction, for querying in
   * getAnimatedFraction(). This is calculated by interpolating the fraction
   * (range: [0, 1]) in the current iteration.
   */
  float current_fraction_ = 0.f;

  /**
   * Tracks the time (in milliseconds) when the last frame arrived.
   */
  int64_t last_frame_time_ = -1;

  /**
   * Tracks the time (in milliseconds) when the first frame arrived. Note the
   * frame may arrive during the start delay.
   */
  int64_t first_frame_time_ = -1;

  /**
   * Additional playing state to indicate whether an animator has been
   * start()'d. There is some lag between a call to start() and the first
   * animation frame. We should still note that the animation has been started,
   * even if it's first animation frame has not yet happened, and reflect that
   * state in isRunning(). Note that delayed animations are different: they are
   * not started until their first animation frame, which occurs after their
   * delay elapses.
   */
  bool running_ = false;

  /**
   * Additional playing state to indicate whether an animator has been
   * start()'d, whether or not there is a nonzero startDelay.
   */
  bool started_ = false;

  /**
   * Tracks whether we've notified listeners of the OnAnimationStart() event.
   * This can be complex to keep track of since we notify listeners at different
   * times depending on startDelay and whether start() was called before end().
   */
  bool start_listeners_called_ = false;

  /**
   * Flag that denotes whether the animation is set up and ready to go. Used to
   * set up animation that has not yet been started.
   */
  bool initialized_ = false;

  bool animation_end_requested_ = false;

  // How long the animation should last in ms
  int64_t duration_ = 300;

  // The amount of time in ms to delay starting the animation after start() is
  // called. Note that this start delay is unscaled. When there is a duration
  // scale set on the animator, the scaling factor will be applied to this
  // delay.
  int64_t start_delay_ = 0;

  // The number of times the animation will repeat. The default is 0, which
  // means the animation will play only once
  int repeat_count_ = 0;

  /**
   * The type of repetition that will occur when repeatMode is nonzero. kNormal
   * means the animation will start from the beginning on every new cycle.
   * kAlternate means the animation will reverse directions on each iteration.
   */
  RepeatMode repeat_mode_ = kNormal;

  /**
   * Sets how a animation applies styles to its target before and after its
   * execution.
   */
  FillMode fill_mode_ = kNone;

  /**
   * Whether or not the animator should register for its own animation callback
   * to receive animation pulse.
   */
  bool self_pulse_ = true;

  /**
   * Whether or not the animator has been requested to start without pulsing.
   * This flag gets set in startWithoutPulsing(), and reset in start().
   */
  bool suppress_self_pulse_requested_ = false;

  /**
   * The time interpolator to be used. The elapsed fraction of the animation
   * will be passed through this interpolator to calculate the interpolated
   * fraction, which is then used to calculate the animated values.
   */
  std::unique_ptr<Interpolator> interpolator_;

  /**
   * The set of listeners to be sent events through the life of an animation.
   */
  std::forward_list<AnimatorUpdateListener*> update_listeners_;

  /**
   * Animation handler used to schedule updates for this animation.
   */
  AnimationHandler* animation_handler_ = nullptr;

  /**
   * If set to non-negative value, this will override duration_scale_s_.
   */
  float duration_scale_ = -1.f;

  // System-wide animation scale.
  static float duration_scale_s_;

  // in case the end listener called more than once under forwards mode
  bool end_listeners_called_ = false;
};

}  // namespace clay

#endif  // CLAY_GFX_ANIMATION_VALUE_ANIMATOR_H_
