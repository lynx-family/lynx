// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_GESTURE_HANDLER_HANDLER_NATIVE_GESTURE_HANDLER_H_
#define CLAY_UI_GESTURE_HANDLER_HANDLER_NATIVE_GESTURE_HANDLER_H_

#include <memory>

#include "clay/ui/gesture_handler/handler/pan_gesture_handler.h"

namespace clay {

class NativeGestureHandler : public PanGestureHandler {
 public:
  NativeGestureHandler(int sign, PageView *page_view,
                       std::shared_ptr<GestureDetector> gesture_detector,
                       fml::WeakPtr<GestureArenaMember> gesture_arena_member);

  ~NativeGestureHandler() override = default;
};

}  // namespace clay

#endif  // CLAY_UI_GESTURE_HANDLER_HANDLER_NATIVE_GESTURE_HANDLER_H_
