// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/runtime/js/js_realm.h"

#include "base/include/fml/message_loop.h"
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

namespace {

// Copy only stateless corejs exports and host objects into page contexts.
constexpr const char* kNewShareGroupCoreJSExports[] = {
    // corejs functions statically installed on nativeGlobal (index.card.ts).
    "loadCard",
    "destroyCard",
    "callDestroyLifetimeFun",
    "loadDynamicComponent",
    "__createEventEmitter",
    "__lynxArrayBufferToBase64",
    "__lynxBase64ToArrayBuffer",
    "LynxSDKCore",
    // Web-ish polyfills installed on nativeGlobal (index.card.ts).
    "Headers",
    "AbortController",
    "AbortSignal",
    "URL",
    "URLSearchParams",
    // Lets the page bundle return an init factory.
    "bundleSupportLoadScript",
    // Host objects shared by reference within the same VM.
    "SystemInfo",
    "LynxJSBI",
    "TextCodecHelper",
};

}  // namespace

JSRealm::JSRealm(std::shared_ptr<js::JSIContext> context)
    : js_context_(context),
      js_env_prepared_(false),
      js_core_loaded_(false),
      global_inited_(false) {}

JSRealm::~JSRealm() {
#if ENABLE_TRACE_PERFETTO
  // Stop profiling while the context and global runtime are still alive.
  profile::RuntimeProfilerManager::GetInstance()->RemoveRuntimeProfiler(
      runtime_profiler_);
  runtime_profiler_.reset();
#endif
}

void JSRealm::InitGlobalObject(
    base::UnsafeOwningPtr<js::Runtime>& runtime,
    std::shared_ptr<js::ConsoleMessagePostMan> post_man,
    const tasm::PageOptions& page_options, bool install_shared_host_objects) {
  if (global_inited_) {
    return;
  }
  global_ = base::MakeUnsafeOwning<js::SingleGlobal>();
  global_->Init(runtime, post_man, page_options, install_shared_host_objects);
  global_inited_ = true;
}

void JSRealm::EnsureConsole(std::shared_ptr<js::ConsoleMessagePostMan> post_man,
                            const tasm::PageOptions& page_options) {
  if (global_) {
    global_->EnsureConsole(post_man, page_options);
  }
}

void JSRealm::EnsureCoreJSLoaded(
    js::Runtime& js_runtime,
    std::vector<std::pair<std::string, std::shared_ptr<js::Buffer>>>&
        js_preload) {
  if (js_core_loaded_) {
    return;
  }
  TRACE_EVENT(LYNX_TRACE_CATEGORY_VITALS, JS_REALM_ENSURE_CORE_JS_LOADED);
  if (js::EvaluatePreloadSources(js_runtime, js_preload)) {
    js_core_loaded_ = true;
  }
}

void JSRealm::PrepareJSEnv(
    base::UnsafeWeakPtr<js::Runtime> js_runtime,
    std::vector<std::pair<std::string, std::shared_ptr<js::Buffer>>>&
        js_preload) {
  auto* rt = js_runtime.Lock();
  if (rt == nullptr) {
    return;
  }

  // Allow deferred corejs loading without replaying other preload scripts.
  if (js_env_prepared_) {
    EnsureCoreJSLoaded(*rt, js_preload);
    return;
  }

  // Prepare the JS env once per context wrapper.
  TRACE_EVENT(LYNX_TRACE_CATEGORY_VITALS, JS_REALM_PREPARE_JS_ENV);
  bool has_core_js = js::EvaluatePreloadSources(*rt, js_preload);
  js_env_prepared_ = true;
  js_core_loaded_ = has_core_js;
  InitNapi(std::move(js_runtime));
}

#if ENABLE_TRACE_PERFETTO
void JSRealm::SetRuntimeProfiler(
    std::shared_ptr<profile::RuntimeProfiler> runtime_profiler) {
  runtime_profiler_ = runtime_profiler;
  profile::RuntimeProfilerManager::GetInstance()->AddRuntimeProfiler(
      runtime_profiler_);
}
#endif

//////////////////////
SharedJSRealm::SharedJSRealm(std::shared_ptr<js::JSIContext> context,
                             const std::string& group_id,
                             ReleaseListener* listener)
    : JSRealm(context), group_id_(group_id), listener_(listener) {}

