// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_RENDERING_DECODE_UTILS_H_
#define CLAY_UI_RENDERING_DECODE_UTILS_H_

#include "clay/gfx/image/decode_priority.h"

namespace clay {

class RenderObject;

class DecodeUtils {
 public:
  static DecodePriority GetDecodePriority(RenderObject* render_object);
};

}  // namespace clay

#endif  // CLAY_UI_RENDERING_DECODE_UTILS_H_
