// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_BINARY_ENCODER_CSS_ENCODER_CSS_PARSER_H_
#define CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_BINARY_ENCODER_CSS_ENCODER_CSS_PARSER_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "core/renderer/css/css_parser_token.h"
#include "core/renderer/css/shared_css_fragment.h"
#include "core/template_bundle/template_codec/binary_encoder/css_encoder/shared_css_fragment.h"
#include "core/template_bundle/template_codec/compile_options.h"
#include "third_party/rapidjson/document.h"

namespace lynx {
namespace tasm {

class CSSParser {
 public:
  CSSParser(const CompileOptions &compile_options);

  bool Parse(const rapidjson::Value &value);

  bool ParseCSSForFiber(const rapidjson::Value &css_map,
                        const rapidjson::Value &css_source);

  ~CSSParser() {}

  static void MergeCSSParseToken(std::shared_ptr<CSSParseToken> &originToken,
                                 std::shared_ptr<CSSParseToken> &newToken);

  // Parse result
  const std::unordered_map<std::string, encoder::SharedCSSFragment *> &
  fragments() {
    return fragments_;
  }

 private:
  // Parse ttss file
  bool ParseOtherTTSS(const rapidjson::Value &value);
  void ParseAppTTSS(const rapidjson::Value &value);

  void ParseCSS(const rapidjson::Value &value, const std::string &path);
  void ParseCSS(const rapidjson::Value &value, const std::string &path,
                const std::vector<int32_t> &dependent_css_list,
                int32_t fragment_id);
  void ParseCSSTokens(CSSParserTokenMap &css, const rapidjson::Value &value,
                      const std::string &path);

  void ParseCSSTokensNew(
      std::vector<encoder::LynxCSSSelectorTuple> &selector_tuple_lists,
      CSSParserTokenMap &css, const rapidjson::Value &value,
      const std::string &path);

  void ParseCSSKeyframes(encoder::CSSKeyframesTokenMapForEncode &keyframes,
                         const rapidjson::Value &value,
                         const std::string &path);
  void ParseCSSFontFace(encoder::CSSFontFaceTokenMapForEncode &fontfaces,
                        const rapidjson::Value &value, const std::string &path);

  // For fiber
  void ParseCSS(const rapidjson::Value &map, const rapidjson::Value &id,
                const rapidjson::Value &source);

  std::vector<std::unique_ptr<encoder::SharedCSSFragment>>
      shared_css_fragments_;

  std::unordered_map<std::string, encoder::SharedCSSFragment *> fragments_;
  const CompileOptions &compile_options_;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_BINARY_ENCODER_CSS_ENCODER_CSS_PARSER_H_
