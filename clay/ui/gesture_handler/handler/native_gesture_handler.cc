// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/gesture_handler/handler/native_gesture_handler.h"

#include "clay/ui/gesture_handler/handler/pan_gesture_handler.h"

namespace clay {

NativeGestureHandler::NativeGestureHandler(
    int sign, PageView *page_view,
    std::shared_ptr<GestureDetector> gesture_detector,
    fml::WeakPtr<GestureArenaMember> gesture_arena_member)
    : PanGestureHandler(sign, page_view, gesture_detector,
                        gesture_arena_member) {}

}  // namespace clay
