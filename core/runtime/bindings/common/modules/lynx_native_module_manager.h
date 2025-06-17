// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_BINDINGS_COMMON_MODULES_LYNX_NATIVE_MODULE_MANAGER_H_
#define CORE_RUNTIME_BINDINGS_COMMON_MODULES_LYNX_NATIVE_MODULE_MANAGER_H_

#include <memory>
#include <string>
#include <utility>

#include "core/public/jsb/native_module_factory.h"

namespace lynx {
namespace pub {
class LynxNativeModuleManager {
 public:
  LynxNativeModuleManager() = default;
  virtual ~LynxNativeModuleManager() = default;
  std::shared_ptr<piper::LynxNativeModule> GetModule(const std::string &name);

  // Used for create CModule
  void SetModuleFactory(
      std::unique_ptr<piper::NativeModuleFactory> module_factory) {
    if (module_factory) {
      module_factories_.push_back(std::move(module_factory));
    }
  };
  // Used for create PlatformModule
  void SetPlatformModuleFactory(
      std::unique_ptr<piper::NativeModuleFactory> module_factory);
  piper::NativeModuleFactory *GetPlatformModuleFactory();

 private:
  // Managed by LynxModuleManager
  base::InlineVector<std::unique_ptr<piper::NativeModuleFactory>, 4>
      module_factories_;
  // Maybe use by platform.
  // When re-attaching in standalone mode, it needs to support modification, so
  // it needs to be accessed by the platform, so it is placed independently.
  std::unique_ptr<piper::NativeModuleFactory> platform_module_factory_;
};
}  // namespace pub

}  // namespace lynx

#endif  // CORE_RUNTIME_BINDINGS_COMMON_MODULES_LYNX_NATIVE_MODULE_MANAGER_H_
