// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_EMBEDDER_MODULE_LYNX_MODULE_MANAGER_NAPI_H_
#define PLATFORM_EMBEDDER_MODULE_LYNX_MODULE_MANAGER_NAPI_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

#include "core/public/lynx_runtime_proxy.h"
#include "core/runtime/js/bindings/modules/lynx_module_manager.h"
#include "platform/embedder/module/lynx_module_factory_napi.h"

namespace lynx {
namespace embedder {

class LynxModuleManagerNAPI
    : public std::enable_shared_from_this<LynxModuleManagerNAPI> {
 public:
  LynxModuleManagerNAPI(
      void* context,
      std::shared_ptr<runtime::js::LynxModuleManager> module_manager,
      std::unordered_map<std::string, std::pair<napi_module_creator, void*>>
          module_creators);
  void SetupRuntimeLifecycleListener(
      std::shared_ptr<shell::LynxRuntimeProxy> runtime_proxy);

  void OnRuntimeAttach(Napi::Env env);

  // Called from Main thread before LynxView destroyed.
  void Detach();

 private:
  void* view_context_;
  std::shared_ptr<runtime::js::LynxModuleManager> module_manager_;
  std::unordered_map<std::string, std::pair<napi_module_creator, void*>>
      module_creators_;
  LynxModuleFactoryNAPI* module_factory_ = nullptr;
};

}  // namespace embedder
}  // namespace lynx

#endif  // PLATFORM_EMBEDDER_MODULE_LYNX_MODULE_MANAGER_NAPI_H_
