// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CLAY_SHELL_PLATFORM_WINDOWS_EGL_CHILD_WINDOW_WIN_H_
#define CLAY_SHELL_PLATFORM_WINDOWS_EGL_CHILD_WINDOW_WIN_H_

#include <windows.h>

#include <memory>

#include "base/include/fml/thread.h"

namespace clay {
namespace egl {

class ChildWindowWin {
 public:
  explicit ChildWindowWin(HWND parent_window);

  ChildWindowWin(const ChildWindowWin&) = delete;
  ChildWindowWin& operator=(const ChildWindowWin&) = delete;

  ~ChildWindowWin();

  void Initialize();
  HWND window() const { return window_; }

  void Resize(int width, int height);

 private:
  fml::RefPtr<fml::TaskRunner> task_runner_;

  HWND parent_window_ = nullptr;
  HWND window_ = nullptr;
};

}  // namespace egl
}  // namespace clay

#endif  // CLAY_SHELL_PLATFORM_WINDOWS_EGL_CHILD_WINDOW_WIN_H_
