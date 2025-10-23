// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_ANIMATION_ANIMATION_EVENT_HANDLER_H_
#define CLAY_GFX_ANIMATION_ANIMATION_EVENT_HANDLER_H_

#include "clay/gfx/animation/animation_data.h"
#include "clay/public/style_types.h"

namespace clay {
class AnimationEventHandler {
 public:
  AnimationEventHandler() = default;
  virtual ~AnimationEventHandler() = default;

  virtual void OnAnimationEvent(const AnimationParams& animation_params) = 0;
  virtual void OnTransitionEvent(const AnimationParams& animation_params,
                                 ClayAnimationPropertyType property_type) = 0;
};
}  // namespace clay

#endif  // CLAY_GFX_ANIMATION_ANIMATION_EVENT_HANDLER_H_
