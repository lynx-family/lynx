// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/bindings/napi/worklet/napi_loader_ui.h"

#include <memory>

#include "core/renderer/worklet/lepus_lynx.h"
#include "core/runtime/bindings/napi/worklet/napi_lepus_lynx.h"

#ifdef USE_PRIMJS_NAPI
#include "third_party/napi/include/primjs_napi_defines.h"
#endif

namespace lynx {
namespace worklet {

NapiLoaderUI::NapiLoaderUI(lepus::QuickContext* context) : context_(context) {}

void NapiLoaderUI::OnAttach(Napi::Env env) {
  SetNapiEnvToLEPUSContext(env);

  // Set Lynx To Napi Env
  lynx_ = LepusLynx::Create(
      env, context_->name(),
      static_cast<tasm::TemplateAssembler*>(context_->GetDelegate()));
  constexpr const static char* kGlobalLynxName = "lepusLynx";
  env.Global()[kGlobalLynxName] =
      NapiLepusLynx::Wrap(std::unique_ptr<LepusLynx>(lynx_), env);
}

void NapiLoaderUI::OnDetach(Napi::Env env) {
  auto use_env = static_cast<napi_env>(env);
  if (!use_env) {
    return;
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
  static thread_local base::NoDestructor<
      std::unordered_map<napi_env, lepus::QuickContext*>>
      map{};
  return *map;
}

void NapiLoaderUI::SetNapiEnvToLEPUSContext(Napi::Env env) {
  context_->set_napi_env(reinterpret_cast<void*>(static_cast<napi_env>(env)));
  NapiEnvToContextMap()[static_cast<napi_env>(env)] = context_;
}

}  // namespace worklet
}  // namespace lynx

#ifdef USE_PRIMJS_NAPI
#include "third_party/napi/include/primjs_napi_undefs.h"
#endif
