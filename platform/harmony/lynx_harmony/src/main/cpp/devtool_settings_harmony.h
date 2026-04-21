// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_DEVTOOL_SETTINGS_HARMONY_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_DEVTOOL_SETTINGS_HARMONY_H_

#include <database/preferences/oh_preferences.h>
#include <napi/native_api.h>

#include <string>

namespace lynx {
namespace tasm {
namespace harmony {

class DevToolSettingsHarmony {
 public:
  DevToolSettingsHarmony();
  ~DevToolSettingsHarmony();

  static napi_value Init(napi_env env, napi_value exports);
  static DevToolSettingsHarmony& GetInstance();

  void SetPersistedBoolean(const std::string& key, bool value);
  bool GetPersistedBoolean(const std::string& key, bool default_value);
  void SyncBooleanToNative(const std::string& key, bool value);

 private:
  DevToolSettingsHarmony(const DevToolSettingsHarmony&) = delete;
  DevToolSettingsHarmony& operator=(const DevToolSettingsHarmony&) = delete;

  bool InitPreferences();

  static napi_value Constructor(napi_env env, napi_callback_info info);
  static napi_value SetPersistedBooleanNAPI(napi_env env,
                                            napi_callback_info info);
  static napi_value GetPersistedBooleanNAPI(napi_env env,
                                            napi_callback_info info);
  static napi_value SyncBooleanToNativeNAPI(napi_env env,
                                            napi_callback_info info);

  OH_Preferences* preference_ = nullptr;
};

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_DEVTOOL_SETTINGS_HARMONY_H_
