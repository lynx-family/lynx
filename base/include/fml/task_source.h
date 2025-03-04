// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_INCLUDE_FML_TASK_SOURCE_H_
#define BASE_INCLUDE_FML_TASK_SOURCE_H_

#include <queue>

#include "base/include/fml/delayed_task.h"
#include "base/include/fml/macros.h"
#include "base/include/fml/task_queue_id.h"
#include "base/include/fml/task_source_grade.h"

namespace lynx {
namespace fml {

class MessageLoopTaskQueues;

/**
 * A Source of tasks for the `MessageLoopTaskQueues` task dispatcher. This is a
 * wrapper around a primary and secondary task heap with the difference between
 * them being that the secondary task heap can be paused and resumed by the task
 * dispatcher. `TaskSourceGrade` determines what task heap the task is assigned
 * to.
 *
 * Registering Tasks
 * -----------------
 * The task dispatcher associates a task source with each `TaskQueueID`. When
 * the user of the task dispatcher registers a task, the task is in-turn
 * registered with the `TaskSource` corresponding to the `TaskQueueID`.
 *
 * Processing Tasks
 * ----------------
 * Task dispatcher provides the event loop a way to acquire tasks to run via
 * `GetNextTaskToRun`. Task dispatcher asks the underlying `TaskSource` for the
 * next task.
 */
class TaskSource {
 public:
  struct TopTask {
    TaskQueueId task_queue_id;
    const DelayedTask& task;
  };

  struct TopTaskResult {
    TaskQueueId task_queue_id;
    mutable base::closure task;
  };

  /// Construts a TaskSource with the given `task_queue_id`.
  explicit TaskSource(TaskQueueId task_queue_id);

  ~TaskSource();

  /// Drops the pending tasks from both primary and secondary task heaps.
  void ShutDown();

  /// Adds a task to the corresponding task heap as dictated by the
  /// `TaskSourceGrade` of the `DelayedTask`.
  void RegisterTask(DelayedTask task);

  /// Pops the task heap corresponding to the `TaskSourceGrade`.
  void PopTask(TaskSourceGrade grade);

  /// Returns the number of pending tasks. Excludes the tasks from the secondary
  /// heap if it's paused.
  size_t GetNumPendingTasks() const;

  /// Returns true if `GetNumPendingTasks` is zero.
  bool IsEmpty() const;

  /// Returns the top task based on scheduled time, taking into account whether
  /// the secondary heap has been paused or not.
  TopTask Top() const;

 private:
  const fml::TaskQueueId task_queue_id_;
  fml::DelayedTaskQueue primary_task_queue_;
  fml::DelayedTaskQueue emergency_task_queue_;
  fml::DelayedTaskQueue micro_task_queue_;
  // we not care about the target time of idle tasks, just FIFO is enough.
  std::queue<DelayedTask> idle_task_queue_;

  BASE_DISALLOW_COPY_ASSIGN_AND_MOVE(TaskSource);
};

}  // namespace fml
}  // namespace lynx

#endif  // BASE_INCLUDE_FML_TASK_SOURCE_H_
