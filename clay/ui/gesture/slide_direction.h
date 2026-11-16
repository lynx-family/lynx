// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_GESTURE_SLIDE_DIRECTION_H_
#define CLAY_UI_GESTURE_SLIDE_DIRECTION_H_

#include <cstdint>
#include <type_traits>

namespace clay {

enum class SlideDirection : uint8_t {
  kNone = 0,
  kLeft = 1 << 0,
  kRight = 1 << 1,
  kHorizontal = kLeft | kRight,
  kUp = 1 << 2,
  kDown = 1 << 3,
  kVertical = kUp | kDown,
  kAll = kHorizontal | kVertical,
};

inline SlideDirection operator|(SlideDirection lhs, SlideDirection rhs) {
  return static_cast<SlideDirection>(
      static_cast<std::underlying_type_t<SlideDirection>>(lhs) |
      static_cast<std::underlying_type_t<SlideDirection>>(rhs));
}

inline SlideDirection& operator|=(SlideDirection& lhs, SlideDirection rhs) {
  lhs = lhs | rhs;
  return lhs;
}

inline SlideDirection GetSlideDirectionForAngleRange(float start, float end) {
  if (start > end) {
    return SlideDirection::kNone;
  }

  SlideDirection direction = SlideDirection::kNone;
  if ((start <= -135 && end >= -180) || (start <= 180 && end >= 135)) {
    direction |= SlideDirection::kLeft;
  }
  if (start <= 45 && end >= -45) {
    direction |= SlideDirection::kRight;
  }
  if (start <= 135 && end >= 45) {
    direction |= SlideDirection::kUp;
  }
  if (start <= -45 && end >= -135) {
    direction |= SlideDirection::kDown;
  }
  return direction;
}

}  // namespace clay

#endif  // CLAY_UI_GESTURE_SLIDE_DIRECTION_H_
