// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/src/fml/concurrent_loop_backend_std.h"

#include <thread>

#include "base/include/fml/fml_trace_event_def.h"
#include "base/include/fml/platform/thread_config_setter.h"
#include "base/include/fml/thread.h"
#include "base/src/base_trace/base_trace_event_def.h"
#include "base/src/base_trace/trace_event.h"
#include "build/build_config.h"

#if defined(OS_IOS)
extern "C" void* objc_autoreleasePoolPush(void);
extern "C" void objc_autoreleasePoolPop(void*);
#endif

namespace lynx {
namespace fml {

thread_local ConcurrentLoopBackendStd*
    ConcurrentLoopBackendStd::g_current_worker = nullptr;

namespace {
constexpr uint32_t kWorkerSleepMultipleMicroseconds = 340;
constexpr uint32_t kWorkerMaxIdleMicroseconds = 34000;
}  // namespace

ConcurrentLoopBackendStd::ConcurrentLoopBackendStd(
    const std::string& name_prefix, Thread::ThreadPriority priority,
    size_t worker_count, Thread::ThreadConfigSetter setter)
    : worker_count_(std::max<size_t>(worker_count, 1u)),
      setter_(std::move(setter)) {
  const uint32_t max_worker_count = static_cast<uint32_t>(worker_count_);
  worker_count_atomic_.store(max_worker_count);
  workers_.reserve(max_worker_count);
  for (uint32_t i = 0; i < max_worker_count; ++i) {
    base::closure setup_thread = [name_prefix, i, priority, this]() {
      const auto config = fml::Thread::ThreadConfig(
          std::string{name_prefix + std::to_string(i + 1)}, priority);
      if (setter_) {
        setter_(config);
      } else {
#if defined(OS_IOS) || defined(OS_ANDROID)
        PlatformThreadPriority::Setter(config);
#else
        Thread::SetCurrentThreadName(config);
#endif
      }
      WorkerMain(i);
    };
    workers_.emplace_back(std::move(setup_thread));
  }
}

ConcurrentLoopBackendStd::~ConcurrentLoopBackendStd() {
  Terminate();
  for (auto& worker : workers_) {
    worker.join();
  }
}

bool ConcurrentLoopBackendStd::RunsTasksOnCurrentThreadWorker() const {
  return g_current_worker == this;
}

void ConcurrentLoopBackendStd::PostTask(base::closure task) {
  if (!task) {
    return;
  }

  // Don't just drop tasks on the floor in case of shutdown.
  if (shutdown_) {
    // TODO(zhengsenyao): Uncomment LOG code when LOG available
    //    DLOGW(
    //        "Tried to post a task to shutdown concurrent message "
    //        "loop. The task will be executed on the callers thread.");
    task();
    return;
  }

  std::unique_lock lock(tasks_mutex_);
  tasks_.push(std::move(task));
  lock.unlock();

  task_count_.fetch_add(1);

  if (worker_count_atomic_.load() <= 0) {
    notify_condition_.notify_all();
  }

  return;
}

void ConcurrentLoopBackendStd::WorkerMain(uint32_t index) {
  g_current_worker = this;

  const uint32_t sleep_microseconds =
      kWorkerSleepMultipleMicroseconds * (index + 1);
  const uint32_t max_sleep_count =
      kWorkerMaxIdleMicroseconds / sleep_microseconds;
  uint32_t sleep_count_down = 0;
  while (true) {
    uint32_t task_count = task_count_.load();
    while (task_count > 0 &&
           !task_count_.compare_exchange_weak(task_count, task_count - 1)) {
    }

    if (task_count > 0) {
      base::closure task;

      std::unique_lock lock(tasks_mutex_);
      if (tasks_.size() != 0) {
        task = std::move(tasks_.front());
        tasks_.pop();
      }
      lock.unlock();

      if (task) {
#if defined(OS_IOS)
        void* pool = objc_autoreleasePoolPush();
#endif
        task();
        task = nullptr;
#if defined(OS_IOS)
        objc_autoreleasePoolPop(pool);
#endif
      }

      std::uint32_t worker_count = worker_count_atomic_.load();
      if (worker_count < workers_.size() && worker_count < (task_count - 1)) {
        notify_condition_.notify_all();
      }
      continue;
    }

    if (shutdown_) {
      break;
    }

    if (sleep_count_down == 0) {
      --worker_count_atomic_;
      std::unique_lock lock(notify_mutex_);
      notify_condition_.wait(
          lock, [&]() { return task_count_.load() > 0 || shutdown_; });
      lock.unlock();
      ++worker_count_atomic_;
      sleep_count_down = max_sleep_count;
      BASE_TRACE_EVENT(LYNX_BASE_TRACE_CATEGORY, CONCURRENT_WORKER_AWOKE);
    } else {
      --sleep_count_down;
      std::this_thread::sleep_for(
          std::chrono::microseconds(sleep_microseconds));
    }
  }

  g_current_worker = nullptr;
}

void ConcurrentLoopBackendStd::Terminate() {
  shutdown_ = true;
  notify_condition_.notify_all();
}

}  // namespace fml
}  // namespace lynx
