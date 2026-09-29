// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_LYNX_DEVTOOL_NATIVE_MODULE_NATIVE_MODULE_INVOCATION_CONTEXT_IMPL_H_
#define DEVTOOL_LYNX_DEVTOOL_NATIVE_MODULE_NATIVE_MODULE_INVOCATION_CONTEXT_IMPL_H_

#include <memory>
#include <string>

#include "core/inspector/observer/native_module_record_observer.h"

namespace lynx {
namespace devtool {

class NativeModuleInvocationContextImpl final
    : public runtime::js::NativeModuleInvocationContext {
 public:
  NativeModuleInvocationContextImpl(
      std::weak_ptr<runtime::js::NativeModuleRecordObserver> observer,
      std::string module_name, std::string method_name);

  std::shared_ptr<runtime::js::NativeModuleInvocationContext>
  WithCallbackArgumentIndex(int32_t callback_argument_index) const override;

  void OnInvoke(lepus::Value arguments, const runtime::CallbackMap& callbacks,
                bool success, std::optional<lepus::Value> result,
                int32_t error_code,
                const std::string& error_message) const override;
  void OnCallback(lepus::Value result) const override;

 private:
  NativeModuleInvocationContextImpl(
      std::weak_ptr<runtime::js::NativeModuleRecordObserver> observer,
      int64_t invocation_id, std::string module_name, std::string method_name,
      int32_t callback_argument_index);

  const std::weak_ptr<runtime::js::NativeModuleRecordObserver> observer_;
  const int64_t invocation_id_;
  const std::string module_name_;
  const std::string method_name_;
  const int32_t callback_argument_index_;
};

}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_LYNX_DEVTOOL_NATIVE_MODULE_NATIVE_MODULE_INVOCATION_CONTEXT_IMPL_H_
