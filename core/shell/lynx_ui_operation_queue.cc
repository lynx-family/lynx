// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/shell/lynx_ui_operation_queue.h"

#include <utility>

#include "base/include/debug/lynx_assert.h"
#include "base/trace/native/trace_event.h"
#include "core/base/lynx_trace_categories.h"
#include "core/base/threading/task_runner_manufactor.h"
#include "core/services/long_task_timing/long_task_monitor.h"

namespace lynx {
namespace shell {

void LynxUIOperationQueue::EnqueueUIOperation(UIOperation operation) {
  operations_.Push(std::move(operation));
}

void LynxUIOperationQueue::EnqueueHighPriorityOperation(UIOperation operation) {
  high_priority_operations_.Push(std::move(operation));
}

void LynxUIOperationQueue::Flush() {
  if (!enable_flush_) {
    return;
  }
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "LynxUIOperationQueue.Flush");
  auto high_priority_operations = high_priority_operations_.PopAll();
  auto operations = operations_.PopAll();
  // need move, else LynxUI may invoke Flush again when Flush...
  ConsumeOperations(high_priority_operations, operations);
}

void LynxUIOperationQueue::SetEnableFlush(bool enable_flush) {
  enable_flush_ = enable_flush;
}

void LynxUIOperationQueue::Destroy() { destroyed_ = true; }

void LynxUIOperationQueue::ForceFlush() { Flush(); }

void LynxUIOperationQueue::ConsumeOperations(
    const base::ConcurrentQueue<UIOperation>::IterableContainer&
        high_priority_operations,
    const base::ConcurrentQueue<UIOperation>::IterableContainer& operations) {
  tasm::timing::LongTaskMonitor::Scope longTaskScope(
      instance_id_, tasm::timing::kUIOperationFlushTask,
      tasm::timing::kTaskNameLynxUIOperationQueueConsumeOperations);
  for (auto& operation : high_priority_operations) {
    TRACE_EVENT(LYNX_TRACE_CATEGORY,
                "LynxUIOperationQueue::ExecuteHighPriorityOperation");
    operation();
  }

  for (auto& operation : operations) {
    TRACE_EVENT(LYNX_TRACE_CATEGORY, "LynxUIOperationQueue::ExecuteOperation");
    operation();
  }

  if (error_callback_ == nullptr) {
    return;
  }

  auto& error = base::ErrorStorage::GetInstance().GetError();
  if (error != nullptr) {
    // TODO(yuanzhiwen): handle LynxErrorLevel::Fatal in platform.
    if (error->should_abort_) {
      if (tasm::LynxEnv::GetInstance().IsDevToolComponentAttach() &&
          !tasm::LynxEnv::GetInstance().IsLogBoxEnabled()) {
        LOGF("error_message: " << error->error_message_
                               << " fix_suggestion: " << error->error_message_);
        return;
      } else {
        error->error_level_ = base::LynxErrorLevel::Error;
      }
    }
    error_callback_(std::move(*error));
    lynx::base::ErrorStorage::GetInstance().Reset();
  }
}

}  // namespace shell
}  // namespace lynx
