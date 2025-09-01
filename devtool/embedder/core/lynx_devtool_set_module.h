// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_EMBEDDER_CORE_LYNX_DEVTOOL_SET_MODULE_H_
#define DEVTOOL_EMBEDDER_CORE_LYNX_DEVTOOL_SET_MODULE_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

#include "core/public/jsb/lynx_native_module.h"

namespace lynx {
namespace devtool {

class LynxDevToolSetModule : public piper::LynxNativeModule {
 public:
  static std::shared_ptr<piper::LynxNativeModule> Create() {
    return std::make_shared<LynxDevToolSetModule>();
  }

  static const std::string &GetName() { return name_; }

  LynxDevToolSetModule();

  ~LynxDevToolSetModule() override = default;

  base::expected<std::unique_ptr<pub::Value>, std::string> InvokeMethod(
      const std::string &method_name, std::unique_ptr<pub::Value> args,
      size_t count, const piper::CallbackMap &callbacks) override;

  std::unique_ptr<pub::Value> IsLynxDebugEnabled(
      std::unique_ptr<pub::Value> args, const piper::CallbackMap &callbacks);
  std::unique_ptr<pub::Value> SwitchLynxDebug(
      std::unique_ptr<pub::Value> args, const piper::CallbackMap &callbacks);
  std::unique_ptr<pub::Value> IsDevToolEnabled(
      std::unique_ptr<pub::Value> args, const piper::CallbackMap &callbacks);
  std::unique_ptr<pub::Value> SwitchDevTool(
      std::unique_ptr<pub::Value> args, const piper::CallbackMap &callbacks);
  std::unique_ptr<pub::Value> IsLogBoxEnabled(
      std::unique_ptr<pub::Value> args, const piper::CallbackMap &callbacks);
  std::unique_ptr<pub::Value> SwitchLogBox(std::unique_ptr<pub::Value> args,
                                           const piper::CallbackMap &callbacks);
  std::unique_ptr<pub::Value> IsDomTreeEnabled(
      std::unique_ptr<pub::Value> args, const piper::CallbackMap &callbacks);
  std::unique_ptr<pub::Value> EnableDomTree(
      std::unique_ptr<pub::Value> args, const piper::CallbackMap &callbacks);
  std::unique_ptr<pub::Value> IsQuickjsDebugEnabled(
      std::unique_ptr<pub::Value> args, const piper::CallbackMap &callbacks);
  std::unique_ptr<pub::Value> SwitchQuickjsDebug(
      std::unique_ptr<pub::Value> args, const piper::CallbackMap &callbacks);
  std::unique_ptr<pub::Value> IsLongPressMenuEnabled(
      std::unique_ptr<pub::Value> args, const piper::CallbackMap &callbacks);
  std::unique_ptr<pub::Value> SwitchLongPressMenu(
      std::unique_ptr<pub::Value> args, const piper::CallbackMap &callbacks);

  void Destroy() override;

 private:
  std::unique_ptr<pub::Value> GetSwitch(const std::string &key);
  std::unique_ptr<pub::Value> SetSwitch(std::unique_ptr<pub::Value> args,
                                        const std::string &key);

  void RegisterMethod(
      const piper::NativeModuleMethod &method,
      piper::LynxNativeModule::NativeModuleInvocation invocation) {
    methods_.emplace(method.name, method);
    invocations_.emplace(method.name, std::move(invocation));
  }

  static const std::string name_;
  std::unordered_map<std::string,
                     piper::LynxNativeModule::NativeModuleInvocation>
      invocations_;
};

}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_EMBEDDER_CORE_LYNX_DEVTOOL_SET_MODULE_H_
