// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/lepusng/napi/worklet/napi_loader_ui.h"

#include <memory>

#include "base/include/value/base_string.h"
#include "core/renderer/worklet/lepus_lynx.h"
#include "core/runtime/lepusng/jsvalue_helper.h"
#include "core/runtime/lepusng/napi/worklet/napi_lepus_lynx.h"

namespace {

void* GetQuickContextNapiEnv(lynx::lepus::QuickContext* quick_context) {
  return quick_context->napi_env();
}

}  // namespace

#ifdef USE_PRIMJS_NAPI
#include "third_party/napi/include/primjs_napi_defines.h"
#endif

namespace lynx {
namespace worklet {
namespace {

constexpr char kGetModuleLoader[] = "getModuleLoader";

lepus::Value GetModuleLoader(runtime::MTSContext* context, lepus::Value*, int) {
  auto* quick_context = static_cast<lepus::QuickContext*>(context);
  auto env = reinterpret_cast<napi_env>(GetQuickContextNapiEnv(quick_context));
  if (env == nullptr) {
    return lepus::Value(lepus::Value::kCreateAsUndefinedTag);
  }
  Napi::HandleScope scope(env);
  Napi::Object loader = Napi::Env(env).Loader();
  return MK_JS_LEPUS_VALUE(
      quick_context->context(),
      *reinterpret_cast<LEPUSValue*>(static_cast<napi_value>(loader)));
}

}  // namespace

NapiLoaderUI::NapiLoaderUI(runtime::MTSRuntime* context) : context_(context) {}

void NapiLoaderUI::OnAttach(Napi::Env env) {
  SetNapiEnvToLEPUSContext(env);
  auto* delegate = context_->GetDelegate();
  if (delegate != nullptr) {
    delegate->OnNapiEnvironmentAttached(
        reinterpret_cast<void*>(static_cast<napi_env>(env)));
  }

  context_->SetPropertyToLynx(
      BASE_STATIC_STRING(kGetModuleLoader),
      MK_JS_LEPUS_VALUE(
          runtime::MTSRuntime::ToQuickContext(context_)->context(),
          runtime::MTSRuntime::ToQuickContext(context_)->NewBindingFunction(
              &GetModuleLoader)));

  // Set Lynx To Napi Env
  lynx_ = LepusLynx::Create(
      env, context_->name(),
      static_cast<tasm::TemplateAssembler*>(context_->GetDelegate()));
  constexpr const static char* kGlobalLynxName = "lepusLynx";
  Napi::HandleScope handle_scope(env);
  env.Global()[kGlobalLynxName] =
      NapiLepusLynx::Wrap(std::unique_ptr<LepusLynx>(lynx_), env);
}

void NapiLoaderUI::OnDetach(Napi::Env env) {
  auto use_env = static_cast<napi_env>(env);
  if (!use_env) {
    return;
  }
  auto* delegate = context_->GetDelegate();
  if (delegate != nullptr) {
    delegate->OnNapiEnvironmentDetached(reinterpret_cast<void*>(use_env));
  }
  auto& map = NapiEnvToContextMap();
  auto iter = map.find(use_env);
  if (iter == map.end()) {
    return;
  }
  auto* quick_context = iter->second;
  quick_context->set_napi_env(nullptr);
  map.erase(iter);

  lynx_ = nullptr;
}

void NapiLoaderUI::InvokeLepusBridge(const int32_t callback_id,
                                     const lepus::Value& data) {
  lynx_->InvokeLepusBridge(callback_id, data);
}

lepus::QuickContext* NapiLoaderUI::GetQuickContextFromNapiEnv(Napi::Env env) {
  auto& context_map = NapiLoaderUI::NapiEnvToContextMap();
  auto iter = context_map.find(static_cast<napi_env>(env));
  if (iter == context_map.end()) {
    return nullptr;
  }
  return iter->second;
}

std::unordered_map<napi_env, lepus::QuickContext*>&
NapiLoaderUI::NapiEnvToContextMap() {
  static thread_local std::unordered_map<napi_env, lepus::QuickContext*> map;
  return map;
}

void NapiLoaderUI::SetNapiEnvToLEPUSContext(Napi::Env env) {
  auto quick_context = runtime::MTSRuntime::ToQuickContext(context_);
  if (quick_context == nullptr) {
    return;
  }
  auto use_env = static_cast<napi_env>(env);
  quick_context->set_napi_env(reinterpret_cast<void*>(use_env));
  NapiEnvToContextMap()[use_env] = quick_context;
}

}  // namespace worklet
}  // namespace lynx

#ifdef USE_PRIMJS_NAPI
#include "third_party/napi/include/primjs_napi_undefs.h"
#endif
