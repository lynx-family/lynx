// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_ANIMATION_PICTURE_ANIMATION_TYPE_H_
#define CLAY_GFX_ANIMATION_PICTURE_ANIMATION_TYPE_H_

#include <utility>
#include <vector>

#include "clay/gfx/rendering_backend.h"

namespace clay {

enum class DynamicOpType {
  kNone = 0,
  kSetBackgroundColor = 1,
  kSetTextColor = 2
};

#ifndef ENABLE_SKITY
using DynamicOps = std::vector<std::pair<DynamicOpType, int32_t>>;
#else
using DynamicOps =
    std::vector<std::pair<DynamicOpType, skity::RecordedOpOffset>>;
#endif  // ENABLE_SKITY

}  // namespace clay

#endif  // CLAY_GFX_ANIMATION_PICTURE_ANIMATION_TYPE_H_
