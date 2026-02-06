// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RUNTIME_PROFILE_V8_V8_RUNTIME_PROFILER_HELPER_H_
#define CORE_RUNTIME_PROFILE_V8_V8_RUNTIME_PROFILER_HELPER_H_

#include <memory>

namespace lynx {
namespace runtime {
namespace profile {

void AddSingleLynxTraceV8RuntimeProfiler(
    std::shared_ptr<lynx::runtime::profile::V8RuntimeProfilerWrapper>
        runtime_profiler_wrapper);

void RemoveSingleLynxTraceV8RuntimeProfiler();
}  // namespace profile
}  // namespace runtime
}  // namespace lynx
#endif  // CORE_RUNTIME_PROFILE_V8_V8_RUNTIME_PROFILER_HELPER_H_
