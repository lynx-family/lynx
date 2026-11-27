// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_LYNX_ADAPTOR_KEY_MODIFIERS_H_
#define CLAY_LYNX_ADAPTOR_KEY_MODIFIERS_H_

#include "clay/public/event_delegate.h"

#if OS_WIN
#include <Windows.h>
#elif OS_MAC
#include <ApplicationServices/ApplicationServices.h>
#include <Carbon/Carbon.h>
#endif

namespace clay {

inline KeyModifiers GetKeyModifiers(const EventDelegate* delegate) {
  if (delegate) {
    if (auto modifiers = delegate->GetKeyModifiersOverride()) return *modifiers;
  }
  KeyModifiers result;
#if OS_WIN
  result.alt = (GetKeyState(VK_LMENU) | GetKeyState(VK_RMENU)) & 0x8000;
  result.control =
      (GetKeyState(VK_LCONTROL) | GetKeyState(VK_RCONTROL)) & 0x8000;
  result.shift = (GetKeyState(VK_LSHIFT) | GetKeyState(VK_RSHIFT)) & 0x8000;
  result.meta = (GetKeyState(VK_LWIN) | GetKeyState(VK_RWIN)) & 0x8000;
#elif OS_MAC
  auto pressed = [](CGKeyCode left, CGKeyCode right) {
    return CGEventSourceKeyState(kCGEventSourceStateHIDSystemState, left) ||
           CGEventSourceKeyState(kCGEventSourceStateHIDSystemState, right);
  };
  result.alt = pressed(kVK_Option, kVK_RightOption);
  result.control = pressed(kVK_Control, kVK_RightControl);
  result.shift = pressed(kVK_Shift, kVK_RightShift);
  result.meta = pressed(kVK_Command, kVK_RightCommand);
#endif
  return result;
}

}  // namespace clay

#endif  // CLAY_LYNX_ADAPTOR_KEY_MODIFIERS_H_
