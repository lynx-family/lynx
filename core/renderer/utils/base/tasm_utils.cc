// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/utils/base/tasm_utils.h"

#include "base/include/value/base_string.h"
#include "core/renderer/tasm/config.h"
#include "core/runtime/vm/lepus/table.h"

namespace lynx {
namespace tasm {

lepus::Value GenerateSystemInfo(const lepus::Value* config) {
  BASE_STATIC_STRING_DECL(kPlatform, "platform");
  BASE_STATIC_STRING_DECL(kPixelRatio, "pixelRatio");
  BASE_STATIC_STRING_DECL(kPixelWidth, "pixelWidth");
  BASE_STATIC_STRING_DECL(kPixelHeight, "pixelHeight");
  BASE_STATIC_STRING_DECL(kLynxSdkVersion, "lynxSdkVersion");
  BASE_STATIC_STRING_DECL(kEngineVersion, "engineVersion");
  BASE_STATIC_STRING_DECL(kTheme, "theme");

  static base::String kPlatformValue(Config::Platform());

  // add for global setting
  lepus::Value system_info = lepus::Value(lepus::Dictionary::Create());
  system_info.SetProperty(kPlatform, lepus::Value(kPlatformValue));
  system_info.SetProperty(kPixelRatio, lepus::Value(Config::pixelRatio()));
  system_info.SetProperty(kPixelWidth, lepus::Value(Config::pixelWidth()));
  system_info.SetProperty(kPixelHeight, lepus::Value(Config::pixelHeight()));
  system_info.SetProperty(kLynxSdkVersion,
                          lepus::Value(Config::GetCurrentLynxVersion()));
  system_info.SetProperty(kEngineVersion,
                          lepus::Value(Config::GetCurrentLynxVersion()));

  bool has_theme = false;
  if (config != nullptr && config->IsObject()) {
    auto theme = config->GetProperty(kTheme);
    if (theme.IsObject()) {
      system_info.SetProperty(kTheme, theme);
      has_theme = true;
    }
  }
  if (!has_theme) {
    // add default
    system_info.SetProperty(kTheme,
                            lepus::Value(lynx::lepus::Dictionary::Create()));
  }

  return system_info;
}

}  // namespace tasm
}  // namespace lynx
