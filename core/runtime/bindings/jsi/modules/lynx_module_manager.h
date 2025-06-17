// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_BINDINGS_JSI_MODULES_LYNX_MODULE_MANAGER_H_
#define CORE_RUNTIME_BINDINGS_JSI_MODULES_LYNX_MODULE_MANAGER_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "base/include/no_destructor.h"
#include "base/include/vector.h"
#include "core/public/jsb/native_module_factory.h"
#include "core/public/lynx_runtime_proxy.h"
#include "core/runtime/bindings/common/modules/lynx_native_module_manager.h"
#include "core/runtime/bindings/jsi/modules/lynx_jsi_module_binding.h"
#include "core/runtime/bindings/jsi/modules/lynx_module.h"
#include "core/runtime/bindings/jsi/modules/module_delegate.h"
#include "core/runtime/bindings/jsi/modules/module_interceptor.h"

namespace lynx {
namespace piper {
// issue: #1510
// LynxModuleUtils::LynxModuleManagerAllowList
// inline static alternative
namespace LynxModuleUtils {
struct LynxModuleManagerAllowList {
  static const std::unordered_set<std::string> &get() {
    static base::NoDestructor<std::unordered_set<std::string>> storage_{
        {"LynxTestModule", "NetworkingModule", "NavigationModule"}};
    return *storage_.get();
  }
};
}  // namespace LynxModuleUtils

using LynxJSIModuleBindingPtr =
    std::shared_ptr<lynx::piper::LynxJSIModuleBinding>;

class LynxModuleManager {
 public:
  LynxJSIModuleBindingPtr bindingPtr;

  explicit LynxModuleManager(
      std::unique_ptr<pub::LynxNativeModuleManager> native_module_manager) {
    native_module_manager_ = std::move(native_module_manager);
  }
  virtual ~LynxModuleManager();

  void initBindingPtr(std::weak_ptr<LynxModuleManager> weak_manager,
                      const std::shared_ptr<ModuleDelegate> &delegate);
  // init interceptor
  void InitModuleInterceptor();
  void SetTemplateUrl(const std::string &url);

  void SetRecordID(int64_t record_id) { record_id_ = record_id; };
#if ENABLE_TESTBENCH_REPLAY
  std::shared_ptr<GroupInterceptor> GetGroupInterceptor() {
    return group_interceptor_;
  }
#endif

  void SetModuleFactory(
      std::unique_ptr<piper::NativeModuleFactory> module_factory) {
    if (native_module_manager_) {
      native_module_manager_->SetModuleFactory(std::move(module_factory));
    }
  };

  std::weak_ptr<shell::LynxRuntimeProxy> runtime_proxy;
  std::shared_ptr<ModuleDelegate> delegate;
  int64_t record_id_ = 0;

 protected:
  virtual std::shared_ptr<LynxModule> GetModule(
      const std::string &name, const std::shared_ptr<ModuleDelegate> &delegate);

 private:
  LynxModuleProviderFunction BindingFunc(
      std::weak_ptr<LynxModuleManager> weak_manager,
      const std::shared_ptr<ModuleDelegate> &delegate);

  // Used for create nativeModule , contains CModule and PlatformModule
  std::unique_ptr<pub::LynxNativeModuleManager> native_module_manager_;
  // JSIModule cache
  std::unordered_map<std::string, std::shared_ptr<LynxModule>> module_map_;
  std::shared_ptr<GroupInterceptor> group_interceptor_;
};

}  // namespace piper
}  // namespace lynx

#endif  // CORE_RUNTIME_BINDINGS_JSI_MODULES_LYNX_MODULE_MANAGER_H_
