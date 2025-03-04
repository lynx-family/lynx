// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_TASM_CONFIG_H_
#define CORE_RENDERER_TASM_CONFIG_H_

#include <mutex>
#include <string>
#include <vector>

#include "base/include/base_export.h"
#include "core/renderer/utils/lynx_env.h"
#include "core/template_bundle/template_codec/version.h"
#include "third_party/rapidjson/document.h"

// use to check engine and js match
// ENGINE_VERSION /  MIN_SUPPORTED_VERSION / NEED_CONSOLE_VERSION is deprecated
// Use TARGET_CLI_VERSION / MIN_SUPPORTED_TARGET_CLI_VERSION to do binary check
// They should be updated when release
#define ENGINE_VERSION "0.2.0.0"         // deprecated
#define MIN_SUPPORTED_VERSION "0.1.0.0"  // deprecated
#define NEED_CONSOLE_VERSION "0.1.0.0"
#define DEFAULT_FONT_SIZE_DP 14
#define DEFAULT_FONT_SCALE 1.f

#define LYNX_VERSION tasm::V_3_2  // need updated when release lynx
#define LYNX_TASM_MAX_SUPPORTED_VERSION \
  tasm::V_3_2  // need updated when release @lynx-js/tasm
#define MIN_SUPPORTED_LYNX_VERSION tasm::V_1_0  // updated when break change
// control features developed between release
// use this version to avoid break change
#define FEATURE_CONTROL_VERSION tasm::V_1_1
#define FEATURE_RADON_VERSION tasm::V_1_5

// feature implemented in feature_2_control_version:
// 1. closure
// 2. switch
#define FEATURE_CONTROL_VERSION_2 tasm::V_1_4
#define FEATURE_CSS_EXTERNAL_CLASS_VERSION tasm::V_1_6
// TODO(liyanbo): Only when all css parser in C++,can open this feature.
#define FEATURE_CSS_VALUE_VERSION tasm::V_2_0
#define FEATURE_CSS_STYLE_VARIABLES tasm::V_2_0
#define FEATURE_CSS_FONT_FACE_EXTENSION tasm::V_2_7
#define FEATURE_HEADER_EXT_INFO_VERSION tasm::V_1_6
#define FEATURE_DYNAMIC_COMPONENT_VERSION tasm::V_1_6
#define FEATURE_RADON_DYNAMIC_COMPONENT_VERSION tasm::V_2_1
#define BUGFIX_DYNAMIC_COMPONENT_DEFAULT_PROPS_VERSION tasm::V_2_6
#define FEATURE_DYNAMIC_COMPONENT_CONFIG tasm::V_2_7
#define FEATURE_DYNAMIC_COMPONENT_FALLBACK tasm::V_2_10
#define FEATURE_TEMPLATE_INFO tasm::V_2_7
#define FEATURE_FLEXIBLE_TEMPLATE tasm::V_2_8
#define FEATURE_FIBER_ARCH tasm::V_2_8
#define FEATURE_CONTEXT_GLOBAL_DATA tasm::V_2_8
#define FEATURE_TRIAL_OPTIONS_VERSION tasm::V_2_5
#define FEATURE_COMPONENT_CONFIG tasm::V_2_6
#define LYNX_VERSION_1_6 tasm::V_1_6
#define FEATURE_TEMPLATE_SCRIPT tasm::V_2_3
#define FEATURE_NEW_RENDER_PAGE tasm::V_2_1

