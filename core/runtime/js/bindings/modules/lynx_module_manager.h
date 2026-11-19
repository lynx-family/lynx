// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_JS_BINDINGS_MODULES_LYNX_MODULE_MANAGER_H_
#define CORE_RUNTIME_JS_BINDINGS_MODULES_LYNX_MODULE_MANAGER_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "base/include/log/log_context.h"
#include "base/include/no_destructor.h"
#include "base/include/vector.h"
#include "core/public/jsb/native_module_factory.h"
#include "core/public/lynx_runtime_proxy.h"
#include "core/runtime/common/bindings/modules/lynx_native_module_manager.h"
#include "core/runtime/js/bindings/modules/lynx_jsi_module_binding.h"
#include "core/runtime/js/bindings/modules/lynx_module.h"
#include "core/runtime/js/bindings/modules/module_delegate.h"
#include "core/runtime/js/bindings/modules/module_interceptor.h"

namespace lynx {
namespace runtime {
class ExtensionModuleFactory;
namespace js {
class LynxModuleManager;
class NativeModuleRecordObserver;
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

using LynxJSIModuleBindingPtr = std::shared_ptr<LynxJSIModuleBinding>;

// Borrows the page runtime and the engine's standard module wrapper for a
// synchronous lookup. The resolver may retain the resulting wrapper.
struct MissingNativeModuleRequest {
  Runtime &runtime;
  const std::string &name;
  base::LynxEntityId view;
  const std::shared_ptr<ModuleDelegate> &delegate;
  LynxModuleManager &manager;
  std::shared_ptr<LynxModule> (*wrap)(
      const MissingNativeModuleRequest &,
      const std::shared_ptr<LynxNativeModule> &);
};

// LynxModuleManager is the implementation of LynxNativeModuleManager , Use JSI
// FFI to bind JS Env
class LynxModuleManager : public pub::LynxNativeModuleManager {
 public:
  LynxJSIModuleBindingPtr bindingPtr;

  explicit LynxModuleManager() = default;
  explicit LynxModuleManager(
      pub::LynxNativeModuleManager &&native_module_manager)
      : pub::LynxNativeModuleManager(std::move(native_module_manager)) {
    delegate_ = pub::LynxNativeModuleManager::GetModuleDelegate();
    context_id_ = pub::LynxNativeModuleManager::GetContextID();
    record_id_ = pub::LynxNativeModuleManager::record_id_;
  }
  virtual ~LynxModuleManager();

  void SetLogContext(const base::LogContext &log_context);

  const base::LogContext &GetLogContext() const { return log_context_; }

  void initBindingPtr(std::weak_ptr<LynxModuleManager> weak_manager,
                      const std::shared_ptr<ModuleDelegate> &delegate);
  using MissingModuleResolver = void (*)(const MissingNativeModuleRequest &,
                                         Value &);
  static void SetMissingModuleResolver(MissingModuleResolver resolver);

  void InitModuleInterceptor();
  void SetTemplateUrl(const std::string &url);
  void SetNativeModuleRecordObserver(
      std::shared_ptr<NativeModuleRecordObserver> observer) {
    native_module_record_observer_ = std::move(observer);
  }
  const std::shared_ptr<NativeModuleRecordObserver> &
  GetNativeModuleRecordObserver() const {
    return native_module_record_observer_;
  }
  void SetRecordID(int64_t record_id) override {
    LynxNativeModuleManager::SetRecordID(record_id);
    record_id_ = record_id;
  }
  ManagerType Type() override { return ManagerType::JSI; };
#if ENABLE_TESTBENCH_REPLAY
  std::shared_ptr<GroupInterceptor> GetGroupInterceptor() {
    return group_interceptor_;
  }
#endif

  void SetExtensionModuleFactory(
      const std::shared_ptr<ExtensionModuleFactory> &factory) {
    extension_module_factory_ = factory;
  }
  const std::shared_ptr<ExtensionModuleFactory> &GetExtensionModuleFactory() {
    return extension_module_factory_;
  }

  std::weak_ptr<shell::LynxRuntimeProxy> runtime_proxy;
  std::shared_ptr<ModuleDelegate> delegate_;
  int64_t record_id_ = 0;

 protected:
  virtual std::shared_ptr<LynxModule> GetModule(
      const std::string &name, const std::shared_ptr<ModuleDelegate> &delegate);

 private:
  std::shared_ptr<LynxModule> WrapNativeModule(
      const std::string &name, const std::shared_ptr<ModuleDelegate> &delegate,
      const std::shared_ptr<LynxNativeModule> &native_module);

  base::LogContext log_context_;
  int64_t context_id_ = -1;
  LynxJSIModuleBinding::Provider BindingFunc(
      std::weak_ptr<LynxModuleManager> weak_manager,
      const std::shared_ptr<ModuleDelegate> &delegate);

  // JSIModule cache
  std::unordered_map<std::string, std::shared_ptr<LynxModule>> module_map_;
  std::shared_ptr<GroupInterceptor> group_interceptor_;
  std::shared_ptr<ExtensionModuleFactory> extension_module_factory_ = nullptr;
  std::shared_ptr<NativeModuleRecordObserver> native_module_record_observer_ =
      nullptr;
};

}  // namespace js

}  // namespace runtime
}  // namespace lynx

#endif  // CORE_RUNTIME_JS_BINDINGS_MODULES_LYNX_MODULE_MANAGER_H_
