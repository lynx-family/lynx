// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_EXAMPLE_GLFW_EVENT_LOOP_H_
#define CLAY_EXAMPLE_GLFW_EVENT_LOOP_H_

#include <chrono>
#include <deque>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>

#include "clay/public/clay.h"

namespace clay {
namespace example {

// An event loop implementation that supports Engine tasks scheduling in the
// GLFW event loop.
class EventLoop {
 public:
  using TaskExpiredCallback = std::function<void(const ClayTask*)>;

  // Creates an event loop running on the given thread, calling
  // |on_task_expired| to run tasks.
  EventLoop(std::thread::id main_thread_id,
            const TaskExpiredCallback& on_task_expired);

  ~EventLoop();

  // Disallow copy.
  EventLoop(const EventLoop&) = delete;
  EventLoop& operator=(const EventLoop&) = delete;

  // Returns if the current thread is the thread used by this event loop.
  bool RunsTasksOnCurrentThread() const;

  // Waits for the next event, processes it, and returns.
  //
  // Expired engine events, if any, are processed as well. The optional
  // timeout should only be used when events not managed by this loop need to be
  // processed in a polling manner.
  void WaitForEvents(
      std::chrono::nanoseconds max_wait = std::chrono::nanoseconds::max());

  // Posts a engine task to the event loop for delayed execution.
  void PostTask(ClayTask task, uint64_t target_time_nanos);

 protected:
  using TaskTimePoint = std::chrono::steady_clock::time_point;

  // Returns the timepoint corresponding to a engine task time.
  static TaskTimePoint TimePointFromEngineTime(uint64_t target_time_nanos);

  // Returns the mutex used to control the task queue. Subclasses may safely
  // lock this mutex in the abstract methods below.
  std::mutex& GetTaskQueueMutex() { return task_queue_mutex_; }

  // Waits until the given time, or a Wake() call.
  void WaitUntil(const TaskTimePoint& time);

  // Wakes the main thread from a WaitUntil call.
  void Wake();

  struct Task {
    uint64_t order;
    TaskTimePoint fire_time;
    ClayTask task;

    struct Comparer {
      bool operator()(const Task& a, const Task& b) {
        if (a.fire_time == b.fire_time) {
          return a.order > b.order;
        }
        return a.fire_time > b.fire_time;
      }
    };
  };
  std::thread::id main_thread_id_;
  TaskExpiredCallback on_task_expired_;
  std::mutex task_queue_mutex_;
  std::priority_queue<Task, std::deque<Task>, Task::Comparer> task_queue_;
};

}  // namespace example
}  // namespace clay

#endif  // CLAY_EXAMPLE_GLFW_EVENT_LOOP_H_
