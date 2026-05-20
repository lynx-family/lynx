// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_BINARY_ENCODER_CSS_ENCODER_CSS_RULE_PARSER_H_
#define CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_BINARY_ENCODER_CSS_ENCODER_CSS_RULE_PARSER_H_

#include <memory>
#include <string>
#include <vector>

#include "core/template_bundle/template_codec/binary_encoder/css_encoder/shared_css_fragment.h"
#include "core/template_bundle/template_codec/compile_options.h"
#include "third_party/rapidjson/document.h"

namespace lynx {
namespace tasm {

class CSSRuleParser {
 public:
  struct Diagnostic {
    std::string type;
    std::string name;
    int line = -1;
    int column = -1;
  };

  explicit CSSRuleParser(const CompileOptions& compile_options)
      : compile_options_(compile_options) {}

  std::unique_ptr<encoder::SharedCSSFragment> ParseCSSRules(
      const rapidjson::Value& ttss, const std::string& path,
      const std::vector<int32_t>& dependent_css_list, int32_t fragment_id);

  const std::vector<Diagnostic>& diagnostics() const { return diagnostics_; }

 private:
  std::unique_ptr<encoder::LynxStyleRule> ParseStyleRule(
      const rapidjson::Value& rule, const std::string& path);

  std::unique_ptr<encoder::LynxStyleRuleCondition> ParseConditionRule(
      const rapidjson::Value& rule, const std::string& path);

  std::unique_ptr<encoder::LynxStyleRuleKeyframes> ParseKeyframesRule(
      const rapidjson::Value& rule, const std::string& path);

  std::unique_ptr<encoder::LynxStyleRuleFontFace> ParseFontFaceRule(
      const rapidjson::Value& rule, const std::string& path);

  const CompileOptions& compile_options_;
  size_t rule_index_counter_ = 0;
  std::vector<Diagnostic> diagnostics_;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_BINARY_ENCODER_CSS_ENCODER_CSS_RULE_PARSER_H_
