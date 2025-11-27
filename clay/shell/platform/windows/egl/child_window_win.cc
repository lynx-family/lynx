// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/shell/platform/windows/egl/child_window_win.h"

#include "clay/fml/logging.h"
#include "clay/ui/common/isolate.h"

namespace clay {
namespace egl {

namespace {

ATOM g_window_class;

void InitializeWindowClass() {
  if (g_window_class) {
    return;
  }
  WNDCLASSEX window_class = {0};
  window_class.cbSize = sizeof(WNDCLASSEX);
  window_class.style = CS_OWNDC;
  window_class.lpfnWndProc = DefWindowProc;
  window_class.cbClsExtra = 0;
  window_class.cbWndExtra = 0;
  window_class.hInstance = GetModuleHandle(nullptr);
  window_class.hIcon = nullptr;
  window_class.hCursor = nullptr;
  window_class.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
  window_class.lpszMenuName = nullptr;
  window_class.lpszClassName = L"Intermediate D3D Window";
  window_class.hIconSm = nullptr;
  g_window_class = RegisterClassEx(&window_class);
  if (!g_window_class) {
    FML_LOG(ERROR) << "RegisterClassEx failed: " << GetLastError();
  }
}

}  // namespace

ChildWindowWin::ChildWindowWin(HWND parent_window)
    : parent_window_(parent_window) {
  Initialize();
}

void ChildWindowWin::Initialize() {
  if (window_) {
    return;
  }
  RECT window_rect;
  GetClientRect(parent_window_, &window_rect);
  SIZE size = {window_rect.right - window_rect.left,
               window_rect.bottom - window_rect.top};
  InitializeWindowClass();

  HWND window =
      CreateWindowEx(WS_EX_NOPARENTNOTIFY | WS_EX_LAYERED | WS_EX_LAYERED |
                         WS_EX_TRANSPARENT | WS_EX_NOREDIRECTIONBITMAP,
                     L"Intermediate D3D Window", L"",
                     WS_CHILDWINDOW | WS_DISABLED | WS_VISIBLE, 0, 0, size.cx,
                     size.cy, parent_window_, nullptr, nullptr, nullptr);

  if (!window) {
    std::cerr << "CreateWindowEx failed: " << GetLastError() << std::endl;
    return;
  }
  window_ = window;
}
ChildWindowWin::~ChildWindowWin() { DestroyWindow(window_); }

void ChildWindowWin::Resize(int width, int height) {
  // Force a resize and redraw (but not a move, activate, etc.).
  if (!task_runner_) {
    task_runner_ = clay::Isolate::Instance().GetPlatformTaskRunner();
  }
  task_runner_->PostTask([this, width, height]() {
    if (window_) {
      SetWindowPos(window_, nullptr, 0, 0, width, height,
                   SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOCOPYBITS |
                       SWP_NOOWNERZORDER | SWP_NOZORDER);
    }
  });
}

}  // namespace egl

}  // namespace clay
