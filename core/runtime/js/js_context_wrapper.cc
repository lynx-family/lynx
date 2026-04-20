// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/runtime/js/js_context_wrapper.h"

#include "base/lynx_trace_categories.h"
#include "base/trace/native/trace_event.h"
#include "core/inspector/console_message_postman.h"
#include "core/runtime/common/napi/napi_environment.h"
#include "core/runtime/js/bindings/global.h"
#include "core/runtime/js/runtime_constant.h"
#include "core/runtime/js/utils.h"
#include "core/runtime/profile/runtime_profiler_manager.h"
#include "core/runtime/trace/runtime_trace_event_def.h"

namespace lynx {
namespace runtime {

JSContextWrapper::JSContextWrapper(
    std::shared_ptr<runtime::js::JSIContext> context)
    : js_context_(context),
      js_env_prepared_(false),
      js_core_loaded_(false),
      global_inited_(false) {}

void JSContextWrapper::EnsureCoreJSLoaded(
    runtime::js::Runtime& js_runtime,
    std::vector<std::pair<std::string, std::shared_ptr<runtime::js::Buffer>>>&
        js_preload) {
  if (js_core_loaded_) {
    return;
  }
  TRACE_EVENT(LYNX_TRACE_CATEGORY_VITALS,
              JS_CONTEXT_WRAPPER_ENSURE_CORE_JS_LOADED);
  if (runtime::js::EvaluatePreloadSources(js_runtime, js_preload)) {
    js_core_loaded_ = true;
  }
}

void JSContextWrapper::prepareJSEnv(
    std::weak_ptr<runtime::js::Runtime> js_runtime,
    std::vector<std::pair<std::string, std::shared_ptr<runtime::js::Buffer>>>&
        js_preload) {
  std::shared_ptr<runtime::js::Runtime> rt = js_runtime.lock();
  if (!rt) {
    return;
  }

  // This method may be invoked multiple times (e.g. multiple runtimes created
  // for the same shared context). We must guarantee the env preparation is
  // executed only once, while allowing a deferred corejs load to be completed
  // later without replaying other preload scripts.
  if (js_env_prepared_) {
    EnsureCoreJSLoaded(*rt, js_preload);
    return;
  }

  // Prepare the JS env once per context wrapper.
  TRACE_EVENT(LYNX_TRACE_CATEGORY_VITALS, JS_CONTEXT_WRAPPER_PREPARE_JS_ENV);
  bool has_core_js = runtime::js::EvaluatePreloadSources(*rt, js_preload);
  js_env_prepared_ = true;
  js_core_loaded_ = has_core_js;
  InitNapi(rt);
}

#if ENABLE_TRACE_PERFETTO
void JSContextWrapper::SetRuntimeProfiler(
    std::shared_ptr<profile::RuntimeProfiler> runtime_profiler) {
  runtime_profiler_ = runtime_profiler;
  profile::RuntimeProfilerManager::GetInstance()->AddRuntimeProfiler(
      runtime_profiler_);
}
#endif

//////////////////////
SharedJSContextWrapper::SharedJSContextWrapper(
    std::shared_ptr<runtime::js::JSIContext> context,
    const std::string& group_id, ReleaseListener* listener)
    : JSContextWrapper(context), group_id_(group_id), listener_(listener) {}

void SharedJSContextWrapper::Def() {
  // global has owner the js context, when only global own the js context, can
  // release now
  if (js_context_.use_count() == 2) {  // TODO : be trick, global has one, and
                                       // the Runtime call this has one...
    // TODO : release of global_ will trigger another Def() call
    if (global_ != nullptr) {
      global_.reset();
      if (listener_ != nullptr) {
        listener_->OnRelease(group_id_);
      }
    }
#if ENABLE_NAPI_BINDING
    if (napi_environment_) {
      LOGI("global napi detaching runtime");
      lifecycle_observer_->OnRuntimeDetach();
      napi_environment_->Detach();
      napi_environment_.reset();
    }
#endif
#if ENABLE_TRACE_PERFETTO
    profile::RuntimeProfilerManager::GetInstance()->RemoveRuntimeProfiler(
        runtime_profiler_);
    runtime_profiler_ = nullptr;
#endif
  }
}

void SharedJSContextWrapper::EnsureConsole(
    std::shared_ptr<runtime::js::ConsoleMessagePostMan> post_man,
    const tasm::PageOptions& page_options) {
  if (isGlobalInited() && global_) {
    global_->EnsureConsole(post_man, page_options);
  }
}

void SharedJSContextWrapper::initGlobal(
    std::shared_ptr<runtime::js::Runtime>& rt,
    std::shared_ptr<runtime::js::ConsoleMessagePostMan> post_man,
    const tasm::PageOptions& page_options) {
  if (global_inited_) {
    return;
  }
  std::shared_ptr<runtime::js::SharedContextGlobal> global =
      std::make_shared<runtime::js::SharedContextGlobal>();
  global->Init(rt, post_man, page_options);
  global_inited_ = true;
  global_ = global;
}

void SharedJSContextWrapper::InitNapi(
    std::shared_ptr<runtime::js::Runtime>& js_runtime) {
#if ENABLE_NAPI_BINDING
  TRACE_EVENT_BEGIN(LYNX_TRACE_CATEGORY_VITALS, PREPARE_NAPI_ENV);
  napi_environment_ = std::make_unique<runtime::js::NapiEnvironment>(
      std::make_unique<runtime::js::NapiEnvironment::Delegate>());
  auto proxy = runtime::js::NapiRuntimeProxy::Create(js_runtime, nullptr);
  proxy->MarkSafeNapi();
  LOGI("napi attaching with proxy: " << proxy.get());
  if (proxy) {
    napi_environment_->SetRuntimeProxy(std::move(proxy));
    napi_environment_->Attach();
  }
  lifecycle_observer_ = std::make_unique<RuntimeLifecycleObserverImpl>();
  lifecycle_observer_->OnRuntimeAttach(napi_environment_->proxy()->Env());
  TRACE_EVENT_END(LYNX_TRACE_CATEGORY_VITALS);
#endif
}

void SharedJSContextWrapper::AddLifecycleListener(
    std::unique_ptr<RuntimeLifecycleListenerDelegate> listener) {
#if ENABLE_NAPI_BINDING
  lifecycle_observer_->AddEventListener(std::move(listener));
#endif
}

NoneSharedJSContextWrapper::NoneSharedJSContextWrapper(
    std::shared_ptr<runtime::js::JSIContext> context)
    : JSContextWrapper(context) {}

NoneSharedJSContextWrapper::NoneSharedJSContextWrapper(
    std::shared_ptr<runtime::js::JSIContext> context,
    SharedJSContextWrapper::ReleaseListener* listener)
    : JSContextWrapper(context), listener_(listener) {}

void NoneSharedJSContextWrapper::Def() {
  if (js_context_.use_count() == 1) {
    global_.reset();
#if ENABLE_TRACE_PERFETTO
    profile::RuntimeProfilerManager::GetInstance()->RemoveRuntimeProfiler(
        runtime_profiler_);
    runtime_profiler_ = nullptr;
#endif
  }
}

void NoneSharedJSContextWrapper::EnsureConsole(
    std::shared_ptr<runtime::js::ConsoleMessagePostMan> post_man,
    const tasm::PageOptions& page_options) {
  if (isGlobalInited() && global_) {
    global_->EnsureConsole(post_man, page_options);
  }
}

void NoneSharedJSContextWrapper::initGlobal(
    std::shared_ptr<runtime::js::Runtime>& js_runtime,
    std::shared_ptr<runtime::js::ConsoleMessagePostMan> post_man,
    const tasm::PageOptions& page_options) {
  if (global_inited_) {
    return;
  }
  std::shared_ptr<runtime::js::SingleGlobal> global =
      std::make_shared<runtime::js::SingleGlobal>();
  global->Init(js_runtime, post_man, page_options);
  global_inited_ = true;
  global_ = global;
}

}  // namespace runtime
}  // namespace lynx
