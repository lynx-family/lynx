// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_DEVTOOL_SRC_MAIN_CPP_DEVTOOL_ENV_HARMONY_H_
#define PLATFORM_HARMONY_LYNX_DEVTOOL_SRC_MAIN_CPP_DEVTOOL_ENV_HARMONY_H_

#include <database/preferences/oh_preferences.h>

#include <string>
#include <unordered_map>

#include "base/include/platform/harmony/napi_util.h"

namespace lynx {
namespace devtool {

class DevToolEnvHarmony {
 public:
  DevToolEnvHarmony();
  ~DevToolEnvHarmony();
  static napi_value Init(napi_env env, napi_value exports);
  static void InitDevToolEnv();
  static DevToolEnvHarmony &GetInstance();
  void SetSwitch(const std::string &key, bool value,
                 bool is_persistent = false);
  bool GetSwitch(const std::string &key);

  static bool NeedPersistent(const std::string &key);

 private:
  DevToolEnvHarmony(const DevToolEnvHarmony &) = delete;
  DevToolEnvHarmony &operator=(const DevToolEnvHarmony &) = delete;

  bool InitPreferences();
  bool SetPersistentSwitch(const std::string &key, bool value);

  static napi_value Constructor(napi_env env, napi_callback_info info);
  static napi_value SetSwitchNAPI(napi_env env, napi_callback_info info);
  static napi_value GetSwitchNAPI(napi_env env, napi_callback_info info);
  static napi_value InitDevToolEnvNAPI(napi_env env, napi_callback_info info);
  static napi_value SetAppInfo(napi_env env, napi_callback_info info);
  static napi_value InitDevToolSetModule(napi_env env, napi_callback_info info);

  static std::unordered_map<std::string, bool> s_persistent_default_value_;
  OH_Preferences *preference_ = nullptr;
};

}  // namespace devtool
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_DEVTOOL_SRC_MAIN_CPP_DEVTOOL_ENV_HARMONY_H_