#define LYNX_VERSION_1_0 tasm::V_1_0
#define LYNX_VERSION_1_1 tasm::V_1_1
#define LYNX_VERSION_1_2 tasm::V_1_2
#define LYNX_VERSION_1_3 tasm::V_1_3
#define LYNX_VERSION_1_4 tasm::V_1_4
#define LYNX_VERSION_1_5 tasm::V_1_5
#define LYNX_VERSION_1_6 tasm::V_1_6
#define LYNX_VERSION_2_0 tasm::V_2_0
#define LYNX_VERSION_2_1 tasm::V_2_1
#define LYNX_VERSION_2_2 tasm::V_2_2
#define LYNX_VERSION_2_3 tasm::V_2_3
#define LYNX_VERSION_2_4 tasm::V_2_4
#define LYNX_VERSION_2_5 tasm::V_2_5
#define LYNX_VERSION_2_6 tasm::V_2_6
#define LYNX_VERSION_2_7 tasm::V_2_7
#define LYNX_VERSION_2_8 tasm::V_2_8
#define LYNX_VERSION_2_9 tasm::V_2_9
#define LYNX_VERSION_2_10 tasm::V_2_10
#define LYNX_VERSION_2_11 tasm::V_2_11
#define LYNX_VERSION_2_12 tasm::V_2_12
#define LYNX_VERSION_2_13 tasm::V_2_13
#define LYNX_VERSION_2_14 tasm::V_2_14
#define LYNX_VERSION_2_15 tasm::V_2_15
#define LYNX_VERSION_2_16 tasm::V_2_16
#define LYNX_VERSION_2_17 tasm::V_2_17
#define LYNX_VERSION_2_18 tasm::V_2_18
#define LYNX_VERSION_3_0 tasm::V_3_0
#define LYNX_VERSION_3_1 tasm::V_3_1
#define LYNX_VERSION_3_2 tasm::V_3_2

#define LYNX_LEPUS_VERSION "2.3.0"

namespace lynx {
namespace tasm {
struct CompileOptions;
class Config {
 public:
  BASE_EXPORT_FOR_DEVTOOL static void InitializeVersion(
      const std::string& os_version);

  static inline const std::string& GetOsVersion() {
    return Instance()->os_version_;
  }

  static void InitPixelValues(int width, int height, float ratio) {
    Instance()->pixel_width_ = width;
    Instance()->pixel_height_ = height;
    Instance()->pixel_ratio_ = ratio;
  }

  // pixel_width_ and pixel_height_ are "Not maintained by
  // each LynxView independently", so they will be polluted. Use values
  // screen_width_ and screen_height_ in LynxEnvConfig stored in ElementManager.
  // BTW, the unit of pixel_width_ and pixel_height_ are physical pixel, while
  // screen_width_ and screen_height_ are layoutUnit.
  static inline int pixelWidth() { return Instance()->pixel_width_; }

  static inline int pixelHeight() { return Instance()->pixel_height_; }

  static inline float pixelRatio() { return Instance()->pixel_ratio_; }

  static inline bool GetConfig(LynxEnv::Key key,
                               const CompileOptions& compile_options) {
    return Instance()->GetConfigInternal(key, compile_options);
  }

  static inline std::string GetConfigString(
      LynxEnv::Key key, const CompileOptions& compile_options) {
    return Instance()->GetConfigStringInternal(key, compile_options);
  }

  static inline float DefaultFontScale() {
    return Instance()->default_font_scale_;
  }

  static inline const std::string& GetNeedConsoleVersion() {
    return Instance()->need_console_version_;
  }
  static inline const std::string& GetVersion() { return Instance()->version_; }
  static inline const std::string& GetMinSupportedVersion() {
    return Instance()->min_supported_version_;
  }

  static bool IsHigherOrEqual(const std::string& target_v,
                              const base::Version& base) {
    constexpr const static char* kNull = "null";
    // check for context compile, if target_v == "null", higher than any other
    // versions
    if (target_v == kNull) {
      return true;
    }
    return IsHigherOrEqual(base::Version(target_v), base);
  }

  static bool IsHigherOrEqual(const base::Version& target,
                              const base::Version& base) {
    return target >= base;
  }

  static inline const std::string& GetMinSupportLynxVersion() {
    return Instance()->min_supported_lynx_version_;
  }

  static inline const std::string& GetCurrentLynxVersion() {
    return Instance()->lynx_version_;
  }

  // TODO(zhixuan): Remove after experiment is done.
  static bool TrialAsyncHydration();

  static const char* Platform();

 private:
  BASE_EXPORT_FOR_DEVTOOL static Config* Instance();
  Config();
  bool GetConfigInternal(LynxEnv::Key key,
                         const CompileOptions& compile_options);
  std::string GetConfigStringInternal(LynxEnv::Key key,
                                      const CompileOptions& compile_options);

 private:
  float default_font_scale_;
  std::string os_version_;
  std::string version_;                // deprecated
  std::string min_supported_version_;  // deprecated
  std::string need_console_version_;

  // used these to do version check
  std::string lynx_version_;
  std::string min_supported_lynx_version_;
  int pixel_width_, pixel_height_;
  float pixel_ratio_;
};
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_TASM_CONFIG_H_
