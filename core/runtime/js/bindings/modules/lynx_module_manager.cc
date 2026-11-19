// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/js/bindings/modules/lynx_module_manager.h"

#include <atomic>

#include "core/public/jsb/extension_module_factory.h"
#include "core/runtime/js/bindings/interceptor/interceptor_factory.h"
#include "core/runtime/js/bindings/modules/lynx_jsi_module.h"
#include "core/runtime/js/bindings/modules/module_interceptor.h"
#include "core/runtime/js/bindings/modules/module_interceptor_registry.h"

namespace lynx {
namespace runtime {
namespace js {
namespace {
std::atomic<LynxModuleManager::MissingModuleResolver> missing_module_resolver{
    nullptr};
}  // namespace

void LynxModuleManager::SetMissingModuleResolver(
    MissingModuleResolver resolver) {
  missing_module_resolver.store(resolver, std::memory_order_release);
}
LynxModuleManager::~LynxModuleManager() {
  LOGI(GetLogContext() << " NativeModule: ~LynxJSIModuleManager");
  for (auto &module : module_map_) {
    module.second->Destroy();
  }
}

void LynxModuleManager::SetLogContext(const base::LogContext &log_context) {
  log_context_ = log_context;
  if (auto delegate = GetModuleDelegate()) {
    delegate->SetLogContext(log_context);
  }
  for (auto &module : module_map_) {
    module.second->SetLogContext(log_context);
  }
}

void LynxModuleManager::initBindingPtr(
    std::weak_ptr<LynxModuleManager> weak_manager,
    const std::shared_ptr<ModuleDelegate> &delegate) {
  bindingPtr = std::make_shared<LynxJSIModuleBinding>(
      BindingFunc(weak_manager, delegate));
  pub::LynxNativeModuleManager::SetModuleDelegate(delegate);
  if (delegate) {
    delegate->SetLogContext(log_context_);
  }
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
  std::shared_ptr<LynxNativeModule> native_module =
      pub::LynxNativeModuleManager::GetModule(name);
  if (!native_module && extension_module_factory_) {
    native_module = extension_module_factory_->CreateModule(name);
  }
  if (!native_module) return nullptr;
  return module_map_
      .emplace(name, WrapNativeModule(name, delegate, native_module))
      .first->second;
}

std::shared_ptr<LynxModule> LynxModuleManager::WrapNativeModule(
    const std::string &name, const std::shared_ptr<ModuleDelegate> &delegate,
    const std::shared_ptr<LynxNativeModule> &native_module) {
  auto module = std::make_shared<LynxJSIModule>(name, delegate, native_module);
  module->SetLogContext(log_context_);
  native_module->SetDelegate(module);
  native_module->SetRuntimeProxy(runtime_proxy);
  native_module->SetContextID(context_id_);
  module->SetModuleInterceptor(group_interceptor_);
  module->SetNativeModuleRecordObserver(native_module_record_observer_);
  module->SetRecordID(record_id_);
  return module;
}

LynxJSIModuleBinding::Provider LynxModuleManager::BindingFunc(
    std::weak_ptr<LynxModuleManager> weak_manager,
    const std::shared_ptr<ModuleDelegate> &delegate) {
  const auto resolve = missing_module_resolver.load(std::memory_order_acquire);
  return [weak_manager, delegate, resolve](
             const std::string &name,
             LynxJSIModuleBinding::LookupFallback *fallback) {
    auto manager = weak_manager.lock();
    if (manager) {
      auto ptr = manager->GetModule(name, delegate);
      if (ptr != nullptr) {
        return ptr;
      }
      if (resolve && fallback && manager->group_interceptor_ &&
          manager->group_interceptor_->HasInvocationHooks()) {
        resolve({fallback->runtime, name, manager->log_context_.view_id,
                 delegate, *manager,
                 [](const MissingNativeModuleRequest &request,
                    const std::shared_ptr<LynxNativeModule> &native) {
                   return request.manager.WrapNativeModule(
                       request.name, request.delegate, native);
                 }},
                fallback->result);
        if (!fallback->result.isNull()) return std::shared_ptr<LynxModule>();
      }
    }
    // ptr == nullptr
    // issue: #1510
    static const base::LogContext kUnavailableContext;
    UNUSED_LOG_VARIABLE const auto &log_context =
        manager ? manager->GetLogContext() : kUnavailableContext;
    if (!LynxModuleUtils::LynxModuleManagerAllowList::get().count(name)) {
      LOGW(log_context << " NativeModule: LynxJSIModule, try to find module: "
                       << name << "failed. manager: " << manager);
    } else {
      LOGV(log_context << " NativeModule: LynxJSIModule, module: " << name
                       << " is not found but it is in the allow list");
    }
    return std::shared_ptr<LynxModule>(nullptr);
  };
}

void LynxModuleManager::InitModuleInterceptor() {
  group_interceptor_ = ModuleInterceptorRegistry::Install(
      InterceptorFactory::CreateGroupInterceptor());
}

void LynxModuleManager::SetTemplateUrl(const std::string &url) {
  if (group_interceptor_) {
    group_interceptor_->SetTemplateUrl(url);
  }
}

}  // namespace js

}  // namespace runtime
}  // namespace lynx
