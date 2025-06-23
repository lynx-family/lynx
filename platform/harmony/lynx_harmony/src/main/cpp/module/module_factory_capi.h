// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_MODULE_MODULE_FACTORY_CAPI_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_MODULE_MODULE_FACTORY_CAPI_H_

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

#include "core/public/jsb/native_module_factory.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/lynx_context.h"

namespace lynx {
namespace harmony {

using HarmonyModuleCreator =
    std::function<std::shared_ptr<piper::LynxNativeModule>(
        const std::shared_ptr<tasm::harmony::LynxContext>& context)>;

class ModuleFactoryCAPI : public piper::NativeModuleFactory {
 public:
  explicit ModuleFactoryCAPI(
      const std::shared_ptr<tasm::harmony::LynxContext>& context)
      : context_(context) {}

  std::shared_ptr<piper::LynxNativeModule> CreateModule(
      const std::string& name) override;
  void RegisterModule(const std::string& name, HarmonyModuleCreator creator);

 private:
  void InitNativeModules();
  std::mutex mutex_;
  std::unordered_map<std::string, HarmonyModuleCreator> module_creators_;
  std::weak_ptr<tasm::harmony::LynxContext> context_;
};

}  // namespace harmony
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_MODULE_MODULE_FACTORY_CAPI_H_
