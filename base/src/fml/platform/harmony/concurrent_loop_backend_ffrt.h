// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_SRC_FML_PLATFORM_HARMONY_CONCURRENT_LOOP_BACKEND_FFRT_H_
#define BASE_SRC_FML_PLATFORM_HARMONY_CONCURRENT_LOOP_BACKEND_FFRT_H_

#include <cstddef>
#include <memory>
#include <string>

#include "base/include/fml/concurrent_message_loop_backend.h"
#include "base/include/fml/thread.h"

// Forward-declared to avoid pulling ffrt headers into this translation unit.
namespace ffrt {
class queue;
}  // namespace ffrt

namespace lynx {
namespace fml {

// ConcurrentLoopBackend implementation backed by ffrt::queue.
class ConcurrentLoopBackendFFRT final : public ConcurrentLoopBackend {
 public:
  ConcurrentLoopBackendFFRT(const std::string& name_prefix,
                            Thread::ThreadPriority priority,
                            size_t worker_count,
                            Thread::ThreadConfigSetter setter = nullptr);
  ~ConcurrentLoopBackendFFRT() override;

  void PostTask(base::closure task) override;
  bool RunsTasksOnCurrentThreadWorker() const override;
  size_t GetWorkerCount() const override { return worker_count_; }
  void Terminate() override;

 private:
  std::unique_ptr<ffrt::queue> queue_;
  size_t worker_count_;
  // Stored for parity with BackendStd; currently unused.
  Thread::ThreadConfigSetter setter_;
  // Set by Terminate(); gates PostTask to fall back to synchronous execution.
  // The actual FFRT queue destruction happens in the destructor.
  std::atomic_bool terminated_{false};

  static thread_local ConcurrentLoopBackendFFRT* g_current_worker;
};

}  // namespace fml
}  // namespace lynx

#endif  // BASE_SRC_FML_PLATFORM_HARMONY_CONCURRENT_LOOP_BACKEND_FFRT_H_
