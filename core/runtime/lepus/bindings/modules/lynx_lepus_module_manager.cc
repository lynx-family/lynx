// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/lepus/bindings/modules/lynx_lepus_module_manager.h"

#include "core/runtime/lepus/vm_context.h"
#include "core/runtime/lepusng/jsvalue_helper.h"
#include "core/runtime/lepusng/quick_context.h"

namespace lynx {
namespace lepus {

LynxLepusModuleManager::LynxLepusModuleManager(
    pub::LynxNativeModuleManager&& native_module_manager)
    : pub::LynxNativeModuleManager(std::move(native_module_manager)) {
  execute_delegate_ = std::make_shared<LepusModuleDelegate>();
  execute_delegate_->SetEngineActor(
      pub::LynxNativeModuleManager::GetEngineActor());
  execute_delegate_->SetFacadeActor(
      pub::LynxNativeModuleManager::GetFacadeActor());
}

Value LynxLepusModuleManager::GetModule(Context* context,
                                        const std::string& name) {
  // find module from cache
  auto itr = module_map_.find(name);
  if (itr != module_map_.end()) {
    return itr->second->ToLepusValue(context);
  }

  // create new native module
  std::shared_ptr<piper::LynxNativeModule> native_module =
      pub::LynxNativeModuleManager::GetModule(name);

  if (native_module) {
    fml::RefPtr<LynxLepusModule> lynx_lepus_module =
        fml::MakeRefCounted<LynxLepusModule>(name, native_module);
    lynx_lepus_module->SetExecuteDelegate(execute_delegate_);
    native_module->SetDelegate(execute_delegate_);
    itr = module_map_.emplace(name, lynx_lepus_module).first;
    return itr->second->ToLepusValue(context);
  }
  LOGE("NativeModules: "
       << "LynxLepusModuleManager::GetModule failed, ModuleName: " << name);
  return Value();
}

LynxLepusModuleManager::~LynxLepusModuleManager() {
  LOGI("NativeModule: ~LynxLepusModuleManager");
  module_map_.clear();
}

}  // namespace lepus
}  // namespace lynx
