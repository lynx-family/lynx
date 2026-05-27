// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/template_bundle/template_codec/binary_encoder/css_encoder/css_rule_parser.h"

#include <cstring>
#include <utility>

#include "base/include/log/logging.h"
#include "core/renderer/css/ng/parser/css_parser_token_range.h"
#include "core/renderer/css/ng/parser/css_tokenizer.h"
#include "core/renderer/css/ng/selector/css_parser_context.h"
#include "core/renderer/css/ng/selector/css_selector_parser.h"
#include "core/template_bundle/template_codec/binary_encoder/css_encoder/css_font_face_token.h"
#include "core/template_bundle/template_codec/binary_encoder/css_encoder/css_keyframes_token.h"
#include "core/template_bundle/template_codec/binary_encoder/css_encoder/css_parse_token_group.h"
#include "core/template_bundle/template_codec/binary_encoder/css_encoder/css_parser.h"
#include "core/template_bundle/template_codec/binary_encoder/css_encoder/css_parser_token.h"

namespace lynx {
namespace tasm {

std::unique_ptr<encoder::LynxStyleRule> CSSRuleParser::ParseStyleRule(
    const rapidjson::Value &rule, const std::string &path) {
  const size_t position = rule_index_counter_++;
  size_t flattened_size = 0;
  std::unique_ptr<css::LynxCSSSelector[]> selector_arr;
  fml::RefPtr<CSSParseToken> properties;

  if (!rule.HasMember(SELECTORTEXT)) {
    return nullptr;
  }
  const auto &selector_text = rule[SELECTORTEXT];
  if (!selector_text.HasMember("value") || !selector_text["value"].IsString()) {
    return nullptr;
  }
  std::string selector = selector_text["value"].GetString();

  css::CSSParserContext context;
  css::CSSTokenizer tokenizer(selector);
  const auto parser_tokens = tokenizer.TokenizeToEOF();
  css::CSSParserTokenRange range(parser_tokens);
  css::LynxCSSSelectorVector selector_vector =
      css::CSSSelectorParser::ParseSelector(range, &context);

  flattened_size = css::CSSSelectorParser::FlattenedSize(selector_vector);
  if (flattened_size == 0) {
    Diagnostic d{"selector", selector};
    CSSParser::ExtractLoc(selector_text, "loc", d.line, d.column);
    diagnostics_.emplace_back(d);
    return nullptr;
  }
  selector_arr = std::make_unique<css::LynxCSSSelector[]>(flattened_size);
  css::CSSSelectorParser::AdoptSelectorVector(
      selector_vector, selector_arr.get(), flattened_size);

  if (!rule.HasMember(STYLE)) {
    return nullptr;
  }
  const rapidjson::Value &css_style = rule[STYLE];
  const rapidjson::Value &style_variables = rule[STYLE_VARIABLES];
  properties = fml::AdoptRef(new encoder::CSSParseToken(
      css_style, selector, path, style_variables, compile_options_));

  return std::make_unique<encoder::LynxStyleRule>(
      flattened_size, position, std::move(selector_arr), std::move(properties));
}

std::unique_ptr<encoder::LynxStyleRuleCondition>
CSSRuleParser::ParseConditionRule(const rapidjson::Value &rule,
                                  const std::string &path) {
  if (!rule[TYPE].IsString()) {
    return nullptr;
  }
  const auto &type = rule[TYPE].GetString();
  encoder::LynxStyleRuleBase::RuleType rule_type = CSSRuleType::kUnknown;
  if (strcmp(type, "MediaRule") == 0) {
    rule_type = CSSRuleType::kMedia;
  } else if (strcmp(type, "SupportsRule") == 0) {
    rule_type = CSSRuleType::kSupports;
  } else {
    diagnostics_.emplace_back(Diagnostic{"rule", type});
    return nullptr;
  }

  auto condition_rule =
      std::make_unique<encoder::LynxStyleRuleCondition>(rule_type);

  if (rule.HasMember("prelude")) {
    const auto &prelude = rule["prelude"];
    if (prelude.HasMember("value") && prelude["value"].IsString()) {
      condition_rule->condition = prelude["value"].GetString();
    }
  }

  if (rule.HasMember("rules") && rule["rules"].IsArray()) {
    const auto &child_rules = rule["rules"];
    for (rapidjson::SizeType i = 0; i < child_rules.Size(); i++) {
      const auto &child = child_rules[i];
      if (!child.HasMember(TYPE) || !child[TYPE].IsString()) {
        continue;
      }
      const char *child_type = child[TYPE].GetString();
      if (strcmp(child_type, STYLE_RULE) == 0) {
        auto child_rule = ParseStyleRule(child, path);
        if (child_rule) {
          condition_rule->child_rules.push_back(std::move(child_rule));
        }
      } else {
        diagnostics_.emplace_back(
            Diagnostic{"condition child rule", child_type});
      }
    }
  }

  return condition_rule;
}

std::unique_ptr<encoder::LynxStyleRuleKeyframes>
CSSRuleParser::ParseKeyframesRule(const rapidjson::Value &rule,
                                  const std::string &path) {
  auto keyframes_rule = std::make_unique<encoder::LynxStyleRuleKeyframes>();
  keyframes_rule->name =
      encoder::CSSKeyframesToken::GetCSSKeyframesTokenName(rule);
  keyframes_rule->properties = fml::AdoptRef(
      new encoder::CSSKeyframesToken(rule, path, compile_options_));
  return keyframes_rule;
}

std::unique_ptr<encoder::LynxStyleRuleFontFace>
CSSRuleParser::ParseFontFaceRule(const rapidjson::Value &rule,
                                 const std::string &path) {
  auto fontface_rule = std::make_unique<encoder::LynxStyleRuleFontFace>();
  fontface_rule->family = CSSFontFaceToken::GetCSSFontFaceTokenKey(rule);
  fontface_rule->properties.emplace_back(
      std::make_shared<CSSFontFaceToken>(rule, path));
  return fontface_rule;
}

std::unique_ptr<encoder::SharedCSSFragment> CSSRuleParser::ParseCSSRules(
    const rapidjson::Value &ttss, const std::string &path,
    const std::vector<int32_t> &dependent_css_list, int32_t fragment_id) {
  rule_index_counter_ = 0;
  std::vector<std::unique_ptr<encoder::LynxStyleRuleBase>> rules;

  for (rapidjson::SizeType i = 0; i < ttss.Size(); i++) {
    const auto &rule = ttss[i];
    if (!rule.HasMember(TYPE) || !rule[TYPE].IsString()) {
      continue;
    }
    const char *type = rule[TYPE].GetString();

    if (strcmp(type, STYLE_RULE) == 0) {
      auto style_rule = ParseStyleRule(rule, path);
      if (style_rule) {
        rules.push_back(std::move(style_rule));
      }
    } else if (strcmp(type, "MediaRule") == 0 ||
               strcmp(type, "SupportsRule") == 0) {
      auto condition_rule = ParseConditionRule(rule, path);
      if (condition_rule) {
        rules.push_back(std::move(condition_rule));
      }
    } else if (encoder::CSSKeyframesToken::IsCSSKeyframesToken(rule)) {
      rules.push_back(ParseKeyframesRule(rule, path));
    } else if (CSSFontFaceToken::IsCSSFontFaceToken(rule)) {
      rules.push_back(ParseFontFaceRule(rule, path));
    } else {
      diagnostics_.emplace_back(Diagnostic{"rule", type});
    }
  }

  return std::make_unique<encoder::SharedCSSFragment>(
      fragment_id, dependent_css_list, std::move(rules));
}

}  // namespace tasm
}  // namespace lynx
