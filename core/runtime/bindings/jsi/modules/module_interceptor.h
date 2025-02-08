// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_BINDINGS_JSI_MODULES_MODULE_INTERCEPTOR_H_
#define CORE_RUNTIME_BINDINGS_JSI_MODULES_MODULE_INTERCEPTOR_H_

#include <memory>
#include <string>

#include "base/include/vector.h"
#include "core/runtime/bindings/jsi/modules/lynx_module.h"
#include "core/runtime/bindings/jsi/modules/module_delegate.h"
#include "core/runtime/jsi/jsi.h"

namespace lynx {
namespace piper {
/**
 * Result for ModuleMethodInterceptor.
 * `handled` meaning whether the Module Method is handled and stop
 * propagation.
 * `result` is the Module Method result, only valid when `handled`
 * is true.
 */
struct ModuleInterceptorResult {
  bool handled;
  Value result;
};

/**
 * Intercept mould method call.
 * This object should only alive in JS Thread.
 */
class ModuleInterceptor {
 public:
  virtual ModuleInterceptorResult InterceptModuleMethod(
      const std::shared_ptr<LynxModule>& module,
      const LynxModule::MethodMetadata& method, Runtime* rt,
      const std::shared_ptr<piper::ModuleDelegate>& delegate,
      const piper::Value* args, size_t count,
      const std::unique_ptr<pub::Value>& pub_args, const CallbackMap& callbacks,
      piper::NativeModuleInfoCollectorPtr timing_collector) const = 0;

  virtual void BeforeInvokeMethod(
      const LynxModule::MethodMetadata& method,
      const std::unique_ptr<pub::Value>& args,
      const NativeModuleInfoCollectorPtr& timing_collector) {}
  virtual void OnCallbackInvoked(const NativeModuleInfoCollectorPtr& timing,
                                 ModuleCallback* callback) {}
  virtual ~ModuleInterceptor() = default;
  virtual void SetTemplateUrl(const std::string& url) = 0;
};

class GroupInterceptor : public ModuleInterceptor {
 public:
  ModuleInterceptorResult InterceptModuleMethod(
      const std::shared_ptr<LynxModule>& module,
      const LynxModule::MethodMetadata& method, Runtime* rt,
      const std::shared_ptr<piper::ModuleDelegate>& delegate,
      const piper::Value* args, size_t count,
      const std::unique_ptr<pub::Value>& pub_args, const CallbackMap& callbacks,
      piper::NativeModuleInfoCollectorPtr timing_collector) const override;

  void BeforeInvokeMethod(
      const LynxModule::MethodMetadata& method,
      const std::unique_ptr<pub::Value>& args,
      const NativeModuleInfoCollectorPtr& timing_collector) override;
  void OnCallbackInvoked(const NativeModuleInfoCollectorPtr& timing,
                         ModuleCallback* callback) override;
  void SetTemplateUrl(const std::string& url) override;

  void AddInterceptor(std::unique_ptr<ModuleInterceptor> interceptor);

 private:
  base::InlineVector<std::shared_ptr<ModuleInterceptor>, 4> interceptors_;
};

}  // namespace piper
}  // namespace lynx
#endif  // CORE_RUNTIME_BINDINGS_JSI_MODULES_MODULE_INTERCEPTOR_H_
