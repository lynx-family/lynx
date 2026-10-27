// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_PLATFORM_KEYBOARD_BRIDGE_H_
#define CLAY_UI_PLATFORM_KEYBOARD_BRIDGE_H_

#include <memory>

#include "base/include/fml/task_runner.h"
#include "clay/ui/component/base_view.h"
#include "clay/ui/platform/keyboard_types.h"

namespace clay {

class KeyboardClient {
 public:
  virtual fml::RefPtr<fml::TaskRunner> GetTaskRunner() = 0;
  virtual FloatSize KeyboardHostViewSize() = 0;
  virtual void AddToKeyboardHostView(BaseView* keyboard_view) = 0;
  virtual void RemoveFromKeyboardHostView(BaseView* keyboard_view) = 0;
  virtual void OnKeyboardEvent(std::unique_ptr<KeyEvent> key_event) = 0;
  virtual void OnDeleteSurroundingText(int before_length, int after_length) = 0;
  virtual void OnPerformAction(KeyboardAction action) = 0;
  virtual void OnFinishInput() = 0;

  virtual void PlatformShowSoftInput(int type, int action) = 0;
  virtual void PlatformHideSoftInput() = 0;
};

class KeyboardBridge {
 public:
  explicit KeyboardBridge(KeyboardClient* client);
  ~KeyboardBridge() = default;

  void RequestSoftKeyboard(KeyboardInputType type, KeyboardAction action,
                           PageView* page_view);
  void HideSoftKeyboard();

  KeyboardClient* client_;
};
}  // namespace clay
#endif  // CLAY_UI_PLATFORM_KEYBOARD_BRIDGE_H_
