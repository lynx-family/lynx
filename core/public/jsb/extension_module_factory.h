// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_PUBLIC_JSB_EXTENSION_MODULE_FACTORY_H_
#define CORE_PUBLIC_JSB_EXTENSION_MODULE_FACTORY_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

#include "core/public/jsb/lynx_extension_module.h"
#include "core/public/jsb/native_module_factory.h"

#ifdef USE_PRIMJS_NAPI
#include "third_party/napi/include/primjs_napi_defines.h"
#endif

namespace lynx {
namespace piper {

using ExtensionModuleCreator =
    std::function<std::shared_ptr<piper::LynxExtensionModule>()>;

struct ModuleCreatorInfo {
  ExtensionModuleCreator creator;
  bool lazy_create;
};

class ExtensionModuleFactory : public NativeModuleFactory {
 public:
  ExtensionModuleFactory()
      : env_(nullptr),
        vsync_observer_(nullptr),
        task_runner_(nullptr),
        ui_delegate_(nullptr) {}
  virtual ~ExtensionModuleFactory() = default;

  // Called on the main thread
  virtual void OnLynxViewCreate(tasm::UIDelegate* ui_delegate) {
    ui_delegate_ = ui_delegate;
  }

  // Called on the main thread
  virtual void OnLynxViewDestroy() {
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& pair : module_map_) {
      pair.second->SetLynxViewDestroyedState();
    }
    ui_delegate_ = nullptr;
  }

  void OnRuntimeInit(const fml::RefPtr<fml::TaskRunner>& task_runner) {
    for (const auto& pair : module_map_) {
      pair.second->SetRuntimeInitState(task_runner);
    }
    task_runner_ = task_runner;
  }

  // Called on the BTS thread
  void OnRuntimeAttach(
      napi_env env,
      const std::shared_ptr<runtime::IVSyncObserver>& vsync_observer) {
    for (const auto& pair : module_map_) {
      pair.second->SetRuntimeAttachedState(env, vsync_observer);
    }
    env_ = env;
    vsync_observer_ = vsync_observer;
  }

  // Called on the BTS thread
  void OnRuntimeReady(napi_env env, napi_value lynx, const std::string& url) {
    for (const auto& pair : module_map_) {
      pair.second->SetRuntimeReadyState(env, lynx, url);
    }
  }

  // Called on the BTS thread
  void OnRuntimeDetach() {
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& pair : module_map_) {
      pair.second->SetRuntimeDetachedState();
    }
  }

  void OnEnterForeground() {
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& pair : module_map_) {
      pair.second->SetEnteringForegroundState();
    }
  }

  void OnEnterBackground() {
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& pair : module_map_) {
      pair.second->SetEnteringBackgroundState();
    }
  }

  // LynxExtensionModule is created using lazy loading by default. If lazy
  // loading is not used, it is created when OnLynxViewCreate is called.
  virtual void RegisterExtensionModule(const std::string& name,
                                       ExtensionModuleCreator creator,
                                       bool lazy_create = true) {
    std::lock_guard<std::mutex> lock(mutex_);
    ModuleCreatorInfo info{std::move(creator), lazy_create};
    module_creators_.emplace(name, std::move(info));
  }

 protected:
  std::unordered_map<std::string, std::shared_ptr<LynxExtensionModule>>
      module_map_;
  std::unordered_map<std::string, ModuleCreatorInfo> module_creators_;
  // The env_ Only accessible in BTS thread
  napi_env env_;
  std::shared_ptr<runtime::IVSyncObserver> vsync_observer_;
  fml::RefPtr<fml::TaskRunner> task_runner_;
  tasm::UIDelegate* ui_delegate_;
};

}  // namespace piper
}  // namespace lynx

#ifdef USE_PRIMJS_NAPI
#include "third_party/napi/include/primjs_napi_undefs.h"
#endif

#endif  // CORE_PUBLIC_JSB_EXTENSION_MODULE_FACTORY_H_
