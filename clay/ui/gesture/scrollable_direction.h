// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_GESTURE_SCROLLABLE_DIRECTION_H_
#define CLAY_UI_GESTURE_SCROLLABLE_DIRECTION_H_

#include <cstdint>
#include <type_traits>

namespace clay {

enum class ScrollableDirection : uint8_t {
  kNone = 0,
  kUpwards = 1 << 0,
  kDownwards = 1 << 1,
  kLeftwards = 1 << 2,
  kRightwards = 1 << 3,
  kVertical = kUpwards | kDownwards,
  kHorizontal = kLeftwards | kRightwards,
  kAll = kVertical | kHorizontal,
};

inline ScrollableDirection operator|(const ScrollableDirection& lhs,
                                     const ScrollableDirection& rhs) {
  return static_cast<ScrollableDirection>(
      static_cast<std::underlying_type_t<ScrollableDirection>>(lhs) |
      static_cast<std::underlying_type_t<ScrollableDirection>>(rhs));
}

inline ScrollableDirection& operator|=(ScrollableDirection& lhs,
                                       const ScrollableDirection& rhs) {
  lhs = lhs | rhs;
  return lhs;
}

inline ScrollableDirection operator&(const ScrollableDirection& lhs,
                                     const ScrollableDirection& rhs) {
  return static_cast<ScrollableDirection>(
      static_cast<std::underlying_type_t<ScrollableDirection>>(lhs) &
      static_cast<std::underlying_type_t<ScrollableDirection>>(rhs));
}

}  // namespace clay

#endif  // CLAY_UI_GESTURE_SCROLLABLE_DIRECTION_H_
