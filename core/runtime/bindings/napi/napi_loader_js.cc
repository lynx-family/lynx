// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/bindings/napi/napi_loader_js.h"

#include <utility>

#include "third_party/binding/napi/shim/shim_napi.h"

#ifdef USE_PRIMJS_NAPI
#include "third_party/napi/include/primjs_napi_defines.h"
#endif

namespace lynx {
namespace piper {

using Module = NapiEnvironment::Module;

NapiLoaderJS::NapiLoaderJS(const std::string& id) : id_(id) {}

Napi::Value TriggerGC(const Napi::CallbackInfo& info) {
  auto runtime =
      piper::NapiEnvironment::From(info.Env())->GetJSRuntime().lock();

  if (runtime) {
    TRACE_EVENT(LYNX_TRACE_CATEGORY, "TriggerGC");
    runtime->RequestGC();
  }

  return info.Env().Undefined();
}

Napi::Value TriggerGCForTesting(const Napi::CallbackInfo& info) {
  auto runtime =
      piper::NapiEnvironment::From(info.Env())->GetJSRuntime().lock();

  if (runtime) {
    TRACE_EVENT(LYNX_TRACE_CATEGORY, "TriggerGCForTesting");
    runtime->RequestGCForTesting();
  }

  return info.Env().Undefined();
}

static Napi::Value InstallGC(const Napi::CallbackInfo& info) {
  if (info.Length() < 1 || !info[0].IsObject()) {
    Napi::Error::New(info.Env(),
                     "Invalid arguments, expecting: lynx.InstallGC(target)")
        .ThrowAsJavaScriptException();
    return Napi::Value();
  }

  Napi::Object target = info[0].As<Napi::Object>();
  target["triggerGC"] =
      Napi::Function::New(info.Env(), &TriggerGC, "triggerGC");
  if constexpr (LYNX_ENABLE_E2E_TEST) {
    target["triggerGCForTesting"] = Napi::Function::New(
        info.Env(), &TriggerGCForTesting, "triggerGCForTesting");
  }
  return Napi::Value();
}

static Napi::Value LoadLazyModule(const Napi::CallbackInfo& info) {
  if (info.Length() <= 1 || !info[0].IsString() || !info[1].IsObject()) {
    Napi::Error::New(
        info.Env(),
        "Invalid arguments, expecting: lynx.loadModule(<String>, <Object>)")
        .ThrowAsJavaScriptException();
    return Napi::Value();
  }

  Napi::String name = info[0].As<Napi::String>();
  Napi::Object target = info[1].As<Napi::Object>();
  auto* module =
      piper::NapiEnvironment::From(info.Env())->GetModule(name.Utf8Value());
  if (!module) {
    std::string msg("Module not registered: ");
    msg += name;
    LOGE("napi " << msg);
    return Napi::Value();
  }

  module->OnLoad(target);
  return Napi::Value();
}

static Napi::Value InstallNapiModules(const Napi::CallbackInfo& info) {
  // Install all instant modules on 'lynx' object.
  DCHECK(info.Length() > 0);
  Napi::Object lynx = info[0].As<Napi::Object>();
  NapiEnvironment::From(info.Env())->delegate()->LoadInstantModules(lynx);

  // Install lazy module hook.
  lynx["loadModule"] =
      Napi::Function::New(info.Env(), &LoadLazyModule, "loadModule");
  lynx["_installGC"] = Napi::Function::New(info.Env(), &InstallGC, "installGC");

  return Napi::Value();
}

void NapiLoaderJS::OnAttach(Napi::Env env) {
  napi_env raw_env = env;
  if (raw_env && raw_env->ctx) {
    LOGI("napi OnAttach env: " << raw_env << ", ctx: " << raw_env->ctx
                               << ", id: " << id_);
    Napi::HandleScope scope(env);
    std::string hook_name("installNapiModulesOnRT");
    hook_name += id_;
    env.Global()[hook_name.c_str()] =
        Napi::Function::New(env, &InstallNapiModules, hook_name.c_str());
  }
}

void NapiLoaderJS::OnDetach(Napi::Env env) {
  napi_env raw_env = env;
  if (raw_env && raw_env->ctx) {
    Napi::HandleScope handle_scope(env);
    for (auto& m : modules_) {
      m.second->OnEnvDetach(env);
    }
    LOGI("napi OnDetach env: " << raw_env << ", ctx: " << raw_env->ctx
                               << ", id: " << id_);
  }
}

void NapiLoaderJS::RegisterModule(const std::string& name,
                                  std::unique_ptr<Module> module) {
  modules_[name] = std::move(module);
}

Module* NapiLoaderJS::GetModule(const std::string& name) {
  auto it = modules_.find(name);
  if (it != modules_.end()) {
    return it->second.get();
  }
  return nullptr;
}

void NapiLoaderJS::LoadInstantModules(Napi::Object& lynx) {
  if (loaded_) return;
  loaded_ = true;
  for (auto& m : modules_) {
    if (m.second->IsLazy()) {
      continue;
    }
    m.second->OnLoad(lynx);
  }
}

}  // namespace piper
}  // namespace lynx
