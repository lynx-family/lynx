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
#include "core/runtime/bindings/jsi/modules/lynx_module.h"
#include "core/runtime/bindings/jsi/modules/lynx_module_binding.h"
#include "core/runtime/bindings/jsi/modules/module_delegate.h"
#include "lynx/core/runtime/bindings/jsi/modules/module_interceptor.h"

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

using LynxModuleBindingPtr = std::shared_ptr<lynx::piper::LynxModuleBinding>;

class LynxModuleManager {
 public:
  LynxModuleBindingPtr bindingPtr;

  LynxModuleManager() = default;
  virtual ~LynxModuleManager();

  void initBindingPtr(std::weak_ptr<LynxModuleManager> weak_manager,
                      const std::shared_ptr<ModuleDelegate> &delegate);

  void SetRecordID(int64_t record_id) { record_id_ = record_id; };

  void InitModuleInterceptor();

  void SetModuleFactory(std::unique_ptr<NativeModuleFactory> module_factory) {
    if (module_factory) {
      module_factories_.push_back(std::move(module_factory));
    }
  }

  void SetPlatformModuleFactory(
      std::unique_ptr<NativeModuleFactory> module_factory);

  NativeModuleFactory *GetPlatformModuleFactory();

  void SetTemplateUrl(const std::string &url);

#if ENABLE_TESTBENCH_REPLAY
  std::shared_ptr<GroupInterceptor> GetGroupInterceptor() {
    return group_interceptor_;
  }
#endif

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

  std::unordered_map<std::string, std::shared_ptr<LynxModule>> module_map_;

  // Managed by LynxModuleManager
  base::InlineVector<std::unique_ptr<NativeModuleFactory>, 4> module_factories_;
  // Maybe use by platform.
  // When re-attaching in standalone mode, it needs to support modification, so
  // it needs to be accessed by the platform, so it is placed independently.
  std::unique_ptr<NativeModuleFactory> platform_module_factory_;

  std::shared_ptr<GroupInterceptor> group_interceptor_;
};

}  // namespace piper
}  // namespace lynx

#endif  // CORE_RUNTIME_BINDINGS_JSI_MODULES_LYNX_MODULE_MANAGER_H_
