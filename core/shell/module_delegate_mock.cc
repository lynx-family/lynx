// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <utility>

#include "core/shell/module_delegate_impl.h"

namespace lynx {
namespace shell {
int64_t ModuleDelegateImpl::RegisterJSCallbackFunction(piper::Function func) {
  return 0;
}

void ModuleDelegateImpl::CallJSCallback(
    const std::shared_ptr<piper::ModuleCallback>& callback,
    int64_t id_to_delete) {}

void ModuleDelegateImpl::OnErrorOccurred(base::LynxError error) {}

void ModuleDelegateImpl::OnMethodInvoked(const std::string& module_name,
                                         const std::string& method_name,
                                         int32_t code) {}

void ModuleDelegateImpl::FlushJSBTiming(piper::NativeModuleInfo timing) {}

void ModuleDelegateImpl::RunOnJSThread(base::closure func) {}

void ModuleDelegateImpl::RunOnPlatformThread(base::closure func) {}

}  // namespace shell
}  // namespace lynx
