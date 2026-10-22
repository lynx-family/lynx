// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_JS_BINDINGS_MODULES_HOST_SCRIPT_MODULE_INTERCEPTOR_H_
#define CORE_RUNTIME_JS_BINDINGS_MODULES_HOST_SCRIPT_MODULE_INTERCEPTOR_H_

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "core/runtime/js/bindings/modules/module_interceptor.h"
#include "core/shell/host_script/runtime/interceptor.h"

namespace lynx::runtime::js {
// The registered instance is stateless; each active call gets its own instance.
class HostScriptModuleInterceptor final : public ModuleInterceptor {
 public:
  std::shared_ptr<ModuleInterceptor> CreateInvocation(
      const std::string& module, const std::string& method,
      base::LynxEntityId view) override;
  void RewriteArguments(Runtime& rt, const Value* args, size_t count,
                        std::vector<Value>& replacement) override;
  bool HandlesCall() const override;
  bool RewriteResult(Runtime& rt, Value& result) override;
  void BeforeCallback(int argument_index,
                      std::unique_ptr<pub::Value>& args) override;
  ModuleInterceptorResult InterceptModuleMethod(
      const std::shared_ptr<LynxModule>& module,
      const LynxModule::MethodMetadata& method, Runtime* rt,
      const std::shared_ptr<ModuleDelegate>& delegate, const Value* args,
      size_t count, const std::unique_ptr<pub::Value>& pub_args,
      const CallbackMap& callbacks,
      NativeModuleInfoCollectorPtr timing_collector) const override;
  void SetTemplateUrl(const std::string&) override {}

 private:
  lepus::Value BuildEvent() const;
  std::shared_ptr<shell::Interceptor> Provider(shell::InterceptKind kind) const;
  std::weak_ptr<shell::Interceptor> owner_;
  int64_t id_ = 0;
  base::LynxEntityId view_ = 0;
  std::string module_;
  std::string method_;
  shell::InterceptResult decision_;
};
}  // namespace lynx::runtime::js
#endif  // CORE_RUNTIME_JS_BINDINGS_MODULES_HOST_SCRIPT_MODULE_INTERCEPTOR_H_
