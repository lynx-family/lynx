// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_ANIMATION_ANIMATOR_LISTENER_ADAPTER_H_
#define CLAY_GFX_ANIMATION_ANIMATOR_LISTENER_ADAPTER_H_

#include "clay/gfx/animation/animator_listener.h"

namespace clay {

/**
 * This adapter class provides empty implementations of the methods from
 * AnimatorListener and AnimatorPauseListener.
 */
class AnimatorListenerAdapter : public AnimatorListener,
                                public AnimatorPauseListener,
                                public AnimatorUpdateListener {
 public:
  void OnAnimationStart(Animator& animation) override {}
  void OnAnimationEnd(Animator& animation) override {}
  void OnAnimationCancel(Animator& animation) override {}
  void OnAnimationRepeat(Animator& animation) override {}

  void OnAnimationPause(Animator& animation) override {}
  void OnAnimationResume(Animator& animation) override {}

  void OnAnimationUpdate(ValueAnimator& animation) override {}
};

}  // namespace clay

#endif  // CLAY_GFX_ANIMATION_ANIMATOR_LISTENER_ADAPTER_H_
