// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_INCLUDE_FML_CONCURRENT_MESSAGE_LOOP_H_
#define BASE_INCLUDE_FML_CONCURRENT_MESSAGE_LOOP_H_

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <memory>
#include <queue>
#include <string>
#include <vector>

#include "base/include/closure.h"
#include "base/include/fml/thread.h"

namespace lynx {
namespace fml {

class ConcurrentTaskRunner;

class ConcurrentMessageLoop
    : public std::enable_shared_from_this<ConcurrentMessageLoop> {
 public:
  static std::shared_ptr<ConcurrentMessageLoop> Create(
      size_t worker_count = std::thread::hardware_concurrency());
  static std::shared_ptr<ConcurrentMessageLoop> Create(
      const Thread::ThreadConfigSetter& setter,
      size_t worker_count = std::thread::hardware_concurrency());

  explicit ConcurrentMessageLoop(
      const std::string& name_prefix,
      Thread::ThreadPriority priority = Thread::ThreadPriority::NORMAL,
      size_t worker_count = std::thread::hardware_concurrency());
  explicit ConcurrentMessageLoop(
      const std::string& name_prefix, const Thread::ThreadConfigSetter& setter,
      Thread::ThreadPriority priority = Thread::ThreadPriority::NORMAL,
      size_t worker_count = std::thread::hardware_concurrency());

  ~ConcurrentMessageLoop();

  void PostTask(base::closure task);

  size_t GetWorkerCount() const;

  std::shared_ptr<ConcurrentTaskRunner> GetTaskRunner();

  void Terminate();

 private:
  std::mutex notify_mutex_;
  std::condition_variable notify_condition_;
  std::vector<std::thread> workers_;
  std::atomic<std::uint32_t> worker_count_ = 0;
  std::mutex tasks_mutex_;
  std::queue<base::closure> tasks_;
  std::atomic<std::uint32_t> task_count_ = 0;
  std::atomic_bool shutdown_ = false;

  void WorkerMain(uint32_t index);
};

class ConcurrentTaskRunner : public BasicTaskRunner {
 public:
  explicit ConcurrentTaskRunner(std::weak_ptr<ConcurrentMessageLoop> weak_loop);

  virtual ~ConcurrentTaskRunner();

  void PostTask(lynx::base::closure task) override;

 private:
  friend ConcurrentMessageLoop;

  std::weak_ptr<ConcurrentMessageLoop> weak_loop_;

  BASE_DISALLOW_COPY_AND_ASSIGN(ConcurrentTaskRunner);
};

}  // namespace fml
}  // namespace lynx

namespace fml {
using lynx::fml::ConcurrentMessageLoop;
using lynx::fml::ConcurrentTaskRunner;
}  // namespace fml

#endif  // BASE_INCLUDE_FML_CONCURRENT_MESSAGE_LOOP_H_
