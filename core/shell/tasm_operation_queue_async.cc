// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/shell/tasm_operation_queue_async.h"

#include <utility>

#include "base/include/log/logging.h"
#include "base/trace/native/trace_event.h"
#include "core/base/lynx_trace_categories.h"

namespace lynx {
namespace shell {

// @note: run on layout thread
void TASMOperationQueueAsync::EnqueueOperation(TASMOperation operation) {
  pending_operations_.emplace_back(std::move(operation));
}

void TASMOperationQueueAsync::EnqueueTrivialOperation(TASMOperation operation) {
  pending_operations_.emplace_back(std::move(operation), true);
}

// @note: run on layout thread
void TASMOperationQueueAsync::AppendPendingTask() {
  std::unique_lock<std::mutex> local_lock(mutex_);

  AppendPendingTaskLocked();
}

// @note: run on tasm thread
// para type bool: can reduce hood lock
bool TASMOperationQueueAsync::Flush() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "TASMOperationQueueAsync::Flush");

  bool result = false;

  std::vector<TASMOperationWrapper> v_ops;
  {
    std::unique_lock<std::mutex> local_lock(mutex_);

    if (is_append_pending_task_needed_during_flush_) {
      AppendPendingTaskLocked();
    }

    v_ops = std::move(ready_operations_);
    ready_operations_.reserve(kOperationArrayReserveSize);
  }

  for (auto& [operation, is_trivial] : v_ops) {
    operation();
    result |= (!is_trivial);
  }
  return result;
}

void TASMOperationQueueAsync::SetAppendPendingTaskNeededDuringFlush(
    bool needed) {
  is_append_pending_task_needed_during_flush_ = needed;
}

void TASMOperationQueueAsync::AppendPendingTaskLocked() {
  if (!ready_operations_.empty()) {
    if (!pending_operations_.empty()) {
      ready_operations_.insert(
          ready_operations_.end(),
          std::make_move_iterator(pending_operations_.begin()),
          std::make_move_iterator(pending_operations_.end()));
      pending_operations_.clear();
    }
  } else {
    // ready_operations_ is empty, just swap. pending_operations_ will use
    // ready_operations_'s buffer and no need to reallocate.
    std::swap(ready_operations_, pending_operations_);
  }
}

}  // namespace shell
}  // namespace lynx
