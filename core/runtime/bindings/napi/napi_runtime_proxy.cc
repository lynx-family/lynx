// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/bindings/napi/napi_runtime_proxy.h"

#include "core/runtime/bindings/napi/napi_runtime_proxy_quickjs_factory.h"
#include "core/runtime/bindings/napi/napi_runtime_proxy_v8_factory.h"
#include "third_party/binding/napi/callback_helper.h"
#include "third_party/binding/napi/shim/shim_napi_env.h"
#include "third_party/binding/napi/shim/shim_napi_runtime.h"

#ifdef USE_PRIMJS_NAPI
#include "third_party/napi/include/primjs_napi_defines.h"
#endif

#include "core/runtime/bindings/napi/napi_runtime_proxy_quickjs.h"
#include "core/runtime/jsi/quickjs/quickjs_runtime.h"

#if defined(OS_IOS) || defined(OS_OSX)
#include "core/runtime/bindings/napi/napi_runtime_proxy_jsc.h"
#include "core/runtime/jsi/jsc/jsc_runtime.h"
#endif

BASE_EXPORT void RegisterV8RuntimeProxyFactory(
    lynx::piper::NapiRuntimeProxyV8Factory *factory) {
  lynx::piper::NapiRuntimeProxy::SetFactory(factory);
}

namespace lynx {
namespace piper {

// static
std::unique_ptr<NapiRuntimeProxy> NapiRuntimeProxy::Create(
    std::shared_ptr<Runtime> runtime, runtime::TemplateDelegate *delegate) {
  switch (runtime->type()) {
    case JSRuntimeType::v8: {
      LOGI("Creating napi proxy using v8 factory: " << s_factory);
      if (s_factory) {
        auto proxy_v8 = s_factory->Create(runtime, delegate);
        proxy_v8->SetJSRuntime(runtime);
        return proxy_v8;
      }
      return nullptr;
    }
    case JSRuntimeType::jsc: {
#if defined(OS_IOS) || defined(OS_OSX)
      LOGI("Creating napi proxy jsc");
      auto jsc_runtime = std::static_pointer_cast<JSCRuntime>(runtime);
      auto context = jsc_runtime->getSharedContext();
      auto jsc_context = std::static_pointer_cast<JSCContextWrapper>(context);
      auto proxy_jsc = NapiRuntimeProxyJSC::Create(jsc_context, delegate);
      proxy_jsc->SetJSRuntime(runtime);
      return proxy_jsc;
#else
      return nullptr;
#endif
    }
    case JSRuntimeType::quickjs: {
      LOGI("Creating napi proxy quickjs");
      auto qjs_runtime = std::static_pointer_cast<QuickjsRuntime>(runtime);
      auto context = qjs_runtime->getSharedContext();
      auto qjs_context =
          std::static_pointer_cast<QuickjsContextWrapper>(context);
      auto proxy_qjs =
          NapiRuntimeProxyQuickjs::Create(qjs_context->getContext(), delegate);
      proxy_qjs->SetJSRuntime(runtime);
      return proxy_qjs;
    }
  }
}
// the life cycle of shared_ptr to DelegateObserver is the same as
// NapiRuntimeProxy weak_ptr delegate_observer used to watch whether runtime is
// detached
void PostNAPIJSTask(napi_foreground_cb js_cb, void *data, void *task_ctx) {
  auto delegate_observer = std::weak_ptr<DelegateObserver>(
      *static_cast<std::shared_ptr<DelegateObserver> *>(task_ctx));
  delegate_observer.lock().get()->PostJSTask(
      [delegate_observer, js_cb, data]() {
        if (delegate_observer.lock()) {
          js_cb(data);
        }
      });
}

NapiRuntimeProxy::NapiRuntimeProxy(runtime::TemplateDelegate *delegate)
    : env_(napi_new_env()) {
  delegate_observer_ = std::make_shared<DelegateObserver>(delegate);
  napi_runtime_configuration runtime_conf = napi_create_runtime_configuration();
  napi_runtime_config_foreground_handler(runtime_conf, PostNAPIJSTask,
                                         &delegate_observer_);
  napi_attach_runtime_with_configuration(env_, runtime_conf);
  napi_delete_runtime_configuration(runtime_conf);
}

NapiRuntimeProxy::~NapiRuntimeProxy() { napi_free_env(env_); }

void NapiRuntimeProxy::Attach() {}

void NapiRuntimeProxy::Detach() { napi_detach_runtime(env_); }

void NapiRuntimeProxy::SetupLoader() {
  auto runtime = GetJSRuntime().lock();
  napi_env raw_env = env_;
  if (runtime && raw_env && raw_env->ctx) {
    Napi::ContextScope context_scope(env_);
    loader_ = "napiLoaderOnRT" + std::to_string(runtime->getRuntimeId());
    LOGI("NAPI Setup Loader: " << loader_);
    napi_setup_loader(env_, loader_.c_str());
    const char *kNapiMarker = "napiSharedMarker";
    Napi::HandleScope scope(env_);
    if (env_.Global().Has(kNapiMarker).FromMaybe(false)) {
      LOGW("NAPI used in shared context");
    } else {
      env_.Global().Set(kNapiMarker, Napi::Object::New(env_));
    }
  }
}

void NapiRuntimeProxy::RemoveLoader() {
  napi_env raw_env = env_;
  if (raw_env && raw_env->ctx && !loader_.empty()) {
    Napi::HandleScope handle_scope(env_);
    if (env_.Global().Has(loader_.c_str()).FromMaybe(false)) {
      LOGI("NAPI Remove Loader: " << loader_);
      env_.Global().Delete(loader_.c_str());
    }
  }
}

namespace {
void ReportError(Napi::Object exception) {
  LOGI("Report JS Error, exception: " << exception.ToString().Utf8Value());
  Napi::Value app_id = exception.Env().Global()["currentAppId"];

  uint32_t current_id = app_id.As<Napi::Number>().Uint32Value();
  Napi::Value apps = exception.Env().Global()["multiApps"];
  Napi::Object apps_obj = apps.As<Napi::Object>();
  Napi::Value app_proxy = apps_obj[current_id];

  // Return if app_proxy is null or undefined after card destroy
  if (app_proxy.IsNull() || app_proxy.IsUndefined()) {
    return;
  }
  Napi::Object app_proxy_obj = app_proxy.As<Napi::Object>();

  // Run JS ReportError USER_RUNTIME_ERROR
  Napi::Object lynx_obj;
  if (app_proxy_obj.Has("lynx").FromMaybe(false)) {
    Napi::Value lynx = app_proxy_obj["lynx"];
    lynx_obj = lynx.As<Napi::Object>();
  }

  if (lynx_obj.Has("reportError").FromMaybe(false)) {
    Napi::Value report_error = lynx_obj["reportError"];
    if (report_error.IsFunction()) {
      report_error.As<Napi::Function>().Call({exception});
    }
  }
}
}  // namespace

void NapiRuntimeProxy::SetUncaughtExceptionHandler() {
  binding::CallbackHelper::SetUncaughtExceptionHandler(env_, &ReportError);
}

NapiRuntimeProxyV8Factory *NapiRuntimeProxy::s_factory = nullptr;

// static
void NapiRuntimeProxy::SetFactory(NapiRuntimeProxyV8Factory *factory) {
  s_factory = factory;
}

}  // namespace piper
}  // namespace lynx
