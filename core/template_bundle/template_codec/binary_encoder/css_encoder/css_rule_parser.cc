// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/template_bundle/template_codec/binary_encoder/css_encoder/css_rule_parser.h"

#include <cstring>
#include <unordered_map>
#include <utility>

#include "base/include/log/logging.h"
#include "base/include/value/array.h"
#include "core/renderer/css/ng/css_ng_utils.h"
#include "core/renderer/css/ng/media_query/media_query_set.h"
#include "core/renderer/css/ng/parser/css_parser_token_range.h"
#include "core/renderer/css/ng/parser/css_parser_token_stream.h"
#include "core/renderer/css/ng/parser/css_tokenizer.h"
#include "core/renderer/css/ng/parser/font_face_parser.h"
#include "core/renderer/css/ng/parser/media_query_parser.h"
#include "core/renderer/css/ng/parser/supports_condition_parser.h"
#include "core/renderer/css/ng/selector/css_parser_context.h"
#include "core/renderer/css/ng/selector/css_selector_parser.h"
#include "core/renderer/css/ng/supports/supports_condition.h"
#include "core/template_bundle/template_codec/binary_encoder/css_encoder/css_font_face_token.h"
#include "core/template_bundle/template_codec/binary_encoder/css_encoder/css_keyframes_token.h"
#include "core/template_bundle/template_codec/binary_encoder/css_encoder/css_parse_token_group.h"
#include "core/template_bundle/template_codec/binary_encoder/css_encoder/css_parser.h"
#include "core/template_bundle/template_codec/binary_encoder/css_encoder/css_parser_token.h"

