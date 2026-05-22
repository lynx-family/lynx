// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPONENT_KEY_EVENT_HANDLER_H_
#define CLAY_UI_COMPONENT_KEY_EVENT_HANDLER_H_

#include <functional>

#include "clay/gfx/geometry/float_point.h"
#include "clay/gfx/geometry/float_size.h"
#include "clay/ui/event/gesture_event.h"
#include "clay/ui/event/key_event.h"

namespace clay {

// KeyEventHandler is refactored from EventHandler, separating focus and gesture
// logics. Gesture logics are moved into ui/gesture, and focus related
// are not used so far.
class KeyEventHandler {
 public:
  KeyEventHandler();
  virtual ~KeyEventHandler() = default;

  // key event
  virtual bool OnKeyEvent(const KeyEvent* event);

  void SetKeyDownCallback(std::function<bool(KeyCode)> callback);
  void SetKeyUpCallback(std::function<bool(KeyCode)> callback);

 protected:
  std::function<bool(KeyCode)> key_down_callback_;
  std::function<bool(KeyCode)> key_up_callback_;
};

}  // namespace clay
#endif  // CLAY_UI_COMPONENT_KEY_EVENT_HANDLER_H_
