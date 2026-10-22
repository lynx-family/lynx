// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_GESTURE_TESTING_STUBS_PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_GESTURE_UNIFIED_GESTURE_HANDLER_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_GESTURE_TESTING_STUBS_PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_GESTURE_UNIFIED_GESTURE_HANDLER_H_

#include <arkui/ui_input_event.h>

#include <cstdint>
#include <memory>

#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx::tasm::harmony {

class EventTarget;
class GestureArenaMember;
class LynxContext;

// Legacy host baselines must not enter the adapter that requires the device
// SDK.
class UnifiedGestureHandler {
 public:
  explicit UnifiedGestureHandler(LynxContext*) {
    ADD_FAILURE() << "Legacy gesture baseline selected the unified adapter";
  }
  int ReplaceMember(std::weak_ptr<GestureArenaMember>) { return 0; }
  void RemoveMember(int) {}
  bool ContainsMember(int) const { return false; }
  bool ContainsGesture(int, uint32_t) const { return false; }
  void SetGestureState(int, uint32_t, int) {}
  void SetResponseChain(std::weak_ptr<EventTarget>) {}
  void HandleInput(const ArkUI_UIInputEvent*) {}
  void SetVelocity(float, float) {}
  void CancelInteraction() {}
  void Reset() {}
};

}  // namespace lynx::tasm::harmony

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_GESTURE_TESTING_STUBS_PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_GESTURE_UNIFIED_GESTURE_HANDLER_H_
