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

#include "clay/gfx/animation/value_animator.h"

#include <algorithm>
#include <cmath>
#include <optional>
#include <utility>

#include "clay/fml/logging.h"
#include "clay/gfx/animation/animation_data.h"
#include "clay/gfx/animation/animation_handler.h"
#include "clay/gfx/animation/interpolator.h"

namespace clay {

float ValueAnimator::duration_scale_s_ = 1.0f;
static ValueAnimator::RepeatMode FromClayAnimationDirectionType(
    ClayAnimationDirectionType type) {
  ValueAnimator::RepeatMode mode;
  switch (type) {
    case ClayAnimationDirectionType::kNormal:
      mode = ValueAnimator::kNormal;
      break;
    case ClayAnimationDirectionType::kReverse:
      mode = ValueAnimator::kReverse;
      break;
    case ClayAnimationDirectionType::kAlternate:
      mode = ValueAnimator::kAlternate;
      break;
    case ClayAnimationDirectionType::kAlternateReverse:
      mode = ValueAnimator::kAlternateReverse;
      break;
    default:
      mode = ValueAnimator::kNormal;
      break;
  }
  return mode;
}

static ValueAnimator::FillMode FromClayAnimationFillModeType(
    ClayAnimationFillModeType type) {
  ValueAnimator::FillMode mode;
  switch (type) {
    case ClayAnimationFillModeType::kNone:
      mode = ValueAnimator::kNone;
      break;
    case ClayAnimationFillModeType::kForwards:
      mode = ValueAnimator::kForwards;
      break;
    case ClayAnimationFillModeType::kBackwards:
      mode = ValueAnimator::kBackward;
      break;
    case ClayAnimationFillModeType::kBoth:
      mode = ValueAnimator::kBoth;
      break;
    default:
      mode = ValueAnimator::kNone;
      break;
  }
  return mode;
}

// NOTE: There is a known issue: When `animation-iteration-count` is set to 0,
// `iteration_count` will be `-1`, which will cause the animation to play
// infinitely. This problem should be solved in Lynx, and then we may need to
// update the related logic.
ValueAnimator::ValueAnimator(const AnimationData& animation_data) {
  Animator::SetAnimationName(animation_data.name);
  SetAnimationData(animation_data);
}

ValueAnimator::~ValueAnimator() { RemoveAnimationCallback(); }

/**
 * This function is called immediately before processing the first
 * animation frame of an animation. If there is a nonzero start_delay, the
 * function is called after that delay ends. It takes care of the final
 * initialization steps for the animation.
 */
void ValueAnimator::InitAnimation() {
  if (!initialized_) {
    initialized_ = true;
  }
}

int64_t ValueAnimator::GetTotalDuration() {
  if (repeat_count_ == kInfinite) {
    return kDurationInfinite;
  } else {
    return start_delay_ + (duration_ * (repeat_count_ + 1));
  }
}

/**
 * Sets the position of the animation to the specified point in time. This time
 * should be between 0 and the total duration of the animation, including any
 * repetition. If the animation has not yet been started, then it will not
 * advance forward after it is set to this time; it will simply set the time to
 * this value and perform any appropriate actions based on that time. If the
 * animation is already running, then SetCurrentPlayTime() will set the current
 * playing time to this value and continue playing from that point.
 *
 * @param play_time The time, in milliseconds, to which the animation is
 * advanced or rewound.
 */
void ValueAnimator::SetCurrentPlayTime(int64_t play_time) {
  float fraction =
      duration_ > 0 ? static_cast<float>(play_time) / duration_ : 1;
  SetCurrentFraction(fraction);
}

/**
 * Sets the position of the animation to the specified fraction. This fraction
 * should be between 0 and the total fraction of the animation, including any
 * repetition. That is, a fraction of 0 will position the animation at the
 * beginning, a value of 1 at the end, and a value of 2 at the end of a
 * reversing animator that repeats once. If the animation has not yet been
 * started, then it will not advance forward after it is set to this fraction;
 * it will simply set the fraction to this value and perform any appropriate
 * actions based on that fraction. If the animation is already running, then
 * SetCurrentFraction() will set the current fraction to this value and continue
 * playing from that point. {@link Animator.AnimatorListener} events are not
 * called due to changing the fraction; those events are only processed while
 * the animation is running.
 *
 * @param fraction The fraction to which the animation is advanced or rewound.
 * Values outside the range of 0 to the maximum fraction for the animator will
 * be clamped to the correct range.
 */
void ValueAnimator::SetCurrentFraction(float fraction) {
  InitAnimation();
  fraction = ClampFraction(fraction);
  start_time_committed_ =
      true;  // do not allow start time to be compensated for jank
  if (IsPulsingInternal()) {
    int64_t seekTime = static_cast<int64_t>(GetScaledDuration() * fraction);
    int64_t current_time = GetAnimationHandler()->GetCurrentAnimationTime();
    // Only modify the start time when the animation is running. Seek fraction
    // will ensure non-running animations skip to the correct start time.
    start_time_ = current_time - seekTime;
  } else {
    // If the animation loop hasn't started, or during start delay, the
    // startTime will be adjusted once the delay has passed based on seek
    // fraction.
    seek_fraction_ = fraction;
  }
  overall_fraction_ = fraction;
  float currentIterationFraction =
      GetCurrentIterationFraction(fraction, reversing_);
  AnimateValue(currentIterationFraction);
}

/**
 * Calculates current iteration based on the overall fraction. The overall
 * fraction will be in the range of [0, repeat_count_ + 1]. Both current
 * iteration and fraction in the current iteration can be derived from it.
 */
int ValueAnimator::GetCurrentIteration(float fraction) {
  fraction = ClampFraction(fraction);
  // If the overall fraction is a positive integer, we consider the current
  // iteration to be complete. In other words, the fraction for the current
  // iteration would be 1, and the current iteration would be overall fraction
  // - 1.
  float iteration = std::floor(fraction);
  if (fraction == iteration && fraction > 0) {
    iteration--;
  }
  return static_cast<int>(iteration);
}

/**
 * Calculates the fraction of the current iteration, taking into account whether
 * the animation should be played backwards. E.g. When the animation is played
 * backwards in an iteration, the fraction for that iteration will go from 1.f
 * to 0.f.
 */
float ValueAnimator::GetCurrentIterationFraction(float fraction,
                                                 bool in_reverse) {
  fraction = ClampFraction(fraction);
  int iteration = GetCurrentIteration(fraction);
  float currentFraction = fraction - iteration;
  return ShouldPlayBackward(iteration, in_reverse) ? 1.f - currentFraction
                                                   : currentFraction;
}

/**
 * Clamps fraction into the correct range: [0, repeat_count_ + 1]. If repeat
 * count is infinite, no upper bound will be set for the fraction.
 *
 * @param fraction fraction to be clamped
 * @return fraction clamped into the range of [0, repeat_count_ + 1]
 */
float ValueAnimator::ClampFraction(float fraction) {
  if (fraction < 0) {
    fraction = 0;
  } else if (repeat_count_ != kInfinite) {
    fraction = std::min(fraction, repeat_count_ + 1.f);
  }
  return fraction;
}

/**
 * Calculates the direction of animation playing (i.e. forward or backward),
 * based on 1) whether the entire animation is being reversed, 2) repeat mode
 * applied to the current iteration.
 */
bool ValueAnimator::ShouldPlayBackward(int iteration, bool in_reverse) {
  if (iteration > 0 &&
      (repeat_mode_ == kAlternate || repeat_mode_ == kAlternateReverse) &&
      (iteration < (repeat_count_ + 1) || repeat_count_ == kInfinite)) {
    // if we were seeked to some other iteration in a reversing animator,
    // figure out the correct direction to start playing based on the iteration
    if (in_reverse) {
      return (iteration % 2) == 0;
    } else {
      return (iteration % 2) != 0;
    }
  } else {
    return in_reverse;
  }
}

/**
 * Gets the current position of the animation in time, which is equal to the
 * current time minus the time that the animation started. An animation that is
 * not yet started will return a value of zero, unless the animation has has its
 * play time set via
 * {@link #SetCurrentPlayTime(int64_t)} or {@link #SetCurrentFraction(float)},
 * in which case it will return the time that was set.
 *
 * @return The current position in time of the animation.
 */
int64_t ValueAnimator::GetCurrentPlayTime() {
  if (!initialized_ || (!started_ && seek_fraction_ < 0)) {
    return 0;
  }
  if (seek_fraction_ >= 0) {
    return static_cast<int64_t>(duration_ * seek_fraction_);
  }
  float durationScale = ResolveDurationScale();
  if (durationScale == 0.f) {
    durationScale = 1.f;
  }
  return static_cast<int64_t>(
      (GetAnimationHandler()->GetCurrentAnimationTime() - start_time_) /
      durationScale);
}

void ValueAnimator::SetStartDelay(int64_t start_delay) {
  // Clamp start delay to non-negative range.
  if (start_delay < 0) {
    start_delay = 0;
  }
  start_delay_ = start_delay;
}

void ValueAnimator::SetInterpolator(std::unique_ptr<Interpolator> value) {
  if (value) {
    interpolator_ = std::move(value);
  } else {
    interpolator_ = Interpolator::CreateDefaultInterpolator();
  }
}

void ValueAnimator::NotifyStartListeners() {
  if (!start_listeners_called_) {
    std::forward_list<AnimatorListener*> tmp_listeners = listeners_;
    for (AnimatorListener* listener : tmp_listeners) {
      listener->OnAnimationStart(*this);
    }
  }
  start_listeners_called_ = true;
}

/**
 * Start the animation playing. This version of Start() takes a bool flag that
 * indicates whether the animation should play in reverse. The flag is usually
 * false, but may be set to true if called from the Reverse() method.
 *
 * <p>The animation started by calling this method will be run on the thread
 * that called this method. This thread should have a Looper on it (a runtime
 * exception will be thrown if this is not the case). Also, if the animation
 * will animate properties of objects in the view hierarchy, then the calling
 * thread should be the UI thread for that view hierarchy.</p>
 *
 * @param play_backwards Whether the ValueAnimator should start playing in
 * reverse.
 */
void ValueAnimator::Start(bool play_backwards) {
  reversing_ = play_backwards;
  self_pulse_ = !suppress_self_pulse_requested_;
  // Special case: reversing from seek-to-0 should act as if not seeked at all.
  if (play_backwards && seek_fraction_ != -1 && seek_fraction_ != 0) {
    if (repeat_count_ == kInfinite) {
      // Calculate the fraction of the current iteration.
      float fraction =
          static_cast<float>(seek_fraction_ - std::floor(seek_fraction_));
      seek_fraction_ = 1 - fraction;
    } else {
      seek_fraction_ = 1 + repeat_count_ - seek_fraction_;
    }
  }
  started_ = true;
  end_listeners_called_ = false;
  paused_ = false;
  running_ = false;
  animation_end_requested_ = false;
  // Resets last_frame_time_ when Start() is called, so that if the animation
  // was running, calling Start() would put the animation in the
  // started-but-not-yet-reached-the-first-frame phase.
  last_frame_time_ = -1;
  first_frame_time_ = -1;
  start_time_ = -1;
  AddAnimationCallback(0);

  if (start_delay_ == 0 || seek_fraction_ >= 0 || reversing_) {
    // If there's no start delay, init the animation and notify start listeners
    // right away to be consistent with the previous behavior. Otherwise,
    // postpone this until the first frame after the start delay.
    StartAnimation();
    if (seek_fraction_ == -1) {
      // No seek, start at play time 0. Note that the reason we are not using
      // fraction 0 is because for animations with 0 duration, we want to be
      // consistent with pre-N behavior: skip to the final value immediately.
      SetCurrentPlayTime(0);
    } else {
      SetCurrentFraction(seek_fraction_);
    }
  }
}

void ValueAnimator::StartWithoutPulsing(bool in_reverse) {
  suppress_self_pulse_requested_ = true;
  if (in_reverse) {
    Reverse();
  } else {
    Start();
  }
  suppress_self_pulse_requested_ = false;
}

void ValueAnimator::Cancel() {
  // If end has already been requested, through a previous End() or Cancel()
  // call, no-op until animation starts again.
  if (animation_end_requested_) {
    return;
  }
  // Only cancel if the animation is actually running or has been started and is
  // about to run Only notify listeners if the animator has actually started
  if ((started_ || running_) && !end_listeners_called_) {
    if (!running_) {
      // If it's not yet running, then start listeners weren't called. Call them
      // now.
      NotifyStartListeners();
    }
    std::forward_list<AnimatorListener*> tmp_listeners = listeners_;
    for (AnimatorListener* listener : tmp_listeners) {
      listener->OnAnimationCancel(*this);
    }
  }
  std::forward_list<AnimatorListener*> tmp_listeners = listeners_;
  for (AnimatorListener* listener : tmp_listeners) {
    listener->OnAnimationRemove(*this);
  }
  RemoveAnimationCallback();
  Reset();
}

void ValueAnimator::End() {
  if (!running_) {
    // Special case if the animation has not yet started; get it ready for
    // ending
    StartAnimation();
    started_ = true;
  } else if (!initialized_) {
    InitAnimation();
  }
  AnimateValue(ShouldPlayBackward(repeat_count_, reversing_) ? 0.f : 1.f);
  EndAnimation();
}

void ValueAnimator::Resume() {
  if (paused_ && !resumed_) {
    resumed_ = true;
    if (pause_time_ > 0) {
      AddAnimationCallback(0);
    }
  }
  Animator::Resume();
}

void ValueAnimator::Pause() {
  bool previouslyPaused = paused_;
  Animator::Pause();
  if (!previouslyPaused && paused_) {
    pause_time_ = -1;
    resumed_ = false;
  }
}

bool ValueAnimator::IsRunning() { return running_; }

bool ValueAnimator::IsStarted() { return started_; }

void ValueAnimator::Reverse() {
  if (IsPulsingInternal()) {
    int64_t current_time = GetAnimationHandler()->GetCurrentAnimationTime();
    int64_t current_play_time = current_time - start_time_;
    int64_t timeLeft = GetScaledDuration() - current_play_time;
    start_time_ = current_time - timeLeft;
    start_time_committed_ =
        true;  // do not allow start time to be compensated for jank
    reversing_ = !reversing_;
  } else if (started_) {
    reversing_ = !reversing_;
    End();
  } else {
    Start(true);
  }
}

bool ValueAnimator::CanReverse() { return true; }

std::unique_ptr<ValueAnimator> ValueAnimator::Clone() const {
  std::unique_ptr<ValueAnimator> clone = std::make_unique<ValueAnimator>();
  clone->animation_name_ = animation_name_;
  clone->paused_ = paused_;
  clone->start_time_ = start_time_;
  clone->start_time_committed_ = start_time_committed_;
  clone->seek_fraction_ = seek_fraction_;
  clone->pause_time_ = pause_time_;
  clone->resumed_ = resumed_;
  clone->reversing_ = reversing_;
  clone->overall_fraction_ = overall_fraction_;
  clone->current_fraction_ = current_fraction_;
  clone->last_frame_time_ = last_frame_time_;
  clone->first_frame_time_ = first_frame_time_;
  clone->running_ = running_;
  clone->started_ = started_;
  clone->start_listeners_called_ = false;
  clone->initialized_ = initialized_;
  clone->animation_end_requested_ = animation_end_requested_;
  clone->duration_ = duration_;
  clone->start_delay_ = start_delay_;
  clone->repeat_count_ = repeat_count_;
  clone->repeat_mode_ = repeat_mode_;
  clone->fill_mode_ = fill_mode_;
  clone->self_pulse_ = self_pulse_;
  clone->suppress_self_pulse_requested_ = suppress_self_pulse_requested_;
  clone->interpolator_ = interpolator_->Clone();
  clone->duration_scale_ = duration_scale_;
  return clone;
}

void ValueAnimator::Reset() {
  animation_end_requested_ = true;
  paused_ = false;
  running_ = false;
  started_ = false;
  start_listeners_called_ = false;
  last_frame_time_ = -1;
  first_frame_time_ = -1;
  start_time_ = -1;
  // reversing_ needs to be reset *after* notifying the listeners for the end
  // callbacks.
  reversing_ = false;
}

/**
 * Called internally to end an animation by removing it from the animations
 * list. Must be called on the UI thread.
 */
void ValueAnimator::EndAnimation(bool remove) {
  if ((animation_end_requested_ || end_listeners_called_) && !remove) {
    return;
  }
  if (started_ && !running_) {
    // If it's not yet running, then start listeners weren't called. Call them
    // now.
    NotifyStartListeners();
  }
  // `running_` should remain true when forwarding.
  if (remove) {
    running_ = false;
  }
  // should not notify end event duplicated.
  // end_listeners_called_ will be set false when animator restart.
  if ((started_ || running_) && !end_listeners_called_) {
    std::forward_list<AnimatorListener*> tmp_listeners = listeners_;
    for (AnimatorListener* l : tmp_listeners) {
      l->OnAnimationEnd(*this);
    }
  }
  end_listeners_called_ = true;
  if (!remove) {
    return;
  }
  std::forward_list<AnimatorListener*> tmp_listeners = listeners_;
  for (AnimatorListener* l : tmp_listeners) {
    l->OnAnimationRemove(*this);
  }
  RemoveAnimationCallback();
  Reset();
}

/**
 * Called internally to start an animation by adding it to the active animations
 * list. Must be called on the UI thread.
 */
void ValueAnimator::StartAnimation() {
  animation_end_requested_ = false;
  InitAnimation();
  running_ = true;
  if (seek_fraction_ >= 0) {
    overall_fraction_ = seek_fraction_;
  } else {
    overall_fraction_ = 0.f;
  }
  NotifyStartListeners();
}

/**
 * Internal only: This tracks whether the animation has gotten on the animation
 * loop. Note this is different than {@link #IsRunning()} in that the latter
 * tracks the time after Start() is called (or after start delay if any), which
 * may be before the animation loop starts.
 */
bool ValueAnimator::IsPulsingInternal() { return last_frame_time_ >= 0; }

/**
 * This internal function processes a single animation frame for a given
 * animation. The current_time parameter is the timing pulse sent by the
 * handler, used to calculate the elapsed duration, and therefore the elapsed
 * fraction, of the animation. The return value indicates whether the animation
 * should be ended (which happens when the elapsed time of the animation exceeds
 * the animation's duration, including the repeatCount).
 *
 * @param current_time The current time, as tracked by the static timing handler
 * @return true if the animation's duration, including any repetitions due to
 * <code>repeatCount</code> has been exceeded and the animation should be ended.
 */
bool ValueAnimator::AnimateBasedOnTime(int64_t current_time) {
  bool done = false;
  if (running_) {
    int64_t scaledDuration = GetScaledDuration();
    float fraction =
        scaledDuration > 0
            ? static_cast<float>(current_time - start_time_) / scaledDuration
            : 1.f;
    float lastFraction = overall_fraction_;
    bool newIteration =
        static_cast<int>(fraction) > static_cast<int>(lastFraction);
    bool lastIterationFinished =
        (fraction >= repeat_count_ + 1) && (repeat_count_ != kInfinite);
    if (scaledDuration == 0) {
      // 0 duration animator, ignore the repeat count and skip to the end
      done = true;
    } else if (newIteration && !lastIterationFinished) {
      // Time to repeat
      for (auto l : listeners_) {
        l->OnAnimationRepeat(*this);
      }
    } else if (lastIterationFinished) {
      done = true;
    }
    overall_fraction_ = ClampFraction(fraction);
    float currentIterationFraction =
        GetCurrentIterationFraction(overall_fraction_, reversing_);
    AnimateValue(currentIterationFraction);
  }
  return done;
}

/**
 * Internal use only.
 *
 * This method does not modify any fields of the animation. It should be called
 * when seeking in an AnimatorSet. When the last play time and current play time
 * are of different repeat iterations,
 * {@link
 * android.view.animation.Animation.AnimationListener#OnAnimationRepeat(Animation)}
 * will be called.
 */
void ValueAnimator::AnimateBasedOnPlayTime(int64_t current_play_time,
                                           int64_t last_play_time,
                                           bool in_reverse) {
  if (current_play_time < 0 || last_play_time < 0) {
    FML_UNREACHABLE();
  }

  InitAnimation();
  // Check whether repeat callback is needed only when repeat count is non-zero
  if (repeat_count_ > 0) {
    int iteration = static_cast<int>(current_play_time / duration_);
    int lastIteration = static_cast<int>(last_play_time / duration_);

    // Clamp iteration to [0, repeat_count_]
    iteration = std::min(iteration, repeat_count_);
    lastIteration = std::min(lastIteration, repeat_count_);

    if (iteration != lastIteration) {
      for (auto l : listeners_) {
        l->OnAnimationRepeat(*this);
      }
    }
  }

  if (repeat_count_ != kInfinite &&
      current_play_time >= (repeat_count_ + 1) * duration_) {
    SkipToEndValue(in_reverse);
  } else {
    // Find the current fraction:
    float fraction = current_play_time / static_cast<float>(duration_);
    fraction = GetCurrentIterationFraction(fraction, in_reverse);
    AnimateValue(fraction);
  }
}

/**
 * Internal use only.
 * Skips the animation value to end/start, depending on whether the play
 * direction is forward or backward.
 *
 * @param in_reverse whether the end value is based on a reverse direction. If
 * yes, this is equivalent to skip to start value in a forward playing
 * direction.
 */
void ValueAnimator::SkipToEndValue(bool in_reverse) {
  InitAnimation();
  float endFraction = in_reverse ? 0.f : 1.f;
  if (repeat_count_ % 2 == 1 &&
      (repeat_mode_ == kAlternate || repeat_mode_ == kAlternateReverse)) {
    // This would end on fraction = 0
    endFraction = 0.f;
  }
  AnimateValue(endFraction);
}

/**
 * Processes a frame of the animation, adjusting the start time if needed.
 *
 * @param frame_time The frame time.
 * @return true if the animation has ended.
 * @hide
 */
bool ValueAnimator::DoAnimationFrame(int64_t frame_time) {
  if (start_time_ < 0) {
    // First frame. If there is start delay, start delay count down will happen
    // *after* this frame.
    start_time_ = std::max(
        static_cast<int64_t>(0),
        reversing_ ? frame_time
                   : frame_time + static_cast<int64_t>(start_delay_ *
                                                       ResolveDurationScale()));
  }

  RestartAnimationIfNeeded(frame_time);

  bool fill_backwards = fill_mode_ & kBackward;
  // Handle pause/resume
  if (paused_) {
    if (pause_time_ < 0) {
      pause_time_ = frame_time;
    }

    if (pause_time_ >= start_time_ || fill_backwards) {
      running_ = true;
      AnimateBasedOnTime(std::max(pause_time_, start_time_));
    }
    return false;
  } else if (resumed_) {
    resumed_ = false;
    if (pause_time_ > 0) {
      // Offset by the duration that the animation was paused
      start_time_ += (frame_time - pause_time_);
    }
  }

  if (!running_) {
    // If not running, that means the animation is in the start delay phase of a
    // forward running animation. In the case of reversing, we want to run start
    // delay in the end.
    if (!fill_backwards && start_time_ > frame_time && seek_fraction_ == -1) {
      // This is when no seek fraction is set during start delay. If developers
      // change the seek fraction during the delay, animation will start from
      // the seeked position right away.
      return false;
    } else {
      // If running_ is not set by now, that means non-zero start delay,
      // no seeking, not reversing. At this point, start delay has passed.
      StartAnimation();
    }
  }

  if (!start_listeners_called_) {
    NotifyStartListeners();
  }

  if (last_frame_time_ < 0) {
    if (seek_fraction_ >= 0) {
      int64_t seekTime =
          static_cast<int64_t>(GetScaledDuration() * seek_fraction_);
      start_time_ = frame_time - seekTime;
      seek_fraction_ = -1;
    }
    start_time_committed_ =
        false;  // allow start time to be compensated for jank
  }
  last_frame_time_ = frame_time;
  // The frame time might be before the start time during the first frame of
  // an animation.  The "current time" must always be on or after the start
  // time to avoid animating frames at negative time intervals.  In practice,
  // this is very rare and only happens when seeking backwards.
  int64_t current_time = std::max(frame_time, start_time_);
  bool finished = AnimateBasedOnTime(current_time);

  if (finished) {
    EndAnimation(!(fill_mode_ & kForwards));
  }
  return finished;
}

bool ValueAnimator::PulseAnimationFrame(int64_t frame_time) {
  if (self_pulse_) {
    // Pulse animation frame will *always* be after calling Start(). If
    // self_pulse_ isn't set to false at this point, that means child animators
    // did not call super's Start(). This can happen when the Animator is just a
    // non-animating wrapper around a real functional animation. In this case,
    // we can't really pulse a frame into the animation, because the animation
    // cannot necessarily be properly initialized (i.e. no start/end values
    // set).
    return false;
  }
  return DoAnimationFrame(frame_time);
}

void ValueAnimator::RemoveAnimationCallback() {
  if (!self_pulse_) {
    return;
  }
  GetAnimationHandler()->RemoveCallback(this);
}

void ValueAnimator::AddAnimationCallback(int64_t delay) {
  if (!self_pulse_) {
    return;
  }
  GetAnimationHandler()->AddAnimationFrameCallback(this, delay);
}

/**
 * This method is called with the elapsed fraction of the animation during every
 * animation frame. This function turns the elapsed fraction into an
 * interpolated fraction and then into an animated value (from the evaluator.
 * The function is called mostly during animation updates, but it is also called
 * when the End() function is called, to set the final value on the property.
 *
 * @param fraction The elapsed fraction of the animation.
 */
void ValueAnimator::AnimateValue(float fraction) {
  if (interpolator_) {
    fraction = interpolator_->Interpolate(fraction);
  }
  current_fraction_ = fraction;
  for (auto l : update_listeners_) {
    l->OnAnimationUpdate(*this);
  }
}

/**
 * Returns the current animation fraction, which is the elapsed/interpolated
 * fraction used in the most recent frame update on the animation.
 */
float ValueAnimator::GetAnimatedFraction() {
  // For the following reverse modes, we just return a different elapsed
  // fraction, apart from this, these modes are handled just like their
  // non-reverse counterparts.
  if (repeat_mode_ == kReverse || repeat_mode_ == kAlternateReverse) {
    return 1.0f - current_fraction_;
  } else {
    return current_fraction_;
  }
}

// Return the number of animations currently running.
int ValueAnimator::GetCurrentAnimationsCount() {
  return GetAnimationHandler()->GetAnimationCount();
}

// Return the AnimationHandler that will be used to schedule updates for
// this animator.
AnimationHandler* ValueAnimator::GetAnimationHandler() {
  static AnimationHandler sDefaultHandler;
  return animation_handler_ ? animation_handler_ : &sDefaultHandler;
}

// Sets the animation handler used to schedule updates for this animator
// or null to use the default handler.
void ValueAnimator::SetAnimationHandler(AnimationHandler* animation_handler) {
  animation_handler_ = animation_handler;
}

void ValueAnimator::SetAnimationData(AnimationData animation_data) {
  duration_ = animation_data.duration;
  repeat_mode_ = FromClayAnimationDirectionType(animation_data.direction);
  fill_mode_ = FromClayAnimationFillModeType(animation_data.fill_mode);
  repeat_count_ = animation_data.iteration_count;
  start_delay_ = animation_data.delay;
  start_time_ += start_delay_;
  std::unique_ptr<Interpolator> value =
      Interpolator::Create(animation_data.timing_func);
  if (value) {
    interpolator_ = std::move(value);
  } else {
    interpolator_ = Interpolator::CreateDefaultInterpolator();
  }
}

void ValueAnimator::RestartAnimationIfNeeded(int64_t current_time) {
  auto fill_forwards = fill_mode_ & kForwards;
  // when animation fillmode change forwards from backwards ,"running_" needs to
  // be set to true
  if (fill_forwards) {
    running_ = true;
  }

  if (start_time_ + duration_ > current_time && end_listeners_called_) {
    start_listeners_called_ = false;
    end_listeners_called_ = false;
    // When the fillmode changes from forwards to non-forwards we need to
    // set view value with original_value_
    if (!fill_forwards) {
      std::forward_list<AnimatorListener*> tmp_listeners = listeners_;
      for (AnimatorListener* listener : tmp_listeners) {
        listener->OnAnimationRemove(*this);
      }
    }
    NotifyStartListeners();
  }
}

}  // namespace clay
