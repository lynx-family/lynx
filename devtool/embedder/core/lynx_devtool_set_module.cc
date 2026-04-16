// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/embedder/core/lynx_devtool_set_module.h"

#include <string>
#include <unordered_map>
#include <utility>

#include "core/public/pub_value.h"
#include "core/renderer/utils/devtool_lifecycle.h"
#include "core/renderer/utils/lynx_env.h"
#include "core/value_wrapper/value_impl_lepus.h"
#include "devtool/embedder/core/env_embedder.h"

namespace lynx {
namespace devtool {

const std::string LynxDevToolSetModule::name_ = "LynxDevToolSetModule";

LynxDevToolSetModule::LynxDevToolSetModule() {
  RegisterMethod(
      runtime::NativeModuleMethod("isLynxDebugEnabled", 0),
      reinterpret_cast<runtime::LynxNativeModule::NativeModuleInvocation>(
          &LynxDevToolSetModule::IsLynxDebugEnabled));
  RegisterMethod(
      runtime::NativeModuleMethod("switchLynxDebug", 1),
      reinterpret_cast<runtime::LynxNativeModule::NativeModuleInvocation>(
          &LynxDevToolSetModule::SwitchLynxDebug));
  RegisterMethod(
      runtime::NativeModuleMethod("isDevToolEnabled", 0),
      reinterpret_cast<runtime::LynxNativeModule::NativeModuleInvocation>(
          &LynxDevToolSetModule::IsDevToolEnabled));
  RegisterMethod(
      runtime::NativeModuleMethod("switchDevTool", 1),
      reinterpret_cast<runtime::LynxNativeModule::NativeModuleInvocation>(
          &LynxDevToolSetModule::SwitchDevTool));
  RegisterMethod(
      runtime::NativeModuleMethod("isLogBoxEnabled", 0),
      reinterpret_cast<runtime::LynxNativeModule::NativeModuleInvocation>(
          &LynxDevToolSetModule::IsLogBoxEnabled));
  RegisterMethod(
      runtime::NativeModuleMethod("switchLogBox", 1),
      reinterpret_cast<runtime::LynxNativeModule::NativeModuleInvocation>(
          &LynxDevToolSetModule::SwitchLogBox));
  RegisterMethod(
      runtime::NativeModuleMethod("isDomTreeEnabled", 0),
      reinterpret_cast<runtime::LynxNativeModule::NativeModuleInvocation>(
          &LynxDevToolSetModule::IsDomTreeEnabled));
  RegisterMethod(
      runtime::NativeModuleMethod("enableDomTree", 1),
      reinterpret_cast<runtime::LynxNativeModule::NativeModuleInvocation>(
          &LynxDevToolSetModule::EnableDomTree));
  RegisterMethod(
      runtime::NativeModuleMethod("isQuickjsDebugEnabled", 0),
      reinterpret_cast<runtime::LynxNativeModule::NativeModuleInvocation>(
          &LynxDevToolSetModule::IsQuickjsDebugEnabled));
  RegisterMethod(
      runtime::NativeModuleMethod("switchQuickjsDebug", 1),
      reinterpret_cast<runtime::LynxNativeModule::NativeModuleInvocation>(
          &LynxDevToolSetModule::SwitchQuickjsDebug));
  RegisterMethod(
      runtime::NativeModuleMethod("isLongPressMenuEnabled", 0),
      reinterpret_cast<runtime::LynxNativeModule::NativeModuleInvocation>(
          &LynxDevToolSetModule::IsLongPressMenuEnabled));
  RegisterMethod(
      runtime::NativeModuleMethod("switchLongPressMenu", 1),
      reinterpret_cast<runtime::LynxNativeModule::NativeModuleInvocation>(
          &LynxDevToolSetModule::SwitchLongPressMenu));
}

base::expected<std::unique_ptr<pub::Value>, std::string>
LynxDevToolSetModule::InvokeMethod(const std::string &method_name,
                                   std::unique_ptr<pub::Value> args,
                                   size_t count,
                                   const runtime::CallbackMap &callbacks) {
  auto it = invocations_.find(method_name);
  if (it != invocations_.end()) {
    return (this->*(it->second))(std::move(args), callbacks);
  }
  return base::unexpected(
      std::string("LynxDevToolSetModule::InvokeMethod method_name:") +
      method_name + " not found");
}

std::unique_ptr<pub::Value> LynxDevToolSetModule::IsLynxDebugEnabled(
    std::unique_ptr<pub::Value> args, const runtime::CallbackMap &callbacks) {
  bool enabled = tasm::DevToolLifecycle::GetInstance().IsEnabled();
  // TODO(mitchilling): remove this value merge after lifecycle implemented on
  // all platforms
  enabled |= EnvEmbedder::GetSwitch(tasm::LynxEnv::kLynxDebugEnabled);
  lepus::Value lepus_value = lepus::Value(enabled);
  return std::make_unique<PubLepusValue>(std::move(lepus_value));
}

std::unique_ptr<pub::Value> LynxDevToolSetModule::SwitchLynxDebug(
    std::unique_ptr<pub::Value> args, const runtime::CallbackMap &callbacks) {
  if (!args) {
    return nullptr;
  }
  auto lepus_args = pub::ValueUtils::ConvertValueToLepusValue(*(args.get()));
  if (!lepus_args.IsArray() || lepus_args.Array()->size() != 1) {
    return nullptr;
  }

  // TODO(mitchilling): remove this value set after lifecycle implemented on all
  // platforms
  SetSwitch(std::move(args), tasm::LynxEnv::kLynxDebugEnabled);
  // FIXME(mitchilling): Trying to enable may not take effect.

  bool switch_value = lepus_args.Array()->get(0).Bool();
  if (switch_value) {
    lynx::tasm::DevToolLifecycle::GetInstance().OnEnabled();
  } else {
    lynx::tasm::DevToolLifecycle::GetInstance().OnDisabled();
  }
  return nullptr;
}

std::unique_ptr<pub::Value> LynxDevToolSetModule::IsDevToolEnabled(
    std::unique_ptr<pub::Value> args, const runtime::CallbackMap &callbacks) {
  return GetSwitch(tasm::LynxEnv::kLynxDevToolEnable);
}

std::unique_ptr<pub::Value> LynxDevToolSetModule::SwitchDevTool(
    std::unique_ptr<pub::Value> args, const runtime::CallbackMap &callbacks) {
  return SetSwitch(std::move(args), tasm::LynxEnv::kLynxDevToolEnable);
}

std::unique_ptr<pub::Value> LynxDevToolSetModule::IsLogBoxEnabled(
    std::unique_ptr<pub::Value> args, const runtime::CallbackMap &callbacks) {
  return GetSwitch(tasm::LynxEnv::kLynxEnableLogBox);
}

std::unique_ptr<pub::Value> LynxDevToolSetModule::SwitchLogBox(
    std::unique_ptr<pub::Value> args, const runtime::CallbackMap &callbacks) {
  return SetSwitch(std::move(args), tasm::LynxEnv::kLynxEnableLogBox);
}

std::unique_ptr<pub::Value> LynxDevToolSetModule::IsDomTreeEnabled(
    std::unique_ptr<pub::Value> args, const runtime::CallbackMap &callbacks) {
  return GetSwitch(tasm::LynxEnv::kLynxEnableDomTree);
}

std::unique_ptr<pub::Value> LynxDevToolSetModule::EnableDomTree(
    std::unique_ptr<pub::Value> args, const runtime::CallbackMap &callbacks) {
  return SetSwitch(std::move(args), tasm::LynxEnv::kLynxEnableDomTree);
}

std::unique_ptr<pub::Value> LynxDevToolSetModule::IsQuickjsDebugEnabled(
    std::unique_ptr<pub::Value> args, const runtime::CallbackMap &callbacks) {
  return GetSwitch(tasm::LynxEnv::kLynxEnableQuickJS);
}

std::unique_ptr<pub::Value> LynxDevToolSetModule::SwitchQuickjsDebug(
    std::unique_ptr<pub::Value> args, const runtime::CallbackMap &callbacks) {
  return SetSwitch(std::move(args), tasm::LynxEnv::kLynxEnableQuickJS);
}

std::unique_ptr<pub::Value> LynxDevToolSetModule::IsLongPressMenuEnabled(
    std::unique_ptr<pub::Value> args, const runtime::CallbackMap &callbacks) {
  return GetSwitch(tasm::LynxEnv::kLynxEnableLongPressMenu);
}

std::unique_ptr<pub::Value> LynxDevToolSetModule::SwitchLongPressMenu(
    std::unique_ptr<pub::Value> args, const runtime::CallbackMap &callbacks) {
  return SetSwitch(std::move(args), tasm::LynxEnv::kLynxEnableLongPressMenu);
}

std::unique_ptr<pub::Value> LynxDevToolSetModule::GetSwitch(
    const std::string &key) {
  bool result = EnvEmbedder::GetSwitch(key);
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
  EnvEmbedder::SetSwitch(key, switch_value);
  return std::unique_ptr<pub::Value>(nullptr);
}

void LynxDevToolSetModule::Destroy() { LOGI("LynxDevToolSetModule Destroy"); }

}  // namespace devtool
}  // namespace lynx
