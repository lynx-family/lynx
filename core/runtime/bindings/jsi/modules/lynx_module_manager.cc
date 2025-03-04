// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/bindings/jsi/modules/lynx_module_manager.h"

#include "core/runtime/bindings/jsi/modules/lynx_module_impl.h"
#include "lynx/core/runtime/bindings/jsi/interceptor/interceptor_factory.h"

namespace lynx {
namespace piper {

LynxModuleManager::~LynxModuleManager() {
  LOGI("~LynxModuleManager");
  for (auto &module : module_map_) {
    module.second->Destroy();
  }
}

void LynxModuleManager::initBindingPtr(
    std::weak_ptr<LynxModuleManager> weak_manager,
    const std::shared_ptr<ModuleDelegate> &delegate) {
  bindingPtr = std::make_shared<lynx::piper::LynxModuleBinding>(
      BindingFunc(weak_manager, delegate));
#if ENABLE_TESTBENCH_REPLAY
  this->delegate = delegate;
#endif
}

std::shared_ptr<LynxModule> LynxModuleManager::GetModule(
    const std::string &name, const std::shared_ptr<ModuleDelegate> &delegate) {
  auto itr = module_map_.find(name);
  if (itr != module_map_.end()) {
    return itr->second;
  }

  std::shared_ptr<LynxNativeModule> native_module;
  for (const auto &module_factory : module_factories_) {
    native_module = module_factory->CreateModule(name);
    if (native_module) {
      break;
    }
  }

  std::shared_ptr<LynxModule> lynx_module;
  if (!native_module && platform_module_factory_) {
    platform_module_factory_->SetModuleExtraInfo(delegate);
    // TODO(zhangqun): when android refact finished, delete this.
#if OS_IOS || OS_TVOS || OS_OSX
    native_module = platform_module_factory_->CreateModule(name);
#else
    lynx_module = platform_module_factory_->CreatePlatformModule(name);
#endif
  }

  if (native_module) {
    auto lynx_module_impl =
        std::make_shared<LynxModuleImpl>(name, delegate, native_module);
    native_module->SetDelegate(lynx_module_impl);
    native_module->SetRuntimeProxy(runtime_proxy);
    lynx_module = std::move(lynx_module_impl);
  }
  if (lynx_module) {
#if ENABLE_TESTBENCH_RECORDER
    lynx_module->SetRecordID(record_id_);
#endif
    lynx_module->SetModuleInterceptor(group_interceptor_);
    itr = module_map_.emplace(name, std::move(lynx_module)).first;
    return itr->second;
  } else {
    return std::shared_ptr<LynxModule>(nullptr);
  }
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
      LOGW("LynxModule, try to find module: " << name << "failed. manager: "
                                              << manager);
    } else {
      LOGV("LynxModule, module: "
           << name << " is not found but it is in the allow list");
    }
    return std::shared_ptr<LynxModule>(nullptr);
  };
}

void LynxModuleManager::InitModuleInterceptor() {
  group_interceptor_ = InterceptorFactory::CreateGroupInterceptor();
}

void LynxModuleManager::SetPlatformModuleFactory(
    std::unique_ptr<NativeModuleFactory> module_factory) {
  platform_module_factory_ = std::move(module_factory);
}

NativeModuleFactory *LynxModuleManager::GetPlatformModuleFactory() {
  return platform_module_factory_.get();
}

void LynxModuleManager::SetTemplateUrl(const std::string &url) {
  if (group_interceptor_) {
    group_interceptor_->SetTemplateUrl(url);
  }
}

}  // namespace piper
}  // namespace lynx
