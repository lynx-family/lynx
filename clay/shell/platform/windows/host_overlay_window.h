// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_PLATFORM_WINDOWS_HOST_OVERLAY_WINDOW_H_
#define CLAY_SHELL_PLATFORM_WINDOWS_HOST_OVERLAY_WINDOW_H_

#include <Windows.h>
#include <dwmapi.h>

#include <memory>
#include <utility>

#include "clay/shell/platform/windows/flutter_window.h"
#include "clay/shell/platform/windows/flutter_windows_engine.h"

namespace clay {

struct WindowRect {
  int32_t left;
  int32_t top;
  int32_t width;
  int32_t height;
};

class HostOverlayWindow {
 public:
  HostOverlayWindow(FlutterWindowsEngine* engine);
  virtual ~HostOverlayWindow();

  static std::unique_ptr<HostOverlayWindow> CreateRegularWindow(
      FlutterWindowsEngine* engine, const Rect& rect, LPCWSTR title);
  FlutterViewId GetFlutterViewId() const;
  // Returns the backing window handle, or nullptr if the native window is not
  // created or has already been destroyed.
  HWND GetWindowHandle() const;

  // Returns the HWND of the FlutterView hosted in this window.
  HWND GetFlutterViewWindowHandle() const;

 protected:
  struct HostOverlayWindowInitializationParams {
    DWORD window_style;
    DWORD extended_window_style;
    Rect const initial_window_rect;
    LPCWSTR title;
    std::optional<HWND> const& owner_window;
    int nCmdShow = SW_SHOWNORMAL;
  };

  // Returns the instance pointer for |hwnd| or nullptr if invalid.
  static HostOverlayWindow* GetThisFromHandle(HWND hwnd);

  // Processes and routes salient window messages for mouse handling,
  // size change and DPI. Delegates handling of these to member overloads that
  // inheriting classes can handle.
  virtual LRESULT HandleMessage(HWND hwnd, UINT message, WPARAM wparam,
                                LPARAM lparam);

  // Sets the focus to the child view window of |window|.
  static void FocusRootViewOf(HostOverlayWindow* window);

  // OS callback called by message pump. Handles the WM_NCCREATE message which
  // is passed when the non-client area is being created and enables automatic
  // non-client DPI scaling so that the non-client area automatically
  // responds to changes in DPI. Delegates other messages to the controller.
  static LRESULT WndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);

  // Initialize the underlying native window and the Flutter view.
  //
  // See:
  // - https://learn.microsoft.com/windows/win32/winmsg/window-styles
  // - https://learn.microsoft.com/windows/win32/winmsg/extended-window-styles
  void InitializeFlutterView(
      HostOverlayWindowInitializationParams const& params);
  // The Flutter engine that owns this window.
  FlutterWindowsEngine* engine_;
  // Backing handle for this window.
  HWND window_handle_;
  std::unique_ptr<FlutterWindowsView> view_;
};

}  // namespace clay

#endif  // CLAY_SHELL_PLATFORM_WINDOWS_HOST_OVERLAY_WINDOW_H_
