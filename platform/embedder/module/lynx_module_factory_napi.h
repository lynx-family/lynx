// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_EMBEDDER_MODULE_LYNX_MODULE_FACTORY_NAPI_H_
#define PLATFORM_EMBEDDER_MODULE_LYNX_MODULE_FACTORY_NAPI_H_

#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>

#include "core/public/jsb/native_module_factory.h"
#include "platform/embedder/module/lynx_native_module_napi.h"
#include "platform/embedder/public/capi/lynx_native_module_capi.h"
#include "third_party/weak-node-api/headers/node_api.h"

#ifdef USE_WEAK_SUFFIX_NAPI
#include "third_party/weak-node-api/headers/weak_napi_defines.h"
#endif

namespace lynx {
namespace embedder {

class LynxModuleFactoryNAPI : public runtime::NativeModuleFactory {
 public:
  LynxModuleFactoryNAPI(
      Napi::Env env,
      std::unordered_map<std::string, std::pair<napi_module_creator, void*>>
          module_creators);

  ~LynxModuleFactoryNAPI() override;

  // Called from Main thread before LynxView destroyed.
  void Detach();

  std::shared_ptr<runtime::LynxNativeModule> CreateModule(
      const std::string& name) override;

 protected:
  LynxModuleFactoryNAPI(
      void* view_context,
      std::unordered_map<std::string, std::pair<napi_module_creator, void*>>
          module_creators);

  std::shared_ptr<runtime::LynxNativeModule> CreateModuleWithEnv(
      const std::string& name, napi_env env);

  void BindViewContext(napi_env env);
  void UnbindViewContext(napi_env env);

  std::atomic<napi_env> env_{nullptr};

 private:
  void* view_context_ = nullptr;
  std::mutex mutex_;
  std::unordered_map<std::string, std::pair<napi_module_creator, void*>>
      module_creators_;
  bool is_detached_ = false;
};

class LynxMTSModuleFactoryNAPI : public LynxModuleFactoryNAPI {
 public:
  LynxMTSModuleFactoryNAPI(
      void* view_context,
      std::unordered_map<std::string, std::pair<napi_module_creator, void*>>
          module_creators);

  ~LynxMTSModuleFactoryNAPI() override;

  void AttachOpaqueContext(void* context) override;
  void DetachOpaqueContext(void* context) override;
};
}  // namespace embedder
}  // namespace lynx

#ifdef USE_WEAK_SUFFIX_NAPI
#include "third_party/weak-node-api/headers/weak_napi_undefs.h"
#endif

#endif  // PLATFORM_EMBEDDER_MODULE_LYNX_MODULE_FACTORY_NAPI_H_
