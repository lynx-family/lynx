// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/gesture_handler/common/gesture_extra_bundle.h"

namespace clay {

void GestureExtraBundle::ResetSimultaneousDelta() {
  simultaneous_delta_x_ = 0.0f;
  simultaneous_delta_y_ = 0.0f;
  is_need_consumed_simultaneous_gesture_ = false;
}

}  // namespace clay
