// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SHELL_HOST_SCRIPT_RUNTIME_HOST_SCRIPT_MODULE_INTERCEPTOR_H_
#define CORE_SHELL_HOST_SCRIPT_RUNTIME_HOST_SCRIPT_MODULE_INTERCEPTOR_H_

#include <memory>
#include <vector>

#include "core/runtime/js/bindings/modules/module_interceptor.h"

namespace lynx::shell {

/**
 * Exposes BTS native module calls to Host Script as jsb.call, jsb.result and
 * jsb.callback. Each stage is dispatched only while a BTS provider handles it
 * for a live view. A mock decision is final: it replaces the native call and
 * skips the result and callback stages of that invocation. Native Promises are
 * not covered.
 */
class HostScriptModuleInterceptor final
    : public runtime::js::ModuleInterceptor {
 public:
  // Adds the interceptor ahead of the built-in ones of every module manager
  // initialized afterwards. Idempotent.
  static void Register();

  std::shared_ptr<runtime::js::ModuleCallContext> BeforeConvertArgs(
      const std::shared_ptr<runtime::js::LynxModule>& module,
      const runtime::js::LynxModule::MethodMetadata& method,
      runtime::js::Runtime* rt, const runtime::js::Value* args, size_t count,
      std::vector<runtime::js::Value>& rewritten) override;
  runtime::js::ModuleInterceptorResult InterceptModuleMethod(
      const std::shared_ptr<runtime::js::LynxModule>& module,
      const runtime::js::LynxModule::MethodMetadata& method,
      runtime::js::Runtime* rt,
      const std::shared_ptr<runtime::js::ModuleDelegate>& delegate,
      const runtime::js::Value* args, size_t count,
      const std::unique_ptr<pub::Value>& pub_args,
      const runtime::CallbackMap& callbacks,
      runtime::js::NativeModuleInfoCollectorPtr timing_collector,
      runtime::js::ModuleCallContext* context = nullptr) const override;
  bool AfterInvokeMethod(runtime::js::Runtime* rt,
                         runtime::js::ModuleCallContext* context,
                         runtime::js::Value& result) override;
  void OnInvokeMethodEnd(
      runtime::js::ModuleCallContext* context,
      const std::shared_ptr<runtime::js::ModuleDelegate>& delegate,
      const runtime::CallbackMap& callbacks, bool args_converted) override;
  void WillInvokeCallback(runtime::js::ModuleCallback& callback,
                          std::unique_ptr<pub::Value>& args) override;

 private:
  class Call;
  static Call* GetCall(runtime::js::ModuleCallContext* context);
};

}  // namespace lynx::shell

#endif  // CORE_SHELL_HOST_SCRIPT_RUNTIME_HOST_SCRIPT_MODULE_INTERCEPTOR_H_
