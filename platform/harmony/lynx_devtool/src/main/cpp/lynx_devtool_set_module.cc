// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_devtool/src/main/cpp/lynx_devtool_set_module.h"

#include <string>
#include <unordered_map>
#include <utility>

#include "core/public/pub_value.h"
#include "core/renderer/utils/lynx_env.h"
#include "core/value_wrapper/value_impl_lepus.h"
#include "platform/harmony/lynx_devtool/src/main/cpp/devtool_env_harmony.h"

namespace lynx {
namespace devtool {

const std::string LynxDevToolSetModule::name_ = "LynxDevToolSetModule";

LynxDevToolSetModule::LynxDevToolSetModule() {
  RegisterMethod(
      piper::NativeModuleMethod("isLynxDebugEnabled", 0),
      reinterpret_cast<piper::LynxNativeModule::NativeModuleInvocation>(
          &LynxDevToolSetModule::IsLynxDebugEnabled));
  RegisterMethod(
      piper::NativeModuleMethod("switchLynxDebug", 1),
      reinterpret_cast<piper::LynxNativeModule::NativeModuleInvocation>(
          &LynxDevToolSetModule::SwitchLynxDebug));
  RegisterMethod(
      piper::NativeModuleMethod("isDevToolEnabled", 0),
      reinterpret_cast<piper::LynxNativeModule::NativeModuleInvocation>(
          &LynxDevToolSetModule::IsDevToolEnabled));
  RegisterMethod(
      piper::NativeModuleMethod("switchDevTool", 1),
      reinterpret_cast<piper::LynxNativeModule::NativeModuleInvocation>(
          &LynxDevToolSetModule::SwitchDevTool));
  RegisterMethod(
      piper::NativeModuleMethod("isLogBoxEnabled", 0),
      reinterpret_cast<piper::LynxNativeModule::NativeModuleInvocation>(
          &LynxDevToolSetModule::IsLogBoxEnabled));
  RegisterMethod(
      piper::NativeModuleMethod("switchLogBox", 1),
      reinterpret_cast<piper::LynxNativeModule::NativeModuleInvocation>(
          &LynxDevToolSetModule::SwitchLogBox));
  RegisterMethod(
      piper::NativeModuleMethod("isDomTreeEnabled", 0),
      reinterpret_cast<piper::LynxNativeModule::NativeModuleInvocation>(
          &LynxDevToolSetModule::IsDomTreeEnabled));
  RegisterMethod(
      piper::NativeModuleMethod("enableDomTree", 1),
      reinterpret_cast<piper::LynxNativeModule::NativeModuleInvocation>(
          &LynxDevToolSetModule::EnableDomTree));
  RegisterMethod(
      piper::NativeModuleMethod("isQuickjsDebugEnabled", 0),
      reinterpret_cast<piper::LynxNativeModule::NativeModuleInvocation>(
          &LynxDevToolSetModule::IsQuickjsDebugEnabled));
  RegisterMethod(
      piper::NativeModuleMethod("switchQuickjsDebug", 1),
      reinterpret_cast<piper::LynxNativeModule::NativeModuleInvocation>(
          &LynxDevToolSetModule::SwitchQuickjsDebug));
  RegisterMethod(
      piper::NativeModuleMethod("isLongPressMenuEnabled", 0),
      reinterpret_cast<piper::LynxNativeModule::NativeModuleInvocation>(
          &LynxDevToolSetModule::IsLongPressMenuEnabled));
  RegisterMethod(
      piper::NativeModuleMethod("switchLongPressMenu", 1),
      reinterpret_cast<piper::LynxNativeModule::NativeModuleInvocation>(
          &LynxDevToolSetModule::SwitchLongPressMenu));
}

base::expected<std::unique_ptr<pub::Value>, std::string>
LynxDevToolSetModule::InvokeMethod(const std::string &method_name,
                                   std::unique_ptr<pub::Value> args,
                                   size_t count,
                                   const piper::CallbackMap &callbacks) {
  auto it = invocations_.find(method_name);
  if (it != invocations_.end()) {
    return (this->*(it->second))(std::move(args), callbacks);
  }
  return base::unexpected(
      std::string("LynxDevToolSetModule::InvokeMethod method_name:") +
      method_name + " not found");
}

std::unique_ptr<pub::Value> LynxDevToolSetModule::IsLynxDebugEnabled(
    std::unique_ptr<pub::Value> args, const piper::CallbackMap &callbacks) {
  return GetSwitch(tasm::LynxEnv::kLynxDebugEnabled);
}

std::unique_ptr<pub::Value> LynxDevToolSetModule::SwitchLynxDebug(
    std::unique_ptr<pub::Value> args, const piper::CallbackMap &callbacks) {
  return SetSwitch(std::move(args), tasm::LynxEnv::kLynxDebugEnabled);
}

std::unique_ptr<pub::Value> LynxDevToolSetModule::IsDevToolEnabled(
    std::unique_ptr<pub::Value> args, const piper::CallbackMap &callbacks) {
  return GetSwitch(tasm::LynxEnv::kLynxDevToolEnable);
}

std::unique_ptr<pub::Value> LynxDevToolSetModule::SwitchDevTool(
    std::unique_ptr<pub::Value> args, const piper::CallbackMap &callbacks) {
  return SetSwitch(std::move(args), tasm::LynxEnv::kLynxDevToolEnable);
}

std::unique_ptr<pub::Value> LynxDevToolSetModule::IsLogBoxEnabled(
    std::unique_ptr<pub::Value> args, const piper::CallbackMap &callbacks) {
  return GetSwitch(tasm::LynxEnv::kLynxEnableLogBox);
}

std::unique_ptr<pub::Value> LynxDevToolSetModule::SwitchLogBox(
    std::unique_ptr<pub::Value> args, const piper::CallbackMap &callbacks) {
  return SetSwitch(std::move(args), tasm::LynxEnv::kLynxEnableLogBox);
}

std::unique_ptr<pub::Value> LynxDevToolSetModule::IsDomTreeEnabled(
    std::unique_ptr<pub::Value> args, const piper::CallbackMap &callbacks) {
  return GetSwitch(tasm::LynxEnv::kLynxEnableDomTree);
}

std::unique_ptr<pub::Value> LynxDevToolSetModule::EnableDomTree(
    std::unique_ptr<pub::Value> args, const piper::CallbackMap &callbacks) {
  return SetSwitch(std::move(args), tasm::LynxEnv::kLynxEnableDomTree);
}

std::unique_ptr<pub::Value> LynxDevToolSetModule::IsQuickjsDebugEnabled(
    std::unique_ptr<pub::Value> args, const piper::CallbackMap &callbacks) {
  return GetSwitch(tasm::LynxEnv::kLynxEnableQuickJS);
}

std::unique_ptr<pub::Value> LynxDevToolSetModule::SwitchQuickjsDebug(
    std::unique_ptr<pub::Value> args, const piper::CallbackMap &callbacks) {
  return SetSwitch(std::move(args), tasm::LynxEnv::kLynxEnableQuickJS);
}

std::unique_ptr<pub::Value> LynxDevToolSetModule::IsLongPressMenuEnabled(
    std::unique_ptr<pub::Value> args, const piper::CallbackMap &callbacks) {
  return GetSwitch(tasm::LynxEnv::kLynxEnableLongPressMenu);
}

std::unique_ptr<pub::Value> LynxDevToolSetModule::SwitchLongPressMenu(
    std::unique_ptr<pub::Value> args, const piper::CallbackMap &callbacks) {
  return SetSwitch(std::move(args), tasm::LynxEnv::kLynxEnableLongPressMenu);
}

std::unique_ptr<pub::Value> LynxDevToolSetModule::GetSwitch(
    const std::string &key) {
  bool result = DevToolEnvHarmony::GetInstance().GetSwitch(key);
  lepus::Value lepus_value = lepus::Value(result);
  return std::make_unique<PubLepusValue>(std::move(lepus_value));
}

std::unique_ptr<pub::Value> LynxDevToolSetModule::SetSwitch(
    std::unique_ptr<pub::Value> args, const std::string &key) {
  auto lepus_args = pub::ValueUtils::ConvertValueToLepusValue(*(args.get()));
  if (lepus_args.Array()->size() != 1) {
    return std::unique_ptr<pub::Value>(nullptr);
  }

  bool switch_value = lepus_args.Array()->get(0).Bool();
  DevToolEnvHarmony::GetInstance().SetSwitch(
      key, switch_value, DevToolEnvHarmony::NeedPersistent(key));
  return std::unique_ptr<pub::Value>(nullptr);
}

void LynxDevToolSetModule::Destroy() { LOGI("LynxDevToolSetModule Destroy"); }

}  // namespace devtool
}  // namespace lynx
