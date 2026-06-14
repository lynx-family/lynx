// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/embedder/lynx_devtool/devtool_env_embedder.h"

#include "base/include/no_destructor.h"
#include "core/renderer/utils/devtool_lifecycle.h"
#include "core/renderer/utils/lynx_env.h"
#include "devtool/embedder/core/debug_bridge_embedder.h"
#include "platform/embedder/devtool_settings_embedder.h"
#include "third_party/debug_router/src/debug_router/common/debug_router.h"

namespace lynx {
namespace embedder {

DevToolEnvEmbedder& DevToolEnvEmbedder::GetInstance() {
  static base::NoDestructor<DevToolEnvEmbedder> instance;
  return *instance;
}

DevToolEnvEmbedder::DevToolEnvEmbedder() {
  debugrouter::common::DebugRouter::GetInstance().EnableAllSessions();
  devtool::DebugBridgeEmbedder::GetInstance();
  DevToolSettingsEmbedder::GetInstance().SyncToNative();

  auto& lifecycle = lynx::tasm::DevToolLifecycle::GetInstance();
  lifecycle.OnAttached();
  lifecycle.OnEnabled();
  lifecycle.OnInitialized();
}

void DevToolEnvEmbedder::EnableLynxDebug(bool enable) {
  // TODO(mitchilling): remove this value set after lifecycle implemented on all
  // platforms
  SetDevToolSwitch(tasm::LynxEnv::kLynxDebugEnabled, enable);
  if (enable) {
    lynx::tasm::DevToolLifecycle::GetInstance().OnEnabled();
    lynx::tasm::DevToolLifecycle::GetInstance().OnInitialized();
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
  auto& settings = DevToolSettingsEmbedder::GetInstance();
  if (key == tasm::LynxEnv::kLynxDevToolEnable) {
    settings.SetDevToolEnabled(value);
  } else if (key == tasm::LynxEnv::kLynxEnableLogBox) {
    settings.SetLogBoxEnabled(value);
  } else if (key == tasm::LynxEnv::kLynxEnableQuickJS) {
    settings.SetQuickJSDebugEnabled(value);
  } else if (key == tasm::LynxEnv::kLynxEnableDomTree) {
    settings.SetDOMTreeEnabled(value);
  } else if (key == tasm::LynxEnv::kLynxEnableLongPressMenu) {
    settings.SetLongPressMenuEnabled(value);
  } else if (key == tasm::LynxEnv::kLynxEnableLaunchRecord) {
    settings.SetLaunchRecordEnabled(value);
#if (OS_WIN || OS_OSX) && JS_ENGINE_TYPE == 0
  } else if (key == tasm::LynxEnv::kLynxEnableV8) {
    settings.SetV8Enabled(value);
#endif
  } else {
    tasm::LynxEnv::GetInstance().SetBoolLocalEnv(key, value);
  }
}

bool DevToolEnvEmbedder::GetDevToolSwitch(std::string key) const {
  auto& settings = DevToolSettingsEmbedder::GetInstance();
  if (key == tasm::LynxEnv::kLynxDevToolEnable) {
    return settings.IsDevToolEnabled();
  } else if (key == tasm::LynxEnv::kLynxEnableLogBox) {
    return settings.IsLogBoxEnabled();
  } else if (key == tasm::LynxEnv::kLynxEnableQuickJS) {
    return settings.IsQuickJSDebugEnabled();
  } else if (key == tasm::LynxEnv::kLynxEnableDomTree) {
    return settings.IsDOMTreeEnabled();
  } else if (key == tasm::LynxEnv::kLynxEnableLongPressMenu) {
    return settings.IsLongPressMenuEnabled();
  } else if (key == tasm::LynxEnv::kLynxEnableLaunchRecord) {
    return settings.IsLaunchRecordEnabled();
#if (OS_WIN || OS_OSX) && JS_ENGINE_TYPE == 0
  } else if (key == tasm::LynxEnv::kLynxEnableV8) {
    return settings.IsV8Enabled();
#endif
  }
  return tasm::LynxEnv::GetInstance().GetBoolEnv(key, false);
}

bool DevToolEnvEmbedder::IsLogBoxEnabled() const {
  return DevToolSettingsEmbedder::GetInstance().IsLogBoxEnabled();
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
}  // namespace lynx
