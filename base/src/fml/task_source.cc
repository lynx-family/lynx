// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define FML_USED_ON_EMBEDDER

#include "base/include/fml/task_source.h"

namespace lynx {
namespace fml {

namespace {

// we use 50ms for a idle period
// https://w3c.github.io/requestidlecallback/#why50
constexpr int64_t kIdlePeriod = 50;  // milliseconds

}  // namespace

TaskSource::TaskSource(TaskQueueId task_queue_id)
    : task_queue_id_(task_queue_id) {
  primary_task_queue_.reserve(64);
  emergency_task_queue_.reserve(32);
  micro_task_queue_.reserve(32);
}

TaskSource::~TaskSource() { ShutDown(); }

void TaskSource::ShutDown() { TakePendingTasks(); }

TaskSource::PendingTasks TaskSource::TakePendingTasks() {
  PendingTasks pending_tasks;
  pending_tasks.primary = std::move(primary_task_queue_);
  pending_tasks.emergency = std::move(emergency_task_queue_);
  pending_tasks.idle = std::move(idle_task_queue_);
  pending_tasks.micro = std::move(micro_task_queue_);
  primary_task_queue_ = {};
  emergency_task_queue_ = {};
  idle_task_queue_ = {};
  micro_task_queue_ = {};
  return pending_tasks;
}

void TaskSource::RegisterTask(DelayedTask task) {
  switch (task.GetTaskSourceGrade()) {
    case TaskSourceGrade::kUserInteraction:
      primary_task_queue_.push(std::move(task));
      break;
    case TaskSourceGrade::kUnspecified:
      primary_task_queue_.push(std::move(task));
      break;
    case TaskSourceGrade::kEmergency:
      emergency_task_queue_.push(std::move(task));
      break;
    case TaskSourceGrade::kIdle:
      idle_task_queue_.push(std::move(task));
      break;
    case TaskSourceGrade::kMicrotask:
      micro_task_queue_.push(std::move(task));
      break;
  }
}

void TaskSource::PopTask(TaskSourceGrade grade) {
  switch (grade) {
    case TaskSourceGrade::kUserInteraction:
      primary_task_queue_.pop();
      break;
    case TaskSourceGrade::kUnspecified:
      primary_task_queue_.pop();
      break;
    case TaskSourceGrade::kEmergency:
      emergency_task_queue_.pop();
      break;
    case TaskSourceGrade::kIdle:
      idle_task_queue_.pop();
      break;
    case TaskSourceGrade::kMicrotask:
      micro_task_queue_.pop();
      break;
  }
}

size_t TaskSource::GetNumPendingTasks() const {
  return primary_task_queue_.size() + emergency_task_queue_.size() +
         idle_task_queue_.size() + micro_task_queue_.size();
}

bool TaskSource::IsEmpty() const { return GetNumPendingTasks() == 0; }

TaskSource::TopTask TaskSource::Top() const {
  if (!micro_task_queue_.empty()) {
    const auto& microtask_top = micro_task_queue_.top();
    return {
        .task_queue_id = task_queue_id_,
        .task = microtask_top,
    };
  }

  if (!emergency_task_queue_.empty()) {
    const auto& emergency_top = emergency_task_queue_.top();
    return {
        .task_queue_id = task_queue_id_,
        .task = emergency_top,
    };
  }

  if (!primary_task_queue_.empty()) {
    const auto& primary_top = primary_task_queue_.top();
    // if there are primary tasks in a idle period,
    // the idle tasks will be suspended.
    if (idle_task_queue_.empty() ||
        (primary_top.GetTargetTime() - TimePoint::Now()).ToMilliseconds() <=
            kIdlePeriod) {
      return {
          .task_queue_id = task_queue_id_,
          .task = primary_top,
      };
    }
  }

  // TODO(zhengsenyao): Replace LYNX_BASE_CHECK with CHECK when CHECK available.
  LYNX_BASE_CHECK(!idle_task_queue_.empty());
  const auto& idle_top = idle_task_queue_.front();
  return {
      .task_queue_id = task_queue_id_,
      .task = idle_top,
  };
}

const DelayedTask* TaskSource::TopOrNull() const {
  if (!micro_task_queue_.empty()) {
    return &micro_task_queue_.top();
  }

  if (!emergency_task_queue_.empty()) {
    return &emergency_task_queue_.top();
  }

  if (!primary_task_queue_.empty()) {
    const auto& primary_top = primary_task_queue_.top();
    // if there are primary tasks in a idle period,
    // the idle tasks will be suspended.
    if (idle_task_queue_.empty() ||
        (primary_top.GetTargetTime() - TimePoint::Now()).ToMilliseconds() <=
            kIdlePeriod) {
      return &primary_top;
    }
  }

  if (!idle_task_queue_.empty()) {
    return &idle_task_queue_.front();
  }

  return nullptr;
}

}  // namespace fml
}  // namespace lynx
