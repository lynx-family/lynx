// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RENDERER_DOM_ELEMENT_CONTEXT_TASK_QUEUE_H_
#define CORE_RENDERER_DOM_ELEMENT_CONTEXT_TASK_QUEUE_H_
#include <utility>

#include "base/include/closure.h"
#include "base/include/concurrent_queue.h"
namespace lynx {
namespace tasm {
class FiberElement;
class ElementContextTaskQueue {
 public:
  ElementContextTaskQueue(base::MoveOnlyClosure<bool> predicate) {
    predicate_ = std::move(predicate);
  };
  virtual ~ElementContextTaskQueue();
  void EnqueueTask(base::MoveOnlyClosure<void> operation);
  void FlushEnqueuedTasks();

 private:
  base::ConcurrentQueue<base::MoveOnlyClosure<void>> task_queue_{};
  base::MoveOnlyClosure<bool> predicate_;
};
}  // namespace tasm
}  // namespace lynx
#endif  // CORE_RENDERER_DOM_ELEMENT_CONTEXT_TASK_QUEUE_H_
