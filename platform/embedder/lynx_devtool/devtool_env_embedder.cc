// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/embedder/lynx_devtool/devtool_env_embedder.h"

#include "core/renderer/utils/devtool_lifecycle.h"
#include "core/renderer/utils/lynx_env.h"
#include "devtool/embedder/core/debug_bridge_embedder.h"
#include "devtool/embedder/core/env_embedder.h"
#include "platform/embedder/lynx_devtool/switch_persist.h"

namespace lynx {
namespace embedder {

DevToolEnvEmbedder& DevToolEnvEmbedder::GetInstance() {
  static base::NoDestructor<DevToolEnvEmbedder> instance;
  return *instance;
}

DevToolEnvEmbedder::DevToolEnvEmbedder() {
  devtool::DebugBridgeEmbedder::GetInstance();
  /**
   switch_persistent_default_: a map indicating switches' attributes.
   key: switch name.
   value: an array of bool values indicating attributes of current switch.
      The meaning of each value in array is as follows:
        whether needs to be persisted
        default value
   */
  switch_persistent_default_ = {
    {tasm::LynxEnv::kLynxDebugEnabled, {false, true}},
    {tasm::LynxEnv::kLynxDevToolComponentAttach, {false, true}},
    {tasm::LynxEnv::kLynxDevToolEnable, {true, false}},
    {tasm::LynxEnv::kLynxEnableLogBox, {true, true}},
    {tasm::LynxEnv::kLynxEnableQuickJS, {true, true}},
    {tasm::LynxEnv::kLynxEnableDomTree, {true, true}},
    {tasm::LynxEnv::kLynxEnableLongPressMenu, {true, false}},
    {tasm::LynxEnv::kLynxEnableLaunchRecord, {true, true}},
#if (OS_WIN || OS_OSX) && JS_ENGINE_TYPE == 0
    {tasm::LynxEnv::kLynxEnableV8, {true, true}},
#endif
  };
  for (auto& [key, arr] : switch_persistent_default_) {
    bool value = false;
    value =
        arr[0] ? SwitchPersist::GetValueFromPersistent(key, arr[1]) : arr[1];
    tasm::LynxEnv::GetInstance().SetBoolLocalEnv(key, value);
  }
}

bool DevToolEnvEmbedder::NeedPersist(std::string key) {
  if (switch_persistent_default_.find(key) !=
      switch_persistent_default_.end()) {
    return switch_persistent_default_[key][0];
  }
  return false;
}

void DevToolEnvEmbedder::EnableLynxDebug(bool enable) {
  // TODO(mitchilling): remove this value set after lifecycle implemented on all
  // platforms
  SetDevToolSwitch(tasm::LynxEnv::kLynxDebugEnabled, enable);
  if (enable) {
    lynx::tasm::DevToolLifecycle::GetInstance().OnEnabled();
  } else {
    lynx::tasm::DevToolLifecycle::GetInstance().OnDisabled();
  }
}

bool DevToolEnvEmbedder::IsLynxDebugEnabled() const {
  // TODO(mitchilling): remove this value get after lifecycle implemented on all
  // platforms
  return lynx::tasm::DevToolLifecycle::GetInstance().IsEnabled() ||
         GetDevToolSwitch(tasm::LynxEnv::kLynxDebugEnabled);
}

void DevToolEnvEmbedder::EnableDevTool(bool enabled) {
  SetDevToolSwitch(tasm::LynxEnv::kLynxDevToolEnable, enabled);
}

bool DevToolEnvEmbedder::IsDevToolEnabled() const {
  return GetDevToolSwitch(tasm::LynxEnv::kLynxDevToolEnable);
}

void DevToolEnvEmbedder::SetDevToolSwitch(std::string key, bool value) {
  tasm::LynxEnv::GetInstance().SetBoolLocalEnv(key, value);
  if (NeedPersist(key)) {
    SwitchPersist::SetValueToPersistent(key, value);
  }
}

bool DevToolEnvEmbedder::GetDevToolSwitch(std::string key) const {
  // When DevToolEnvEmbedder initialize, all switches in LynxEnv are set with
  // persistent/default value. So we can directly get switch value from LynxEnv.
  return tasm::LynxEnv::GetInstance().GetBoolEnv(key, false);
}

bool DevToolEnvEmbedder::IsLogBoxEnabled() const {
  return GetDevToolSwitch(tasm::LynxEnv::kLynxEnableLogBox);
}

void DevToolEnvEmbedder::SetAppInfo(const std::string& key,
                                    const std::string& value) {
  app_infos_[key] = value;
  devtool::DebugBridgeEmbedder::GetInstance().SetAppInfo(app_infos_);
}
void DevToolEnvEmbedder::SetAppInfo(
    const std::unordered_map<std::string, std::string>& app_info) {
  app_infos_.insert(app_info.begin(), app_info.end());
  devtool::DebugBridgeEmbedder::GetInstance().SetAppInfo(app_info);
}

}  // namespace embedder

namespace devtool {
void EnvEmbedder::SetSwitch(const std::string& key, bool value) {
  embedder::DevToolEnvEmbedder::GetInstance().SetDevToolSwitch(key, value);
}

bool EnvEmbedder::GetSwitch(const std::string& key) {
  return embedder::DevToolEnvEmbedder::GetInstance().GetDevToolSwitch(key);
}

}  // namespace devtool
}  // namespace lynx
