// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/bindings/jsi/modules/harmony/module_factory_harmony.h"

#include <utility>

#include "core/runtime/bindings/jsi/modules/harmony/native_module_harmony.h"

namespace lynx {
namespace harmony {

ModuleFactoryHarmony::ModuleFactoryHarmony(napi_env env,
                                           napi_value module_args[4],
                                           napi_value sendable_module_args[4])
    : platform_module_manager_(std::make_shared<PlatformModuleManager>(
          env, module_args, sendable_module_args)) {}

std::shared_ptr<piper::LynxNativeModule> ModuleFactoryHarmony::CreateModule(
    const std::string& name) {
  std::lock_guard<std::mutex> lock(mutex_);

  auto js_it = platform_module_manager_->JSModuleMap().find(name);
  if (js_it != platform_module_manager_->JSModuleMap().end()) {
    auto local_module = std::make_shared<NativeModuleHarmony>(
        platform_module_manager_, platform_module_manager_->Env(), name,
        js_it->second.first, js_it->second.second);
    return local_module;
  }
  return std::shared_ptr<piper::LynxNativeModule>(nullptr);
}

}  // namespace harmony
}  // namespace lynx
