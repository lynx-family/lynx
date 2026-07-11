// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_INCLUDE_FML_CONCURRENT_MESSAGE_LOOP_H_
#define BASE_INCLUDE_FML_CONCURRENT_MESSAGE_LOOP_H_

#include <atomic>
#include <cstdint>
#include <memory>
#include <string>

#include "base/include/base_export.h"
#include "base/include/closure.h"
#include "base/include/fml/thread.h"

namespace lynx {
namespace fml {

class ConcurrentLoopBackend;
class ConcurrentTaskRunner;

class BASE_EXPORT ConcurrentMessageLoop
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

  // Returns true if the calling thread is one of the worker threads owned by
  // this concurrent message loop.
  bool RunsTasksOnCurrentThreadWorker() const;

  size_t GetWorkerCount() const;

  std::shared_ptr<ConcurrentTaskRunner> GetTaskRunner();

  void Terminate();

 private:
  std::unique_ptr<ConcurrentLoopBackend> backend_;
  std::atomic_bool shutdown_ = false;
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
