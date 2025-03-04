// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/tasm/config.h"

#include <utility>

#include "base/include/log/logging.h"
#include "core/template_bundle/template_codec/compile_options.h"
namespace lynx {
namespace tasm {

Config::Config()
    : default_font_scale_(DEFAULT_FONT_SCALE),
      version_(ENGINE_VERSION),
      min_supported_version_(MIN_SUPPORTED_VERSION),
      need_console_version_(NEED_CONSOLE_VERSION),
      lynx_version_(LYNX_VERSION.ToString()),
      min_supported_lynx_version_(MIN_SUPPORTED_LYNX_VERSION.ToString()) {}

Config* Config::Instance() {
  static Config* kConfig = new Config;
  return kConfig;
}

void Config::InitializeVersion(const std::string& os_version) {
  Instance()->os_version_ = os_version;
}

bool Config::GetConfigInternal(LynxEnv::Key key,
                               const CompileOptions& compile_options) {
  if (compile_options.config_type == CONFIG_TYPE_EXPERIMENT_SETTINGS) {
    return LynxEnv::GetInstance().GetBoolEnv(key, false);
  }
  return false;
}

std::string Config::GetConfigStringInternal(
    LynxEnv::Key key, const CompileOptions& compile_options) {
  if (compile_options.config_type == CONFIG_TYPE_EXPERIMENT_SETTINGS) {
    const std::optional<std::string>& value =
        LynxEnv::GetInstance().GetStringEnv(key);
    return value.has_value() ? *value : "";
  }
  return "";
}

bool Config::TrialAsyncHydration() {
  // cache the setting.
  static bool trail_async_hydration =
      GetConfig(LynxEnv::Key::TRAIL_ASYNC_HYDRATION, CompileOptions());
  return trail_async_hydration;
}

}  // namespace tasm
}  // namespace lynx
