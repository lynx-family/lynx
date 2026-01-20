// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_EMBEDDER_MODULE_LYNX_MODULE_FACTORY_NAPI_H_
#define PLATFORM_EMBEDDER_MODULE_LYNX_MODULE_FACTORY_NAPI_H_

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>

#include "core/public/jsb/native_module_factory.h"
#include "platform/embedder/module/lynx_native_module_napi.h"
#include "platform/embedder/public/capi/lynx_native_module_capi.h"
#include "third_party/binding/napi/shim/shim_napi.h"

#ifdef USE_PRIMJS_NAPI
#include "third_party/napi/include/primjs_napi_defines.h"
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

 private:
  Napi::Env env_;
  std::mutex mutex_;
  std::unordered_map<std::string, std::pair<napi_module_creator, void*>>
      module_creators_;
  bool is_detached_ = false;
};
}  // namespace embedder
}  // namespace lynx

#ifdef USE_PRIMJS_NAPI
#include "third_party/napi/include/primjs_napi_undefs.h"
#endif

#endif  // PLATFORM_EMBEDDER_MODULE_LYNX_MODULE_FACTORY_NAPI_H_
