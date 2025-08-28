// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/bindings/lepus/modules/lynx_lepus_module_manager.h"

#include "core/runtime/vm/lepus/jsvalue_helper.h"
#include "core/runtime/vm/lepus/quick_context.h"
#include "core/runtime/vm/lepus/vm_context.h"

namespace lynx {
namespace lepus {

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
    // native_module->SetDelegate(lynx_jsi_module);
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
