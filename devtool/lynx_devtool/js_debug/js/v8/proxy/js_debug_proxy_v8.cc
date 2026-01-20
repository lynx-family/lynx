// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/js_debug/js/v8/proxy/js_debug_proxy_v8.h"

#include "core/runtime/js/jsi/v8/v8_api.h"
#include "core/runtime/profile/v8/v8_runtime_profiler.h"
#include "devtool/lynx_devtool/js_debug/js/v8/manager/v8_inspector_manager_impl.h"

#if ENABLE_NAPI_BINDING
#include "core/runtime/common/napi/napi_runtime_proxy_v8.h"

extern void RegisterV8RuntimeProxyFactory(
    lynx::runtime::js::NapiRuntimeProxyV8Factory*);
#endif

namespace lynx {
namespace devtool {

std::unique_ptr<runtime::js::RuntimeInspectorManager>
JSDebugProxyV8::CreateRuntimeInspectorManager() {
  return std::make_unique<runtime::js::V8InspectorManagerImpl>();
}

void JSDebugProxyV8::RegisterNapiRuntimeProxy() {
#if ENABLE_NAPI_BINDING
  static runtime::js::NapiRuntimeProxyV8FactoryImpl factory;
  LOGI("js debug: RegisterV8RuntimeProxyFactory: " << &factory);
  RegisterV8RuntimeProxyFactory(&factory);
#endif
}

std::shared_ptr<runtime::js::Runtime> JSDebugProxyV8::MakeRuntime() {
  LOGI("js debug: make V8 runtime");
  return runtime::js::makeV8Runtime();
}

#if ENABLE_TRACE_PERFETTO
std::shared_ptr<runtime::profile::RuntimeProfiler>
JSDebugProxyV8::MakeRuntimeProfiler(
    std::shared_ptr<runtime::js::JSIContext> js_context) {
  LOGI("js debug: make V8 profiler");
  auto v8_profiler = runtime::js::makeV8RuntimeProfiler(js_context);
  return std::make_shared<runtime::profile::V8RuntimeProfiler>(
      std::move(v8_profiler));
}
#endif

}  // namespace devtool
}  // namespace lynx
