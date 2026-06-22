// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/embedder/devtool_settings_embedder.h"

#include "base/include/no_destructor.h"
#include "core/renderer/utils/lynx_env.h"
#include "platform/embedder/switch_persist.h"

namespace lynx {
namespace embedder {

DevToolSettingsEmbedder& DevToolSettingsEmbedder::GetInstance() {
  static base::NoDestructor<DevToolSettingsEmbedder> instance;
  return *instance;
}

void DevToolSettingsEmbedder::SyncToNative() {
  SyncBooleanToNative(tasm::LynxEnv::kLynxDevToolEnable, IsDevToolEnabled());
  SyncBooleanToNative(tasm::LynxEnv::kLynxEnableLogBox, IsLogBoxEnabled());
  SyncBooleanToNative(tasm::LynxEnv::kLynxEnableQuickJS,
                      IsQuickJSDebugEnabled());
  SyncBooleanToNative(tasm::LynxEnv::kLynxEnableDomTree, IsDOMTreeEnabled());
  SyncBooleanToNative(tasm::LynxEnv::kLynxEnableLongPressMenu,
                      IsLongPressMenuEnabled());
  SyncBooleanToNative(tasm::LynxEnv::kLynxEnableLaunchRecord,
                      IsLaunchRecordEnabled());
#if (OS_WIN || OS_OSX) && JS_ENGINE_TYPE == 0
  SyncBooleanToNative(tasm::LynxEnv::kLynxEnableV8, IsV8Enabled());
#endif
}

/**
 * Persistence: true
 * Sync to Native: true
 * Default: false
 */
bool DevToolSettingsEmbedder::IsDevToolEnabled() const {
  return GetPersistedBoolean(tasm::LynxEnv::kLynxDevToolEnable, false);
}

void DevToolSettingsEmbedder::SetDevToolEnabled(bool enabled) {
  SetPersistedBoolean(tasm::LynxEnv::kLynxDevToolEnable, enabled);
  SyncBooleanToNative(tasm::LynxEnv::kLynxDevToolEnable, enabled);
}

/**
 * Persistence: true
 * Sync to Native: true
 * Default: true
 */
bool DevToolSettingsEmbedder::IsLogBoxEnabled() const {
  return GetPersistedBoolean(tasm::LynxEnv::kLynxEnableLogBox, true);
}

void DevToolSettingsEmbedder::SetLogBoxEnabled(bool enabled) {
  SetPersistedBoolean(tasm::LynxEnv::kLynxEnableLogBox, enabled);
  SyncBooleanToNative(tasm::LynxEnv::kLynxEnableLogBox, enabled);
}

/**
 * Persistence: true
 * Sync to Native: true
 * Default: true
 */
bool DevToolSettingsEmbedder::IsQuickJSDebugEnabled() const {
  return GetPersistedBoolean(tasm::LynxEnv::kLynxEnableQuickJS, true);
}

void DevToolSettingsEmbedder::SetQuickJSDebugEnabled(bool enabled) {
  SetPersistedBoolean(tasm::LynxEnv::kLynxEnableQuickJS, enabled);
  SyncBooleanToNative(tasm::LynxEnv::kLynxEnableQuickJS, enabled);
}

/**
 * Persistence: true
 * Sync to Native: true
 * Default: true
 */
bool DevToolSettingsEmbedder::IsDOMTreeEnabled() const {
  return GetPersistedBoolean(tasm::LynxEnv::kLynxEnableDomTree, true);
}

void DevToolSettingsEmbedder::SetDOMTreeEnabled(bool enabled) {
  SetPersistedBoolean(tasm::LynxEnv::kLynxEnableDomTree, enabled);
  SyncBooleanToNative(tasm::LynxEnv::kLynxEnableDomTree, enabled);
}

/**
 * Persistence: true
 * Sync to Native: true
 * Default: false
 */
bool DevToolSettingsEmbedder::IsLongPressMenuEnabled() const {
  return GetPersistedBoolean(tasm::LynxEnv::kLynxEnableLongPressMenu, false);
}

void DevToolSettingsEmbedder::SetLongPressMenuEnabled(bool enabled) {
  SetPersistedBoolean(tasm::LynxEnv::kLynxEnableLongPressMenu, enabled);
  SyncBooleanToNative(tasm::LynxEnv::kLynxEnableLongPressMenu, enabled);
}

/**
 * Persistence: true
 * Sync to Native: true
 * Default: true
 */
bool DevToolSettingsEmbedder::IsLaunchRecordEnabled() const {
  return GetPersistedBoolean(tasm::LynxEnv::kLynxEnableLaunchRecord, true);
}

void DevToolSettingsEmbedder::SetLaunchRecordEnabled(bool enabled) {
  SetPersistedBoolean(tasm::LynxEnv::kLynxEnableLaunchRecord, enabled);
  SyncBooleanToNative(tasm::LynxEnv::kLynxEnableLaunchRecord, enabled);
}

#if (OS_WIN || OS_OSX) && JS_ENGINE_TYPE == 0
/**
 * Persistence: true
 * Sync to Native: true
 * Default: true
 */
bool DevToolSettingsEmbedder::IsV8Enabled() const {
  return GetPersistedBoolean(tasm::LynxEnv::kLynxEnableV8, true);
}

void DevToolSettingsEmbedder::SetV8Enabled(bool enabled) {
  SetPersistedBoolean(tasm::LynxEnv::kLynxEnableV8, enabled);
  SyncBooleanToNative(tasm::LynxEnv::kLynxEnableV8, enabled);
}
#endif

bool DevToolSettingsEmbedder::GetPersistedBoolean(const std::string& key,
                                                  bool default_value) const {
  return SwitchPersist::GetValueFromPersistent(key, default_value);
}

void DevToolSettingsEmbedder::SetPersistedBoolean(const std::string& key,
                                                  bool value) {
  SwitchPersist::SetValueToPersistent(key, value);
}

void DevToolSettingsEmbedder::SyncBooleanToNative(const std::string& key,
                                                  bool value) {
  tasm::LynxEnv::GetInstance().SetBoolLocalEnv(key, value);
}

}  // namespace embedder
}  // namespace lynx
