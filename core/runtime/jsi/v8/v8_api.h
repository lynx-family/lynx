// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RUNTIME_JSI_V8_V8_API_H_
#define CORE_RUNTIME_JSI_V8_V8_API_H_

#if OS_ANDROID
#include <jni.h>
#endif

#include <memory>
#include <mutex>

#include "core/runtime/jsi/jsi.h"
#include "core/runtime/profile/v8/v8_runtime_profiler_wrapper.h"

namespace lynx {
namespace piper {

std::unique_ptr<piper::Runtime> makeV8Runtime();

std::shared_ptr<profile::V8RuntimeProfilerWrapper> makeV8RuntimeProfiler(
    std::shared_ptr<piper::JSIContext> js_context);

}  // namespace piper
}  // namespace lynx
#endif  // CORE_RUNTIME_JSI_V8_V8_API_H_
