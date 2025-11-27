// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "clay/shell/platform/windows/window_move_handler.h"

#include <windows.h>

#include "clay/fml/logging.h"

namespace clay {

WindowMoveHandle::WindowMoveHandle(WindowBindingHandler* delegate)
    : delegate_(delegate), weak_factory_(this) {}

void WindowMoveHandle::MoveWindow() {
  HWND top_hwnd = GetTopWindowHwnd();
  if (!top_hwnd) {
    FML_LOG(ERROR) << "GetTopWindowHwnd error; Error getting root window! "
                      "Check the hierarchy of Windows!";
    return;
  }
  // If you use SetCapture to set the focus on the form HWND before
  // SendMessage, SendMessage(hWnd,WM_NCLBUTTONDOWN,HTCAPTION,0) will be
  // invalid. The solution is to add ::ReleaseCapture() before SendMessage()
  // to release the focus;
  ReleaseCapture();
  PostMessage(top_hwnd, WM_SYSCOMMAND, SC_MOVE | HTCAPTION, 0);
}

HWND WindowMoveHandle::GetTopWindowHwnd() {
  auto target = delegate_->GetRenderTarget();
  auto hwnd = std::get<HWND>(target);
  auto top_hwnd = hwnd;
  while (hwnd) {
    hwnd = GetParent(top_hwnd);
    if (hwnd) {
      top_hwnd = hwnd;
    }
  }
  return top_hwnd;
}

}  // namespace clay
