// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef EXPLORER_WINDOWS_LYNX_EXPLORER_LYNX_WINDOW_H_
#define EXPLORER_WINDOWS_LYNX_EXPLORER_LYNX_WINDOW_H_

#include <windows.h>

#include <memory>
#include <string>
#include <utility>

#include "lynx_view.h"
#if ENABLE_TESTBENCH_REPLAY
#include "platform/embedder/lynx_recorder/test_bench_action_manager.h"
#endif

#define LYNX_WINDOW_OPEN 100

namespace lynx {
class LynxWindow {
 public:
  LynxWindow(unsigned int x, unsigned int y, unsigned int width,
             unsigned int height, bool test_bench_replay = false,
             bool enable_napi_addon = false);
  ~LynxWindow();

  HWND GetWindowHandle() const;

  void LoadTemplate(const std::string& url);

  // Release OS resources associated with window.
  void Destroy();

  // If true, closing this window will quit the application.
  void SetQuitOnClose(bool quit_on_close);

  // Return a RECT representing the bounds of the current client area.
  RECT GetClientArea();

  // OS callback called by message pump. Handles the WM_NCCREATE message which
  // is passed when the non-client area is being created and enables automatic
  // non-client DPI scaling so that the non-client area automatically
  // responsponds to changes in DPI. All other messages are handled by
  // MessageHandler.
  static LRESULT CALLBACK WndProc(HWND const window, UINT const message,
                                  WPARAM const wparam,
                                  LPARAM const lparam) noexcept;

 private:
  // Processes and route salient window messages for mouse handling,
  // size change and DPI. Delegates handling of these to member overloads that
  // inheriting classes can handle.
  LRESULT MessageHandler(HWND window, UINT const message, WPARAM const wparam,
                         LPARAM const lparam) noexcept;

  // Retrieves a class instance pointer for |window|
  static LynxWindow* GetThisFromHandle(HWND const window) noexcept;

  // window handle for top level window.
  HWND window_handle_ = nullptr;

  // window handle for hosted content.
  HWND child_content_ = nullptr;

  std::shared_ptr<lynx::pub::LynxView> lynx_view_;
  bool is_test_bench_replay_ = false;
  bool enable_napi_addon_ = false;
#if ENABLE_TESTBENCH_REPLAY
  std::shared_ptr<lynx::embedder::TestBenchActionManager>
      test_bench_action_manager_;
#endif

  bool quit_on_close_ = false;
};
}  // namespace lynx

#endif  // EXPLORER_WINDOWS_LYNX_EXPLORER_LYNX_WINDOW_H_