namespace lynx {
namespace tasm {
namespace {

constexpr const char* kName = "name";
constexpr const char* kValue = "value";

std::unordered_map<std::string, std::string> ExtractFontFaceDescriptors(
    const rapidjson::Value& rule) {
  std::unordered_map<std::string, std::string> descriptors;
  if (!rule.HasMember(STYLE)) return descriptors;

  const rapidjson::Value& style = rule[STYLE];
  if (!style.IsArray()) {
    return descriptors;
  }
  for (const auto& attribute : style.GetArray()) {
    if (!attribute.IsObject() || !attribute.HasMember(kName) ||
        !attribute[kName].IsString() || !attribute.HasMember(kValue) ||
        !attribute[kValue].IsString()) {
      continue;
    }
    descriptors[attribute[kName].GetString()] = attribute[kValue].GetString();
  }
  return descriptors;
}

void ExtractFontFaceDiagnosticLoc(const rapidjson::Value& rule,
                                  CSSRuleParser::Diagnostic& diagnostic) {
  if (!rule.HasMember(STYLE) || !rule[STYLE].IsArray()) return;

  for (const auto& attribute : rule[STYLE].GetArray()) {
    if (!attribute.IsObject()) continue;
    if (attribute.HasMember(kName) && attribute[kName].IsString() &&
        std::string(attribute[kName].GetString()) == "font-family") {
      CSSParser::ExtractLoc(attribute, "keyLoc", diagnostic.line,
                            diagnostic.column);
      return;
    }
  }
}

}  // namespace

std::unique_ptr<encoder::LynxStyleRule> CSSRuleParser::ParseStyleRule(
    const rapidjson::Value& rule, const std::string& path) {
  const size_t position = rule_index_counter_++;
  size_t flattened_size = 0;
  std::unique_ptr<css::LynxCSSSelector[]> selector_arr;
  fml::RefPtr<CSSParseToken> properties;

  if (!rule.HasMember(SELECTORTEXT)) {
    return nullptr;
  }
  const auto& selector_text = rule[SELECTORTEXT];
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
  const rapidjson::Value& css_style = rule[STYLE];
  const rapidjson::Value& style_variables = rule[STYLE_VARIABLES];
  properties = fml::AdoptRef(new encoder::CSSParseToken(
      css_style, selector, path, style_variables, compile_options_));

  return std::make_unique<encoder::LynxStyleRule>(
      flattened_size, position, std::move(selector_arr), std::move(properties));
}

std::unique_ptr<encoder::LynxStyleRuleCondition>
CSSRuleParser::ParseConditionRule(const rapidjson::Value& rule,
                                  const std::string& path) {
  if (!rule[TYPE].IsString()) {
    return nullptr;
  }
  const auto& type = rule[TYPE].GetString();
  CSSRuleType rule_type = CSSRuleType::kUnknown;
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

  std::string condition_text;
  if (rule.HasMember("prelude")) {
    const auto& prelude = rule["prelude"];
    if (prelude.HasMember("value") && prelude["value"].IsString()) {
      condition_text = prelude["value"].GetString();
    }
  }

  if (rule_type == CSSRuleType::kMedia) {
    auto media_queries =
        css::MediaQueryParser::ParseMediaQuerySet(condition_text);
    condition_rule->condition_value = media_queries->ToLepus();
  } else if (rule_type == CSSRuleType::kSupports) {
    auto supports_condition =
        css::SupportsConditionParser::Parse(condition_text);
    if (!supports_condition) {
      Diagnostic d{"condition", condition_text};
      if (rule.HasMember("prelude")) {
        CSSParser::ExtractLoc(rule["prelude"], "loc", d.line, d.column);
      }
      diagnostics_.emplace_back(d);
      return nullptr;
    }
    condition_rule->condition_value = supports_condition->ToLepus();
  }

  if (rule.HasMember("rules") && rule["rules"].IsArray()) {
    const auto& child_rules = rule["rules"];
    for (rapidjson::SizeType i = 0; i < child_rules.Size(); i++) {
      const auto& child = child_rules[i];
      if (!child.HasMember(TYPE) || !child[TYPE].IsString()) {
        continue;
      }
      const char* child_type = child[TYPE].GetString();
      if (strcmp(child_type, STYLE_RULE) == 0) {
        auto child_rule = ParseStyleRule(child, path);
        if (child_rule) {
          condition_rule->child_rules.push_back(std::move(child_rule));
        }
      } else if (encoder::CSSKeyframesToken::IsCSSKeyframesToken(child)) {
        condition_rule->child_rules.push_back(ParseKeyframesRule(child, path));
      } else if (CSSFontFaceToken::IsCSSFontFaceToken(child)) {
        auto fontface_rule = ParseFontFaceRule(child);
        if (fontface_rule) {
          condition_rule->child_rules.push_back(std::move(fontface_rule));
        }
      } else {
        Diagnostic d{"condition child rule", child_type};
        if (child.HasMember("prelude")) {
          CSSParser::ExtractLoc(child["prelude"], "loc", d.line, d.column);
        }
        diagnostics_.emplace_back(d);
      }
    }
  }

  return condition_rule;
}

std::unique_ptr<encoder::LynxStyleRuleKeyframes>
CSSRuleParser::ParseKeyframesRule(const rapidjson::Value& rule,
                                  const std::string& path) {
  auto keyframes_rule = std::make_unique<encoder::LynxStyleRuleKeyframes>();
  keyframes_rule->name =
      encoder::CSSKeyframesToken::GetCSSKeyframesTokenName(rule);
  keyframes_rule->properties = fml::AdoptRef(
      new encoder::CSSKeyframesToken(rule, path, compile_options_));
  return keyframes_rule;
}

std::unique_ptr<encoder::LynxStyleRuleFontFace>
CSSRuleParser::ParseFontFaceRule(const rapidjson::Value& rule) {
  auto descriptors = ExtractFontFaceDescriptors(rule);
  auto parsed_rule = css::FontFaceParser::Parse(descriptors);
  if (!parsed_rule) {
    Diagnostic diagnostic{"font-face", descriptors["font-family"]};
    ExtractFontFaceDiagnosticLoc(rule, diagnostic);
    diagnostics_.emplace_back(std::move(diagnostic));
    return nullptr;
  }
  auto fontface_rule = std::make_unique<encoder::LynxStyleRuleFontFace>();
  fontface_rule->family = parsed_rule->Family();
  fontface_rule->font_face_rule = parsed_rule->ToLepus();
  return fontface_rule;
}

namespace {

// Consume a single <layer-name> from the token stream per CSS Cascading
// and Inheritance Level 5:
//   <layer-name> = <ident> [ '.' <ident> ]*
// Whitespace is NOT permitted between an <ident> and a following '.', nor
// between a '.' and the following <ident>. This mirrors Blink/Servo, which
// both consume the '.' / next ident "without including whitespace".
//
// On success, returns true with `out` filled and the stream advanced past
// the last <ident>. On failure, returns false; the stream position is
// undefined and the caller should treat the entire <layer-name> as
// invalid.
bool ConsumeLayerName(css::CSSParserTokenStream& stream,
                      std::vector<std::string>& out) {
  if (stream.Peek().GetType() != css::kIdentToken) {
    return false;
  }
  out.push_back(css::ustring_helper::to_string(stream.Peek().Value()));
  stream.Consume();
  // No whitespace is consumed here: a '.' is only part of the name when it
  // immediately follows the previous ident.
  while (stream.Peek().GetType() == css::kDelimiterToken &&
         stream.Peek().Delimiter() == u'.') {
    stream.Consume();
    // No whitespace consumed: an <ident> must immediately follow '.'.
    if (stream.Peek().GetType() != css::kIdentToken) {
      return false;
    }
    out.push_back(css::ustring_helper::to_string(stream.Peek().Value()));
    stream.Consume();
  }
  return true;
}

}  // namespace

std::vector<std::unique_ptr<encoder::LynxStyleRuleLayer>>
CSSRuleParser::ParseLayerRule(const rapidjson::Value& rule,
                              const std::string& path) {
  std::vector<std::unique_ptr<encoder::LynxStyleRuleLayer>> result;

  // Producer contract (postcss-based upstream): block form (`@layer foo
  // { ... }`) emits a non-empty `rules` array; statement form
  // (`@layer foo, bar;`) emits `"rules": []`. We therefore use
  // `rules.Size() > 0` as the form discriminator. Empty-body block
  // (`@layer foo {}`) is collapsed to statement form by the producer and
  // is semantically equivalent for cascade ordering, so this heuristic
  // matches the wire-format contract exercised by the unit tests.
  bool is_block = rule.HasMember("rules") && rule["rules"].IsArray() &&
                  rule["rules"].Size() > 0;

  std::string prelude_value;
  if (rule.HasMember("prelude")) {
    const auto& prelude = rule["prelude"];
    if (prelude.HasMember("value") && prelude["value"].IsString()) {
      prelude_value = prelude["value"].GetString();
    }
  }

  if (!is_block) {
    // Statement form: '@layer <layer-name># ;' — at least one <layer-name>,
    // separated by ','. If any name is invalid the whole statement is
    // invalid (spec: a parse error in the prelude makes the entire rule
    // invalid).
    css::CSSTokenizer tokenizer(prelude_value);
    css::CSSParserTokenStream stream(tokenizer);
    stream.ConsumeWhitespace();
    if (stream.AtEnd()) {
      // `@layer ;` (empty prelude) is invalid per spec — statement form
      // requires at least one <layer-name>. Reject the whole rule.
      diagnostics_.emplace_back(Diagnostic{"layer statement", "empty prelude"});
      return {};
    }

    std::vector<std::vector<std::string>> names;
    while (true) {
      std::vector<std::string> name;
      if (!ConsumeLayerName(stream, name)) {
        return {};
      }
      names.push_back(std::move(name));
      stream.ConsumeWhitespace();
      if (stream.AtEnd()) {
        break;
      }
      if (stream.Peek().GetType() != css::kCommaToken) {
        return {};
      }
      stream.Consume();
      stream.ConsumeWhitespace();
    }

    for (auto& name : names) {
      const size_t layer_position = layer_position_++;
      auto layer_rule = std::make_unique<encoder::LynxStyleRuleLayer>(
          CSSRuleType::kLayerStatement);
      layer_rule->layer_position = layer_position;
      layer_rule->name = std::move(name);
      result.push_back(std::move(layer_rule));
    }
    return result;
  }

  // Block form: '@layer <layer-name>? { <rule-list> }'. The prelude must be
  // empty (anonymous layer) or exactly one valid <layer-name>. Comma-
  // separated lists are not allowed in block form. On any prelude parse
  // error we reject the entire block (consistent with statement form).
  std::vector<std::string> block_name;
  {
    css::CSSTokenizer tokenizer(prelude_value);
    css::CSSParserTokenStream stream(tokenizer);
    stream.ConsumeWhitespace();
    if (!stream.AtEnd()) {
      if (!ConsumeLayerName(stream, block_name)) {
        return {};
      }
      stream.ConsumeWhitespace();
      if (!stream.AtEnd()) {
        return {};
      }
    }
  }

  const size_t layer_position = layer_position_++;
  auto layer_rule =
      std::make_unique<encoder::LynxStyleRuleLayer>(CSSRuleType::kLayerBlock);
  layer_rule->layer_position = layer_position;
  layer_rule->name = std::move(block_name);

  const auto& child_rules = rule["rules"];
  for (rapidjson::SizeType i = 0; i < child_rules.Size(); i++) {
    const auto& child = child_rules[i];
    if (!child.HasMember(TYPE) || !child[TYPE].IsString()) {
      continue;
    }
    const char* child_type = child[TYPE].GetString();
    if (strcmp(child_type, STYLE_RULE) == 0) {
      auto child_rule = ParseStyleRule(child, path);
      if (child_rule) {
        layer_rule->child_rules.push_back(std::move(child_rule));
      }
    } else if (strcmp(child_type, "LayerRule") == 0) {
      auto child_layer_rules = ParseLayerRule(child, path);
      for (auto& r : child_layer_rules) {
        layer_rule->child_rules.push_back(std::move(r));
      }
    } else if (strcmp(child_type, "MediaRule") == 0 ||
               strcmp(child_type, "SupportsRule") == 0) {
      auto child_rule = ParseConditionRule(child, path);
      if (child_rule) {
        layer_rule->child_rules.push_back(std::move(child_rule));
      }
    } else if (encoder::CSSKeyframesToken::IsCSSKeyframesToken(child)) {
      layer_rule->child_rules.push_back(ParseKeyframesRule(child, path));
    } else if (CSSFontFaceToken::IsCSSFontFaceToken(child)) {
      auto fontface_rule = ParseFontFaceRule(child);
      if (fontface_rule) {
        layer_rule->child_rules.push_back(std::move(fontface_rule));
      }
    } else {
      diagnostics_.emplace_back(Diagnostic{"layer child rule", child_type});
    }
  }

  result.push_back(std::move(layer_rule));
  return result;
}

std::unique_ptr<encoder::SharedCSSFragment> CSSRuleParser::ParseCSSRules(
    const rapidjson::Value& ttss, const std::string& path,
    const std::vector<int32_t>& dependent_css_list, int32_t fragment_id) {
  rule_index_counter_ = 0;
  layer_position_ = 0;
  std::vector<std::unique_ptr<encoder::LynxStyleRuleBase>> rules;

  for (rapidjson::SizeType i = 0; i < ttss.Size(); i++) {
    const auto& rule = ttss[i];
    if (!rule.HasMember(TYPE) || !rule[TYPE].IsString()) {
      continue;
    }
    const char* type = rule[TYPE].GetString();

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
    } else if (strcmp(type, "LayerRule") == 0) {
      auto layer_rules = ParseLayerRule(rule, path);
      for (auto& r : layer_rules) {
        rules.push_back(std::move(r));
      }
    } else if (encoder::CSSKeyframesToken::IsCSSKeyframesToken(rule)) {
      rules.push_back(ParseKeyframesRule(rule, path));
    } else if (CSSFontFaceToken::IsCSSFontFaceToken(rule)) {
      auto fontface_rule = ParseFontFaceRule(rule);
      if (fontface_rule) {
        rules.push_back(std::move(fontface_rule));
      }
    } else {
      Diagnostic d{"rule", type};
      if (rule.HasMember("prelude")) {
        CSSParser::ExtractLoc(rule["prelude"], "loc", d.line, d.column);
      }
      diagnostics_.emplace_back(d);
    }
  }

  return std::make_unique<encoder::SharedCSSFragment>(
      fragment_id, dependent_css_list, std::move(rules));
}

}  // namespace tasm
}  // namespace lynx
