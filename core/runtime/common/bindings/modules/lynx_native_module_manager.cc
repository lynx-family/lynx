// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/common/bindings/modules/lynx_native_module_manager.h"

namespace lynx {
namespace pub {

std::shared_ptr<piper::LynxNativeModule> LynxNativeModuleManager::GetModule(
    const std::string &name) {
  std::shared_ptr<piper::LynxNativeModule> native_module;
  // Find NativeModuleFactories First
  for (const auto &module_factory : module_factories_) {
    native_module = module_factory->CreateModule(name);
    if (native_module) {
      return native_module;
    }
  }
  // Find PlatformModuleFactories Later
  if (platform_module_factory_) {
    native_module = platform_module_factory_->CreateModule(name);
    if (native_module) {
      return native_module;
    }
  }
  return native_module;
}

void LynxNativeModuleManager::SetPlatformModuleFactory(
    std::shared_ptr<piper::NativeModuleFactory> module_factory) {
  platform_module_factory_ = std::move(module_factory);
}

piper::NativeModuleFactory *
LynxNativeModuleManager::GetPlatformModuleFactory() {
  return platform_module_factory_.get();
}

void LynxNativeModuleManager::SetEngineActor(
    const std::shared_ptr<shell::LynxActor<shell::LynxEngine>> &engine_actor) {
  engine_actor_ = engine_actor;
}

void LynxNativeModuleManager::SetFacadeActor(
    const std::shared_ptr<shell::LynxActor<shell::NativeFacade>>
        &facade_actor) {
  facade_actor_ = facade_actor;
}

}  // namespace pub
}  // namespace lynx
