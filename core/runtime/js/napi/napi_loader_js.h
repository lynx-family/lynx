// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_JS_NAPI_NAPI_LOADER_JS_H_
#define CORE_RUNTIME_JS_NAPI_NAPI_LOADER_JS_H_

#include <map>
#include <memory>
#include <string>

#include "base/include/closure.h"
#include "core/runtime/common/napi/napi_environment.h"

namespace lynx {
namespace piper {

class NapiLoaderJS : public NapiEnvironment::Delegate {
 public:
  NapiLoaderJS(const std::string& id);
#if ENABLE_NAPI_BINDING
  using RuntimeReadyHandler =
      base::MoveOnlyClosure<void, Napi::Env, Napi::Object&>;
  NapiLoaderJS(const std::string& id,
               RuntimeReadyHandler runtime_ready_handler);
#endif
  void OnAttach(Napi::Env env) override;
  void OnDetach(Napi::Env env) override;

  void RegisterModule(const std::string& name,
                      std::unique_ptr<NapiEnvironment::Module> module) override;
  NapiEnvironment::Module* GetModule(const std::string& name) override;
  void LoadInstantModules(Napi::Object& lynx) override;

  void NotifyRuntimeReady(Napi::Env env, Napi::Object& lynx) override;

 private:
  // With shared context, we want each env has one corresponding install hook,
  // so that the hook is independent from others and thus valid throughout its
  // lifetime.
  std::string id_;

  std::map<std::string, std::unique_ptr<NapiEnvironment::Module>> modules_;
  bool loaded_ = false;
#if ENABLE_NAPI_BINDING
  RuntimeReadyHandler runtime_ready_handler_;
#endif
};

}  // namespace piper
}  // namespace lynx

#endif  // CORE_RUNTIME_JS_NAPI_NAPI_LOADER_JS_H_
