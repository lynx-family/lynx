// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_JS_JSI_JSVM_JSVM_API_H_
#define CORE_RUNTIME_JS_JSI_JSVM_JSVM_API_H_
#include <memory>

#include "core/base/lynx_export.h"
#include "core/runtime/profile/runtime_profiler.h"

namespace lynx {
namespace runtime {
namespace js {
LYNX_EXPORT std::shared_ptr<Runtime> makeJSVMRuntime();

LYNX_EXPORT std::shared_ptr<lynx::runtime::profile::RuntimeProfiler>
makeJSVMRuntimeProfiler(std::shared_ptr<JSIContext> js_context);

LYNX_EXPORT bool IsJSVMRuntimeAvailable();
}  // namespace js
}  // namespace runtime
}  // namespace lynx
#endif  // CORE_RUNTIME_JS_JSI_JSVM_JSVM_API_H_
