// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_UTILS_BASE_TASM_WORKER_TASK_RUNNER_H_
#define CORE_RENDERER_UTILS_BASE_TASM_WORKER_TASK_RUNNER_H_

#include <atomic>

#include "base/include/closure.h"

namespace lynx {
namespace tasm {

/// The object for scheduling tasks on a static thread named "TasmWorker".
///
/// When one wants to execute an operation on the static thread "TasmWorker",
/// they post a task to the TasmWorkerTaskRunner associated with its
/// ElementManager.
/// TODO(zhouzhitao): Currently rely on \p fml::TasmWorkerBasicTaskRunner to
/// execute tasks. Migrate to high-performance \p fml::BasicTaskRunner
/// implementation when fml library version is ready.
class TasmWorkerTaskRunner {
 public:
  explicit TasmWorkerTaskRunner();
  virtual ~TasmWorkerTaskRunner() = default;
  TasmWorkerTaskRunner(const TasmWorkerTaskRunner&) = delete;
  TasmWorkerTaskRunner& operator=(const TasmWorkerTaskRunner&) = delete;

  /// Post task to internal TaskRunner
  void PostTask(base::closure task);

  /// Wait for all tasks in internal TaskRunner to complete
  void WaitForCompletion();

 private:
  std::atomic<int> pending_task_count_{0};
};
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_UTILS_BASE_TASM_WORKER_TASK_RUNNER_H_
