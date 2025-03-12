// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/shell/layout_result_manager.h"

#include <utility>

#include "base/include/log/logging.h"

namespace lynx {
namespace shell {

// Calling this method is impossible; therefore, NOTREACHED() is used.
void LayoutResultManager::AppendPendingTask() { NOTREACHED(); }

// Calling this method is impossible; therefore, NOTREACHED() is used.
bool LayoutResultManager::Flush() {
  NOTREACHED();

  return false;
}

// Calling this method is impossible; therefore, NOTREACHED() is used.
void LayoutResultManager::SetAppendPendingTaskNeededDuringFlush(bool needed) {
  NOTREACHED();
}

std::vector<TASMOperationQueue::TASMOperationWrapper>
LayoutResultManager::FetchTASMOperations() {
  auto operations = std::move(operations_);
  operations_.reserve(kOperationArrayReserveSize);

  return operations;
}

bool LayoutResultManager::ExecuteTASMOperations(
    const std::vector<TASMOperationQueue::TASMOperationWrapper>& operations) {
  bool result = false;

  for (auto& [operation, is_trivial] : operations) {
    operation();
    result |= (!is_trivial);
  }
  return result;
}

void LayoutResultManager::EnqueueOnLayoutAfterTask(
    base::closure on_layout_after_task) {
  std::lock_guard local_lock(on_layout_after_tasks_mutex_);

  on_layout_after_tasks_.emplace_back(std::move(on_layout_after_task));
}

void LayoutResultManager::RunOnLayoutAfterTasks() {
  std::vector<base::closure> on_layout_after_tasks;

  {
    std::lock_guard local_lock(on_layout_after_tasks_mutex_);
    on_layout_after_tasks = std::move(on_layout_after_tasks_);
  }

  for (auto& on_layout_after_task : on_layout_after_tasks) {
    on_layout_after_task();
  }
}

}  // namespace shell
}  // namespace lynx
