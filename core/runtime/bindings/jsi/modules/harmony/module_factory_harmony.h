// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_BINDINGS_JSI_MODULES_HARMONY_MODULE_FACTORY_HARMONY_H_
#define CORE_RUNTIME_BINDINGS_JSI_MODULES_HARMONY_MODULE_FACTORY_HARMONY_H_

#include <napi/native_api.h>
#include <node_api.h>

#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "core/public/jsb/native_module_factory.h"
#include "core/runtime/bindings/jsi/modules/harmony/platform_module_manager.h"

namespace lynx {
namespace harmony {

class ModuleFactoryHarmony : public piper::NativeModuleFactory {
 public:
  ModuleFactoryHarmony(napi_env env, napi_value module_args[4],
                       napi_value sendable_module_args[4]);

  std::shared_ptr<piper::LynxNativeModule> CreateModule(
      const std::string& name) override;

 private:
  std::shared_ptr<PlatformModuleManager> platform_module_manager_;
  std::mutex mutex_;
};

}  // namespace harmony
}  // namespace lynx

#endif  // CORE_RUNTIME_BINDINGS_JSI_MODULES_HARMONY_MODULE_FACTORY_HARMONY_H_
