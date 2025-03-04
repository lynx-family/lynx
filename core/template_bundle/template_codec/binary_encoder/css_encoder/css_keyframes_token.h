// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_BINARY_ENCODER_CSS_ENCODER_CSS_KEYFRAMES_TOKEN_H_
#define CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_BINARY_ENCODER_CSS_ENCODER_CSS_KEYFRAMES_TOKEN_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

#include "base/include/debug/lynx_assert.h"
#include "base/include/string/string_utils.h"
#include "core/renderer/css/css_keyframes_token.h"
#include "core/renderer/css/css_parser_token.h"
#include "core/renderer/css/unit_handler.h"
#include "core/template_bundle/template_codec/compile_options.h"

namespace lynx {
namespace encoder {

// TODO(songshourui.null): Subsequently, `encoder::CSSKeyframesToken` will be
// renamed to `encoder::KeyframesRuleForEncode`. Moreover, this class will no
// longer inherit from `tasm::CSSKeyframesToken`, but will hold a
// `tasm::CSSKeyframesToken`.
class CSSKeyframesToken : public tasm::CSSKeyframesToken {
 public:
  CSSKeyframesToken(const rapidjson::Value& value, const std::string& file,
                    const tasm::CompileOptions& compile_options);

  ~CSSKeyframesToken() override {}

  static bool IsCSSKeyframesToken(const rapidjson::Value& value);
  static std::string GetCSSKeyframesTokenName(const rapidjson::Value& value);

  tasm::CSSKeyframesMap& GetKeyframes() {
    if (!raw_styles_.empty()) {
      for (auto keyframe = raw_styles_.begin(); keyframe != raw_styles_.end();
           keyframe++) {
        auto key = keyframe->first;
        tasm::StyleMap* temp_map = styles_[key].get();
        if (temp_map == nullptr) {
          continue;
        }
        auto& raw_style_map = keyframe->second;
        for (auto style : *raw_style_map) {
          // TODO(songshourui.null): The parser_configs_ here are the default
          // CSSParserConfigs. They should be generated according to
          // compile_options_. However, since the previous logic was set up this
          // way, keep it as is for now and see if it needs fixing later.
          tasm::UnitHandler::ProcessCSSValue(style.first, style.second,
                                             *temp_map, parser_configs_);
        }
      }
      raw_styles_.clear();
    }
    return styles_;
  }

 private:
  void ParseStyles(const rapidjson::Value& value);
  void ConvertToCSSAttrsMap(const rapidjson::Value& value,
                            tasm::StyleMap& css_map);

  std::string file_;

  tasm::CSSKeyframesMap styles_;
  tasm::CSSRawKeyframesMap raw_styles_;

  const tasm::CompileOptions compile_options_;
};

}  // namespace encoder
}  // namespace lynx

#endif  // CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_BINARY_ENCODER_CSS_ENCODER_CSS_KEYFRAMES_TOKEN_H_
