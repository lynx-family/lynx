// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/include/fml/platform/win/task_runner_win32.h"

#include "base/include/fml/message_loop_impl.h"

namespace lynx {
namespace fml {

// static
fml::RefPtr<fml::TaskRunner> TaskRunnerWin32::Create() {
  return fml::MakeRefCounted<fml::TaskRunnerWin32>();
}

TaskRunnerWin32::TaskRunnerWin32() : TaskRunner(nullptr) {
  main_thread_id_ = GetCurrentThreadId();
  task_runner_window_ = TaskRunnerWin32Window::GetSharedInstance();
  task_runner_window_->AddDelegate(this);
}

TaskRunnerWin32::~TaskRunnerWin32() {
  task_runner_window_->RemoveDelegate(this);
}

void TaskRunnerWin32::PostTask(base::closure closure) {
  Task task;
  task.fire_time = GetCurrentTimeForTask();
  task.closure = std::move(closure);

  static std::atomic_uint64_t sGlobalTaskOrder(0);

  task.order = ++sGlobalTaskOrder;
  {
    std::lock_guard<std::mutex> lock(task_queue_mutex_);
    task_queue_.push(std::move(task));

    // Make sure the queue mutex is unlocked before waking up the loop. In case
    // the wake causes this thread to be descheduled for the primary thread to
    // process tasks, the acquisition of the lock on that thread while holding
    // the lock here momentarily till the end of the scope is a pessimization.
  }

  task_runner_window_->WakeUp();
}

bool TaskRunnerWin32::RunsTasksOnCurrentThread() {
  return GetCurrentThreadId() == main_thread_id_;
}

std::chrono::nanoseconds TaskRunnerWin32::ProcessTasks() {
  const TaskTimePoint now = GetCurrentTimeForTask();

  std::vector<base::closure> expired_tasks;

  // Process expired tasks.
  {
    std::lock_guard<std::mutex> lock(task_queue_mutex_);
    while (!task_queue_.empty()) {
      const auto& top = task_queue_.top();
      // If this task (and all tasks after this) has not yet expired, there is
      // nothing more to do. Quit iterating.
      if (top.fire_time > now) {
        break;
      }

      // Make a record of the expired task. Do NOT service the task here
      // because we are still holding onto the task queue mutex. We don't want
      // other threads to block on posting tasks onto this thread till we are
      // done processing expired tasks.
      expired_tasks.push_back(std::move(task_queue_.top().closure));

      // Remove the tasks from the delayed tasks queue.
      task_queue_.pop();
    }
  }

  // Fire expired tasks.
  {
    // Flushing tasks here without holing onto the task queue mutex.
    for (const auto& task : expired_tasks) {
      task();
    }
  }

  // Calculate duration to sleep for on next iteration.
  {
    std::lock_guard<std::mutex> lock(task_queue_mutex_);
    const auto next_wake = task_queue_.empty() ? TaskTimePoint::max()
                                               : task_queue_.top().fire_time;

    return std::min(next_wake - now, std::chrono::nanoseconds::max());
  }
}

}  // namespace fml
}  // namespace lynx
