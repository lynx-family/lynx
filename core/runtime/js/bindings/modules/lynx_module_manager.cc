// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/js/bindings/modules/lynx_module_manager.h"

#include "core/public/jsb/extension_module_factory.h"
#include "core/runtime/js/bindings/interceptor/interceptor_factory.h"
#include "core/runtime/js/bindings/modules/lynx_jsi_module.h"

namespace lynx {
namespace piper {

LynxModuleManager::~LynxModuleManager() {
  LOGI("NativeModule: ~LynxJSIModuleManager");
  for (auto &module : module_map_) {
    module.second->Destroy();
  }
}

void LynxModuleManager::initBindingPtr(
    std::weak_ptr<LynxModuleManager> weak_manager,
    const std::shared_ptr<ModuleDelegate> &delegate) {
  bindingPtr = std::make_shared<lynx::piper::LynxJSIModuleBinding>(
      BindingFunc(weak_manager, delegate));
  pub::LynxNativeModuleManager::SetModuleDelegate(delegate);
#if ENABLE_TESTBENCH_REPLAY
  this->delegate_ = delegate;
#endif
}

std::shared_ptr<LynxModule> LynxModuleManager::GetModule(
    const std::string &name, const std::shared_ptr<ModuleDelegate> &delegate) {
  // find module from cache
  auto itr = module_map_.find(name);
  if (itr != module_map_.end()) {
    return itr->second;
  }

  // create new native module
  std::shared_ptr<piper::LynxNativeModule> native_module =
      pub::LynxNativeModuleManager::GetModule(name);
  if (!native_module && extension_module_factory_) {
    native_module = extension_module_factory_->CreateModule(name);
  }
  if (native_module) {
    auto lynx_jsi_module =
        std::make_shared<LynxJSIModule>(name, delegate, native_module);
    // set delegate & proxy
    native_module->SetDelegate(lynx_jsi_module);
    native_module->SetRuntimeProxy(runtime_proxy);
    native_module->SetContextID(context_id_);
    // set interceptor
    lynx_jsi_module->SetModuleInterceptor(group_interceptor_);
#if ENABLE_TESTBENCH_RECORDER
    lynx_jsi_module->SetRecordID(record_id_);
#endif
    itr = module_map_.emplace(name, std::move(lynx_jsi_module)).first;
    return itr->second;
  }
  return std::shared_ptr<LynxModule>(nullptr);
}

LynxModuleProviderFunction LynxModuleManager::BindingFunc(
    std::weak_ptr<LynxModuleManager> weak_manager,
    const std::shared_ptr<ModuleDelegate> &delegate) {
  return [weak_manager, delegate](const std::string &name) {
    auto manager = weak_manager.lock();
    if (manager) {
      auto ptr = manager->GetModule(name, delegate);
      if (ptr != nullptr) {
        return ptr;
      }
    }
    // ptr == nullptr
    // issue: #1510
    if (!LynxModuleUtils::LynxModuleManagerAllowList::get().count(name)) {
      LOGW("NativeModule: LynxJSIModule, try to find module: "
           << name << "failed. manager: " << manager);
    } else {
      LOGV("NativeModule: LynxJSIModule, module: "
           << name << " is not found but it is in the allow list");
    }
    return std::shared_ptr<LynxModule>(nullptr);
  };
}

void LynxModuleManager::InitModuleInterceptor() {
  group_interceptor_ = InterceptorFactory::CreateGroupInterceptor();
}

void LynxModuleManager::SetTemplateUrl(const std::string &url) {
  if (group_interceptor_) {
    group_interceptor_->SetTemplateUrl(url);
  }
}

}  // namespace piper
}  // namespace lynx
