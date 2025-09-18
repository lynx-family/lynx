// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_BINDINGS_COMMON_MODULES_LYNX_NATIVE_MODULE_MANAGER_H_
#define CORE_RUNTIME_BINDINGS_COMMON_MODULES_LYNX_NATIVE_MODULE_MANAGER_H_

#include <memory>
#include <string>
#include <utility>

#include "core/public/jsb/native_module_factory.h"
#include "core/runtime/bindings/jsi/modules/module_delegate.h"

namespace lynx {
namespace shell {
template <typename T>
class LynxActor;
class LynxEngine;
class NativeFacade;
}  // namespace shell

namespace pub {

// LynxNativeModuleManager manages all registered NativeModuleFactory instances.
// It has subclasses LynxJSIModuleManager, LynxNAPIModuleManager, and
// LynxLepusModuleManager. Through LynxNativeModuleManager, NativeModule can be
// obtained. The subclasses encapsulate NativeModule into LynxJSIModule,
// LynxNAPIModule, and LynxLepusModule respectively, corresponding to the JS FFI
// (Foreign Function Interface).
class LynxNativeModuleManager {
 public:
  enum class ManagerType {
    NATIVE,
    JSI,
    NAPI,
    LEPUS,
  };
  LynxNativeModuleManager() = default;
  virtual ~LynxNativeModuleManager() = default;

  // Has unique_ptr ,Delete Copy Constructor.
  LynxNativeModuleManager(const LynxNativeModuleManager &) = delete;
  LynxNativeModuleManager &operator=(const LynxNativeModuleManager &) = delete;

  LynxNativeModuleManager(LynxNativeModuleManager &&) = default;
  LynxNativeModuleManager &operator=(LynxNativeModuleManager &&) = default;

  std::shared_ptr<piper::LynxNativeModule> GetModule(const std::string &name);

  // Set ModuleFactory and Used for create Module
  virtual void SetModuleFactory(
      std::unique_ptr<piper::NativeModuleFactory> module_factory) {
    if (module_factory) {
      module_factories_.push_back(std::move(module_factory));
    }
  };
  // TODO(zhangqun.29): When iOS Platform no longer holds moduleFactory in the
  // form of weakPtr, can directly use SetModuleFactory Used for create
  // PlatformModule
  virtual void SetPlatformModuleFactory(
      std::shared_ptr<piper::NativeModuleFactory> module_factory);

  // Set delegate for module manager.
  // TODO(zhangqun.29): delete this!! & use native module delegate
  virtual void SetModuleDelegate(
      std::shared_ptr<piper::ModuleDelegate> delegate) {
    delegate_ = delegate;
  }

  virtual std::shared_ptr<piper::ModuleDelegate> GetModuleDelegate() {
    return delegate_;
  }

  virtual piper::NativeModuleFactory *GetPlatformModuleFactory();

  // used for LynxRecorder
  virtual void SetRecordID(int64_t record_id) { record_id_ = record_id; };
  int64_t record_id_ = 0;

  virtual ManagerType Type() { return ManagerType::NATIVE; };

  void SetEngineActor(
      const std::shared_ptr<shell::LynxActor<shell::LynxEngine>> &engine_actor);
  void SetFacadeActor(
      const std::shared_ptr<shell::LynxActor<shell::NativeFacade>>
          &facade_actor);

 protected:
  const std::shared_ptr<shell::LynxActor<shell::LynxEngine>> &GetEngineActor() {
    return engine_actor_;
  }
  const std::shared_ptr<shell::LynxActor<shell::NativeFacade>> &
  GetFacadeActor() {
    return facade_actor_;
  }

 private:
  // Managed by LynxNativeModuleManager
  base::InlineVector<std::unique_ptr<piper::NativeModuleFactory>, 4>
      module_factories_;
  // Maybe use by platform.
  // When re-attaching in standalone mode, it needs to support modification, so
  // it needs to be accessed by the platform, so it is placed independently.
  std::shared_ptr<piper::NativeModuleFactory> platform_module_factory_;
  // TODO(zhangqun.29): delete this!! & use native module delegate
  std::shared_ptr<piper::ModuleDelegate> delegate_;

  // use to create delegate
  std::shared_ptr<shell::LynxActor<shell::LynxEngine>> engine_actor_;
  std::shared_ptr<shell::LynxActor<shell::NativeFacade>> facade_actor_;
};
}  // namespace pub

}  // namespace lynx

#endif  // CORE_RUNTIME_BINDINGS_COMMON_MODULES_LYNX_NATIVE_MODULE_MANAGER_H_
