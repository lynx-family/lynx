// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/runtime/js/js_context_wrapper.h"

#include "base/lynx_trace_categories.h"
#include "base/trace/native/trace_event.h"
#include "core/inspector/console_message_postman.h"
#include "core/runtime/common/napi/napi_environment.h"
#include "core/runtime/js/bindings/global.h"
#include "core/runtime/profile/runtime_profiler_manager.h"
#include "core/runtime/trace/runtime_trace_event_def.h"

namespace lynx {
namespace runtime {

JSContextWrapper::JSContextWrapper(std::shared_ptr<piper::JSIContext> context)
    : js_context_(context), js_core_loaded_(false), global_inited_(false) {}

void JSContextWrapper::prepareJSEnv(
    std::weak_ptr<piper::Runtime> js_runtime,
    std::vector<std::pair<std::string, std::shared_ptr<piper::Buffer>>>&
        js_preload) {
  if (js_core_loaded_) {
    return;
  }

  std::shared_ptr<piper::Runtime> rt = js_runtime.lock();
  if (!rt) {
    return;
  }

  // load the lynx_core.js
  piper::Scope scope(*rt);
  for (auto& [url, buffer] : js_preload) {
    auto prep = rt->prepareJavaScript(buffer, url);
    auto ret = rt->evaluatePreparedJavaScript(prep);
    if (!ret.has_value()) {
      rt->reportJSIException(ret.error());
    }
  }
  js_core_loaded_ = true;
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
    std::shared_ptr<piper::JSIContext> context, const std::string& group_id,
    ReleaseListener* listener)
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
    std::shared_ptr<piper::ConsoleMessagePostMan> post_man,
    const tasm::PageOptions& page_options) {
  if (isGlobalInited() && global_) {
    global_->EnsureConsole(post_man, page_options);
  }
}

void SharedJSContextWrapper::initGlobal(
    std::shared_ptr<piper::Runtime>& rt,
    std::shared_ptr<piper::ConsoleMessagePostMan> post_man,
    const tasm::PageOptions& page_options) {
  if (global_inited_) {
    return;
  }
  std::shared_ptr<piper::SharedContextGlobal> global =
      std::make_shared<piper::SharedContextGlobal>();
  global->Init(rt, post_man, page_options);
  global_inited_ = true;
  global_ = global;
}

void SharedJSContextWrapper::InitNapi(
    std::shared_ptr<piper::Runtime>& js_runtime) {
#if ENABLE_NAPI_BINDING
  TRACE_EVENT_BEGIN(LYNX_TRACE_CATEGORY_VITALS, PREPARE_NAPI_ENV);
  napi_environment_ = std::make_unique<piper::NapiEnvironment>(
      std::make_unique<piper::NapiEnvironment::Delegate>());
  auto proxy = piper::NapiRuntimeProxy::Create(js_runtime, nullptr);
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
    std::shared_ptr<piper::JSIContext> context)
    : JSContextWrapper(context) {}

NoneSharedJSContextWrapper::NoneSharedJSContextWrapper(
    std::shared_ptr<piper::JSIContext> context,
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
    std::shared_ptr<piper::ConsoleMessagePostMan> post_man,
    const tasm::PageOptions& page_options) {
  if (isGlobalInited() && global_) {
    global_->EnsureConsole(post_man, page_options);
  }
}

void NoneSharedJSContextWrapper::initGlobal(
    std::shared_ptr<piper::Runtime>& js_runtime,
    std::shared_ptr<piper::ConsoleMessagePostMan> post_man,
    const tasm::PageOptions& page_options) {
  if (global_inited_) {
    return;
  }
  std::shared_ptr<piper::SingleGlobal> global =
      std::make_shared<piper::SingleGlobal>();
  global->Init(js_runtime, post_man, page_options);
  global_inited_ = true;
  global_ = global;
}

}  // namespace runtime
}  // namespace lynx
