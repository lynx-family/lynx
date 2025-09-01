// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_devtool/src/main/cpp/devtool_env_harmony.h"

#include <database/preferences/oh_preferences_err_code.h>
#include <database/preferences/oh_preferences_option.h>
#include <database/preferences/oh_preferences_value.h>
#include <js_native_api.h>

#include <cassert>
#include <cstring>
#include <memory>
#include <set>
#include <utility>
#include <vector>

#include "base/include/platform/harmony/napi_util.h"
#include "core/base/harmony/napi_convert_helper.h"
#include "core/public/jsb/native_module_factory.h"
#include "core/renderer/utils/lynx_env.h"
#include "core/runtime/bindings/jsi/modules/lynx_module_manager.h"
#include "devtool/embedder/core/debug_bridge_embedder.h"
#include "devtool/embedder/core/env_embedder.h"
#include "devtool/embedder/core/lynx_devtool_set_module.h"

namespace lynx {
namespace devtool {

std::unordered_map<std::string, bool>
    DevToolEnvHarmony::s_persistent_default_value_ = {
        {"enable_devtool", false},
        {"enable_debug_mode", false},
        {"enable_logbox", true},
        {"enable_quickjs_debug", true},
        {"enable_dom_tree", true},
        {"enable_launch_record", false},
        {"enable_quickjs_cache", true},
        {"enable_long_press_menu", true},
        {"enable_perf_monitor_debug", true},
        {"error_code_css", false},
        {"enable_pixel_copy", false}};

DevToolEnvHarmony::DevToolEnvHarmony() {
  if (!InitPreferences()) {
    LOGW("InitPreferences failed");
  }

  for (auto &it : s_persistent_default_value_) {
    std::string key = it.first;
    bool defaultValue = it.second;
    bool value = false;
    auto result = OH_Preferences_GetBool(preference_, key.c_str(), &value);
    if (result == OH_Preferences_ErrCode::PREFERENCES_OK) {
      tasm::LynxEnv::GetInstance().SetBoolLocalEnv(key, value);
    } else {
      tasm::LynxEnv::GetInstance().SetBoolLocalEnv(key, defaultValue);
    }
  }
}

DevToolEnvHarmony::~DevToolEnvHarmony() {
  if (preference_ != nullptr) {
    (void)OH_Preferences_Close(preference_);
    preference_ = nullptr;
  }
}

bool DevToolEnvHarmony::InitPreferences() {
  OH_PreferencesOption *option = OH_PreferencesOption_Create();
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
  int errCode = PREFERENCES_OK;
  preference_ = OH_Preferences_Open(option, &errCode);
  (void)OH_PreferencesOption_Destroy(option);
  option = nullptr;
  if (preference_ == nullptr) {
    return false;
  }
  return true;
}

napi_value DevToolEnvHarmony::Init(napi_env env, napi_value exports) {
#define DECLARE_NAPI_STATIC_FUNCTION(name, func) \
  {(name), nullptr, (func), nullptr, nullptr, nullptr, napi_static, nullptr}

  napi_property_descriptor properties[] = {
      DECLARE_NAPI_STATIC_FUNCTION("setSwitch", SetSwitchNAPI),
      DECLARE_NAPI_STATIC_FUNCTION("getSwitch", GetSwitchNAPI),
      DECLARE_NAPI_STATIC_FUNCTION("setAppInfo", SetAppInfo),
      DECLARE_NAPI_STATIC_FUNCTION("initDevToolSetModule",
                                   InitDevToolSetModule),
  };
#undef DECLARE_NAPI_STATIC_FUNCTION
  constexpr size_t size = std::size(properties);
  napi_value cons;
  napi_status status =
      napi_define_class(env, "LynxDevToolEnvHarmony", NAPI_AUTO_LENGTH,
                        Constructor, nullptr, size, properties, &cons);
  assert(status == napi_ok);
  status = napi_set_named_property(env, exports, "LynxDevToolEnvHarmony", cons);
  assert(status == napi_ok);
  return exports;
}

napi_value DevToolEnvHarmony::Constructor(napi_env env,
                                          napi_callback_info info) {
  napi_value js_this;
  napi_get_cb_info(env, info, nullptr, nullptr, &js_this, nullptr);
  return js_this;
}

DevToolEnvHarmony &DevToolEnvHarmony::GetInstance() {
  static base::NoDestructor<DevToolEnvHarmony> instance;
  return *instance;
}

napi_value DevToolEnvHarmony::SetSwitchNAPI(napi_env env,
                                            napi_callback_info info) {
  size_t argc = 2;
  napi_value args[2] = {nullptr};
  // (key:string, value:boolean)
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  std::string key = base::NapiUtil::ConvertToString(env, args[0]);
  bool value = base::NapiUtil::ConvertToBoolean(env, args[1]);
  DevToolEnvHarmony::GetInstance().SetSwitch(key, value, NeedPersistent(key));
  return nullptr;
}

napi_value DevToolEnvHarmony::GetSwitchNAPI(napi_env env,
                                            napi_callback_info info) {
  size_t argc = 1;
  napi_value args[1] = {nullptr};
  // (key:string)
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  std::string key = base::NapiUtil::ConvertToString(env, args[0]);
  napi_value result = nullptr;
  napi_status ret = napi_get_boolean(
      env, DevToolEnvHarmony::GetInstance().GetSwitch(key), &result);
  if (ret != napi_ok) {
    LOGW("Failed to convert boolean value to napi_value");
    return nullptr;
  }
  return result;
}

bool DevToolEnvHarmony::NeedPersistent(const std::string &key) {
  if (s_persistent_default_value_.find(key) !=
      s_persistent_default_value_.end()) {
    return true;
  }
  return false;
}

bool DevToolEnvHarmony::SetPersistentSwitch(const std::string &key,
                                            bool value) {
  auto ret = OH_Preferences_SetBool(preference_, key.c_str(), value);
  if (ret != PREFERENCES_OK) {
    return false;
  }
  return true;
}

void DevToolEnvHarmony::SetSwitch(const std::string &key, bool value,
                                  bool is_persistent) {
  tasm::LynxEnv::GetInstance().SetBoolLocalEnv(key, value);
  if (is_persistent) {
    SetPersistentSwitch(key, value);
  }
}

bool DevToolEnvHarmony::GetSwitch(const std::string &key) {
  if (!tasm::LynxEnv::GetInstance().ContainKey(key)) {
    bool value = false;
    auto result = OH_Preferences_GetBool(preference_, key.c_str(), &value);
    if (result == OH_Preferences_ErrCode::PREFERENCES_OK) {
      tasm::LynxEnv::GetInstance().SetBoolLocalEnv(key, value);
    }
  }
  return tasm::LynxEnv::GetInstance().GetBoolEnv(key, false);
}

napi_value DevToolEnvHarmony::SetAppInfo(napi_env env,
                                         napi_callback_info info) {
  size_t argc = 2;
  napi_value args[2] = {nullptr};
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
  std::vector<std::string> keys;
  base::NapiUtil::ConvertToArrayString(env, args[0], keys);
  std::vector<std::string> values;
  base::NapiUtil::ConvertToArrayString(env, args[1], values);
  if (keys.size() != values.size()) {
    return nullptr;
  }
  std::unordered_map<std::string, std::string> options;
  for (size_t i = 0; i < keys.size(); ++i) {
    options[keys[i]] = values[i];
  }
  DebugBridgeEmbedder::GetInstance().SetAppInfo(options);
  return nullptr;
}

napi_value DevToolEnvHarmony::InitDevToolSetModule(napi_env env,
                                                   napi_callback_info info) {
  size_t argc = 1;
  napi_value args[1] = {nullptr};
  napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

  uint64_t module_manager_num = base::NapiUtil::ConvertToPtr(env, args[0]);

  auto module_manager =
      reinterpret_cast<piper::LynxModuleManager *>(module_manager_num);
  std::unique_ptr<piper::NativeModuleFactory> devtool_module_factory_ =
      std::make_unique<piper::NativeModuleFactory>();
  devtool_module_factory_->Register(LynxDevToolSetModule::GetName(),
                                    LynxDevToolSetModule::Create);
  module_manager->SetModuleFactory(std::move(devtool_module_factory_));

  return nullptr;
}

void EnvEmbedder::SetSwitch(const std::string &key, bool value) {
  devtool::DevToolEnvHarmony::GetInstance().SetSwitch(
      key, value, devtool::DevToolEnvHarmony::NeedPersistent(key));
}

bool EnvEmbedder::GetSwitch(const std::string &key) {
  return devtool::DevToolEnvHarmony::GetInstance().GetSwitch(key);
}

}  // namespace devtool
}  // namespace lynx
