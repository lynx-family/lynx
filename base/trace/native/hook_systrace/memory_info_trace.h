// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_TRACE_NATIVE_HOOK_SYSTRACE_MEMORY_INFO_TRACE_H_
#define BASE_TRACE_NATIVE_HOOK_SYSTRACE_MEMORY_INFO_TRACE_H_

#include <memory>
#include <utility>

#include "base/include/fml/thread.h"
#include "base/include/thread/timed_task.h"

namespace lynx {
namespace trace {

class TraceController;

class MemoryInfoTrace {
 public:
  MemoryInfoTrace(TraceController& owner);
  ~MemoryInfoTrace() = default;
  void DispatchBegin();
  void DispatchEnd();

 private:
  TraceController& owner_;
  fml::Thread thread_;
  std::unique_ptr<lynx::base::TimedTaskManager> timer_;
};

}  // namespace trace
}  // namespace lynx

#endif  // BASE_TRACE_NATIVE_HOOK_SYSTRACE_MEMORY_INFO_TRACE_H_
