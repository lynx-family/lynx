// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/runtime/profile/runtime_profiler.h"

#include <utility>
#ifdef OS_WIN
#include "core/base/threading/task_runner_manufactor.h"
#endif

namespace lynx {
namespace runtime {
namespace profile {

RuntimeProfiler::RuntimeProfiler(bool is_main_thread) {
#if ENABLE_TRACE_PERFETTO
#ifdef OS_WIN
  is_main_thread_running_ = is_main_thread;
  if (is_main_thread) {
    task_runner_ = lynx::base::UIThread::GetRunner();
    return;
  }
#endif
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
    auto task_wrapper = [&cv, &mutex, &done, task = std::move(task)] {
      std::lock_guard<std::mutex> inner_lock(mutex);
      task();
      done = true;
      cv.notify_all();
    };
#ifdef OS_WIN
    if (is_main_thread_running_) {
      task_runner_->PostTask(std::move(task_wrapper));
    } else {
      task_runner_->PostEmergencyTask(std::move(task_wrapper));
    }
#else
    task_runner_->PostEmergencyTask(std::move(task_wrapper));
#endif
    cv.wait(lock, [&done] { return done; });
  }
#endif
}

void RuntimeProfiler::StartProfiling(base::closure task, bool is_create) {
#if ENABLE_TRACE_PERFETTO
  if (is_create || task_runner_->RunsTasksOnCurrentThread()) {
    task();
  } else {
#ifdef OS_WIN
    if (is_main_thread_running_) {
      task_runner_->PostTask(std::move(task));
    } else {
      task_runner_->PostEmergencyTask(std::move(task));
    }
#else
    task_runner_->PostEmergencyTask(std::move(task));
#endif
  }
#endif
}

void RuntimeProfiler::SetupProfiling(base::closure task) {
#if ENABLE_TRACE_PERFETTO
  if (task_runner_->RunsTasksOnCurrentThread()) {
    task();
  } else {
#ifdef OS_WIN
    if (is_main_thread_running_) {
      task_runner_->PostTask(std::move(task));
    } else {
      task_runner_->PostEmergencyTask(std::move(task));
    }
#else
    task_runner_->PostEmergencyTask(std::move(task));
#endif
  }
#endif
}

void RuntimeProfiler::EnableSingleProfiler() {
  if (GetType() != trace::RuntimeProfilerType::v8) {
    return;
  }
  is_single_profiler_ = true;
}

}  // namespace profile
}  // namespace runtime
}  // namespace lynx
