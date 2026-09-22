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

// The corejs-exported globals that a page context needs. lynx_core.js is only
// executed on the group's global context; each page context copies references
// to these entries instead of re-running corejs. Keep this list curated to the
// values corejs statically assigns onto `nativeGlobal`/globalThis (see
// lynx-core index.card.ts / nativeGlobal.ts) plus the native-installed shared
// host objects. Page-level stateful objects reached through the per-page `app`
// object (publishEvent / callFunction / onAppReload ...) are intentionally
// excluded: they live on the app object created by loadCard, not on globalThis.
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
    // Flag corejs installs on globalThis (nativeGlobal.ts); the app-service.js
    // bundle wrapper reads it from its own realm's globalThis to decide whether
    // to return an `init` factory.
    "bundleSupportLoadScript",
    // Stateless shared host objects. The page context skips creating them
    // (Global::Init install_shared_host_objects=false) and instead references
    // the single instances installed on the group's global context. All page
    // runtimes share the same VM/Isolate, so referencing the same JS values
    // across contexts is safe.
    "SystemInfo",
    "LynxJSBI",
    "TextCodecHelper",
};

}  // namespace

JSRealm::JSRealm(std::shared_ptr<js::JSIContext> context)
    : js_context_(std::move(context)) {}

JSRealm::~JSRealm() {
#if ENABLE_TRACE_PERFETTO
  // Stop profiling while the context and global runtime are still alive.
  profile::RuntimeProfilerManager::GetInstance()->RemoveRuntimeProfiler(
      runtime_profiler_);
  runtime_profiler_.reset();
#endif
}

void JSRealm::EnsureConsole(std::shared_ptr<js::ConsoleMessagePostMan> post_man,
                            const tasm::PageOptions& page_options) {
  if (global_) {
    global_->EnsureConsole(post_man, page_options);
  }
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

void JSRealm::EnsureCoreJSLoaded(
    runtime::js::Runtime& js_runtime,
    std::vector<std::pair<std::string, std::shared_ptr<runtime::js::Buffer>>>&
        js_preload) {
  if (js_core_loaded_) {
    return;
  }
  TRACE_EVENT(LYNX_TRACE_CATEGORY_VITALS, JS_REALM_ENSURE_CORE_JS_LOADED);
  if (runtime::js::EvaluatePreloadSources(js_runtime, js_preload)) {
    js_core_loaded_ = true;
  }
}

void JSRealm::PrepareJSEnv(
    base::UnsafeWeakPtr<runtime::js::Runtime> js_runtime,
    std::vector<std::pair<std::string, std::shared_ptr<runtime::js::Buffer>>>&
        js_preload) {
  auto* rt = js_runtime.Lock();
  if (rt == nullptr) {
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

  // Prepare the JS env once per realm.
  TRACE_EVENT(LYNX_TRACE_CATEGORY_VITALS, JS_REALM_PREPARE_JS_ENV);
  bool has_core_js = runtime::js::EvaluatePreloadSources(*rt, js_preload);
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

SharedJSRealm::SharedJSRealm(std::shared_ptr<js::JSIContext> context)
    : JSRealm(std::move(context)) {}

SharedJSRealm::~SharedJSRealm() {
#if ENABLE_NAPI_BINDING
  if (napi_environment_) {
    LOGI("global napi detaching runtime");
    if (lifecycle_observer_) {
      lifecycle_observer_->OnRuntimeDetach();
    }
    napi_environment_->Detach();
    napi_environment_.reset();
  }
#endif
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

void SharedJSRealm::InitNapi(
    base::UnsafeWeakPtr<runtime::js::Runtime> js_runtime) {
#if ENABLE_NAPI_BINDING
  TRACE_EVENT_BEGIN(LYNX_TRACE_CATEGORY_VITALS, PREPARE_NAPI_ENV);
  napi_environment_ = std::make_unique<runtime::js::NapiEnvironment>(
      std::make_unique<runtime::js::NapiEnvironment::Delegate>());
  auto* runtime = js_runtime.Lock();
  if (runtime == nullptr) {
    TRACE_EVENT_END(LYNX_TRACE_CATEGORY_VITALS);
    return;
  }
  auto delegate_observer = std::make_shared<runtime::js::DelegateObserver>(
      fml::MessageLoop::GetCurrent().GetTaskRunner());
  auto proxy = runtime::js::NapiRuntimeProxy::Create(
      *runtime, std::move(delegate_observer));
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
      runtime::js::JSRuntimeTypeToString(runtime->type()));
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
    : JSRealm(std::move(context)) {}

void SingleJSRealm::InitGlobal(
    base::UnsafeOwningPtr<js::Runtime>& runtime,
    std::shared_ptr<js::ConsoleMessagePostMan> post_man,
    const tasm::PageOptions& page_options) {
  InitGlobalObject(runtime, post_man, page_options, true);
}

SharedVMGlobalRealm::SharedVMGlobalRealm(
    std::shared_ptr<js::JSIContext> context, const std::string& group_id)
    : JSRealm(std::move(context)), group_id_(group_id) {}

void SharedVMGlobalRealm::EnsureCoreJSLoaded(
    runtime::js::Runtime& js_runtime,
    std::vector<std::pair<std::string, std::shared_ptr<runtime::js::Buffer>>>&
        js_preload) {
  auto* global_runtime = GetGlobalRuntime();
  if (global_runtime == nullptr) {
    return;
  }
  JSRealm::EnsureCoreJSLoaded(*global_runtime, js_preload);
  CopyGlobalsTo(js_runtime);
}

void SharedVMGlobalRealm::CopyGlobalsTo(runtime::js::Runtime& page_runtime) {
  auto* global_runtime = GetGlobalRuntime();
  if (global_runtime == nullptr) {
    return;
  }
  runtime::js::Scope global_scope(*global_runtime);
  runtime::js::Object global_obj = global_runtime->global();
  runtime::js::Scope page_scope(page_runtime);
  runtime::js::Object page_obj = page_runtime.global();
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

std::shared_ptr<runtime::js::VMInstance> SharedVMGlobalRealm::GetVM() {
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
