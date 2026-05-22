// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/shell/platform/windows/egl/child_window_win.h"

#include "base/include/fml/platform/win/task_runner_win32.h"
#include "base/include/fml/synchronization/waitable_event.h"
#include "clay/fml/logging.h"

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
    return;
  }
}

void CreateWindowsOnThread(const SIZE& size, fml::AutoResetWaitableEvent* event,
                           HWND* child_window, HWND* parent_window) {
  InitializeWindowClass();

  HWND window =
      CreateWindowEx(WS_EX_NOPARENTNOTIFY | WS_EX_LAYERED | WS_EX_LAYERED |
                         WS_EX_TRANSPARENT | WS_EX_NOREDIRECTIONBITMAP,
                     L"Intermediate D3D Window", L"",
                     WS_CHILDWINDOW | WS_DISABLED | WS_VISIBLE, 0, 0, size.cx,
                     size.cy, *parent_window, nullptr, nullptr, nullptr);

  if (!window) {
    std::cerr << "CreateWindowEx failed: " << GetLastError() << std::endl;
    return;
  }
  *child_window = window;
}

}  // namespace

ChildWindowWin::ChildWindowWin(HWND parent_window)
    : parent_window_(parent_window) {
  Initialize();
}

void ChildWindowWin::Initialize() {
  if (window_) return;
  fml::AutoResetWaitableEvent event;
  child_ui_thread_ = std::make_unique<std::thread>([this, &event]() {
    task_runner_ = lynx::fml::TaskRunnerWin32::Create();
    RECT window_rect;
    GetClientRect(parent_window_, &window_rect);
    SIZE size = {window_rect.right - window_rect.left,
                 window_rect.bottom - window_rect.top};
    InitializeWindowClass();
    window_ =
        CreateWindowEx(WS_EX_NOPARENTNOTIFY | WS_EX_LAYERED | WS_EX_LAYERED |
                           WS_EX_TRANSPARENT | WS_EX_NOREDIRECTIONBITMAP,
                       L"Intermediate D3D Window", L"",
                       WS_CHILDWINDOW | WS_DISABLED | WS_VISIBLE, 0, 0, size.cx,
                       size.cy, parent_window_, nullptr, nullptr, nullptr);
    if (!window_) {
      std::cerr << "CreateWindowEx failed: " << GetLastError() << std::endl;
      event.Signal();
      return;
    }
    event.Signal();
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
    DestroyWindow(window_);
  });
  event.Wait();
}
ChildWindowWin::~ChildWindowWin() {
  if (task_runner_ && window_) {
    task_runner_->PostTask([window = window_]() { PostQuitMessage(0); });
  }

  if (child_ui_thread_ && child_ui_thread_->joinable()) {
    child_ui_thread_->join();
  }
  child_ui_thread_.reset();
}

bool ChildWindowWin::Resize(int width, int height) {
  if (!window_ || !task_runner_) return false;
  // Force a resize and redraw (but not a move, activate, etc.).
  task_runner_->PostTask([this, width, height]() {
    if (window_) {
      SetWindowPos(window_, nullptr, 0, 0, width, height,
                   SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOCOPYBITS |
                       SWP_NOOWNERZORDER | SWP_NOZORDER);
    }
  });
  return true;
}

}  // namespace egl

}  // namespace clay
