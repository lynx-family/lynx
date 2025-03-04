// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_PUBLIC_JSB_LYNX_NATIVE_MODULE_H_
#define CORE_PUBLIC_JSB_LYNX_NATIVE_MODULE_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

#include "base/include/closure.h"
#include "base/include/debug/lynx_error.h"
#include "base/include/expected.h"
#include "core/public/jsb/lynx_module_callback.h"
#include "core/public/lynx_runtime_proxy.h"
#include "core/public/pub_value.h"

// TODO(liyanbo.monster): after remove native promise, delete this.
#if OS_IOS || OS_TVOS || OS_OSX
#include "core/runtime/bindings/jsi/modules/lynx_module_timing.h"
#include "core/runtime/bindings/jsi/modules/module_delegate.h"
#include "core/runtime/jsi/jsi.h"
#endif

namespace lynx {
namespace piper {

struct NativeModuleMethod {
  std::string name;
  size_t args_count;
  NativeModuleMethod(const std::string& method_name, size_t count)
      : name(method_name), args_count(count) {}
};

using NativeModuleMethods = std::unordered_map<std::string, NativeModuleMethod>;

/*
 * Upper-level modules can inherit from LynxNativeModule to register their own
 * JSB.
 */
class LynxNativeModule {
 public:
  class Delegate {
   public:
    virtual void InvokeCallback(
        const std::shared_ptr<LynxModuleCallback>& callback) = 0;
    virtual void RunOnJSThread(base::closure func) = 0;
    virtual void RunOnPlatformThread(base::closure func) = 0;
    virtual const std::shared_ptr<pub::PubValueFactory>& GetValueFactory() = 0;
    virtual void OnErrorOccurred(const std::string& module_name,
                                 const std::string& method_name,
                                 base::LynxError error) = 0;
  };

  using NativeModuleInvocation = std::unique_ptr<pub::Value> (
      LynxNativeModule::*)(std::unique_ptr<pub::Value>, const CallbackMap&);

  LynxNativeModule(std::shared_ptr<pub::PubValueFactory> value_factory)
      : value_factory_(std::move(value_factory)) {}

  LynxNativeModule() {}

  virtual ~LynxNativeModule() = default;

  // Find invocation you registered and call invocation with args and callbacks
  virtual base::expected<std::unique_ptr<pub::Value>, std::string> InvokeMethod(
      const std::string& method_name, std::unique_ptr<pub::Value> args,
      size_t count, const CallbackMap& callbacks) = 0;

  // Returns a list of NativeModuleMethod you registered
  virtual const NativeModuleMethods& GetMethodList() const { return methods_; }

  void SetDelegate(std::weak_ptr<Delegate> delegate) { delegate_ = delegate; }
  void SetRuntimeProxy(std::weak_ptr<shell::LynxRuntimeProxy> proxy) {
    runtime_proxy_ = proxy;
  }

  virtual void Destroy() {}

  std::shared_ptr<pub::PubValueFactory> GetValueFactory() {
    return value_factory_;
  }

// TODO(liyanbo.monster): after remove native promise, delete this.
#if OS_IOS || OS_TVOS || OS_OSX
  virtual void EnterInvokeScope(
      Runtime* rt, std::shared_ptr<ModuleDelegate> module_delegate) {}
  virtual void ExitInvokeScope() {}
  virtual std::optional<piper::Value> TryGetPromiseRet() {
    return std::nullopt;
  }
#endif

 protected:
  std::weak_ptr<Delegate> delegate_;
  std::weak_ptr<shell::LynxRuntimeProxy> runtime_proxy_;
  NativeModuleMethods methods_;
  std::shared_ptr<pub::PubValueFactory> value_factory_;
};

}  // namespace piper
}  // namespace lynx

#endif  // CORE_PUBLIC_JSB_LYNX_NATIVE_MODULE_H_
