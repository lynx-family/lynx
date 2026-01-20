// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RUNTIME_JS_JSI_V8_V8_API_H_
#define CORE_RUNTIME_JS_JSI_V8_V8_API_H_

#if OS_ANDROID
#include <jni.h>
#endif

#include <memory>
#include <mutex>

#include "core/runtime/js/jsi/jsi.h"
#include "core/runtime/profile/v8/v8_runtime_profiler_wrapper.h"

namespace lynx {
namespace runtime {
namespace js {
std::unique_ptr<Runtime> makeV8Runtime();

std::shared_ptr<lynx::runtime::profile::V8RuntimeProfilerWrapper>
makeV8RuntimeProfiler(std::shared_ptr<JSIContext> js_context);

}  // namespace js

}  // namespace runtime
}  // namespace lynx
#endif  // CORE_RUNTIME_JS_JSI_V8_V8_API_H_
