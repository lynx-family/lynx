// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPONENT_FOCUS_EVENT_HANDLER_H_
#define CLAY_UI_COMPONENT_FOCUS_EVENT_HANDLER_H_

#include <functional>

namespace clay {

// Decoupling from EventHandler.
class FocusEventHandler {
 public:
  FocusEventHandler() = default;
  virtual ~FocusEventHandler() = default;

  // TODO(hupeng): Bridge to focus.
  virtual void OnFocusChanged(bool focused) {
    if (focus_change_callback_) {
      focus_change_callback_(focused);
    }
  }

  void SetFocusChangeCallback(std::function<void(bool)> callback) {
    focus_change_callback_ = callback;
  }

 protected:
  std::function<void(bool)> focus_change_callback_;
};

}  // namespace clay

#endif  // CLAY_UI_COMPONENT_FOCUS_EVENT_HANDLER_H_
