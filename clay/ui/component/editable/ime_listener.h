// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPONENT_EDITABLE_IME_LISTENER_H_
#define CLAY_UI_COMPONENT_EDITABLE_IME_LISTENER_H_

#include <memory>
#include <string>

#include "clay/ui/event/key_event.h"
#include "clay/ui/platform/keyboard_types.h"

namespace clay {

class IMEListener {
 public:
  virtual void OnCommitText(std::string text) = 0;
  virtual void OnComposingText(std::string text) = 0;
  virtual void OnKeyboardEvent(std::unique_ptr<KeyEvent> key_event) = 0;
  virtual void OnDeleteSurroundingText(int before_length, int after_length) = 0;
  virtual void OnPerformAction(KeyboardAction action) = 0;
  virtual void OnFinishInput() = 0;
};

}  // namespace clay
#endif  // CLAY_UI_COMPONENT_EDITABLE_IME_LISTENER_H_
