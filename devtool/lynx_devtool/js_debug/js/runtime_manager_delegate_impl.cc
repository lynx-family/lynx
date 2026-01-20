// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/js_debug/js/runtime_manager_delegate_impl.h"

#include "core/renderer/utils/lynx_env.h"
#include "core/runtime/js/js_executor.h"
#include "devtool/lynx_devtool/js_debug/inspector_const_extend.h"
#if JS_ENGINE_TYPE == 1
#include "core/runtime/js/jsi/jsc/jsc_api.h"
#endif
#include "devtool/lynx_devtool/js_debug/helper/js_debug_helper.h"

namespace lynx {
namespace devtool {

RuntimeManagerDelegateImpl::~RuntimeManagerDelegateImpl() {
  for (const auto& item : release_vm_callback_) {
    (item.second)();
  }
}

void RuntimeManagerDelegateImpl::BeforeRuntimeCreate(
    bool force_use_lightweight_js_engine) {
  JSDebugHelper::GetInstance()->RegisterNapiRuntimeProxy();
}

void RuntimeManagerDelegateImpl::OnRuntimeReady(
    runtime::js::JSExecutor& executor,
    std::shared_ptr<runtime::js::Runtime>& current_runtime,
    const std::string& group_id) {
  // `enable_bytecode` and `bytecode_source_url` parameters are ignored
  // since bytecode is not allowed to work with devtool.
  current_runtime->SetEnableUserBytecode(false);
  current_runtime->SetBytecodeSourceUrl("");
  current_runtime->InitInspector(executor.GetRuntimeObserver());
}

void RuntimeManagerDelegateImpl::AfterSharedContextCreate(
    const std::string& group_id, runtime::js::JSRuntimeType type) {
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

std::shared_ptr<runtime::js::Runtime> RuntimeManagerDelegateImpl::MakeRuntime(
    bool force_use_lightweight_js_engine, bool use_shared_context,
    const tasm::PageOptions& page_options) {
  // When using a shared js context, create a runtime of the same type as the
  // context.
  if (use_shared_context) {
    return MakeRuntimeForSharedContext(force_use_lightweight_js_engine);
  }
  long v8_enable =
      tasm::LynxEnv::GetInstance().GetV8Enabled(page_options.GetDebuggable());
  switch (v8_enable) {
    case 0:
      return JSDebugHelper::GetInstance()->MakeRuntime(kKeyEngineQuickjs);
    case 1:
      return JSDebugHelper::GetInstance()->MakeRuntime(kKeyEngineV8);
    case 2:
      if (force_use_lightweight_js_engine) {
        return JSDebugHelper::GetInstance()->MakeRuntime(kKeyEngineQuickjs);
      } else {
        return JSDebugHelper::GetInstance()->MakeRuntime(kKeyEngineV8);
      }
    default:
      break;
  }
  LOGF("js debug: MakeRuntime fail! v8_enable: "
       << v8_enable << ", force_use_lightweight_js_engine: "
       << force_use_lightweight_js_engine);
  return nullptr;
}

std::shared_ptr<runtime::js::Runtime>
RuntimeManagerDelegateImpl::MakeRuntimeForSharedContext(
    bool force_use_lightweight_js_engine) {
  LOGI("js debug: create runtime for shared js context!")
  if (force_use_lightweight_js_engine) {
    return JSDebugHelper::GetInstance()->MakeRuntime(kKeyEngineQuickjs);
  } else {
#if JS_ENGINE_TYPE == 1
    LOGI("js debug: make JSC runtime");
    return runtime::js::makeJSCRuntime();
#else
    return JSDebugHelper::GetInstance()->MakeRuntime(kKeyEngineV8);
#endif
  }
  LOGF(
      "js debug: failed to create runtime for shared js context! "
      "force_use_lightweight_js_engine: "
      << force_use_lightweight_js_engine);
  return nullptr;
}

#if ENABLE_TRACE_PERFETTO
std::shared_ptr<runtime::profile::RuntimeProfiler>
RuntimeManagerDelegateImpl::MakeRuntimeProfiler(
    std::shared_ptr<runtime::js::JSIContext> js_context,
    bool force_use_lightweight_js_engine,
    const tasm::PageOptions& page_options) {
  long v8_enable =
      tasm::LynxEnv::GetInstance().GetV8Enabled(page_options.GetDebuggable());
  switch (v8_enable) {
    case 0:
      return JSDebugHelper::GetInstance()->MakeRuntimeProfiler(
          js_context, kKeyEngineQuickjs);
    case 1: {
      return JSDebugHelper::GetInstance()->MakeRuntimeProfiler(js_context,
                                                               kKeyEngineV8);
    }
    case 2:
      if (force_use_lightweight_js_engine) {
        return JSDebugHelper::GetInstance()->MakeRuntimeProfiler(
            js_context, kKeyEngineQuickjs);
      } else {
        return JSDebugHelper::GetInstance()->MakeRuntimeProfiler(js_context,
                                                                 kKeyEngineV8);
      }
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
    runtime::js::JSRuntimeType type, const ReleaseContextCallback& callback) {
  release_context_callback_[type] = callback;
}

void RuntimeManagerDelegateImpl::SetReleaseVMCallback(
    runtime::js::JSRuntimeType type, const ReleaseVMCallback& callback) {
  release_vm_callback_[type] = callback;
}

}  // namespace devtool
}  // namespace lynx
