// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_GENERATOR_PAGE_CONFIG_COMPILE_OPTIONS_HELPER_H_
#define CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_GENERATOR_PAGE_CONFIG_COMPILE_OPTIONS_HELPER_H_

#include <string>

#include "core/template_bundle/template_codec/binary_decoder/lynx_config_constant_auto_gen.h"
#include "core/template_bundle/template_codec/compile_options.h"
#include "third_party/rapidjson/document.h"

namespace lynx {
namespace tasm {

inline void ApplyPageConfigDerivedCompileOptions(
    CompileOptions& compile_options, const std::string& config_json) {
  if (config_json.empty()) {
    return;
  }

  rapidjson::Document doc;
  if (doc.Parse(config_json.c_str()).HasParseError()) {
    return;
  }

  if (doc.HasMember(config::kEnableParseIntFlex) &&
      doc[config::kEnableParseIntFlex].IsBool()) {
    compile_options.enable_parse_int_flex_ =
        doc[config::kEnableParseIntFlex].GetBool();
  }
}

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_GENERATOR_PAGE_CONFIG_COMPILE_OPTIONS_HELPER_H_
