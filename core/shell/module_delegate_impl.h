// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SHELL_MODULE_DELEGATE_IMPL_H_
#define CORE_SHELL_MODULE_DELEGATE_IMPL_H_

#include <memory>
#include <string>

#include "base/include/closure.h"
#include "core/runtime/bindings/jsi/modules/module_delegate.h"
#include "core/runtime/piper/js/lynx_runtime.h"
#include "core/shell/lynx_actor_specialization.h"

namespace lynx {
namespace shell {

class ModuleDelegateImpl : public piper::ModuleDelegate {
 public:
  explicit ModuleDelegateImpl(
      const std::shared_ptr<LynxActor<runtime::LynxRuntime>>& runtime_actor,
      const std::shared_ptr<LynxActor<NativeFacade>>& facade_actor = nullptr)
      : runtime_actor_(runtime_actor), facade_actor_(facade_actor) {}
  ~ModuleDelegateImpl() override = default;

  ModuleDelegateImpl(const ModuleDelegateImpl&) = delete;
  ModuleDelegateImpl& operator=(const ModuleDelegateImpl&) = delete;
  ModuleDelegateImpl(ModuleDelegateImpl&&) = delete;
  ModuleDelegateImpl& operator=(ModuleDelegateImpl&&) = delete;

  int64_t RegisterJSCallbackFunction(piper::Function func) override;
  void CallJSCallback(const std::shared_ptr<piper::ModuleCallback>& callback,
                      int64_t id_to_delete =
                          piper::ModuleCallback::kInvalidCallbackId) override;
  void OnErrorOccurred(base::LynxError error) override;
  void OnMethodInvoked(const std::string& module_name,
                       const std::string& method_name, int32_t code) override;

  void FlushJSBTiming(piper::NativeModuleInfo timing) override;

  void RunOnJSThread(base::closure func) override;

  void RunOnPlatformThread(base::closure func) override;

 private:
  std::shared_ptr<LynxActor<runtime::LynxRuntime>> runtime_actor_;
  std::shared_ptr<LynxActor<NativeFacade>> facade_actor_;
};

}  // namespace shell
}  // namespace lynx

#endif  // CORE_SHELL_MODULE_DELEGATE_IMPL_H_
