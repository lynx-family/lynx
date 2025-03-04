// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_BINDINGS_JSI_MODULES_LYNX_MODULE_IMPL_H_
#define CORE_RUNTIME_BINDINGS_JSI_MODULES_LYNX_MODULE_IMPL_H_

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/include/compiler_specific.h"
#include "base/include/vector.h"
#include "core/public/jsb/lynx_native_module.h"
#include "core/runtime/bindings/jsi/modules/lynx_module.h"
#include "core/value_wrapper/value_impl_lepus.h"

namespace lynx {
namespace piper {
struct InvokeInfo;

class LynxModuleImpl : public LynxModule, public LynxNativeModule::Delegate {
 public:
  LynxModuleImpl(
      const std::string& name, const std::shared_ptr<ModuleDelegate>& delegate,
      const std::shared_ptr<lynx::piper::LynxNativeModule>& native_module)
      : LynxModule(name, delegate), native_module_(native_module) {
    auto factory = native_module->GetValueFactory();
    value_factory_ = factory ? std::move(factory)
                             : std::make_shared<pub::PubValueFactoryDefault>();
    SetMethodMetadata();
  }

  void Destroy() override;

  base::expected<piper::Value, piper::JSINativeException> invokeMethod(
      const MethodMetadata& method, Runtime* rt, const piper::Value* args,
      size_t count) override;

  // LynxNativeModule::Delegate
  void InvokeCallback(
      const std::shared_ptr<LynxModuleCallback>& callback) override;
  void RunOnJSThread(base::closure func) override;
  void RunOnPlatformThread(base::closure func) override;
  const std::shared_ptr<pub::PubValueFactory>& GetValueFactory() override;
  void OnErrorOccurred(const std::string& module_name,
                       const std::string& method_name,
                       base::LynxError error) override;

 private:
  void SetMethodMetadata();
  InvokeInfo* CurrentInvokeInfo();

  std::shared_ptr<LynxNativeModule> native_module_ = nullptr;
  std::shared_ptr<pub::PubValueFactory> value_factory_;

  std::vector<InvokeInfo*> invoke_scopes_;
};

}  // namespace piper
}  // namespace lynx

#endif  // CORE_RUNTIME_BINDINGS_JSI_MODULES_LYNX_MODULE_IMPL_H_
