// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/module/module_factory_capi.h"

#include <utility>

#include "platform/harmony/lynx_harmony/src/main/cpp/module/lynx_exposure_module.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/module/lynx_intersection_observer_module.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/module/lynx_text_info_module.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/module/lynx_ui_method_module.h"
namespace lynx {
namespace harmony {

std::shared_ptr<piper::LynxNativeModule> ModuleFactoryCAPI::CreateModule(
    const std::string& name) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto itr = module_creators_.find(name);
  auto context = context_.lock();
  if (context && itr != module_creators_.end()) {
    return itr->second(context);
  }
  return std::shared_ptr<piper::LynxNativeModule>(nullptr);
}

void ModuleFactoryCAPI::RegisterModule(const std::string& name,
                                       HarmonyModuleCreator creator) {
  std::lock_guard<std::mutex> lock(mutex_);
  module_creators_.emplace(name, std::move(creator));
}

// private
void ModuleFactoryCAPI::InitNativeModules() {
  RegisterModule(LynxUIMethodModule::GetName(), LynxUIMethodModule::Create);
  RegisterModule(LynxExposureModule::GetName(), LynxExposureModule::Create);
  RegisterModule(LynxTextInfoModule::GetName(), LynxTextInfoModule::Create);
  RegisterModule(LynxIntersectionObserverModule::GetName(),
                 LynxIntersectionObserverModule::Create);
}
}  // namespace harmony
}  // namespace lynx
