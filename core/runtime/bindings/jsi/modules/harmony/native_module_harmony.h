// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_BINDINGS_JSI_MODULES_HARMONY_NATIVE_MODULE_HARMONY_H_
#define CORE_RUNTIME_BINDINGS_JSI_MODULES_HARMONY_NATIVE_MODULE_HARMONY_H_

#include <node_api.h>

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "base/include/compiler_specific.h"
#include "core/public/jsb/lynx_native_module.h"
#include "core/runtime/bindings/jsi/modules/harmony/platform_module_manager.h"

namespace lynx {
namespace lepus {
class Value;
}
namespace piper {
class NativeModuleFactory;
}
namespace harmony {
class ModuleFactoryHarmony;

class NativeModuleHarmony : public piper::LynxNativeModule {
 public:
  NativeModuleHarmony(const std::shared_ptr<PlatformModuleManager>& manager,
                      napi_env env, const std::string& name, bool sendable,
                      const std::vector<std::string>& methods);

  base::expected<std::unique_ptr<pub::Value>, std::string> InvokeMethod(
      const std::string& method_name, std::unique_ptr<pub::Value> args,
      size_t count, const piper::CallbackMap& callbacks) override;

  void Destroy() override;

 private:
  static napi_value Callback(napi_env env, napi_callback_info info);

  napi_value InvokeMethod(napi_env env, const lepus::Value& lepus_argv,
                          const std::string& method_name,
                          const std::shared_ptr<Delegate>& delegate,
                          const piper::CallbackMap& callbacks, uint64_t flow_id,
                          const std::string& first_arg) const;

  std::weak_ptr<PlatformModuleManager> platform_manager_;
  napi_env main_env_;

  const bool sendable_ = false;
  const std::string module_name_;
};
}  // namespace harmony
}  // namespace lynx

#endif  // CORE_RUNTIME_BINDINGS_JSI_MODULES_HARMONY_NATIVE_MODULE_HARMONY_H_
