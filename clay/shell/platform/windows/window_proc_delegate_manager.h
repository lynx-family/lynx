// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_PLATFORM_WINDOWS_WINDOW_PROC_DELEGATE_MANAGER_H_
#define CLAY_SHELL_PLATFORM_WINDOWS_WINDOW_PROC_DELEGATE_MANAGER_H_

#include <dxgi.h>
#include <stddef.h>
#include <stdint.h>
#include <windows.h>

#include <map>
#include <optional>

// Function pointer type for top level WindowProc delegate registration.
//
// The user data will be whatever was passed to
// FlutterDesktopRegisterTopLevelWindowProcHandler.
//
// Implementations should populate |result| and return true if the WindowProc
// was handled and further handling should stop. |result| is ignored if the
// function returns false.
typedef bool (*FlutterDesktopWindowProcCallback)(
    HWND /* hwnd */, UINT /* uMsg */, WPARAM /*wParam*/, LPARAM /* lParam*/,
    void* /* user data */, LRESULT* result);

namespace clay {

// Handles registration, unregistration, and dispatching for WindowProc
// delegation.
class WindowProcDelegateManager {
 public:
  explicit WindowProcDelegateManager();
  ~WindowProcDelegateManager();

  // Prevent copying.
  WindowProcDelegateManager(WindowProcDelegateManager const&) = delete;
  WindowProcDelegateManager& operator=(WindowProcDelegateManager const&) =
      delete;

  // Adds |delegate| as a delegate to be called for |OnTopLevelWindowProc|.
  //
  // Multiple calls with the same |delegate| will replace the previous
  // registration, even if |user_data| is different.
  void RegisterTopLevelWindowProcDelegate(
      FlutterDesktopWindowProcCallback delegate, void* user_data);

  // Unregisters |delegate| as a delate for |OnTopLevelWindowProc|.
  void UnregisterTopLevelWindowProcDelegate(
      FlutterDesktopWindowProcCallback delegate);

  // Calls any registered WindowProc delegates.
  //
  // If a result is returned, then the message was handled in such a way that
  // further handling should not be done.
  std::optional<LRESULT> OnTopLevelWindowProc(HWND hwnd, UINT message,
                                              WPARAM wparam, LPARAM lparam);

 private:
  std::map<FlutterDesktopWindowProcCallback, void*>
      top_level_window_proc_handlers_;
};

}  // namespace clay

#endif  // CLAY_SHELL_PLATFORM_WINDOWS_WINDOW_PROC_DELEGATE_MANAGER_H_
