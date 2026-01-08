// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/js_debug/helper/js_debug_proxy_impl.h"

#include "devtool/js_inspect/inspector_const.h"

#if JS_ENGINE_TYPE == 0 || OS_ANDROID
#include "core/runtime/jsi/v8/v8_api.h"
#include "core/runtime/profile/v8/v8_runtime_profiler.h"
#include "devtool/lynx_devtool/js_debug/js/v8/manager/v8_inspector_manager_impl.h"
#endif

#include "core/runtime/jsi/quickjs/quickjs_api.h"
#include "devtool/lynx_devtool/js_debug/js/quickjs/manager/quickjs_inspector_manager_impl.h"
#include "devtool/lynx_devtool/js_debug/lepus/manager/lepus_inspector_manager_impl.h"

#if ENABLE_NAPI_BINDING && (JS_ENGINE_TYPE == 0 || OS_ANDROID)
#include "core/runtime/bindings/napi/napi_runtime_proxy_v8.h"

extern void RegisterV8RuntimeProxyFactory(
    lynx::piper::NapiRuntimeProxyV8Factory*);
#endif

namespace lynx {
namespace devtool {

std::unique_ptr<piper::RuntimeInspectorManager>
JSDebugProxyImpl::CreateRuntimeInspectorManager(const std::string& vm_type) {
  if (vm_type == kKeyEngineV8) {
#if JS_ENGINE_TYPE == 0 || OS_ANDROID
    return std::make_unique<piper::V8InspectorManagerImpl>();
#endif
  } else if (vm_type == kKeyEngineQuickjs) {
    return std::make_unique<piper::QuickjsInspectorManagerImpl>();
  }
  return nullptr;
}

std::unique_ptr<lepus::LepusInspectorManager>
JSDebugProxyImpl::CreateLepusInspectorManager() {
  return std::make_unique<lepus::LepusInspectorManagerImpl>();
}

void JSDebugProxyImpl::RegisterNapiRuntimeProxy() {
#if ENABLE_NAPI_BINDING && (JS_ENGINE_TYPE == 0 || OS_ANDROID)
  static piper::NapiRuntimeProxyV8FactoryImpl factory;
  LOGI("js debug: RegisterV8RuntimeProxyFactory: " << &factory);
  RegisterV8RuntimeProxyFactory(&factory);
#endif
}

std::shared_ptr<piper::Runtime> JSDebugProxyImpl::MakeRuntime(
    const std::string& vm_type) {
  if (vm_type == kKeyEngineV8) {
#if JS_ENGINE_TYPE == 0 || OS_ANDROID
    LOGI("js debug: make V8 runtime");
    return piper::makeV8Runtime();
#endif
  } else if (vm_type == kKeyEngineQuickjs) {
    LOGI("js debug: make Quickjs runtime");
    return piper::makeQuickJsRuntime();
  }
  LOGF("js debug: JSDebugProxyImpl::MakeRuntime fail! vm: " << vm_type);
  return nullptr;
}

#if ENABLE_TRACE_PERFETTO
std::shared_ptr<runtime::profile::RuntimeProfiler>
JSDebugProxyImpl::MakeRuntimeProfiler(
    std::shared_ptr<piper::JSIContext> js_context, const std::string& vm_type) {
  if (vm_type == kKeyEngineV8) {
#if JS_ENGINE_TYPE == 0 || OS_ANDROID
    LOGI("js debug: make V8 profiler");
    auto v8_profiler = piper::makeV8RuntimeProfiler(js_context);
    return std::make_shared<runtime::profile::V8RuntimeProfiler>(
        std::move(v8_profiler));
#endif
  } else if (vm_type == kKeyEngineQuickjs) {
    LOGI("js debug: make Quickjs profiler");
    return piper::makeQuickJsRuntimeProfiler(js_context);
  }
  LOGF("js debug: JSDebugProxyImpl::MakeRuntimeProfiler fail! vm: " << vm_type);
  return nullptr;
}
#endif

}  // namespace devtool
}  // namespace lynx