void SharedJSRealm::Def() {
  // global has owner the js context, when only global own the js context, can
  // release now
  if (js_context_.use_count() == 2) {  // TODO : be trick, global has one, and
                                       // the Runtime call this has one...
    // TODO : release of global_ will trigger another Def() call
    if (global_ != nullptr) {
      global_.Reset();
      owned_global_runtime_.Reset();
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

void SharedJSRealm::InitGlobal(
    base::UnsafeOwningPtr<js::Runtime>& runtime,
    std::shared_ptr<js::ConsoleMessagePostMan> post_man,
    const tasm::PageOptions& page_options) {
  if (global_inited_) {
    return;
  }
  owned_global_runtime_ = std::move(runtime);
  InitGlobalObject(owned_global_runtime_, post_man, page_options, true);
}

void SharedJSRealm::InitNapi(base::UnsafeWeakPtr<js::Runtime> js_runtime) {
#if ENABLE_NAPI_BINDING
  TRACE_EVENT_BEGIN(LYNX_TRACE_CATEGORY_VITALS, PREPARE_NAPI_ENV);
  napi_environment_ = std::make_unique<js::NapiEnvironment>(
      std::make_unique<js::NapiEnvironment::Delegate>());
  auto* runtime = js_runtime.Lock();
  if (runtime == nullptr) {
    TRACE_EVENT_END(LYNX_TRACE_CATEGORY_VITALS);
    return;
  }
  auto delegate_observer = std::make_shared<js::DelegateObserver>(
      fml::MessageLoop::GetCurrent().GetTaskRunner());
  auto proxy =
      js::NapiRuntimeProxy::Create(*runtime, std::move(delegate_observer));
  if (proxy) {
    proxy->SetJSRuntime(std::move(js_runtime));
    proxy->MarkSafeNapi();
    LOGI("napi attaching with proxy: " << proxy.get());
    napi_environment_->SetRuntimeProxy(std::move(proxy));
    napi_environment_->Attach();
  }
  lifecycle_observer_ = std::make_unique<RuntimeLifecycleObserverImpl>();
  lifecycle_observer_->OnRuntimeAttach(
      napi_environment_->proxy()->Env(),
      js::JSRuntimeTypeToString(runtime->type()));
  TRACE_EVENT_END(LYNX_TRACE_CATEGORY_VITALS);
#endif
}

void SharedJSRealm::AddLifecycleListener(
    std::unique_ptr<RuntimeLifecycleListenerDelegate> listener) {
#if ENABLE_NAPI_BINDING
  lifecycle_observer_->AddEventListener(std::move(listener));
#endif
}

SingleJSRealm::SingleJSRealm(std::shared_ptr<js::JSIContext> context)
    : JSRealm(context) {}

void SingleJSRealm::InitGlobal(
    base::UnsafeOwningPtr<js::Runtime>& runtime,
    std::shared_ptr<js::ConsoleMessagePostMan> post_man,
    const tasm::PageOptions& page_options) {
  InitGlobalObject(runtime, post_man, page_options, true);
}

// -------- New "shared Isolate/VM + per-page isolated Context" scheme --------

SharedVMGlobalRealm::SharedVMGlobalRealm(
    std::shared_ptr<js::JSIContext> context, const std::string& group_id)
    : JSRealm(context), group_id_(group_id) {}

void SharedVMGlobalRealm::EnsureCoreJSLoaded(
    js::Runtime& js_runtime,
    std::vector<std::pair<std::string, std::shared_ptr<js::Buffer>>>&
        js_preload) {
  auto* global_runtime = GetGlobalRuntime();
  if (global_runtime == nullptr) {
    return;
  }
  JSRealm::EnsureCoreJSLoaded(*global_runtime, js_preload);
  CopyGlobalsTo(js_runtime);
}

void SharedVMGlobalRealm::CopyGlobalsTo(js::Runtime& page_runtime) {
  auto* global_runtime = GetGlobalRuntime();
  if (global_runtime == nullptr) {
    return;
  }
  js::Scope global_scope(*global_runtime);
  js::Object global_obj = global_runtime->global();
  js::Scope page_scope(page_runtime);
  js::Object page_obj = page_runtime.global();
  size_t copied = 0;
  for (const char* name : kNewShareGroupCoreJSExports) {
    auto value = global_obj.getProperty(*global_runtime, name);
    if (!value || value->isUndefined() || value->isNull()) {
      continue;
    }
    if (page_obj.setProperty(page_runtime, name, std::move(*value))) {
      ++copied;
    }
  }
  LOGI("new_share_group: copy globals group:"
       << group_id_ << " copied:" << copied << "/"
       << (sizeof(kNewShareGroupCoreJSExports) /
           sizeof(kNewShareGroupCoreJSExports[0])));
}

void SharedVMGlobalRealm::InitGlobal(
    base::UnsafeOwningPtr<js::Runtime>& runtime,
    std::shared_ptr<js::ConsoleMessagePostMan> post_man,
    const tasm::PageOptions& page_options) {
  if (global_inited_) {
    return;
  }
  owned_global_runtime_ = std::move(runtime);
  InitGlobalObject(owned_global_runtime_, post_man, page_options, true);
}

std::shared_ptr<js::VMInstance> SharedVMGlobalRealm::GetVM() {
  auto context = GetJSContext();
  return context ? context->getVM() : nullptr;
}

SharedVMPageRealm::SharedVMPageRealm(std::shared_ptr<js::JSIContext> context)
    : JSRealm(std::move(context)) {}

void SharedVMPageRealm::InitGlobal(
    base::UnsafeOwningPtr<js::Runtime>& runtime,
    std::shared_ptr<js::ConsoleMessagePostMan> post_man,
    const tasm::PageOptions& page_options) {
  InitGlobalObject(runtime, post_man, page_options, false);
}

}  // namespace runtime
}  // namespace lynx
