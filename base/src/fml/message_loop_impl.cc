// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define FML_USED_ON_EMBEDDER

#include "base/include/fml/message_loop_impl.h"

#include <algorithm>
#include <vector>

#include "base/trace/native/trace_event.h"
#include "build/build_config.h"

namespace lynx {
namespace fml {

MessageLoopImpl::MessageLoopImpl()
    : task_queue_(MessageLoopTaskQueues::GetInstance()),
      internal_queue_id_(task_queue_->CreateTaskQueue()),
      terminated_(false),
      restriction_duration_(fml::TimeDelta::Max()) {
  Bind(internal_queue_id_);
}

MessageLoopImpl::~MessageLoopImpl() {
  task_queue_->Dispose(internal_queue_id_);
}

void MessageLoopImpl::PostTask(base::closure task, fml::TimePoint target_time,
                               fml::TaskSourceGrade task_source_grade) {
  // TODO(zhengsenyao): Uncomment DCHECK code when DCHECK available.
  // DCHECK(task != nullptr);
  if (terminated_) {
    // If the message loop has already been terminated, PostTask should destruct
    // |task| synchronously within this function.
    return;
  }
  task_queue_->RegisterTask(internal_queue_id_, std::move(task), target_time,
                            task_source_grade);
}

void MessageLoopImpl::AddTaskObserver(intptr_t key, base::closure callback) {
  // TODO(zhengsenyao): Uncomment DCHECK code when DCHECK available.
  // DCHECK(callback != nullptr);
  // DCHECK(MessageLoop::GetCurrent().GetLoopImpl().get() == this)
  //     << "Message loop task observer must be added on the same thread as the
  //     "
  //        "loop.";
  if (callback != nullptr) {
    task_queue_->AddTaskObserver(internal_queue_id_, key, std::move(callback));
  } else {
    // TODO(zhengsenyao): Uncomment LOG code when LOG available
    // LOGE("Tried to add a null TaskObserver.");
  }
}

void MessageLoopImpl::RemoveTaskObserver(intptr_t key) {
  // TODO(zhengsenyao): Uncomment DCHECK code when DCHECK available.
  // DCHECK(MessageLoop::GetCurrent().GetLoopImpl().get() == this)
  //     << "Message loop task observer must be removed from the same thread as
  //     "
  //        "the loop.";
  task_queue_->RemoveTaskObserver(internal_queue_id_, key);
}

void MessageLoopImpl::DoRun() {
  if (terminated_) {
    // Message loops may be run only once.
    return;
  }

  // Allow the implementation to do its thing.
  Run();

  // The loop may have been implicitly terminated. This can happen if the
  // implementation supports termination via platform specific APIs or just
  // error conditions. Set the terminated flag manually.
  terminated_ = true;

  // The message loop is shutting down. Check if there are expired tasks. This
  // is the last chance for expired tasks to be serviced. Make sure the
  // terminated flag is already set so we don't accrue additional tasks now.
  RunExpiredTasksNow();

  // When the message loop is in the process of shutting down, pending tasks
  // should be destructed on the message loop's thread. We have just returned
  // from the implementations |Run| method which we know is on the correct
  // thread. Drop all pending tasks on the floor.
  task_queue_->DisposeTasks(internal_queue_id_);
}

void MessageLoopImpl::DoTerminate() {
  terminated_ = true;
  Terminate();
}

void MessageLoopImpl::FlushTasks(FlushType type) {
  TRACE_EVENT("lynx", "MessageLoop::FlushTasks");

  const auto now = fml::TimePoint::Now();
  bool reach_max_restriction = false;
  std::optional<TaskSource::TopTaskResult> task;
  base::closure invocation;
  do {
    if (queue_ids_.empty()) {
      break;
    }
    task = task_queue_->GetNextTaskToRun(queue_ids_, now);
    if (!task || !task.has_value()) {
      break;
    }

    invocation = std::move((*task).task);
    if (!invocation) {
      break;
    }
    invocation();
    auto observers = task_queue_->GetObserversToNotify(task->task_queue_id);
    for (const auto& observer : observers) {
      (*observer)();
    }
    if (type == FlushType::kSingle) {
      break;
    }
    // Reach maximum restriction duration, break
    if (restriction_duration_ <= fml::TimePoint::Now() - now) {
      reach_max_restriction = true;
      break;
    }
  } while (invocation);

  // Call WakeUp here to flush the remaining tasks when reaching maximum
  // restriction duration.
  if (reach_max_restriction) {
    task_queue_->WakeUp(queue_ids_);
  }
}

void MessageLoopImpl::RunExpiredTasksNow() { FlushTasks(FlushType::kAll); }

void MessageLoopImpl::RunSingleExpiredTaskNow() {
  FlushTasks(FlushType::kSingle);
}

TaskQueueId MessageLoopImpl::GetTaskQueueId() const {
  return internal_queue_id_;
}

std::vector<TaskQueueId> MessageLoopImpl::GetTaskQueueIds() const {
  return queue_ids_;
}

void MessageLoopImpl::Bind(const TaskQueueId& queue_id) {
  queue_ids_.emplace_back(queue_id);
  task_queue_->SetWakeable(queue_id, this);
}

void MessageLoopImpl::UnBind(const TaskQueueId& queue_id) {
  for (auto it = queue_ids_.begin(); it != queue_ids_.end(); ++it) {
    if (*it == queue_id) {
      queue_ids_.erase(it);
      task_queue_->SetWakeable(queue_id, nullptr);
      break;
    }
  }
}

void MessageLoopImpl::SetRestrictionDuration(fml::TimeDelta duration) {
  restriction_duration_ = duration;
}

void MessageLoopImpl::SetVSyncRequest(VSyncRequest vsync_request) {
  vsync_request_ = std::move(vsync_request);
}

bool MessageLoopImpl::CanRunNow() {
  return MessageLoop::GetCurrent().GetLoopImpl().get() == this;
}

}  // namespace fml
}  // namespace lynx
