// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RUNTIME_PROFILE_V8_V8_RUNTIME_PROFILER_H_
#define CORE_RUNTIME_PROFILE_V8_V8_RUNTIME_PROFILER_H_

#if ENABLE_TRACE_PERFETTO
#include <memory>

#include "core/runtime/profile/runtime_profiler.h"
#include "core/runtime/profile/v8/v8_runtime_profiler_wrapper.h"

namespace lynx {
namespace profile {

class V8RuntimeProfiler : public RuntimeProfiler {
 public:
  explicit V8RuntimeProfiler(const std::shared_ptr<V8RuntimeProfilerWrapper>&);
  ~V8RuntimeProfiler() override;
  virtual void StartProfiling(bool is_create) override;
  virtual std::unique_ptr<RuntimeProfile> StopProfiling(
      bool is_destory) override;
  virtual void SetupProfiling(int32_t sampling_interval) override;
  virtual trace::RuntimeProfilerType GetType() override;

 private:
  std::shared_ptr<V8RuntimeProfilerWrapper> impl_;
};
}  // namespace profile
}  // namespace lynx

#endif
#endif  // CORE_RUNTIME_PROFILE_V8_V8_RUNTIME_PROFILER_H_
