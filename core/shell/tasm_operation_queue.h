// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SHELL_TASM_OPERATION_QUEUE_H_
#define CORE_SHELL_TASM_OPERATION_QUEUE_H_

#include <atomic>
#include <condition_variable>
#include <memory>
#include <utility>
#include <vector>

#include "base/include/closure.h"

namespace lynx {
namespace shell {

// type foy sync
// share operations between tasm thread and layout thread
class TASMOperationQueue {
 public:
  using TASMOperation = base::closure;
  constexpr static size_t kOperationArrayReserveSize = 128;

  TASMOperationQueue() { operations_.reserve(kOperationArrayReserveSize); }
  virtual ~TASMOperationQueue() = default;

  virtual void EnqueueOperation(TASMOperation operation);

  // Trivial operations do not affect the result of Flush.
  // The method Flush will return true only when a non-trivial operation is
  // enqueued.
  virtual void EnqueueTrivialOperation(TASMOperation operation);

  virtual bool Flush();
  virtual void AppendPendingTask() {}

  virtual void SetAppendPendingTaskNeededDuringFlush(bool needed) {}

  // first screen operation
  // condition variable for first screen between layout thread and tasm thread
  // push into base class, reduce api impl
  std::atomic_bool has_first_screen_{false};
  std::condition_variable first_screen_cv_;

 protected:
  struct TASMOperationWrapper {
    explicit TASMOperationWrapper(TASMOperation operation,
                                  bool is_trivial = false)
        : operation(std::move(operation)), is_trivial(is_trivial) {}
    ~TASMOperationWrapper() = default;

    TASMOperationWrapper(TASMOperationWrapper const&) = delete;
    TASMOperationWrapper& operator=(TASMOperationWrapper const&) = delete;

    TASMOperationWrapper(TASMOperationWrapper&&) = default;
    TASMOperationWrapper& operator=(TASMOperationWrapper&&) = default;

    TASMOperation operation;
    bool is_trivial;
  };

  std::vector<TASMOperationWrapper> operations_;
};

}  // namespace shell
}  // namespace lynx

#endif  // CORE_SHELL_TASM_OPERATION_QUEUE_H_
