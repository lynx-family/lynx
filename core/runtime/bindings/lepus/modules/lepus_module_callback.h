// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_BINDINGS_LEPUS_MODULES_LEPUS_MODULE_CALLBACK_H_
#define CORE_RUNTIME_BINDINGS_LEPUS_MODULES_LEPUS_MODULE_CALLBACK_H_

#include <memory>
#include <string>
#include <utility>

#include "base/include/value/base_value.h"
#include "core/public/jsb/lynx_module_callback.h"
#include "core/public/jsb/lynx_native_module.h"
#include "core/value_wrapper/value_impl_lepus.h"

namespace lynx {

namespace shell {
template <typename T>
class LynxActor;
class LynxEngine;
class NativeFacade;
}  // namespace shell

namespace lepus {

// ModuleCallback records all the information required for Closure
// execution.ModuleCallback should be called using ModuleDelegate's
// InvokeCallback.
// id: corresponds to the Closure cached in Context::LepusModuleManager。
// args_: Closure execution parameters, using the neutral data type pubValue.
// GetArgs is responsible for converting pubValue to the specific execution data
// type.
class LepusModuleCallback : public piper::LynxModuleCallback {
 public:
  explicit LepusModuleCallback(int64_t callback_id)
      : piper::LynxModuleCallback(callback_id) {}
  virtual ~LepusModuleCallback() = default;

  // Cache closure execution parameters
  void SetArgs(std::unique_ptr<pub::Value> args) override {
    args_ = std::move(args);
  };
  // Get closure execution parameters, convert pubValue to lepusValue.
  lepus::Value GetArgs();
  // Callback type
  Type GetType() const override { return Type::LEPUS; }

 private:
  std::unique_ptr<pub::Value> args_ = nullptr;
};

// ModuleDelegate represents the execution environment of the current Module,
// which allows PlatformModule to use certain methods.
class LepusModuleDelegate : public piper::LynxNativeModule::Delegate {
 public:
  LepusModuleDelegate();
  virtual ~LepusModuleDelegate() = default;

  void InvokeCallback(
      const std::shared_ptr<piper::LynxModuleCallback>& callback,
      base::MoveOnlyClosure<bool> invoke_pre_func = nullptr) override;
  // not supported yet.
  void RunOnJSThread(base::closure func) override{};
  // not supported yet.
  void RunOnPlatformThread(base::closure func) override{};
  void OnErrorOccurred(const std::string& module_name,
                       const std::string& method_name,
                       base::LynxError error) override;

  const std::shared_ptr<pub::PubValueFactory>& GetValueFactory() override {
    return value_factory_;
  };

  void SetEngineActor(
      const std::shared_ptr<shell::LynxActor<shell::LynxEngine>>& engine_actor);

  void SetFacadeActor(
      const std::shared_ptr<shell::LynxActor<shell::NativeFacade>>&
          facade_actor);

 private:
  std::shared_ptr<pub::PubValueFactory> value_factory_;
  std::shared_ptr<shell::LynxActor<shell::LynxEngine>> engine_actor_;
  std::shared_ptr<shell::LynxActor<shell::NativeFacade>> facade_actor_;
};
}  // namespace lepus
}  // namespace lynx

#endif  // CORE_RUNTIME_BINDINGS_LEPUS_MODULES_LEPUS_MODULE_CALLBACK_H_
