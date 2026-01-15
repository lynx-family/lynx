// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/common/napi/napi_runtime_proxy.h"

#include <unordered_map>

#include "core/runtime/common/napi/napi_runtime_proxy_jsvm_factory.h"
#include "core/runtime/common/napi/napi_runtime_proxy_quickjs_factory.h"
#include "core/runtime/common/napi/napi_runtime_proxy_v8_factory.h"
#include "third_party/binding/napi/callback_helper.h"
#include "third_party/binding/napi/shim/shim_napi_env.h"
#include "third_party/binding/napi/shim/shim_napi_runtime.h"

#ifdef USE_PRIMJS_NAPI
#include "third_party/napi/include/primjs_napi_defines.h"
#endif

#include "core/runtime/common/napi/napi_runtime_proxy_quickjs.h"
#include "core/runtime/js/jsi/quickjs/quickjs_runtime.h"

#if defined(OS_IOS) || defined(OS_OSX)
#include "core/runtime/common/napi/napi_runtime_proxy_jsc.h"
#include "core/runtime/js/jsi/jsc/jsc_runtime.h"
#endif

LYNX_EXPORT void RegisterV8RuntimeProxyFactory(
    lynx::piper::NapiRuntimeProxyV8Factory *factory) {
  lynx::piper::NapiRuntimeProxy::SetFactory(factory);
}

