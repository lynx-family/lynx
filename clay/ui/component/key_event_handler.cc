// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/component/key_event_handler.h"

namespace clay {

KeyEventHandler::KeyEventHandler() = default;

bool KeyEventHandler::OnKeyEvent(const KeyEvent* event) {
  KeyCode key_code = static_cast<KeyCode>(event->GetLogical());
  if (event->GetType() == KeyEventType::kDown && key_down_callback_) {
    return key_down_callback_(key_code);
  } else if (event->GetType() == KeyEventType::kUp && key_up_callback_) {
    return key_up_callback_(key_code);
  }

  return false;
}

void KeyEventHandler::SetKeyDownCallback(
    std::function<bool(KeyCode)> callback) {
  key_down_callback_ = callback;
}

void KeyEventHandler::SetKeyUpCallback(std::function<bool(KeyCode)> callback) {
  key_up_callback_ = callback;
}

}  // namespace clay
