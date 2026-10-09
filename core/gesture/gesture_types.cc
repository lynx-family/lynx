// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/gesture/gesture_types.h"

namespace lynx::tasm::gesture {

const char* GestureCallbackName(GestureCallbackType callback) {
  switch (callback) {
    case GestureCallbackType::kTouchesDown:
      return "onTouchesDown";
    case GestureCallbackType::kTouchesMove:
      return "onTouchesMove";
    case GestureCallbackType::kTouchesUp:
      return "onTouchesUp";
    case GestureCallbackType::kTouchesCancel:
      return "onTouchesCancel";
    case GestureCallbackType::kBegin:
      return "onBegin";
    case GestureCallbackType::kStart:
      return "onStart";
    case GestureCallbackType::kUpdate:
      return "onUpdate";
    case GestureCallbackType::kEnd:
      return "onEnd";
  }
}

const char* GestureInputTypeName(InputType type) {
  switch (type) {
    case InputType::kDown:
      return "touchstart";
    case InputType::kMove:
      return "touchmove";
    case InputType::kUp:
      return "touchend";
    case InputType::kCancel:
      return "touchcancel";
    case InputType::kFlingFrame:
      return "unknown";
  }
}

}  // namespace lynx::tasm::gesture
