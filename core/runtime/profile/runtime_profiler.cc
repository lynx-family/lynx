// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/runtime/profile/runtime_profiler.h"

#include <utility>

namespace lynx {
namespace profile {

RuntimeProfiler::RuntimeProfiler() {
#if ENABLE_TRACE_PERFETTO
  task_runner_ =
      fml::MessageLoop::EnsureInitializedForCurrentThread().GetTaskRunner();
#endif
}

void RuntimeProfiler::StopProfiling(base::closure task, bool is_destory) {
#if ENABLE_TRACE_PERFETTO
  if (is_destory || task_runner_->RunsTasksOnCurrentThread()) {
    task();
  } else {
    std::mutex mutex;
    std::condition_variable cv;

    std::unique_lock<std::mutex> lock(mutex);
    bool done = false;
    task_runner_->PostEmergencyTask(
        [&cv, &mutex, &done, task = std::move(task)] {
          std::lock_guard<std::mutex> inner_lock(mutex);
          task();
          done = true;
          cv.notify_all();
        });
    cv.wait(lock, [&done] { return done; });
  }
#endif
}

void RuntimeProfiler::StartProfiling(base::closure task, bool is_create) {
#if ENABLE_TRACE_PERFETTO
  if (is_create || task_runner_->RunsTasksOnCurrentThread()) {
    task();
  } else {
    task_runner_->PostEmergencyTask(std::move(task));
  }
#endif
}

void RuntimeProfiler::SetupProfiling(base::closure task) {
#if ENABLE_TRACE_PERFETTO
  if (task_runner_->RunsTasksOnCurrentThread()) {
    task();
  } else {
    task_runner_->PostEmergencyTask(std::move(task));
  }
#endif
}

}  // namespace profile
}  // namespace lynx
