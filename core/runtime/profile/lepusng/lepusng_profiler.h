// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RUNTIME_PROFILE_LEPUSNG_LEPUSNG_PROFILER_H_
#define CORE_RUNTIME_PROFILE_LEPUSNG_LEPUSNG_PROFILER_H_

#if ENABLE_TRACE_PERFETTO
#include <memory>

#include "core/runtime/profile/runtime_profiler.h"
#include "core/runtime/vm/lepus/context.h"

namespace lynx {
namespace profile {

class LepusNGProfiler : public RuntimeProfiler {
 public:
  explicit LepusNGProfiler(std::shared_ptr<lepus::Context> context);
  ~LepusNGProfiler() override;

  virtual void StartProfiling(bool is_create) override;
  virtual std::unique_ptr<RuntimeProfile> StopProfiling(
      bool is_destory) override;
  virtual void SetupProfiling(int32_t sampling_interval) override;
  virtual trace::RuntimeProfilerType GetType() override;

 private:
  std::weak_ptr<lepus::Context> weak_context_;
};
}  // namespace profile
}  // namespace lynx
#endif
#endif  // CORE_RUNTIME_PROFILE_LEPUSNG_LEPUSNG_PROFILER_H_
