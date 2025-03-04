// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_UTILS_BASE_TASM_WORKER_BASIC_TASK_RUNNER_H_
#define CORE_RENDERER_UTILS_BASE_TASM_WORKER_BASIC_TASK_RUNNER_H_

#include <atomic>
#include <condition_variable>
#include <mutex>

#include "base/include/closure.h"
#include "base/include/concurrent_queue.h"
#include "base/include/fml/macros.h"
#include "base/include/fml/memory/ref_counted.h"
#include "base/include/fml/task_runner.h"
#include "base/include/fml/thread.h"

namespace lynx {
namespace tasm {

/// The object for scheduling tasks on a static thread named "TasmWorker".
///
/// This object inherits \p fml::BasicTaskRunner interface and implemnts
/// PostTask API without \p fml::MessageLoopImpl.
class TasmWorkerBasicTaskRunner
    : public fml::RefCountedThreadSafe<TasmWorkerBasicTaskRunner>,
      public fml::BasicTaskRunner {
 public:
  static TasmWorkerBasicTaskRunner& GetTasmWorkerBasicTaskRunner();

  virtual ~TasmWorkerBasicTaskRunner();

  virtual void PostTask(base::closure task) override;

 private:
  explicit TasmWorkerBasicTaskRunner();
  explicit TasmWorkerBasicTaskRunner(
      const fml::Thread::ThreadConfigSetter& setter);

  void WorkerMain();
  void InitializeRunningThread(const fml::Thread::ThreadConfigSetter& setter);

  void Join();

  base::ConcurrentQueue<base::closure> task_queue_{};
  std::condition_variable task_cond_var_;
  std::mutex mutex_;

  std::atomic_bool joined_{false};
  std::atomic_bool terminated_{false};

  std::thread* task_thread_;

  FML_FRIEND_MAKE_REF_COUNTED(TasmWorkerBasicTaskRunner);
  FML_FRIEND_REF_COUNTED_THREAD_SAFE(TasmWorkerBasicTaskRunner);
  BASE_DISALLOW_COPY_AND_ASSIGN(TasmWorkerBasicTaskRunner);
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_UTILS_BASE_TASM_WORKER_BASIC_TASK_RUNNER_H_
