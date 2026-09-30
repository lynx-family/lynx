// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_GESTURE_TESTING_STUBS_PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_LYNX_CONTEXT_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_GESTURE_TESTING_STUBS_PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_LYNX_CONTEXT_H_

#include <functional>

#include "platform/harmony/lynx_harmony/src/main/cpp/event/gesture_event.h"

namespace lynx {
namespace tasm {
namespace harmony {

// Only the host test target resolves this header; recognizers remain unchanged.
class LynxContext {
 public:
  float ScaledDensity() const { return density; }
  float DevicePixelRatio() const { return density; }
  void HandleGestureEvent(const GestureEvent& event) const { emit(event); }
  void OnGestureRecognizedWithSign(int sign) { recognized_sign = sign; }

  float density = 1;
  int recognized_sign = -1;
  std::function<void(const GestureEvent&)> emit;
};

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_GESTURE_TESTING_STUBS_PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_LYNX_CONTEXT_H_
