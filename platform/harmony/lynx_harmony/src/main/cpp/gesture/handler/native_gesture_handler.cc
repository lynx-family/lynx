// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/gesture/handler/native_gesture_handler.h"

#include "platform/harmony/lynx_harmony/src/main/cpp/event/gesture_event.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/event/touch_event.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/gesture/gesture_arena_member.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/lynx_context.h"

namespace lynx {
namespace tasm {
namespace harmony {
NativeGestureHandler::NativeGestureHandler(
    int sign, LynxContext* lynx_context,
    std::shared_ptr<GestureDetector> gesture_detector,
    std::weak_ptr<GestureArenaMember> gesture_arena_member)
    : PanGestureHandler(sign, lynx_context, gesture_detector,
                        gesture_arena_member){

      };

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
