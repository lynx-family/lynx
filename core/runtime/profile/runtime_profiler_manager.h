// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RUNTIME_PROFILE_RUNTIME_PROFILER_MANAGER_H_
#define CORE_RUNTIME_PROFILE_RUNTIME_PROFILER_MANAGER_H_

#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "base/include/base_export.h"
#include "base/trace/native/trace_controller.h"
#include "core/runtime/profile/runtime_profiler.h"

namespace lynx {
namespace profile {
// export for devtool
BASE_EXPORT lynx::trace::TracePlugin* GetRuntimeProfilerManager();

class BASE_EXPORT RuntimeProfilerManager : public lynx::trace::TracePlugin {
 public:
  static RuntimeProfilerManager* GetInstance();
  RuntimeProfilerManager() = default;
  ~RuntimeProfilerManager() = default;
  /**
   *  RuntimeProfilerManager has 3 steps:
   *    1. Setup: DispatchSetup will be called when trace setups TraceConfig
   *    2. Start: DispatchBegin will be called when trace starts record
   *    3. End: DispatchEnd will be called when trace finishes record
   */
  virtual void DispatchBegin() override;
  virtual void DispatchEnd() override;
  virtual void DispatchSetup(
      const std::shared_ptr<lynx::trace::TraceConfig>& config) override;
  virtual std::string Name() override;
#if ENABLE_TRACE_PERFETTO
  void AddRuntimeProfiler(std::shared_ptr<RuntimeProfiler> runtime_profiler);
  void RemoveRuntimeProfiler(std::shared_ptr<RuntimeProfiler> runtime_profiler);
#endif

 protected:
#if ENABLE_TRACE_PERFETTO
  void SaveRuntimeProfile(
      const std::shared_ptr<RuntimeProfile>& runtime_profile,
      const int32_t index);
  std::vector<std::shared_ptr<RuntimeProfiler>> runtime_profilers_;
  std::mutex lock_;
  std::vector<std::shared_ptr<RuntimeProfile>> profiles_;
  bool is_started_{false};
  int32_t js_profile_interval_{100};
  trace::RuntimeProfilerType js_profiler_type_{
      trace::RuntimeProfilerType::quickjs};
#endif
};

}  // namespace profile
}  // namespace lynx

#endif  // CORE_RUNTIME_PROFILE_RUNTIME_PROFILER_MANAGER_H_
