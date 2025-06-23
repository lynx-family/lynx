// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_GESTURE_HANDLER_NATIVE_GESTURE_HANDLER_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_GESTURE_HANDLER_NATIVE_GESTURE_HANDLER_H_

#include <memory>

#include "platform/harmony/lynx_harmony/src/main/cpp/gesture/handler/pan_gesture_handler.h"

namespace lynx {
namespace tasm {

namespace harmony {

class NativeGestureHandler : public PanGestureHandler {
 public:
  NativeGestureHandler(int sign, LynxContext* lynx_context,
                       std::shared_ptr<GestureDetector> gesture_detector,
                       std::weak_ptr<GestureArenaMember> gesture_arena_member);

  ~NativeGestureHandler() override = default;
};

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_GESTURE_HANDLER_NATIVE_GESTURE_HANDLER_H_
