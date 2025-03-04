// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/js_debug/js/runtime_manager_delegate_impl.h"

#include "core/renderer/utils/lynx_env.h"
#include "core/runtime/piper/js/js_executor.h"
#include "devtool/lynx_devtool/js_debug/inspector_const_extend.h"
#if JS_ENGINE_TYPE == 0 || OS_ANDROID
#include "core/runtime/jsi/v8/v8_api.h"
#include "core/runtime/profile/v8/v8_runtime_profiler.h"
#endif
#include "core/runtime/jsi/quickjs/quickjs_api.h"

#if ENABLE_NAPI_BINDING
#include "core/runtime/bindings/napi/napi_runtime_proxy_v8.h"

extern void RegisterV8RuntimeProxyFactory(
    lynx::piper::NapiRuntimeProxyV8Factory*);
#endif

namespace lynx {
namespace devtool {

RuntimeManagerDelegateImpl::~RuntimeManagerDelegateImpl() {
  for (const auto& item : release_vm_callback_) {
    (item.second)();
  }
}

void RuntimeManagerDelegateImpl::BeforeRuntimeCreate(
    bool force_use_lightweight_js_engine) {
#if !ENABLE_UNITTESTS
#if ENABLE_NAPI_BINDING && (JS_ENGINE_TYPE == 0 || OS_ANDROID)
  static piper::NapiRuntimeProxyV8FactoryImpl factory;
  LOGI("js debug: RegisterV8RuntimeProxyFactory: " << &factory);
  RegisterV8RuntimeProxyFactory(&factory);
#endif
#endif
}

void RuntimeManagerDelegateImpl::OnRuntimeReady(
    piper::JSExecutor& executor,
    std::shared_ptr<piper::Runtime>& current_runtime,
    const std::string& group_id) {
  // `enable_bytecode` and `bytecode_source_url` parameters are ignored
  // since bytecode is not allowed to work with devtool.
  current_runtime->SetEnableUserBytecode(false);
  current_runtime->SetBytecodeSourceUrl("");
  current_runtime->InitInspector(executor.GetRuntimeObserver());
}

void RuntimeManagerDelegateImpl::AfterSharedContextCreate(
    const std::string& group_id, piper::JSRuntimeType type) {
  group_to_engine_type_.emplace(group_id, type);
}

void RuntimeManagerDelegateImpl::OnRelease(const std::string& group_id) {
  auto engine_it = group_to_engine_type_.find(group_id);
  if (engine_it != group_to_engine_type_.end()) {
    auto callback_it = release_context_callback_.find(engine_it->second);
    if (callback_it != release_context_callback_.end()) {
      (callback_it->second)(group_id);
    }
  }
}

std::shared_ptr<piper::Runtime> RuntimeManagerDelegateImpl::MakeRuntime(
    bool force_use_lightweight_js_engine) {
#if !ENABLE_UNITTESTS
  long v8_enable = tasm::LynxEnv::GetInstance().GetV8Enabled();
  switch (v8_enable) {
    case 0:
      LOGI("js debug: make Quickjs runtime");
      return piper::makeQuickJsRuntime();
      break;
    case 1:
#if JS_ENGINE_TYPE == 0 || OS_ANDROID
      LOGI("js debug: make V8 runtime");
      return piper::makeV8Runtime();
#endif
      break;
    case 2:
      if (force_use_lightweight_js_engine) {
        LOGI("js debug: make Quickjs runtime");
        return piper::makeQuickJsRuntime();
      } else {
#if JS_ENGINE_TYPE == 0 || OS_ANDROID
        LOGI("js debug: make V8 runtime");
        return piper::makeV8Runtime();
#endif
      }
      break;
    default:
      break;
  }
  LOGF("js debug: MakeRuntime fail! v8_enable: "
       << v8_enable << ", force_use_lightweight_js_engine: "
       << force_use_lightweight_js_engine);
#endif
  return nullptr;
}

#if ENABLE_TRACE_PERFETTO
std::shared_ptr<profile::RuntimeProfiler>
RuntimeManagerDelegateImpl::MakeRuntimeProfiler(
    std::shared_ptr<piper::JSIContext> js_context,
    bool force_use_lightweight_js_engine) {
  long v8_enable = tasm::LynxEnv::GetInstance().GetV8Enabled();
  switch (v8_enable) {
    case 0:
      LOGI("js debug: make Quickjs profiler");
      return piper::makeQuickJsRuntimeProfiler(js_context);
      break;
    case 1: {
#if JS_ENGINE_TYPE == 0 || OS_ANDROID
      LOGI("js debug: make V8 profiler");
      auto v8_profiler = piper::makeV8RuntimeProfiler(js_context);
      return std::make_shared<profile::V8RuntimeProfiler>(
          std::move(v8_profiler));
#endif
      break;
    }
    case 2:
      if (force_use_lightweight_js_engine) {
        LOGI("js debug: make Quickjs profiler");
        return piper::makeQuickJsRuntimeProfiler(js_context);
      } else {
#if JS_ENGINE_TYPE == 0 || OS_ANDROID
        auto v8_profiler = piper::makeV8RuntimeProfiler(js_context);
        return std::make_shared<profile::V8RuntimeProfiler>(
            std::move(v8_profiler));
#endif
      }
      break;
    default:
      break;
  }
  LOGF("js debug: MakeRuntimeProfiler fail! v8_enable: "
       << v8_enable << ", force_use_lightweight_js_engine: "
       << force_use_lightweight_js_engine);
  return nullptr;
}
#endif  // ENABLE_TRACE_PERFETTO

void RuntimeManagerDelegateImpl::SetReleaseContextCallback(
    piper::JSRuntimeType type, const ReleaseContextCallback& callback) {
  release_context_callback_[type] = callback;
}

void RuntimeManagerDelegateImpl::SetReleaseVMCallback(
    piper::JSRuntimeType type, const ReleaseVMCallback& callback) {
  release_vm_callback_[type] = callback;
}

}  // namespace devtool
}  // namespace lynx
