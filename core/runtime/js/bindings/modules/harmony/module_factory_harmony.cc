// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/js/bindings/modules/harmony/module_factory_harmony.h"

#include <utility>

#include "core/runtime/js/bindings/modules/harmony/native_module_harmony.h"

namespace lynx {
namespace harmony {

ModuleFactoryHarmony::ModuleFactoryHarmony(napi_env env,
                                           napi_value module_args[5],
                                           napi_value sendable_module_args[5])
    : platform_module_manager_(std::make_shared<PlatformModuleManager>(
          env, module_args, sendable_module_args)) {}

std::shared_ptr<runtime::LynxNativeModule> ModuleFactoryHarmony::CreateModule(
    const std::string& name) {
  PlatformModuleManager::ModuleInfo info;
  if (!platform_module_manager_->GetModuleInfo(name, info)) {
    return std::shared_ptr<runtime::LynxNativeModule>(nullptr);
  }
  return std::make_shared<NativeModuleHarmony>(
      platform_module_manager_, platform_module_manager_->Env(), name,
      info.sendable, info.methods, info.sync_methods);
}

}  // namespace harmony
}  // namespace lynx
