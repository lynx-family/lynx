// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_PIPER_JS_LYNX_RUNTIME_HELPER_H_
#define CORE_RUNTIME_PIPER_JS_LYNX_RUNTIME_HELPER_H_

#include <memory>

#include "core/runtime/jsi/jsi.h"
#include "core/runtime/profile/v8/v8_runtime_profiler_wrapper.h"

namespace lynx {
namespace runtime {
class LynxRuntimeHelper {
 public:
  virtual std::unique_ptr<piper::Runtime> MakeRuntime() = 0;
  virtual std::shared_ptr<profile::V8RuntimeProfilerWrapper>
      MakeRuntimeProfiler(std::shared_ptr<piper::JSIContext>) = 0;
};
}  // namespace runtime
}  // namespace lynx

#endif  // CORE_RUNTIME_PIPER_JS_LYNX_RUNTIME_HELPER_H_
