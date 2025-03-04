// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_CSS_PARSER_CSS_PARSER_CONFIGS_H_
#define CORE_RENDERER_CSS_PARSER_CSS_PARSER_CONFIGS_H_

#include <string>

#include "core/renderer/tasm/config.h"
#include "core/template_bundle/template_codec/compile_options.h"
#include "core/template_bundle/template_codec/version.h"

namespace lynx {
namespace tasm {

struct CSSParserConfigs {
  static CSSParserConfigs GetCSSParserConfigsByComplierOptions(
      const CompileOptions& compile_options) {
    CSSParserConfigs config;
    if (!compile_options.target_sdk_version_.empty() &&
        std::isdigit(compile_options.target_sdk_version_[0])) {
      base::Version version(compile_options.target_sdk_version_);
      if (version >= V_2_6) {
        config.enable_length_unit_check = true;
      }
      if (version < V_1_6) {
        config.enable_legacy_parser = true;
      }
      if (version >= V_2_11) {
        config.enable_new_border_handler = true;
      }
      if (version >= V_2_12) {
        config.enable_new_transform_handler = true;
        config.enable_new_flex_handler = true;
        config.enable_new_time_handler = true;
      }
    }
    config.enable_css_strict_mode = compile_options.enable_css_strict_mode_;
    config.remove_css_parser_log = compile_options.remove_css_parser_log_;
    return config;
  }
  // default is disable.
  bool enable_css_strict_mode = false;
  bool remove_css_parser_log = false;
  bool enable_legacy_parser = false;
  bool enable_length_unit_check = false;
  bool enable_new_border_handler = false;
  bool enable_new_transform_handler = false;
  bool enable_new_flex_handler = false;
  bool enable_new_time_handler = false;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_CSS_PARSER_CSS_PARSER_CONFIGS_H_
