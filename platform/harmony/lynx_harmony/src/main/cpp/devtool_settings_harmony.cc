// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/devtool_settings_harmony.h"

#include <database/preferences/oh_preferences_err_code.h>
#include <database/preferences/oh_preferences_option.h>
#include <database/preferences/oh_preferences_value.h>

#include <cassert>
#include <iterator>

#include "base/include/log/logging.h"
#include "base/include/no_destructor.h"
#include "base/include/platform/harmony/napi_util.h"
#include "core/renderer/utils/lynx_env.h"

namespace lynx {
namespace tasm {
namespace harmony {

namespace {

napi_value CreateUndefined(napi_env env) {
  napi_value result = nullptr;
  napi_get_undefined(env, &result);
  return result;
}

}  // namespace

DevToolSettingsHarmony::DevToolSettingsHarmony() {
  if (!InitPreferences()) {
    LOGW("DevToolSettingsHarmony InitPreferences failed");
  }
}

DevToolSettingsHarmony::~DevToolSettingsHarmony() {
  if (preference_ != nullptr) {
    (void)OH_Preferences_Close(preference_);
    preference_ = nullptr;
  }
}

napi_value DevToolSettingsHarmony::Init(napi_env env, napi_value exports) {
#define DECLARE_NAPI_STATIC_FUNCTION(name, func) \
  {(name), nullptr, (func), nullptr, nullptr, nullptr, napi_static, nullptr}

  napi_property_descriptor properties[] = {
      DECLARE_NAPI_STATIC_FUNCTION("setPersistedBoolean",
                                   SetPersistedBooleanNAPI),
      DECLARE_NAPI_STATIC_FUNCTION("getPersistedBoolean",
                                   GetPersistedBooleanNAPI),
      DECLARE_NAPI_STATIC_FUNCTION("syncBooleanToNative",
                                   SyncBooleanToNativeNAPI),
  };
#undef DECLARE_NAPI_STATIC_FUNCTION

  napi_value cons = nullptr;
  napi_status status = napi_define_class(
      env, "DevToolSettingsHarmony", NAPI_AUTO_LENGTH, Constructor, nullptr,
      std::size(properties), properties, &cons);
  assert(status == napi_ok);
  status =
      napi_set_named_property(env, exports, "DevToolSettingsHarmony", cons);
  assert(status == napi_ok);
  return exports;
}

napi_value DevToolSettingsHarmony::Constructor(napi_env env,
                                               napi_callback_info info) {
  napi_value js_this = nullptr;
  napi_get_cb_info(env, info, nullptr, nullptr, &js_this, nullptr);
  return js_this;
}

DevToolSettingsHarmony& DevToolSettingsHarmony::GetInstance() {
  static base::NoDestructor<DevToolSettingsHarmony> instance;
  return *instance;
}

napi_value DevToolSettingsHarmony::SetPersistedBooleanNAPI(
    napi_env env, napi_callback_info info) {
  size_t argc = 2;
  napi_value args[2] = {nullptr};
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  std::string key = base::NapiUtil::ConvertToString(env, args[0]);
  bool value = base::NapiUtil::ConvertToBoolean(env, args[1]);
  DevToolSettingsHarmony::GetInstance().SetPersistedBoolean(key, value);
  return CreateUndefined(env);
}

napi_value DevToolSettingsHarmony::GetPersistedBooleanNAPI(
    napi_env env, napi_callback_info info) {
  size_t argc = 2;
  napi_value args[2] = {nullptr};
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  std::string key = base::NapiUtil::ConvertToString(env, args[0]);
  bool default_value = base::NapiUtil::ConvertToBoolean(env, args[1]);
  napi_value result = nullptr;
  napi_status status = napi_get_boolean(
      env,
      DevToolSettingsHarmony::GetInstance().GetPersistedBoolean(key,
                                                                default_value),
      &result);
  if (status != napi_ok) {
    LOGW("DevToolSettingsHarmony failed to convert boolean to napi_value");
    return nullptr;
  }
  return result;
}

napi_value DevToolSettingsHarmony::SyncBooleanToNativeNAPI(
    napi_env env, napi_callback_info info) {
  size_t argc = 2;
  napi_value args[2] = {nullptr};
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  std::string key = base::NapiUtil::ConvertToString(env, args[0]);
  bool value = base::NapiUtil::ConvertToBoolean(env, args[1]);
  DevToolSettingsHarmony::GetInstance().SyncBooleanToNative(key, value);
  return CreateUndefined(env);
}

bool DevToolSettingsHarmony::InitPreferences() {
  OH_PreferencesOption* option = OH_PreferencesOption_Create();
  if (option == nullptr) {
    return false;
  }
  int ret = OH_PreferencesOption_SetFileName(option, "DevToolSwitch");
  if (ret != PREFERENCES_OK) {
    (void)OH_PreferencesOption_Destroy(option);
    return false;
  }
  ret = OH_PreferencesOption_SetDataGroupId(option, "");
  if (ret != PREFERENCES_OK) {
    (void)OH_PreferencesOption_Destroy(option);
    return false;
  }
  ret = OH_PreferencesOption_SetBundleName(option, "com.lynx");
  if (ret != PREFERENCES_OK) {
    (void)OH_PreferencesOption_Destroy(option);
    return false;
  }
  int err_code = PREFERENCES_OK;
  preference_ = OH_Preferences_Open(option, &err_code);
  (void)OH_PreferencesOption_Destroy(option);
  return preference_ != nullptr;
}

void DevToolSettingsHarmony::SetPersistedBoolean(const std::string& key,
                                                 bool value) {
  if (preference_ == nullptr) {
    return;
  }
  (void)OH_Preferences_SetBool(preference_, key.c_str(), value);
}

bool DevToolSettingsHarmony::GetPersistedBoolean(const std::string& key,
                                                 bool default_value) {
  if (preference_ == nullptr) {
    return default_value;
  }
  bool value = false;
  auto result = OH_Preferences_GetBool(preference_, key.c_str(), &value);
  if (result == OH_Preferences_ErrCode::PREFERENCES_OK) {
    return value;
  }
  return default_value;
}

void DevToolSettingsHarmony::SyncBooleanToNative(const std::string& key,
                                                 bool value) {
  LynxEnv::GetInstance().SetBoolLocalEnv(key, value);
}

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
