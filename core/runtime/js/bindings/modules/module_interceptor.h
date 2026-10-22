// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_JS_BINDINGS_MODULES_MODULE_INTERCEPTOR_H_
#define CORE_RUNTIME_JS_BINDINGS_MODULES_MODULE_INTERCEPTOR_H_

#include <memory>
#include <string>
#include <vector>

#include "base/include/vector.h"
#include "core/runtime/js/bindings/modules/lynx_module.h"
#include "core/runtime/js/bindings/modules/module_call_context.h"
#include "core/runtime/js/bindings/modules/module_delegate.h"
#include "core/runtime/js/jsi/jsi.h"

namespace lynx {
namespace runtime {
namespace js {
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
 *
 * Extended lifecycle when registered with invocation hooks, in order:
 *   BeforeConvertArgs before the arguments are converted
 *   BeforeInvokeMethod after the arguments are converted
 *   InterceptModuleMethod before the native module is called
 *   AfterInvokeMethod before the result is recorded and returned
 *   OnInvokeMethodEnd always, paired with BeforeConvertArgs
 * and for each callback the invocation registered:
 *   WillInvokeCallback before the callback arguments reach JavaScript
 *   OnCallbackInvoked  after the callback ran
 */
class ModuleInterceptor {
 public:
  virtual ModuleInterceptorResult InterceptModuleMethod(
      const std::shared_ptr<LynxModule>& module,
      const LynxModule::MethodMetadata& method, Runtime* rt,
      const std::shared_ptr<ModuleDelegate>& delegate, const Value* args,
      size_t count, const std::unique_ptr<pub::Value>& pub_args,
      const CallbackMap& callbacks,
      NativeModuleInfoCollectorPtr timing_collector,
      ModuleCallContext* context = nullptr) const {
    return {false, Value::null()};
  }

  // May return a context shared by this invocation and its callbacks. At most
  // one interceptor in a group may supply a context; it must always return its
  // own context type, even while inactive. Other interceptors return nullptr.
  // May also return replacement JS arguments in `rewritten`.
  // Leave it empty to preserve the arguments; replacements must keep count.
  // The caller owns the replacement storage throughout invokeMethod.
  virtual std::shared_ptr<ModuleCallContext> BeforeConvertArgs(
      const std::shared_ptr<LynxModule>& module,
      const LynxModule::MethodMetadata& method, Runtime* rt, const Value* args,
      size_t count, std::vector<Value>& rewritten) {
    return nullptr;
  }
  virtual void BeforeInvokeMethod(
      const LynxModule::MethodMetadata& method,
      const std::unique_ptr<pub::Value>& args,
      const NativeModuleInfoCollectorPtr& timing_collector) {}
  // Returns true if `result` was replaced.
  // Converted arguments belong to the native module and are not available here.
  virtual bool AfterInvokeMethod(Runtime* rt, ModuleCallContext* context,
                                 Value& result) {
    return false;
  }
  // Called even if conversion fails. Only the registered callbacks and state
  // are available; no arguments are borrowed from the invocation stack.
  virtual void OnInvokeMethodEnd(
      ModuleCallContext* context,
      const std::shared_ptr<ModuleDelegate>& delegate,
      const CallbackMap& callbacks, bool args_converted) {}

  // May replace the callback arguments.
  virtual void WillInvokeCallback(ModuleCallback& callback,
                                  std::unique_ptr<pub::Value>& args) {}
  virtual void OnCallbackInvoked(const NativeModuleInfoCollectorPtr& timing,
                                 ModuleCallback* callback) {}
  virtual ~ModuleInterceptor() = default;
  virtual void SetTemplateUrl(const std::string& url) {}
};

/**
 * Runs its interceptors in order. For chains with invocation hooks enabled,
 * resolving stops propagation: interceptors after the resolver receive neither
 * the result nor the callback hooks of that invocation.
 */
class GroupInterceptor : public ModuleInterceptor {
 public:
  ModuleInterceptorResult InterceptModuleMethod(
      const std::shared_ptr<LynxModule>& module,
      const LynxModule::MethodMetadata& method, Runtime* rt,
      const std::shared_ptr<ModuleDelegate>& delegate, const Value* args,
      size_t count, const std::unique_ptr<pub::Value>& pub_args,
      const CallbackMap& callbacks,
      NativeModuleInfoCollectorPtr timing_collector,
      ModuleCallContext* context = nullptr) const override;

  std::shared_ptr<ModuleCallContext> BeforeConvertArgs(
      const std::shared_ptr<LynxModule>& module,
      const LynxModule::MethodMetadata& method, Runtime* rt, const Value* args,
      size_t count, std::vector<Value>& rewritten) override;
  void BeforeInvokeMethod(
      const LynxModule::MethodMetadata& method,
      const std::unique_ptr<pub::Value>& args,
      const NativeModuleInfoCollectorPtr& timing_collector) override;
  bool AfterInvokeMethod(Runtime* rt, ModuleCallContext* context,
                         Value& result) override;
  void OnInvokeMethodEnd(ModuleCallContext* context,
                         const std::shared_ptr<ModuleDelegate>& delegate,
                         const CallbackMap& callbacks,
                         bool args_converted) override;
  void WillInvokeCallback(ModuleCallback& callback,
                          std::unique_ptr<pub::Value>& args) override;
  void OnCallbackInvoked(const NativeModuleInfoCollectorPtr& timing,
                         ModuleCallback* callback) override;
  void SetTemplateUrl(const std::string& url) override;

  // Legacy interceptors use only BeforeInvokeMethod, InterceptModuleMethod and
  // OnCallbackInvoked. Opt in to enable argument/result/callback rewriting and
  // per-invocation state for the chain, including stateless propagation bounds.
  void AddInterceptor(std::unique_ptr<ModuleInterceptor> interceptor,
                      bool enable_invocation_hooks = false);
  // Leading interceptors opt in to invocation hooks and run before the others.
  void AddLeadingInterceptor(std::unique_ptr<ModuleInterceptor> interceptor);
  bool HasInvocationHooks() const { return has_invocation_hooks_; }
  bool empty() const { return interceptors_.empty(); }

 private:
  template <typename Function>
  void ForEachTarget(const ModuleCallContext* context, Function function);

  base::InlineVector<std::unique_ptr<ModuleInterceptor>, 4> interceptors_;
  bool has_invocation_hooks_ = false;
};

}  // namespace js

}  // namespace runtime
}  // namespace lynx
#endif  // CORE_RUNTIME_JS_BINDINGS_MODULES_MODULE_INTERCEPTOR_H_
