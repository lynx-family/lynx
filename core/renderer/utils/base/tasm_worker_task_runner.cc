// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/utils/base/tasm_worker_task_runner.h"

#include <utility>

#include "core/renderer/utils/base/tasm_worker_basic_task_runner.h"

namespace lynx {
namespace tasm {

TasmWorkerTaskRunner::TasmWorkerTaskRunner() {}

void TasmWorkerTaskRunner::PostTask(base::closure task) {
  // Wrapper task to decrement pending_task_count_
  auto wrapper_task = [this, task_ = std::move(task)]() {
    task_();
    // Use atomic operations to decrement count
    pending_task_count_--;
  };
  {
    // Use atomic operations to increment count
    pending_task_count_++;
  }
  TasmWorkerBasicTaskRunner::GetTasmWorkerBasicTaskRunner().PostTask(
      std::move(wrapper_task));
}

void TasmWorkerTaskRunner::WaitForCompletion() {
  int expected = 0;
  while (!pending_task_count_.compare_exchange_weak(
      expected, 0, std::memory_order_acquire, std::memory_order_relaxed)) {
    // Reset expected value because compare_exchange_weak modifies it.
    expected = 0;
  }
}

}  // namespace tasm
}  // namespace lynx
