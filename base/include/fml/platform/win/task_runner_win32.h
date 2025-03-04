// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_INCLUDE_FML_PLATFORM_WIN_TASK_RUNNER_WIN32_H_
#define BASE_INCLUDE_FML_PLATFORM_WIN_TASK_RUNNER_WIN32_H_

#include <windows.h>

#include <deque>
#include <memory>
#include <queue>

#include "base/include/fml/platform/win/task_runner_win32_window.h"
#include "base/include/fml/task_runner.h"

namespace lynx {
namespace fml {

typedef uint64_t (*CurrentTimeProc)();

// A custom task runner that integrates with user32 GetMessage semantics so that
// host app can own its own message loop and flutter still gets to process
// tasks on a timely basis.
class TaskRunnerWin32 : public TaskRunner,
                        public TaskRunnerWin32Window::Delegate {
 public:
  using TaskTimePoint = std::chrono::steady_clock::time_point;

  // Creates a new task runner with the current thread, current time
  // provider, and callback for tasks that are ready to be run.
  static fml::RefPtr<fml::TaskRunner> Create();

  virtual ~TaskRunnerWin32();

  // |TaskRunner|
  void PostTask(base::closure closure) override;

  // |TaskRunner|
  bool RunsTasksOnCurrentThread() override;

  // |TaskRunnerWin32Window::Delegate|
  std::chrono::nanoseconds ProcessTasks() override;

 private:
  TaskRunnerWin32();

  // Returns the current TaskTimePoint that can be used to determine whether a
  // task is expired.
  //
  // Tests can override this to mock up the time.
  TaskTimePoint GetCurrentTimeForTask() const {
    return TaskTimePoint::clock::now();
  }

  struct Task {
    uint64_t order;
    TaskTimePoint fire_time;
    mutable base::closure closure;

    struct Comparer {
      bool operator()(const Task& a, const Task& b) {
        if (a.fire_time == b.fire_time) {
          return a.order > b.order;
        }
        return a.fire_time > b.fire_time;
      }
    };
  };

  std::mutex task_queue_mutex_;
  std::priority_queue<Task, std::deque<Task>, Task::Comparer> task_queue_;

  DWORD main_thread_id_;
  std::shared_ptr<TaskRunnerWin32Window> task_runner_window_;

  FML_FRIEND_MAKE_REF_COUNTED(TaskRunnerWin32);
  FML_FRIEND_REF_COUNTED_THREAD_SAFE(TaskRunnerWin32);

  TaskRunnerWin32(const TaskRunnerWin32&) = delete;
  TaskRunnerWin32& operator=(const TaskRunnerWin32&) = delete;
};

}  // namespace fml
}  // namespace lynx

#endif  // BASE_INCLUDE_FML_PLATFORM_WIN_TASK_RUNNER_WIN32_H_
