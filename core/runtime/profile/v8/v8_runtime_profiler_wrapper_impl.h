// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RUNTIME_PROFILE_V8_V8_RUNTIME_PROFILER_WRAPPER_IMPL_H_
#define CORE_RUNTIME_PROFILE_V8_V8_RUNTIME_PROFILER_WRAPPER_IMPL_H_

#include <memory>
#include <vector>

#include "core/runtime/jsi/v8/v8_isolate_wrapper.h"
#include "core/runtime/profile/v8/v8_runtime_profiler_wrapper.h"
#include "v8-profiler.h"

namespace lynx {
namespace profile {

class V8RuntimeProfilerWrapperImpl : public V8RuntimeProfilerWrapper {
 public:
  static std::shared_ptr<V8RuntimeProfilerWrapperImpl> GetInstance();

  V8RuntimeProfilerWrapperImpl();
  ~V8RuntimeProfilerWrapperImpl();
  void Initialize(std::shared_ptr<piper::V8IsolateInstance> vm);
  virtual void StartProfiling() override;
  virtual std::unique_ptr<V8CpuProfile> StopProfiling() override;
  virtual void SetupProfiling(int32_t sampling_interval) override;

 private:
  void FlattenProfileNodes(std::vector<V8CpuProfileNode>&,
                           const v8::CpuProfileNode* profile_node);
  std::weak_ptr<piper::V8IsolateInstance> v8_isolate_;
  v8::Local<v8::String> title_;
  v8::CpuProfiler* cpu_profiler_;
  uint32_t profiling_count_;
  bool is_inited_;
};

}  // namespace profile
}  // namespace lynx

#endif  // CORE_RUNTIME_PROFILE_V8_V8_RUNTIME_PROFILER_WRAPPER_IMPL_H_
