// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_SRC_FML_CONCURRENT_LOOP_BACKEND_STD_H_
#define BASE_SRC_FML_CONCURRENT_LOOP_BACKEND_STD_H_

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>

#include "base/include/fml/concurrent_message_loop_backend.h"

namespace lynx {
namespace fml {

// Concrete ConcurrentLoopBackend using std::thread.
class ConcurrentLoopBackendStd final : public ConcurrentLoopBackend {
 public:
  ConcurrentLoopBackendStd(const std::string& name_prefix,
                           Thread::ThreadPriority priority,
                           size_t worker_count,
                           Thread::ThreadConfigSetter setter);
  ~ConcurrentLoopBackendStd() override;

  void PostTask(base::closure task) override;
  bool RunsTasksOnCurrentThreadWorker() const override;
  size_t GetWorkerCount() const override { return worker_count_; }
  void Terminate() override;

 private:
  void WorkerMain(uint32_t index);

  std::vector<std::thread> workers_;
  const size_t worker_count_;
  const Thread::ThreadConfigSetter setter_;
  std::atomic<uint32_t> worker_count_atomic_{0};
  std::mutex tasks_mutex_;
  std::queue<base::closure> tasks_;
  std::atomic<uint32_t> task_count_{0};
  std::atomic_bool shutdown_{false};
  std::mutex notify_mutex_;
  std::condition_variable notify_condition_;

  static thread_local ConcurrentLoopBackendStd* g_current_worker;
};

}  // namespace fml
}  // namespace lynx

#endif  // BASE_SRC_FML_CONCURRENT_LOOP_BACKEND_STD_H_
