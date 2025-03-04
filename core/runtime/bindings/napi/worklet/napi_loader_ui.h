// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_BINDINGS_NAPI_WORKLET_NAPI_LOADER_UI_H_
#define CORE_RUNTIME_BINDINGS_NAPI_WORKLET_NAPI_LOADER_UI_H_

#include <unordered_map>

#include "core/runtime/bindings/napi/napi_environment.h"
#include "core/runtime/vm/lepus/quick_context.h"
#include "third_party/binding/napi/shim/shim_napi.h"

#ifdef USE_PRIMJS_NAPI
#include "third_party/napi/include/primjs_napi_defines.h"
#endif

namespace lynx {
namespace worklet {

class LepusLynx;

class NapiLoaderUI : public piper::NapiEnvironment::Delegate {
 public:
  NapiLoaderUI(lepus::QuickContext* context);

  void OnAttach(Napi::Env env) override;
  void OnDetach(Napi::Env env) override;
  lynx::worklet::LepusLynx* lepus_lynx() { return lynx_; }
  void InvokeLepusBridge(const int32_t callback_id, const lepus::Value& data);

  static lepus::QuickContext* GetQuickContextFromNapiEnv(Napi::Env env);

 private:
  static std::unordered_map<napi_env, lepus::QuickContext*>&
  NapiEnvToContextMap();
  void SetNapiEnvToLEPUSContext(Napi::Env env);

  lynx::worklet::LepusLynx* lynx_ = nullptr;
  lepus::QuickContext* context_ = nullptr;
};

}  // namespace worklet
}  // namespace lynx

#ifdef USE_PRIMJS_NAPI
#include "third_party/napi/include/primjs_napi_undefs.h"
#endif

#endif  // CORE_RUNTIME_BINDINGS_NAPI_WORKLET_NAPI_LOADER_UI_H_
