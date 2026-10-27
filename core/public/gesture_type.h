// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_PUBLIC_GESTURE_TYPE_H_
#define CORE_PUBLIC_GESTURE_TYPE_H_

namespace lynx::tasm {

// Enum class for representing different types of gestures.
enum class GestureType : unsigned int {
  PAN = 0,         // Pan gesture
  FLING = 1,       // Fling gesture
  DEFAULT = 2,     // Default gesture
  TAP = 3,         // Tap gesture
  LONG_PRESS = 4,  // Long press gesture
  ROTATION = 5,    // Rotation gesture
  PINCH = 6,       // Pinch gesture
  NATIVE = 7       // Native gesture
};

}  // namespace lynx::tasm

#endif  // CORE_PUBLIC_GESTURE_TYPE_H_
