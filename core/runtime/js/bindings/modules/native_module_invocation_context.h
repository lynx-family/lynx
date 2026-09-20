// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_JS_BINDINGS_MODULES_NATIVE_MODULE_INVOCATION_CONTEXT_H_
#define CORE_RUNTIME_JS_BINDINGS_MODULES_NATIVE_MODULE_INVOCATION_CONTEXT_H_

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "core/public/jsb/lynx_module_callback.h"
#include "core/runtime/common/native_module_interception.h"
#include "core/runtime/js/jsi/jsi.h"

namespace lynx::runtime::js {
class NativeModuleRecordObserver;

// One BTS invocation shared by explicit callbacks and native Promise callbacks.
// Only native metadata and weak consumers survive the synchronous call.
class NativeModuleInvocationContext {
 public:
  class Scope {
   public:
    explicit Scope(std::shared_ptr<NativeModuleInvocationContext> current);
    ~Scope();
    Scope(const Scope&) = delete;
    Scope& operator=(const Scope&) = delete;

   private:
    std::shared_ptr<NativeModuleInvocationContext> previous_;
  };
  static std::shared_ptr<NativeModuleInvocationContext> Current();
  // BTS observation and interception are available only with ENABLE_INSPECTOR.
  static std::shared_ptr<NativeModuleInvocationContext> Create(
      std::weak_ptr<NativeModuleRecordObserver> observer,
      const std::string& module, const std::string& method,
      base::LynxEntityId view);

  NativeModuleInvocationContext(
      std::weak_ptr<NativeModuleRecordObserver> observer, std::string module,
      std::string method,
      base::LynxEntityId view = base::kUnavailableLynxEntityId);

  shell::InterceptResult Call(Runtime& rt, const Value* args, size_t count,
                              std::vector<Value>& rewritten);
  std::optional<lepus::Value> Result(Runtime& rt, Value& result);
  // Apply callback patches before JS conversion and snapshot the effective args
  // for observation. Implicit callbacks (-1) are not part of DevTool coverage.
  std::optional<lepus::Value> PrepareCallback(
      int argument_index, std::unique_ptr<pub::Value>& args);
  void RecordCallback(int argument_index,
                      std::optional<lepus::Value> result) const;

  bool HasObserver() const;
  lepus::Value BuildInvokeRecord(lepus::Value arguments,
                                 const CallbackMap& callbacks, bool success,
                                 std::optional<lepus::Value> result,
                                 int32_t error_code,
                                 const std::string& error_message) const;
  lepus::Value BuildCallbackRecord(int argument_index,
                                   lepus::Value result) const;
  void EmitRecord(const lepus::Value& record) const;

  int64_t invocation_id() const { return info_->id; }
  const std::string& module_name() const { return info_->module; }
  const std::string& method_name() const { return info_->method; }

 private:
  const std::shared_ptr<const NativeModuleInvocationInfo> info_;
  const std::weak_ptr<NativeModuleRecordObserver> observer_;
  std::shared_ptr<NativeModuleInterception> interception_;
};
}  // namespace lynx::runtime::js
#endif  // CORE_RUNTIME_JS_BINDINGS_MODULES_NATIVE_MODULE_INVOCATION_CONTEXT_H_
