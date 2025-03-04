// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_INCLUDE_FML_PLATFORM_WIN_TASK_RUNNER_WIN32_WINDOW_H_
#define BASE_INCLUDE_FML_PLATFORM_WIN_TASK_RUNNER_WIN32_WINDOW_H_

#include <windows.h>

#include <chrono>
#include <memory>
#include <string>
#include <vector>

namespace lynx {
namespace fml {

// Hidden HWND responsible for processing flutter tasks on main thread
class TaskRunnerWin32Window {
 public:
  class Delegate {
   public:
    virtual std::chrono::nanoseconds ProcessTasks() = 0;
  };

  static std::shared_ptr<TaskRunnerWin32Window> GetSharedInstance();

  // Triggers processing delegate tasks on main thread
  void WakeUp();

  void AddDelegate(Delegate* delegate);
  void RemoveDelegate(Delegate* delegate);

  ~TaskRunnerWin32Window();

 private:
  TaskRunnerWin32Window();

  void ProcessTasks();

  void SetTimer(std::chrono::nanoseconds when);

  WNDCLASS RegisterWindowClass();

  LRESULT
  HandleMessage(UINT const message, WPARAM const wparam,
                LPARAM const lparam) noexcept;

  static LRESULT CALLBACK WndProc(HWND const window, UINT const message,
                                  WPARAM const wparam,
                                  LPARAM const lparam) noexcept;

  HWND window_handle_;
  std::wstring window_class_name_;
  std::vector<Delegate*> delegates_;
};

}  // namespace fml
}  // namespace lynx

#endif  // BASE_INCLUDE_FML_PLATFORM_WIN_TASK_RUNNER_WIN32_WINDOW_H_
