// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_TRACE_NATIVE_HOOK_SYSTRACE_CPU_INFO_TRACE_H_
#define BASE_TRACE_NATIVE_HOOK_SYSTRACE_CPU_INFO_TRACE_H_

#include <memory>
#include <utility>
#include <vector>

#include "base/include/fml/thread.h"
#include "base/include/thread/timed_task.h"

namespace lynx {
namespace trace {

class CpuInfoTrace {
 public:
  using CpuFreq = std::pair</*cpu_index*/ uint32_t, /* cpu_freq(GHz) */ float>;

  CpuInfoTrace();
  ~CpuInfoTrace() = default;
  void DispatchBegin();
  void DispatchEnd();

 private:
  const std::vector<CpuFreq> ReadCpuCurFreq();
  fml::Thread thread_;
  std::unique_ptr<lynx::base::TimedTaskManager> timer_;
};

}  // namespace trace
}  // namespace lynx

#endif  // BASE_TRACE_NATIVE_HOOK_SYSTRACE_CPU_INFO_TRACE_H_
