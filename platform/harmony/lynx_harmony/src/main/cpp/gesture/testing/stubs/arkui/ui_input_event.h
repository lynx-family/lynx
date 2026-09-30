// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_GESTURE_TESTING_STUBS_ARKUI_UI_INPUT_EVENT_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_GESTURE_TESTING_STUBS_ARKUI_UI_INPUT_EVENT_H_

#include <cstdint>

enum {
  UI_TOUCH_EVENT_ACTION_CANCEL = 0,
  UI_TOUCH_EVENT_ACTION_DOWN = 1,
  UI_TOUCH_EVENT_ACTION_MOVE = 2,
  UI_TOUCH_EVENT_ACTION_UP = 3,
};

struct ArkUI_UIInputEvent {
  int32_t action;
  float x;
  float y;
};

inline int32_t OH_ArkUI_UIInputEvent_GetAction(
    const ArkUI_UIInputEvent* event) {
  return event ? event->action : -1;
}

inline float OH_ArkUI_PointerEvent_GetXByIndex(const ArkUI_UIInputEvent* event,
                                               uint32_t) {
  return event ? event->x : 0;
}

inline float OH_ArkUI_PointerEvent_GetYByIndex(const ArkUI_UIInputEvent* event,
                                               uint32_t) {
  return event ? event->y : 0;
}

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_GESTURE_TESTING_STUBS_ARKUI_UI_INPUT_EVENT_H_
