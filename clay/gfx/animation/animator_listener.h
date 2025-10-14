// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_ANIMATION_ANIMATOR_LISTENER_H_
#define CLAY_GFX_ANIMATION_ANIMATOR_LISTENER_H_

namespace clay {

class Animator;
class ValueAnimator;

/**
 * An animation listener receives notifications from an animation.
 * Notifications indicate animation related events, such as the end or the
 * repetition of the animation.
 */
class AnimatorListener {
 public:
  virtual void OnAnimationStart(Animator& animation) = 0;
  virtual void OnAnimationEnd(Animator& animation) = 0;
  virtual void OnAnimationCancel(Animator& animation) = 0;
  virtual void OnAnimationRepeat(Animator& animation) = 0;
  // the OnAnimationRemove tells the callback removed from handler. so, the
  // value needs to reset in this method. On the other hand, OnAnimationEnd is
  // used to fire event to frontend.
  virtual void OnAnimationRemove(Animator& animation) {}

  virtual void OnAnimationStart(Animator& animation, bool isReverse) {
    OnAnimationStart(animation);
  }

  virtual ~AnimatorListener() = default;
};

/**
 * A pause listener receives notifications from an animation when the
 * animation is paused or resumed.
 *
 */
class AnimatorPauseListener {
 public:
  virtual void OnAnimationPause(Animator& animation) = 0;
  virtual void OnAnimationResume(Animator& animation) = 0;

  virtual ~AnimatorPauseListener() = default;
};

/**
 * Implementors of this interface can add themselves as update listeners
 * to an ValueAnimator instance to receive callbacks on every animation
 * frame, after the current frame's values have been calculated for that
 * ValueAnimator.
 */
class AnimatorUpdateListener {
 public:
  virtual void OnAnimationUpdate(ValueAnimator& animation) = 0;

  virtual ~AnimatorUpdateListener() = default;
};

}  // namespace clay

#endif  // CLAY_GFX_ANIMATION_ANIMATOR_LISTENER_H_
