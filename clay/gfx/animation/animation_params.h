// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_ANIMATION_ANIMATION_PARAMS_H_
#define CLAY_GFX_ANIMATION_ANIMATION_PARAMS_H_

#include "clay/public/clay.h"

struct AnimationParams {
  ClayEventType event_type;
  const char* animation_name;
};

#endif  // CLAY_GFX_ANIMATION_ANIMATION_PARAMS_H_
