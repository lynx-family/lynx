// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/include/fml/concurrent_message_loop.h"

#include <algorithm>
#include <utility>

#include "base/include/fml/concurrent_message_loop_backend.h"

namespace lynx {
namespace fml {

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
    : ConcurrentMessageLoop(name_prefix, Thread::ThreadConfigSetter{},
                            priority, worker_count) {
}

ConcurrentMessageLoop::ConcurrentMessageLoop(
    const std::string& name_prefix, const Thread::ThreadConfigSetter& setter,
    Thread::ThreadPriority priority, size_t worker_count)
    : backend_(CreateConcurrentLoopBackend(name_prefix, priority,
                                           std::max<size_t>(worker_count, 1u),
                                           setter)),
      shutdown_(false) {
}

ConcurrentMessageLoop::~ConcurrentMessageLoop() {
  // The backend's destructor joins the worker threads.
  Terminate();
}

void ConcurrentMessageLoop::PostTask(base::closure task) {
  if (!task) {
    return;
  }

  // After shutdown, run the task synchronously on the caller's thread
  // rather than dropping it on the floor.
  if (shutdown_.load()) {
    task();
    return;
  }

  backend_->PostTask(std::move(task));
}

bool ConcurrentMessageLoop::RunsTasksOnCurrentThreadWorker() const {
  return backend_->RunsTasksOnCurrentThreadWorker();
}

size_t ConcurrentMessageLoop::GetWorkerCount() const {
  return backend_->GetWorkerCount();
}

std::shared_ptr<ConcurrentTaskRunner> ConcurrentMessageLoop::GetTaskRunner() {
  return std::make_shared<ConcurrentTaskRunner>(weak_from_this());
}

void ConcurrentMessageLoop::Terminate() {
  shutdown_.store(true);
  backend_->Terminate();
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
