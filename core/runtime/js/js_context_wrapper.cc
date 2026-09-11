// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/runtime/js/js_context_wrapper.h"

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

  // Prepare the JS env once per context wrapper.
  TRACE_EVENT(LYNX_TRACE_CATEGORY_VITALS, JS_CONTEXT_WRAPPER_PREPARE_JS_ENV);
  bool has_core_js = runtime::js::EvaluatePreloadSources(*rt, js_preload);
  js_env_prepared_ = true;
  js_core_loaded_ = has_core_js;
  InitNapi(std::move(js_runtime));
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
    base::UnsafeOwningPtr<runtime::js::Runtime>& rt,
    std::shared_ptr<runtime::js::ConsoleMessagePostMan> post_man,
    const tasm::PageOptions& page_options) {
  if (global_inited_) {
    return;
  }
  std::shared_ptr<runtime::js::SharedContextGlobal> global =
      std::make_shared<runtime::js::SharedContextGlobal>();
  global->Init(rt, post_man, page_options,
               /*install_shared_host_objects=*/true);
  global_inited_ = true;
  global_ = global;
}

void SharedJSContextWrapper::InitNapi(
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
    base::UnsafeOwningPtr<runtime::js::Runtime>& js_runtime,
    std::shared_ptr<runtime::js::ConsoleMessagePostMan> post_man,
    const tasm::PageOptions& page_options) {
  if (global_inited_) {
    return;
  }
  std::shared_ptr<runtime::js::SingleGlobal> global =
      std::make_shared<runtime::js::SingleGlobal>();
  global->Init(js_runtime, post_man, page_options,
               /*install_shared_host_objects=*/true);
  global_inited_ = true;
  global_ = global;
}

// -------- New "shared Isolate/VM + per-page isolated Context" scheme --------

NewShareGroupGlobalContextWrapper::NewShareGroupGlobalContextWrapper(
    std::shared_ptr<runtime::js::JSIContext> context,
    const std::string& group_id)
    : JSContextWrapper(context), group_id_(group_id) {}

NewShareGroupGlobalContextWrapper::~NewShareGroupGlobalContextWrapper() {
#if ENABLE_TRACE_PERFETTO
  profile::RuntimeProfilerManager::GetInstance()->RemoveRuntimeProfiler(
      runtime_profiler_);
  runtime_profiler_ = nullptr;
#endif
}

void NewShareGroupGlobalContextWrapper::EnsureCoreJSLoaded(
    runtime::js::Runtime& js_runtime,
    std::vector<std::pair<std::string, std::shared_ptr<runtime::js::Buffer>>>&
        js_preload) {
  auto* global_runtime = GetGlobalRuntime();
  if (global_runtime == nullptr) {
    return;
  }
  JSContextWrapper::EnsureCoreJSLoaded(*global_runtime, js_preload);
  CopyGlobalsTo(js_runtime);
}

void NewShareGroupGlobalContextWrapper::CopyGlobalsTo(
    runtime::js::Runtime& page_runtime) {
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

void NewShareGroupGlobalContextWrapper::EnsureConsole(
    std::shared_ptr<runtime::js::ConsoleMessagePostMan> post_man,
    const tasm::PageOptions& page_options) {
  if (isGlobalInited() && global_) {
    global_->EnsureConsole(post_man, page_options);
  }
}

void NewShareGroupGlobalContextWrapper::initGlobal(
    base::UnsafeOwningPtr<runtime::js::Runtime>& rt,
    std::shared_ptr<runtime::js::ConsoleMessagePostMan> post_man,
    const tasm::PageOptions& page_options) {
  if (global_inited_) {
    return;
  }
  // The global context installs the full set of shared host objects so page
  // contexts can copy them by reference. It uses a SingleGlobal (weak observer)
  // because ownership of the runtime is held by this wrapper below, not by the
  // Global.
  auto global = base::MakeUnsafeOwning<runtime::js::SingleGlobal>();
  global->Init(rt, post_man, page_options,
               /*install_shared_host_objects=*/true);
  // Keep a strong owning reference to the global runtime so the shared VM and
  // global context outlive every page in the group. SingleGlobal only keeps a
  // weak observer, so moving `rt` here does not disturb it.
  owned_global_runtime_ = std::move(rt);
  global_inited_ = true;
  global_ = std::move(global);
}

std::shared_ptr<runtime::js::VMInstance>
NewShareGroupGlobalContextWrapper::GetVM() {
  auto context = getJSContext();
  return context ? context->getVM() : nullptr;
}

NewShareGroupPageContextWrapper::NewShareGroupPageContextWrapper(
    std::shared_ptr<runtime::js::JSIContext> context,
    const std::string& group_id,
    SharedJSContextWrapper::ReleaseListener* listener)
    : JSContextWrapper(context), group_id_(group_id), listener_(listener) {}

void NewShareGroupPageContextWrapper::Def() {
  if (js_context_.use_count() == 1) {
    global_.Reset();
#if ENABLE_TRACE_PERFETTO
    profile::RuntimeProfilerManager::GetInstance()->RemoveRuntimeProfiler(
        runtime_profiler_);
    runtime_profiler_ = nullptr;
#endif
    // A page context is 1:1 with its page runtime; releasing it means this page
    // is gone. Notify RuntimeManager so the group's live page count can be
    // decremented and the global context released after the last page.
    if (listener_ != nullptr) {
      listener_->OnRelease(group_id_);
    }
  }
}

void NewShareGroupPageContextWrapper::EnsureConsole(
    std::shared_ptr<runtime::js::ConsoleMessagePostMan> post_man,
    const tasm::PageOptions& page_options) {
  if (isGlobalInited() && global_) {
    global_->EnsureConsole(post_man, page_options);
  }
}

void NewShareGroupPageContextWrapper::initGlobal(
    base::UnsafeOwningPtr<runtime::js::Runtime>& js_runtime,
    std::shared_ptr<runtime::js::ConsoleMessagePostMan> post_man,
    const tasm::PageOptions& page_options) {
  if (global_inited_) {
    return;
  }
  auto global = base::MakeUnsafeOwning<runtime::js::SingleGlobal>();
  // Skip the stateless shared host objects (SystemInfo / LynxJSBI /
  // TextCodecHelper); they are copied by reference from the group's global
  // context instead of being re-created per page.
  global->Init(js_runtime, post_man, page_options,
               /*install_shared_host_objects=*/false);
  global_inited_ = true;
  global_ = std::move(global);
}

}  // namespace runtime
}  // namespace lynx
