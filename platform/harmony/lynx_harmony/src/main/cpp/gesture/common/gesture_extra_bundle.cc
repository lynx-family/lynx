// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/gesture/common/gesture_extra_bundle.h"

namespace lynx {
namespace tasm {

namespace harmony {

GestureExtraBundle::GestureExtraBundle()
    : gesture_direction_(0),
      simultaneous_delta_x_(0.0f),
      simultaneous_delta_y_(0.0f),
      is_need_consumed_simultaneous_gesture_(false) {}

void GestureExtraBundle::ResetSimultaneousDelta() {
  simultaneous_delta_x_ = 0.0f;
  simultaneous_delta_y_ = 0.0f;
  is_need_consumed_simultaneous_gesture_ = false;
}

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
