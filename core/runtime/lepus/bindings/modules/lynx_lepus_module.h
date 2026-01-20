// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_LEPUS_BINDINGS_MODULES_LYNX_LEPUS_MODULE_H_
#define CORE_RUNTIME_LEPUS_BINDINGS_MODULES_LYNX_LEPUS_MODULE_H_
#include <memory>
#include <string>
#include <utility>

#include "base/include/value/base_value.h"
#include "core/public/jsb/lynx_native_module.h"
#include "core/value_wrapper/value_impl_lepus.h"

namespace lynx {
namespace lepus {

// LepusModule is a further encapsulation of NativeModule, which holds
// NativeModule, representing the Module object in JS. NativeModule is
// implemented by Android & iOS, and LepusModule is responsible for Binding with
// Lepus
class LynxLepusModule : public lepus::RefCounted {
 public:
  LynxLepusModule(
      const std::string& name,
      const std::shared_ptr<lynx::piper::LynxNativeModule>& native_module) {
    value_factory_ = std::make_shared<pub::PubValueFactoryDefault>();
    native_module_ = native_module;
    name_ = name;
  }
  virtual ~LynxLepusModule() = default;

  // Invoke method of LynxModule, which is a method of NativeModule
  Value InvokeMethod(Context* context, const std::string& method_name,
                     const lepus::Value* args, size_t count);
  // Convert LepusModule to LepusValue
  Value ToLepusValue(Context* context);
  // Convert LepusValue to LepusModule
  static LynxLepusModule* ToRuntimeValue(const lepus::Value& lepus_value);

  // Set execute delegate
  void SetExecuteDelegate(
      const std::shared_ptr<piper::LynxNativeModule::Delegate>& delegate) {
    execute_delegate_ = delegate;
  }

  // refcount
  lepus::RefType GetRefType() const override {
    return lepus::RefType::kOtherType;
  };

 private:
  // The object that actually calls LynxModule
  std::shared_ptr<piper::LynxNativeModule> native_module_ = nullptr;
  std::shared_ptr<pub::PubValueFactory> value_factory_;
  // Module name
  std::string name_;
  // Delegate
  std::shared_ptr<piper::LynxNativeModule::Delegate> execute_delegate_;
};

}  // namespace lepus
}  // namespace lynx

#endif  // CORE_RUNTIME_LEPUS_BINDINGS_MODULES_LYNX_LEPUS_MODULE_H_
