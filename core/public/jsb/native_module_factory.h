// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_PUBLIC_JSB_NATIVE_MODULE_FACTORY_H_
#define CORE_PUBLIC_JSB_NATIVE_MODULE_FACTORY_H_

#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>

#include "core/public/jsb/lynx_native_module.h"

// TODO(liyanbo.monster): after platform module refactored, remove this
#include "core/runtime/bindings/jsi/modules/lynx_module.h"
namespace lynx {
namespace piper {
class LynxModuleBinding;
class LynxModuleManager;
}  // namespace piper
}  // namespace lynx

namespace lynx {
namespace piper {

using ModuleCreator = std::function<std::shared_ptr<LynxNativeModule>()>;

class NativeModuleFactory {
 public:
  virtual ~NativeModuleFactory() = default;

  // Default implementation support to register and create C++ module.
  // Different platforms can implement subclasses to register the platform
  // Module
  virtual std::shared_ptr<LynxNativeModule> CreateModule(
      const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto itr = creators_.find(name);
    if (itr == creators_.end()) {
      return nullptr;
    }
    return itr->second();
  };

  // TODO(liyanbo.monster): after platform module refactored, merge this with
  // upper code.
  virtual std::shared_ptr<LynxModule> CreatePlatformModule(
      const std::string& name) {
    return nullptr;
  };
  virtual void SetModuleExtraInfo(std::shared_ptr<ModuleDelegate> delegate) {}

  virtual void Register(const std::string& name, ModuleCreator creator) {
    std::lock_guard<std::mutex> lock(mutex_);
    creators_.emplace(name, std::move(creator));
  }

 private:
  std::mutex mutex_;
  std::unordered_map<std::string, ModuleCreator> creators_;
};

}  // namespace piper
}  // namespace lynx

#endif  // CORE_PUBLIC_JSB_NATIVE_MODULE_FACTORY_H_
