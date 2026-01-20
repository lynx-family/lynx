// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_LYNX_ADAPTOR_NATIVE_MODULE_LYNX_MODULE_BASE_H_
#define CLAY_LYNX_ADAPTOR_NATIVE_MODULE_LYNX_MODULE_BASE_H_

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

#include "base/include/fml/task_runner.h"
#include "core/public/jsb/lynx_native_module.h"

namespace lynx {

class LynxModuleBase : public runtime::LynxNativeModule {
 public:
  LynxModuleBase(uint32_t view_context_id,
                 fml::RefPtr<fml::TaskRunner> task_runner);
  virtual ~LynxModuleBase();

 protected:
  using Invocation = std::function<std::unique_ptr<pub::Value>(
      std::unique_ptr<pub::Value>, const runtime::CallbackMap&)>;

  template <typename T>
  void RegisterMethod(const runtime::NativeModuleMethod& method,
                      std::unique_ptr<pub::Value> (T::*member_fn)(
                          std::unique_ptr<pub::Value>,
                          const runtime::CallbackMap&)) {
    RegisterInvocation(method, [obj = static_cast<T*>(this), member_fn](
                                   std::unique_ptr<pub::Value> arg,
                                   const runtime::CallbackMap& cb) {
      return std::invoke(member_fn, obj, std::move(arg), cb);
    });
  }
  void RegisterInvocation(const runtime::NativeModuleMethod& method,
                          Invocation&& invocation);
  bool ValidateInvocation(const std::string& method_name, size_t count);

  base::expected<std::unique_ptr<lynx::pub::Value>, std::string> InvokeMethod(
      const std::string& method_name, std::unique_ptr<lynx::pub::Value> args,
      size_t count, const runtime::CallbackMap& callbacks) override;

  uint32_t view_context_id_;
  fml::RefPtr<fml::TaskRunner> task_runner_;

  std::unordered_map<std::string, Invocation> invocations_;
};

}  // namespace lynx

#endif  // CLAY_LYNX_ADAPTOR_NATIVE_MODULE_LYNX_MODULE_BASE_H_
