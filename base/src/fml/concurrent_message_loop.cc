// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/include/fml/concurrent_message_loop.h"

#include <thread>

#include "base/include/fml/platform/thread_config_setter.h"
#include "base/trace/native/trace_event.h"
#include "build/build_config.h"

#if defined(OS_IOS)
extern "C" void* objc_autoreleasePoolPush(void);
extern "C" void objc_autoreleasePoolPop(void*);
#endif

namespace lynx {
namespace fml {

static constexpr uint32_t kWorkerSleepMultipleMicroseconds = 340;
static constexpr uint32_t kWorkerMaxIdleMicroseconds = 34000;

std::shared_ptr<ConcurrentMessageLoop> ConcurrentMessageLoop::Create(
    size_t worker_count) {
  return std::shared_ptr<ConcurrentMessageLoop>{new ConcurrentMessageLoop(
      "io.worker.", Thread::ThreadPriority::NORMAL, worker_count)};
}

std::shared_ptr<ConcurrentMessageLoop> ConcurrentMessageLoop::Create(
    const Thread::ThreadConfigSetter& setter, size_t worker_count) {
  return std::shared_ptr<ConcurrentMessageLoop>{new ConcurrentMessageLoop(
      "io.worker.", setter, Thread::ThreadPriority::NORMAL, worker_count)};
}

ConcurrentMessageLoop::ConcurrentMessageLoop(const std::string& name_prefix,
                                             Thread::ThreadPriority priority,
                                             size_t worker_count)
    : ConcurrentMessageLoop(name_prefix,
#if defined(OS_IOS) || defined(OS_ANDROID)
                            PlatformThreadPriority::Setter,
#else
                            Thread::SetCurrentThreadName,
#endif
                            priority, worker_count) {
}

ConcurrentMessageLoop::ConcurrentMessageLoop(
    const std::string& name_prefix, const Thread::ThreadConfigSetter& setter,
    Thread::ThreadPriority priority, size_t worker_count) {
  uint32_t max_worker_count =
      std::max<uint32_t>(static_cast<uint32_t>(worker_count), 1u);
  worker_count_.store(max_worker_count);
  workers_.reserve(max_worker_count);
  for (uint32_t i = 0; i < max_worker_count; ++i) {
    base::closure setup_thread = [name_prefix, i, priority, setter, this]() {
      const auto config = fml::Thread::ThreadConfig(
          std::string{name_prefix + std::to_string(i + 1)}, priority);
      setter(config);
      WorkerMain(i);
    };
    workers_.emplace_back(std::move(setup_thread));
  }
}

ConcurrentMessageLoop::~ConcurrentMessageLoop() {
  Terminate();
  for (auto& worker : workers_) {
    worker.join();
  }
}

size_t ConcurrentMessageLoop::GetWorkerCount() const { return workers_.size(); }

void ConcurrentMessageLoop::PostTask(base::closure task) {
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

  if (worker_count_.load() <= 0) {
    notify_condition_.notify_all();
  }

  return;
}

void ConcurrentMessageLoop::WorkerMain(uint32_t index) {
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
#if defined(OS_IOS)
        objc_autoreleasePoolPop(pool);
#endif
      }

      std::uint32_t worker_count = worker_count_.load();
      if (worker_count < workers_.size() && worker_count < (task_count - 1)) {
        notify_condition_.notify_all();
      }
      continue;
    }

    if (shutdown_) {
      break;
    }

    if (sleep_count_down == 0) {
      --worker_count_;
      std::unique_lock lock(notify_mutex_);
      notify_condition_.wait(
          lock, [&]() { return task_count_.load() > 0 || shutdown_; });
      lock.unlock();
      ++worker_count_;
      sleep_count_down = max_sleep_count;
      TRACE_EVENT("lynx", "ConcurrentWorker AWoke");
    } else {
      --sleep_count_down;
      std::this_thread::sleep_for(
          std::chrono::microseconds(sleep_microseconds));
    }
  }
}

std::shared_ptr<ConcurrentTaskRunner> ConcurrentMessageLoop::GetTaskRunner() {
  return std::make_shared<ConcurrentTaskRunner>(weak_from_this());
}

void ConcurrentMessageLoop::Terminate() {
  shutdown_ = true;
  notify_condition_.notify_all();
}

ConcurrentTaskRunner::ConcurrentTaskRunner(
    std::weak_ptr<ConcurrentMessageLoop> weak_loop)
    : weak_loop_(std::move(weak_loop)) {}

ConcurrentTaskRunner::~ConcurrentTaskRunner() = default;

void ConcurrentTaskRunner::PostTask(lynx::base::closure task) {
  if (!task) {
    return;
  }

  if (auto loop = weak_loop_.lock()) {
    loop->PostTask(std::move(task));
    return;
  }

  task();
}

}  // namespace fml
}  // namespace lynx
