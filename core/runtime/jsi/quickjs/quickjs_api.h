// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RUNTIME_JSI_QUICKJS_QUICKJS_API_H_
#define CORE_RUNTIME_JSI_QUICKJS_QUICKJS_API_H_

#if defined(OS_ANDROID)
#include <jni.h>
#endif

#include <memory>

#include "base/include/base_export.h"
#include "core/runtime/profile/runtime_profiler.h"

namespace lynx {
namespace piper {

BASE_EXPORT_FOR_DEVTOOL std::unique_ptr<piper::Runtime> makeQuickJsRuntime();
std::shared_ptr<VMInstance> CreateQuickJsVM(const StartupData*, bool sync);
void BindQuickjsVMToCurrentThread(std::shared_ptr<piper::VMInstance>& vm);

BASE_EXPORT_FOR_DEVTOOL std::unique_ptr<profile::RuntimeProfiler>
makeQuickJsRuntimeProfiler(std::shared_ptr<piper::JSIContext> js_context);

}  // namespace piper
}  // namespace lynx
#endif  // CORE_RUNTIME_JSI_QUICKJS_QUICKJS_API_H_
