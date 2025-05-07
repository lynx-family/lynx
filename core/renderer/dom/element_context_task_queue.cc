// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/renderer/dom/element_context_task_queue.h"

#include "base/trace/native/trace_event.h"
#include "core/renderer/dom/fiber/fiber_element.h"
#include "core/renderer/utils/lynx_env.h"
namespace lynx {
namespace tasm {
ElementContextTaskQueue::~ElementContextTaskQueue() {
  task_queue_.ReversePopAll();
}
void ElementContextTaskQueue::EnqueueTask(
    base::MoveOnlyClosure<void> operation) {
  if (predicate_()) {
    task_queue_.Push(std::move(operation));
  } else {
    operation();
  }
}
void ElementContextTaskQueue::FlushEnqueuedTasks() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FlushEnqueuedTasks");
  auto tasks = task_queue_.PopAll();
  for (auto& task : tasks) {
    task();
  }
}
}  // namespace tasm
}  // namespace lynx
