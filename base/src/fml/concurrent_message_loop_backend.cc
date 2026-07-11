// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/include/fml/concurrent_message_loop_backend.h"

#include <memory>

#include "base/include/fml/thread.h"
#if defined(OS_HARMONY)
#include "base/src/fml/platform/harmony/concurrent_loop_backend_ffrt.h"
#endif
#include "base/src/fml/concurrent_loop_backend_std.h"

namespace lynx {
namespace fml {

std::unique_ptr<ConcurrentLoopBackend> CreateConcurrentLoopBackend(
    const std::string& name_prefix, Thread::ThreadPriority priority,
    size_t worker_count, Thread::ThreadConfigSetter setter) {
#if defined(OS_HARMONY)
  return std::make_unique<ConcurrentLoopBackendFFRT>(
      name_prefix, priority, worker_count, std::move(setter));
#elif defined(OS_IOS) || defined(OS_OSX)
  return std::make_unique<ConcurrentLoopBackendStd>(
      name_prefix, priority, worker_count, std::move(setter));
#else
  return std::make_unique<ConcurrentLoopBackendStd>(
      name_prefix, priority, worker_count, std::move(setter));
#endif
}

}  // namespace fml
}  // namespace lynx
