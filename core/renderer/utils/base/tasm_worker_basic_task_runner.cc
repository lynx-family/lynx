// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/utils/base/tasm_worker_basic_task_runner.h"

#include <utility>
#include <vector>

#include "base/include/fml/platform/thread_config_setter.h"
#include "base/include/fml/thread.h"
#include "base/include/no_destructor.h"

namespace lynx {
namespace tasm {

TasmWorkerBasicTaskRunner&
TasmWorkerBasicTaskRunner::GetTasmWorkerBasicTaskRunner() {
  static TasmWorkerBasicTaskRunner worker;
  return worker;
}

TasmWorkerBasicTaskRunner::TasmWorkerBasicTaskRunner()
    : TasmWorkerBasicTaskRunner(
#if defined(OS_IOS) || defined(OS_ANDROID)
          fml::PlatformThreadPriority::Setter
#else
          fml::Thread::SetCurrentThreadName
#endif
      ) {
}

TasmWorkerBasicTaskRunner::~TasmWorkerBasicTaskRunner() { Join(); }

void TasmWorkerBasicTaskRunner::InitializeRunningThread(
    const fml::Thread::ThreadConfigSetter& setter) {
  base::closure setup_thread = [setter, this]() {
    const auto config = fml::Thread::ThreadConfig(
        "TasmWorker", fml::Thread::ThreadPriority::HIGH);
    setter(config);
    WorkerMain();
  };
  static base::NoDestructor<std::thread> tasm_worker(std::move(setup_thread));
  task_thread_ = tasm_worker.get();
}

TasmWorkerBasicTaskRunner::TasmWorkerBasicTaskRunner(
    const fml::Thread::ThreadConfigSetter& setter) {
  InitializeRunningThread(setter);
}

void TasmWorkerBasicTaskRunner::WorkerMain() {
  while (!terminated_) {
    {
      std::unique_lock<std::mutex> lock(mutex_);
      task_cond_var_.wait(lock, [this] { return !task_queue_.Empty(); });
    }
    auto tasks = task_queue_.PopAll();
    for (auto it = tasks.begin(); it != tasks.end(); ++it) {
      (*it)();
    }
  }
}

void TasmWorkerBasicTaskRunner::PostTask(base::closure task) {
  if (terminated_) {
    // If the TasmWorkerBasicTaskRunner has already been terminated, PostTask
    // should destruct |task| synchronously within this function.
    return;
  }
  task_queue_.Push(std::move(task));
  {
    std::unique_lock<std::mutex> lock(mutex_);
    // Notify new task is ready.
    task_cond_var_.notify_one();
  }
}

void TasmWorkerBasicTaskRunner::Join() {
  if (joined_) {
    return;
  }
  joined_ = true;
  PostTask([this]() { this->terminated_ = true; });
  task_thread_->join();
}

}  // namespace tasm
}  // namespace lynx
