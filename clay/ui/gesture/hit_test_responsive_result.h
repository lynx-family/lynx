// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_GESTURE_HIT_TEST_RESPONSIVE_RESULT_H_
#define CLAY_UI_GESTURE_HIT_TEST_RESPONSIVE_RESULT_H_

#include "clay/ui/gesture/gesture_recognizer.h"
#include "clay/ui/gesture/scrollable_direction.h"

namespace clay {

union HitTestResponsiveResult {
  int32_t value = 0;
  // CAUTION: Don't change the order of these fields, or you will need to update
  // related code on the platform side.
  struct {
    ScrollableDirection scrollable_direction : 4;
    GestureRecognizerType recognized_gesture_type : 4;
    bool tappable : 1;
    bool should_block_native_event : 1;
    bool has_consume_slide_event : 1;
    bool slide_event_consumed : 1;
    bool has_longpress_event : 1;
  };
};

}  // namespace clay

#endif  // CLAY_UI_GESTURE_HIT_TEST_RESPONSIVE_RESULT_H_
