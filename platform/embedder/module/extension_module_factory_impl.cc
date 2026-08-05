// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/embedder/module/extension_module_factory_impl.h"

#include "platform/embedder/module/lynx_extension_module_priv.h"

namespace lynx {
namespace embedder {

std::shared_ptr<runtime::LynxNativeModule>
ExtensionModuleFactoryImpl::CreateModule(const std::string& name) {
  std::lock_guard<std::mutex> lock(mutex_);
  // Use cached module of creating without lazy loading
  auto itr = module_map_.find(name);
  if (itr != module_map_.end()) {
    std::static_pointer_cast<ExtensionModuleImpl>(itr->second)
        ->SetupNapiModule();
    return itr->second;
  }

  // Create C Module
  auto extension_module_creator = extension_module_creators_.find(name);
  if (extension_module_creator != extension_module_creators_.end()) {
    lynx_extension_module_t* ret =
        std::get<0>(extension_module_creator->second)(
            std::get<2>(extension_module_creator->second));
    auto module = std::make_shared<ExtensionModuleImpl>(ret);
    module->SetRuntimeInitState(task_runner_);
    module->SetRuntimeAttachedState(opaque_env_, vsync_observer_);
    module->SetupNapiModule();
    // The module will be cached in LynxNativeModuleManager.
    return module;
  }

  return std::shared_ptr<runtime::LynxNativeModule>(nullptr);
}

void ExtensionModuleFactoryImpl::OnLynxViewCreate(
    lynx_view_t* lynx_view, tasm::UIDelegate* ui_delegate) {
  for (const auto& pair : extension_module_creators_) {
    if (std::get<1>(pair.second)) {
      // If lazy loading is used, the Module will be created in the CreateModule
      // method.
      continue;
    }
    lynx_extension_module_t* ret =
        std::get<0>(pair.second)(std::get<2>(pair.second));
    auto itr =
        module_map_
            .emplace(pair.first, std::make_shared<ExtensionModuleImpl>(ret))
            .first;
    std::static_pointer_cast<ExtensionModuleImpl>(itr->second)
        ->SetLynxViewCreatedState(lynx_view, ui_delegate);
  }
}

void LynxMTSExtensionModuleFactoryNAPI::AttachOpaqueContext(void* context) {
  env_.store(context, std::memory_order_release);
}

void LynxMTSExtensionModuleFactoryNAPI::DetachOpaqueContext(void* context) {
  auto expected_env = context;
  env_.compare_exchange_strong(expected_env, nullptr,
                               std::memory_order_acq_rel);
}

void LynxMTSExtensionModuleFactoryNAPI::Detach() {
  std::lock_guard<std::mutex> lock(mutex_);
  extension_module_creators_.clear();
  env_.store(nullptr, std::memory_order_release);
  is_detached_ = true;
}

std::shared_ptr<runtime::LynxNativeModule>
LynxMTSExtensionModuleFactoryNAPI::CreateModule(const std::string& name) {
  auto env = env_.load(std::memory_order_acquire);
  if (env == nullptr) {
    return nullptr;
  }

  std::tuple<extension_module_creator, bool, void*> creator_info;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (is_detached_) {
      return nullptr;
    }
    auto creator = extension_module_creators_.find(name);
    if (creator == extension_module_creators_.end()) {
      return nullptr;
    }
    creator_info = creator->second;
  }

  lynx_extension_module_t* c_module =
      std::get<0>(creator_info)(std::get<2>(creator_info));
  if (c_module == nullptr) {
    return nullptr;
  }
  auto module = std::make_shared<ExtensionModuleImpl>(c_module);
  if (!module->SetupNapiModuleWithEnv(env)) {
    return nullptr;
  }
  return module;
}

}  // namespace embedder
}  // namespace lynx
