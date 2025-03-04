// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_SHELL_TASM_OPERATION_QUEUE_ASYNC_H_
#define CORE_SHELL_TASM_OPERATION_QUEUE_ASYNC_H_

#include <mutex>
#include <vector>

#include "core/shell/tasm_operation_queue.h"

namespace lynx {
namespace shell {

class TASMOperationQueueAsync final : public TASMOperationQueue {
 public:
  // Utilize super's existing vector.
  TASMOperationQueueAsync()
      : TASMOperationQueue(), pending_operations_(operations_) {
    ready_operations_.reserve(kOperationArrayReserveSize);
  }
  ~TASMOperationQueueAsync() = default;

  // begin override
  void EnqueueOperation(TASMOperation operation) override;

  void EnqueueTrivialOperation(TASMOperation operation) override;

  bool Flush() override;
  void AppendPendingTask() override;

  void SetAppendPendingTaskNeededDuringFlush(bool needed) override;
  // end

 private:
  // Must be called with mutex_ held.
  void AppendPendingTaskLocked();

  // enqueue and dequeue operate on different thread
  // need use lock for operations
  std::mutex mutex_;
  std::vector<TASMOperationWrapper>& pending_operations_;
  std::vector<TASMOperationWrapper> ready_operations_;

  bool is_append_pending_task_needed_during_flush_{false};
};

}  // namespace shell
}  // namespace lynx
#endif  // CORE_SHELL_TASM_OPERATION_QUEUE_ASYNC_H_
