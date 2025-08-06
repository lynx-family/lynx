// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_JSI_JSVM_JSVM_API_H_
#define CORE_RUNTIME_JSI_JSVM_JSVM_API_H_
#include <memory>

#include "base/include/base_export.h"
#include "core/runtime/profile/runtime_profiler.h"

namespace lynx {
namespace piper {
BASE_EXPORT std::shared_ptr<piper::Runtime> makeJSVMRuntime();

BASE_EXPORT std::shared_ptr<profile::RuntimeProfiler> makeJSVMRuntimeProfiler(
    std::shared_ptr<piper::JSIContext> js_context);

BASE_EXPORT bool IsJSVMRuntimeAvailable();
}  // namespace piper
}  // namespace lynx
#endif  // CORE_RUNTIME_JSI_JSVM_JSVM_API_H_
