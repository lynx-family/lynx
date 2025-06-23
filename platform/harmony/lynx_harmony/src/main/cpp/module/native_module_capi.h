// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_MODULE_NATIVE_MODULE_CAPI_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_MODULE_NATIVE_MODULE_CAPI_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

#include "core/public/jsb/lynx_native_module.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/lynx_context.h"

namespace lynx {
namespace harmony {

class NativeModuleCAPI : public piper::LynxNativeModule {
 public:
  ~NativeModuleCAPI() override = default;

  base::expected<std::unique_ptr<pub::Value>, std::string> InvokeMethod(
      const std::string& method_name, std::unique_ptr<pub::Value> args,
      size_t count, const piper::CallbackMap& callbacks) override;

 protected:
  // Register your own invocation. You can varify method by NativeModuleMethod
  void RegisterMethod(
      const piper::NativeModuleMethod& method,
      piper::LynxNativeModule::NativeModuleInvocation invocation) {
    methods_.emplace(method.name, method);
    invocations_.emplace(method.name, std::move(invocation));
  }

  std::unordered_map<std::string, NativeModuleInvocation> invocations_;
};

}  // namespace harmony
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_MODULE_NATIVE_MODULE_CAPI_H_
