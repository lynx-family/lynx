// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_JS_BINDINGS_MODULES_NATIVE_MODULE_INVOCATION_CONTEXT_H_
#define CORE_RUNTIME_JS_BINDINGS_MODULES_NATIVE_MODULE_INVOCATION_CONTEXT_H_

#include <cstdint>
#include <memory>
#include <optional>
#include <string>

#include "base/include/value/base_value.h"
#include "core/public/jsb/lynx_module_callback.h"

namespace lynx {
namespace runtime {
namespace js {

class NativeModuleRecordObserver;

// Carries the identity of a single NativeModule invocation. Callback contexts
// derived from it keep the same invocation id and record their own argument
// index.
class NativeModuleInvocationContext {
 public:
  NativeModuleInvocationContext(
      std::weak_ptr<NativeModuleRecordObserver> observer,
      std::string module_name, std::string method_name);
  ~NativeModuleInvocationContext() = default;

  std::shared_ptr<NativeModuleInvocationContext> WithCallbackArgumentIndex(
      int32_t callback_argument_index) const;

  lepus::Value BuildInvokeRecord(lepus::Value arguments,
                                 const CallbackMap& callbacks, bool success,
                                 std::optional<lepus::Value> result,
                                 int32_t error_code,
                                 const std::string& error_message) const;

  lepus::Value BuildCallbackRecord(lepus::Value result) const;

  void EmitRecord(const lepus::Value& record) const;

  int64_t invocation_id() const { return invocation_id_; }
  const std::string& module_name() const { return module_name_; }
  const std::string& method_name() const { return method_name_; }
  int32_t callback_argument_index() const { return callback_argument_index_; }

 private:
  NativeModuleInvocationContext(
      std::weak_ptr<NativeModuleRecordObserver> observer, int64_t invocation_id,
      std::string module_name, std::string method_name,
      int32_t callback_argument_index);

  const std::weak_ptr<NativeModuleRecordObserver> observer_;
  const int64_t invocation_id_;
  const std::string module_name_;
  const std::string method_name_;
  const int32_t callback_argument_index_;
};

}  // namespace js
}  // namespace runtime
}  // namespace lynx

#endif  // CORE_RUNTIME_JS_BINDINGS_MODULES_NATIVE_MODULE_INVOCATION_CONTEXT_H_
