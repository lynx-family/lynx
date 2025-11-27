// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_PLATFORM_WINDOWS_TASK_RUNNER_WINDOW_H_
#define CLAY_SHELL_PLATFORM_WINDOWS_TASK_RUNNER_WINDOW_H_

#include <windows.h>

#include <chrono>
#include <memory>
#include <string>
#include <vector>

namespace clay {

// Hidden HWND responsible for processing flutter tasks on main thread
class TaskRunnerWindow {
 public:
  class Delegate {
   public:
    // Executes expired task, and returns the duration until the next task
    // deadline if exists, otherwise returns `std::chrono::nanoseconds::max()`.
    //
    // Each platform implementation must call this to schedule the tasks.
    virtual std::chrono::nanoseconds ProcessTasks() = 0;
  };

  static std::shared_ptr<TaskRunnerWindow> GetSharedInstance();

  // Triggers processing delegate tasks on main thread
  void WakeUp();

  void AddDelegate(Delegate* delegate);
  void RemoveDelegate(Delegate* delegate);

  ~TaskRunnerWindow();

 private:
  TaskRunnerWindow();

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
}  // namespace clay

#endif  // CLAY_SHELL_PLATFORM_WINDOWS_TASK_RUNNER_WINDOW_H_
