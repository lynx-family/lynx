// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_INCLUDE_FML_CONCURRENT_MESSAGE_LOOP_BACKEND_H_
#define BASE_INCLUDE_FML_CONCURRENT_MESSAGE_LOOP_BACKEND_H_

#include <memory>
#include <string>

#include "base/include/base_export.h"
#include "base/include/closure.h"
#include "base/include/fml/thread.h"

namespace lynx {
namespace fml {

class ConcurrentLoopBackend {
 public:
  virtual ~ConcurrentLoopBackend() = default;

  virtual void PostTask(base::closure task) = 0;
  virtual bool RunsTasksOnCurrentThreadWorker() const = 0;
  virtual size_t GetWorkerCount() const = 0;
  virtual void Terminate() = 0;
};

BASE_EXPORT std::unique_ptr<ConcurrentLoopBackend> CreateConcurrentLoopBackend(
    const std::string& name_prefix,
    Thread::ThreadPriority priority,
    size_t worker_count,
    Thread::ThreadConfigSetter setter = nullptr);

}  // namespace fml
}  // namespace lynx

#endif  // BASE_INCLUDE_FML_CONCURRENT_MESSAGE_LOOP_BACKEND_H_
