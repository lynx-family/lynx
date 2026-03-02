// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "base/include/log/logging.h"
#include "core/base/lynx_export.h"
#include "core/runtime/profile/runtime_profiler_manager.h"
#include "core/runtime/profile/v8/v8_runtime_profiler.h"
#include "core/runtime/profile/v8/v8_runtime_profiler_wrapper.h"

namespace lynx {
namespace runtime {
namespace profile {
namespace {
#if ENABLE_TRACE_PERFETTO
static std::shared_ptr<lynx::runtime::profile::V8RuntimeProfiler>
    v8_runtime_profiler = nullptr;
#endif
}  // namespace

LYNX_EXPORT void AddSingleLynxTraceV8RuntimeProfiler(
    std::shared_ptr<lynx::runtime::profile::V8RuntimeProfilerWrapper>
        runtime_profiler_wrapper,
    bool is_main_thread) {
#if ENABLE_TRACE_PERFETTO
  if (runtime_profiler_wrapper == nullptr) {
    return;
  }
  if (v8_runtime_profiler != nullptr) {
    LOGI(
        "AddLynxTraceV8RuntimeProfiler failed, v8_runtime_profiler already "
        "exists");
    return;
  }
  v8_runtime_profiler =
      std::make_shared<lynx::runtime::profile::V8RuntimeProfiler>(
          runtime_profiler_wrapper, is_main_thread);
  v8_runtime_profiler->EnableSingleProfiler();
  lynx::runtime::profile::RuntimeProfilerManager::GetInstance()
      ->AddRuntimeProfiler(v8_runtime_profiler);
#endif
}

LYNX_EXPORT void RemoveSingleLynxTraceV8RuntimeProfiler() {
#if ENABLE_TRACE_PERFETTO
  lynx::runtime::profile::RuntimeProfilerManager::GetInstance()
      ->RemoveRuntimeProfiler(v8_runtime_profiler);
  v8_runtime_profiler = nullptr;
#endif
}
}  // namespace profile
}  // namespace runtime
}  // namespace lynx
