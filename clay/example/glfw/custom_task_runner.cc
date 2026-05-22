// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/example/glfw/custom_task_runner.h"

#include "base/include/fml/message_loop_impl.h"
#include "base/include/fml/message_loop_task_queues.h"
#include "clay/fml/logging.h"

namespace clay {
namespace example {

CustomTaskRunner::CustomTaskRunner(DispatchTable table,
                                   size_t embedder_identifier)
    : TaskRunner(nullptr /* loop implementation*/),
      embedder_identifier_(embedder_identifier),
      dispatch_table_(std::move(table)),
      placeholder_id_(
          fml::MessageLoopTaskQueues::GetInstance()->CreateTaskQueue()) {
  FML_DCHECK(dispatch_table_.post_task_callback);
  FML_DCHECK(dispatch_table_.runs_task_on_current_thread_callback);
}

CustomTaskRunner::~CustomTaskRunner() = default;

size_t CustomTaskRunner::GetEmbedderIdentifier() const {
  return embedder_identifier_;
}

void CustomTaskRunner::PostTask(lynx::base::closure task) {
  PostTaskForTime(std::move(task), fml::TimePoint::Now());
}

void CustomTaskRunner::PostTaskForTime(lynx::base::closure task,
                                       fml::TimePoint target_time) {
  if (!task) {
    return;
  }

  uint64_t baton = 0;

  {
    // Release the lock before the jump via the dispatch table.
    std::scoped_lock lock(tasks_mutex_);
    if (thread_host_destroyed_) {
      FML_LOG(ERROR) << "Clay attempted to post a task to a destroyed "
                        "thread host. Maybe leaked.";
      return;
    }
    baton = ++last_baton_;
    pending_tasks_[baton] = std::make_pair(target_time, std::move(task));
    dispatch_table_.post_task_callback(this, baton, target_time);
  }
}

void CustomTaskRunner::PostDelayedTask(lynx::base::closure task,
                                       fml::TimeDelta delay) {
  PostTaskForTime(std::move(task), fml::TimePoint::Now() + delay);
}

bool CustomTaskRunner::RunsTasksOnCurrentThread() {
  std::scoped_lock lock(tasks_mutex_);
  if (thread_host_destroyed_) {
    FML_LOG(ERROR) << "Clay attempted to call RunsTasksOnCurrentThread"
                      "to a destroyed thread host.";
    return false;
  }
  return dispatch_table_.runs_task_on_current_thread_callback();
}

void CustomTaskRunner::OnThreadHostDestroyed() {
  std::unordered_map<uint64_t, std::pair<fml::TimePoint, lynx::base::closure>>
      tasks;

  {
    std::scoped_lock lock(tasks_mutex_);
    thread_host_destroyed_ = true;
    tasks = std::move(pending_tasks_);
  }

  fml::TimePoint now = fml::TimePoint::Now();

  for (auto& it : tasks) {
    auto& [time, cb] = it.second;
    if (time <= now) {
      cb();
    }
  }
}

bool CustomTaskRunner::PostTask(uint64_t baton) {
  lynx::base::closure task;

  {
    std::scoped_lock lock(tasks_mutex_);
    if (thread_host_destroyed_) {
      FML_LOG(ERROR) << "Clay attempted to post a task to a destroyed "
                        "thread host. Maybe leaked.";
      return false;
    }
    const auto& found = pending_tasks_.find(baton);
    if (found == pending_tasks_.end()) {
      FML_LOG(ERROR) << "Clay attempted to post an unknown task.";
      return false;
    }
    task = std::move(found->second.second);
    pending_tasks_.erase(found);

    // Let go of the tasks mutex befor executing the task.
  }

  FML_DCHECK(task);
  task();
  return true;
}

// |fml::TaskRunner|
fml::TaskQueueId CustomTaskRunner::GetTaskQueueId() { return placeholder_id_; }

}  // namespace example
}  // namespace clay