// TODO(yangguangzhao.solace): remove this when jsvm refact finished.
LYNX_EXPORT void RegisterJSVMRuntimeProxyFactory(
    lynx::piper::NapiRuntimeProxyJSVMFactory *factory) {
  lynx::piper::NapiRuntimeProxy::SetJSVMRuntimeProxyFactory(factory);
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
    case JSRuntimeType::jsvm: {
      LOGI("Creating napi proxy using jsvm factory: " << s_jsvm_factory);
      if (s_jsvm_factory) {
        auto proxy_jsvm = s_jsvm_factory->Create(runtime, delegate);
        proxy_jsvm->SetJSRuntime(runtime);
        return proxy_jsvm;
      }
      return nullptr;
    }
  }
}
// the life cycle of shared_ptr to DelegateObserver is the same as
// NapiRuntimeProxy weak_ptr delegate_observer used to watch whether runtime is
// detached
void PostNAPIJSTask(napi_foreground_cb js_cb, void *data, void *task_ctx) {
  auto delegate_observer = std::weak_ptr<DelegateObserver>(
      *static_cast<std::shared_ptr<DelegateObserver> *>(task_ctx));
  auto locked_observer = delegate_observer.lock();
  if (locked_observer) {
    locked_observer->PostJSTask([delegate_observer, js_cb, data]() {
      if (delegate_observer.lock()) {
        js_cb(data);
      }
    });
  }
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
    if (is_safe_napi_) {
      loader_ = "__lynxNapiLoader";
    } else {
      loader_ = "napiLoaderOnRT" + std::to_string(runtime->getRuntimeId());
    }
    LOGI("NAPI Setup Loader: " << loader_);
    napi_setup_loader(env_, loader_.c_str());
    const char *kNapiMarker = "napiSharedMarker";
    Napi::HandleScope scope(env_);
    // compatible with single context
    if (runtime->getGroupId() == "-1") {
      env_.Global().Set("__lynxNapiLoader", env_.Global().Get(loader_.c_str()));
    }
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

NapiRuntimeProxyJSVMFactory *NapiRuntimeProxy::s_jsvm_factory = nullptr;
// static
void NapiRuntimeProxy::SetJSVMRuntimeProxyFactory(
    NapiRuntimeProxyJSVMFactory *factory) {
  s_jsvm_factory = factory;
}

namespace {
static const uint64_t kRestrictedModuleRegistryKey =
    reinterpret_cast<uint64_t>(&kRestrictedModuleRegistryKey);
class RestrictedModuleRegistry {
 public:
  std::unordered_map<std::string, Napi::ObjectReference> loaded_modules;
};

Napi::Value LoadRestrictedModule(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();
  Napi::Value modname = info[0];

  if (!modname.IsString()) {
    Napi::TypeError::New(env, "Expect 1st argument to be string")
        .ThrowAsJavaScriptException();
    return Napi::Value();
  }

  std::string modname_str = info[0].As<Napi::String>().Utf8Value();

  auto *registry = env.GetInstanceData<RestrictedModuleRegistry>(
      kRestrictedModuleRegistryKey);
  auto it = registry->loaded_modules.find(modname_str);
  if (it != registry->loaded_modules.end() && !it->second.IsEmpty()) {
    LOGI("Load restricted napi module succeed from cache: " << modname_str);
    return it->second.Value();
  }

  const napi_module *module = napi_find_module(modname_str.c_str());

  if (!module) {
    Napi::Error::New(env,
                     ("NAPI Module [" + modname_str + "] not found").c_str())
        .ThrowAsJavaScriptException();
    return Napi::Value();
  }

  // module_env is a copy of napi_env.
  // The napi_run_script and napi_get_global functions of module_env are hooked
  // to prevent users from calling napi_run_script and napi_get_global functions
  // in the lynx module, which could affect the stability of the lynx script
  // runtime.
  napi_env module_env = info.Env();
  napi_value result =
      module->nm_register_func(module_env, Napi::Object::New(env));
  Napi::Object exports = Napi::Object(env, result);
  if (!exports.IsEmpty() && exports.IsObject()) {
    registry->loaded_modules[modname_str].Reset(exports, 1);
    LOGI("Load restricted napi module succeed: " << modname_str);
    return exports;
  } else {
    // error happened
    Napi::Error::New(env, ("NAPI Module [" + modname_str +
                           "] load failed, the exports is not an object.")
                              .c_str())
        .ThrowAsJavaScriptException();
    return Napi::Value();
  }
}

// These two functions are used to hook the napi_run_script and napi_get_global
// functions of the napi env. To ensure the stability of the lynx script
// runtime, these two functions are not allowed to be called in the lynx module.
napi_status LynxHookedNapiRunScript(napi_env env, const char *script,
                                    size_t length, const char *filename,
                                    napi_value *result) {
  env->napi_throw_error(env, "lynx module runtime exception",
                        "napi_run_script is not allowed in lynx module.");
  return napi_generic_failure;
}

napi_status LynxHookedNapiGetGlobal(napi_env env, napi_value *result) {
  env->napi_throw_error(env, "lynx module runtime exception",
                        "napi_get_global is not allowed in lynx module.");
  return napi_generic_failure;
}

}  // namespace

// Setup restricted napi loader. This loader is used for users to load
// their own injected modules. For the modules loaded by this loader, the
// abilities of running scripts and getting the global object are disabled.
void RestrictedNapiRuntimeProxyDecorator::SetupLoader() {
  auto runtime = GetJSRuntime().lock();
  Napi::Env env = Env();
  napi_env raw_env = env;

  // The napi_run_script and napi_get_global functions of restricted napi_env
  // are hooked to prevent users from calling napi_run_script and
  // napi_get_global functions in the lynx module, which could affect the
  // stability of the lynx script runtime.
  get_global_func_ = raw_env->napi_get_global;
  raw_env->napi_run_script = LynxHookedNapiRunScript;
  raw_env->napi_get_global = LynxHookedNapiGetGlobal;

  if (runtime && raw_env && raw_env->ctx) {
    Napi::ContextScope context_scope(env);
    Napi::HandleScope handle_scope(env);
    loader_ = "napiRestrictedLoader" + std::to_string(runtime->getRuntimeId());
    LOGI("NAPI Setup Restricted Loader: " << loader_);
    auto *registry = env.GetInstanceData<RestrictedModuleRegistry>(
        kRestrictedModuleRegistryKey);
    if (registry) {
      return;
    }
    registry = new RestrictedModuleRegistry();
    env.SetInstanceData(kRestrictedModuleRegistryKey, registry);

    Napi::Object exports = Napi::Object::New(env);
    exports["load"] = Napi::Function::New(env, &LoadRestrictedModule, "load");
    GetGlobal().Set(loader_.c_str(), exports);
    return;
  }
}

void RestrictedNapiRuntimeProxyDecorator::RemoveLoader() {
  Napi::Env env = Env();
  napi_env raw_env = env;
  if (raw_env && raw_env->ctx && !loader_.empty()) {
    Napi::HandleScope handle_scope(env);
    Napi::Object global = GetGlobal();
    if (global.Has(loader_.c_str()).FromMaybe(false)) {
      LOGI("NAPI Remove Restricted Loader: " << loader_);
      global.Delete(loader_.c_str());
    }
  }
}

Napi::Object RestrictedNapiRuntimeProxyDecorator::GetGlobal() {
  if (!get_global_func_) {
    return Napi::Object();
  }
  napi_value raw_global;
  get_global_func_(proxy_->Env(), &raw_global);
  return Napi::Object(proxy_->Env(), raw_global);
}

}  // namespace piper
}  // namespace lynx
