// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_TRACE_NATIVE_HOOK_SYSTRACE_HOOK_SYSTEM_TRACE_H_
#define BASE_TRACE_NATIVE_HOOK_SYSTRACE_HOOK_SYSTEM_TRACE_H_

#include <memory>

#include "base/trace/native/hook_systrace/cpu_info_trace.h"
#include "base/trace/native/hook_systrace/memory_info_trace.h"

namespace lynx {
namespace trace {

class TraceController;

class HookSystemTrace {
 public:
  struct SetupConfig {
    bool cpu_trace_enabled{true};
    bool memory_info_trace_enabled{true};
  };

  HookSystemTrace(TraceController& owner) : memory_info_trace_(owner) {}
  ~HookSystemTrace() = default;

  void Install(const SetupConfig& config);

  void Uninstall();

 private:
  static void InstallSystemTraceHooks();
  static void UninstallSystemTraceHooks();

  CpuInfoTrace cpu_info_trace_;
  MemoryInfoTrace memory_info_trace_;
};
}  // namespace trace
}  // namespace lynx
#endif  // BASE_TRACE_NATIVE_HOOK_SYSTRACE_HOOK_SYSTEM_TRACE_H_
