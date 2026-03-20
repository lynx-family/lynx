// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_PUBLIC_JSB_LYNX_EXTENSION_MODULE_H_
#define CORE_PUBLIC_JSB_LYNX_EXTENSION_MODULE_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

#include "base/include/fml/task_runner.h"
#include "core/public/jsb/lynx_native_module.h"
#include "core/public/ui_delegate.h"
#include "core/public/vsync_observer_interface.h"

namespace lynx {
namespace piper {

class LynxExtensionModule : public LynxNativeModule {
 public:
  virtual void SetLynxViewCreatedState(tasm::UIDelegate* ui_delegate) = 0;
  virtual void SetLynxViewDestroyedState() = 0;
  virtual void SetRuntimeInitState(
      const fml::RefPtr<fml::TaskRunner>& task_runner) = 0;
  virtual void SetRuntimeAttachedState(
      void* opaque_env,
      const std::shared_ptr<runtime::IVSyncObserver>& vsync_observer) = 0;
  virtual void SetRuntimeReadyState(void* opaque_env, void* opaque_lynx,
                                    const std::string& url) = 0;
  virtual void SetRuntimeDetachedState() = 0;

  virtual void SetEnteringForegroundState() = 0;
  virtual void SetEnteringBackgroundState() = 0;

  void RegisterMethod(
      const piper::NativeModuleMethod& method,
      piper::LynxNativeModule::NativeModuleInvocation invocation) {
    methods_.emplace(method.name, method);
    invocations_.emplace(method.name, std::move(invocation));
  }

 protected:
  std::unordered_map<std::string,
                     piper::LynxNativeModule::NativeModuleInvocation>
      invocations_;
};

}  // namespace piper
}  // namespace lynx

#endif  // CORE_PUBLIC_JSB_LYNX_EXTENSION_MODULE_H_
