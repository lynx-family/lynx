// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_BINDINGS_JSI_MODULES_MODULE_DELEGATE_H_
#define CORE_RUNTIME_BINDINGS_JSI_MODULES_MODULE_DELEGATE_H_

#include <memory>
#include <string>

#include "base/include/closure.h"
#include "core/runtime/bindings/jsi/modules/lynx_jsi_module_callback.h"

namespace lynx {
namespace piper {
struct NativeModuleInfo;

class ModuleDelegate {
 public:
  ModuleDelegate() = default;
  virtual ~ModuleDelegate() = default;

  ModuleDelegate(const ModuleDelegate&) = delete;
  ModuleDelegate& operator=(const ModuleDelegate&) = delete;
  ModuleDelegate(ModuleDelegate&&) = delete;
  ModuleDelegate& operator=(ModuleDelegate&&) = delete;

  virtual int64_t RegisterJSCallbackFunction(Function func) = 0;
  // ret just is post to js thread or not
  virtual void CallJSCallback(
      const std::shared_ptr<ModuleCallback>& callback,
      int64_t id_to_delete = ModuleCallback::kInvalidCallbackId) = 0;
  virtual void OnErrorOccurred(base::LynxError error) = 0;
  virtual void OnMethodInvoked(const std::string& module_name,
                               const std::string& method_name,
                               int32_t code) = 0;
  virtual void FlushJSBTiming(piper::NativeModuleInfo timing) = 0;
  // for android, MethodInvoker will handle a set of promise
  // on js thread, have no choice but provide this method
  virtual void RunOnJSThread(base::closure func) = 0;
  virtual void RunOnPlatformThread(base::closure func) = 0;
};

}  // namespace piper
}  // namespace lynx

#endif  // CORE_RUNTIME_BINDINGS_JSI_MODULES_MODULE_DELEGATE_H_
