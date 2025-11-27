// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_PLATFORM_WINDOWS_WINDOW_MOVE_HANDLER_H_
#define CLAY_SHELL_PLATFORM_WINDOWS_WINDOW_MOVE_HANDLER_H_

#include "base/include/fml/memory/weak_ptr.h"
#include "clay/shell/platform/windows/window_binding_handler.h"

namespace clay {
class WindowMoveHandle {
 public:
  explicit WindowMoveHandle(WindowBindingHandler* delegate);

  HWND GetTopWindowHwnd();

  void MoveWindow();

 private:
  // The delegate for cursor updates.
  WindowBindingHandler* delegate_;

  fml::WeakPtrFactory<WindowMoveHandle> weak_factory_;
};
}  // namespace clay

#endif  // CLAY_SHELL_PLATFORM_WINDOWS_WINDOW_MOVE_HANDLER_H_
