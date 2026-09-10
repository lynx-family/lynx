// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/template_bundle/lynxml/lynxml_style_parser.h"

#include <cmath>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/include/string/string_utils.h"
#include "core/renderer/css/css_property.h"
#include "core/renderer/css/css_utils.h"
#include "core/renderer/css/ng/css_ng_utils.h"
#include "core/renderer/css/ng/parser/css_parser_token_stream.h"
#include "core/renderer/css/ng/parser/css_tokenizer.h"
#include "core/renderer/css/ng/selector/css_parser_context.h"
#include "core/renderer/css/ng/selector/css_selector_parser.h"
#include "core/renderer/css/ng/style/style_rule.h"
#include "core/renderer/css/parser/css_parser_configs.h"
#include "core/renderer/css/parser/css_string_parser.h"

namespace lynx {
namespace tasm {
namespace {

using css::CSSParserTokenStream;
using ParseError = std::optional<std::string>;

class StyleParserImpl {
 public:
  explicit StyleParserImpl(const CompileOptions& compile_options)
      : parser_configs_(CSSParserConfigs::GetCSSParserConfigsByComplierOptions(
            compile_options)),
        fragment_(std::make_unique<SharedCSSFragment>(0)) {
    fragment_->SetEnableCSSInvalidation();
    fragment_->SetEnableCSSSelector();
  }

  LynxMLStyleParseResult Parse(const std::string& style) {
    css::CSSTokenizer tokenizer(style);
    CSSParserTokenStream stream(tokenizer);

    stream.ConsumeWhitespace();
    while (!stream.AtEnd()) {
      ParseError rule_error;
      switch (stream.Peek().GetType()) {
        case css::kAtKeywordToken:
          rule_error = ConsumeAtRule(stream);
          break;
        default:
          rule_error = ConsumeQualifiedRule(stream);
          break;
      }
      if (rule_error) {
        return {false, nullptr, std::move(*rule_error)};
      }
      stream.ConsumeWhitespace();
    }

    if (!stream.Peek().IsEOF()) {
      return {false, nullptr,
              ErrorAt(stream.LookAheadOffset(), "unexpected block terminator")};
    }

    if (!keyframes_.empty()) {
      fragment_->SetKeyFramesRuleMap(std::move(keyframes_));
    }
    return {true, std::move(fragment_), {}};
  }

 private:
  ParseError ConsumeAtRule(CSSParserTokenStream& stream) {
    const size_t offset = stream.LookAheadOffset();
    if (css::ToLowerASCII(stream.Peek().Value()) == "keyframes") {
      return ConsumeKeyframesRule(stream);
    }
    return ErrorAt(offset, "unsupported at-rule");
  }

  ParseError ConsumeQualifiedRule(CSSParserTokenStream& stream) {
    const size_t selector_offset = stream.LookAheadOffset();
    auto selector_tokens =
        stream.ConsumeUntilPeekedTypeIs<css::kLeftBraceToken>();
    if (stream.Peek().GetType() != css::kLeftBraceToken) {
      return ErrorAt(stream.LookAheadOffset(), "expected '{'");
    }
    css::CSSParserContext context;
    auto selector_vector =
        css::CSSSelectorParser::ParseSelector(selector_tokens, &context);
    const size_t flattened_size =
        css::CSSSelectorParser::FlattenedSize(selector_vector);
    if (selector_vector.empty() || flattened_size == 0) {
      return ErrorAt(selector_offset, "invalid selector syntax");
    }
    auto selector_array =
        std::make_unique<css::LynxCSSSelector[]>(flattened_size);
    css::CSSSelectorParser::AdoptSelectorVector(
        selector_vector, selector_array.get(), flattened_size);

    RawStyleMap attributes;
    RawStyleMap important_attributes;
    CSSVariableMap style_variables;
    {
      CSSParserTokenStream::BlockGuard guard(stream);
      auto declaration_error = ConsumeDeclarations(
          stream, attributes, important_attributes, style_variables);
      if (declaration_error) {
        return declaration_error;
      }
    }

    AddRule(std::move(selector_array), std::move(attributes),
            std::move(important_attributes), std::move(style_variables));
    return std::nullopt;
  }

  ParseError ConsumeKeyframesRule(CSSParserTokenStream& stream) {
    const size_t offset = stream.LookAheadOffset();
    stream.ConsumeIncludingWhitespace();
    const auto& name_token = stream.Peek();
    if ((name_token.GetType() != css::kIdentToken &&
         name_token.GetType() != css::kStringToken) ||
        name_token.Value().empty()) {
      return ErrorAt(stream.LookAheadOffset(), "expected a keyframes name");
    }
    const base::String name(css::ustring_helper::to_string(name_token.Value()));
    stream.ConsumeIncludingWhitespace();
    if (stream.Peek().GetType() != css::kLeftBraceToken) {
      return ErrorAt(stream.LookAheadOffset(), "expected '{'");
    }

    CSSKeyframesContent parsed;
    CSSRawKeyframesContent raw;
    CSSKeyframesCustomPropertyContent custom;
    {
      CSSParserTokenStream::BlockGuard guard(stream);
      stream.ConsumeWhitespace();
      while (!stream.AtEnd()) {
        auto selectors = ParseKeyframeSelectors(
            stream.ConsumeUntilPeekedTypeIs<css::kLeftBraceToken>());
        if (!selectors || stream.Peek().GetType() != css::kLeftBraceToken) {
          return ErrorAt(stream.LookAheadOffset(), "invalid keyframe selector");
        }
        RawStyleMap attributes, important;
        CSSVariableMap variables;
        {
          CSSParserTokenStream::BlockGuard declaration_guard(stream);
          if (auto error = ConsumeDeclarations(stream, attributes, important,
                                               variables)) {
            return error;
          }
        }
        auto styles = std::make_shared<StyleMap>();
        auto raw_styles = std::make_shared<RawStyleMap>(std::move(attributes));
        auto custom_properties = std::make_shared<CustomPropertiesMap>();
        for (const auto& [property, value] : variables) {
          custom_properties->insert_or_assign(
              property, CSSValue(lepus::Value(value), CSSValuePattern::STRING));
        }
        for (float key : *selectors) {
          parsed.insert_or_assign(key, styles);
          if (!raw_styles->empty()) raw.insert_or_assign(key, raw_styles);
          if (!custom_properties->empty()) {
            custom.insert_or_assign(key, custom_properties);
          }
        }
        stream.ConsumeWhitespace();
      }
      if (stream.Peek().IsEOF()) {
        return ErrorAt(offset, "unclosed keyframes block");
      }
    }
    auto token = fml::MakeRefCounted<CSSKeyframesToken>(parser_configs_);
    token->SetKeyframesContent(std::move(parsed));
    token->SetRawKeyframesContent(std::move(raw));
    token->SetKeyframesCustomPropertyContent(std::move(custom));
    keyframes_.insert_or_assign(name, std::move(token));
    return std::nullopt;
  }

  std::optional<std::vector<float>> ParseKeyframeSelectors(
      css::CSSParserTokenRange tokens) {
    std::vector<float> result;
    tokens.ConsumeWhitespace();
    while (!tokens.AtEnd()) {
      const auto& token = tokens.ConsumeIncludingWhitespace();
      const std::string value = css::ToLowerASCII(token.Value());
      if (token.GetType() == css::kIdentToken && value == "from") {
        result.push_back(0);
      } else if (token.GetType() == css::kIdentToken && value == "to") {
        result.push_back(1);
      } else if (token.GetType() == css::kPercentageToken &&
                 std::isfinite(token.NumericValue()) &&
                 token.NumericValue() >= 0 && token.NumericValue() <= 100) {
        result.push_back(static_cast<float>(token.NumericValue() / 100));
      } else {
        return std::nullopt;
      }
      if (!tokens.AtEnd() &&
          tokens.ConsumeIncludingWhitespace().GetType() != css::kCommaToken) {
        return std::nullopt;
      }
    }
    return result.empty() ? std::nullopt
                          : std::make_optional(std::move(result));
  }

  ParseError ConsumeDeclarations(CSSParserTokenStream& stream,
                                 RawStyleMap& attributes,
                                 RawStyleMap& important_attributes,
                                 CSSVariableMap& style_variables) {
    stream.ConsumeWhitespace();
    while (!stream.AtEnd()) {
      if (stream.Peek().GetType() == css::kSemicolonToken) {
        stream.ConsumeIncludingWhitespace();
        continue;
      }

      const size_t declaration_offset = stream.LookAheadOffset();
      const auto& property_token = stream.Peek();
      if (property_token.GetType() != css::kIdentToken) {
        return ErrorAt(declaration_offset, "expected a property name");
      }
      std::string property =
          css::ustring_helper::to_string(property_token.Value());
      const bool is_custom_property = CSSProperty::IsCustomProperty(
          property.c_str(), static_cast<uint32_t>(property.length()));
      if (!is_custom_property) {
        property = css::ToLowerASCII(property_token.Value());
      }
      stream.ConsumeIncludingWhitespace();
      if (stream.Peek().GetType() != css::kColonToken) {
        return ErrorAt(stream.LookAheadOffset(), "expected ':'");
      }
      stream.ConsumeIncludingWhitespace();

      const size_t value_start = stream.LookAheadOffset();
      stream.ConsumeUntilPeekedTypeIs<css::kSemicolonToken>();
      const size_t value_end = stream.LookAheadOffset();
      std::string value =
          base::TrimString(css::ustring_helper::to_string(stream.StringRangeAt(
                               value_start, value_end - value_start)),
                           " \t\n\r\f", base::TRIM_ALL);
      if (value.empty()) {
        return ErrorAt(value_start, "expected a property value");
      }

      const char* parsed_value_start = value.c_str();
      uint32_t parsed_value_length = static_cast<uint32_t>(value.length());
      const bool important =
          StripImportant(value.c_str(), static_cast<uint32_t>(value.length()),
                         &parsed_value_start, &parsed_value_length);
      std::string parsed_value(parsed_value_start, parsed_value_length);
      if (parsed_value.empty()) {
        return ErrorAt(value_start, "expected a property value");
      }
      if (is_custom_property) {
        style_variables.insert_or_assign(base::String(property),
                                         base::String(parsed_value));
      } else {
        const CSSPropertyID property_id = CSSProperty::GetPropertyID(property);
        if (!CSSProperty::IsPropertyValid(property_id)) {
          return ErrorAt(declaration_offset,
                         "unknown property '" + property + "'");
        }
        RawStyleMap& target = important ? important_attributes : attributes;
        CSSStringParser variable_parser(
            parsed_value.c_str(), static_cast<uint32_t>(parsed_value.length()),
            parser_configs_);
        CSSValue css_value = variable_parser.ParseVariable();
        if (variable_parser.HasMetVarToken()) {
          target.insert_or_assign(property_id, std::move(css_value));
        } else {
          target.insert_or_assign(
              property_id,
              CSSValue(lepus::Value(parsed_value), CSSValuePattern::STRING));
        }
      }

      if (!stream.AtEnd()) {
        stream.ConsumeIncludingWhitespace();
      }
    }

    if (stream.Peek().IsEOF()) {
      return ErrorAt(stream.LookAheadOffset(), "unclosed declaration block");
    }
    return std::nullopt;
  }

  void AddRule(std::unique_ptr<css::LynxCSSSelector[]> selector_array,
               RawStyleMap attributes, RawStyleMap important_attributes,
               CSSVariableMap style_variables) {
    auto token = fml::MakeRefCounted<CSSParseToken>(parser_configs_);
    token->raw_attributes() = std::move(attributes);
    token->raw_important_attributes() = std::move(important_attributes);
    token->SetStyleVariables(std::move(style_variables));
    fragment_->AddStyleRule(fml::MakeRefCounted<css::StyleRule>(
        std::move(selector_array), std::move(token)));
  }

  std::string ErrorAt(size_t offset, std::string message) {
    return "invalid style at offset " + std::to_string(offset) + ": " +
           std::move(message);
  }

  CSSParserConfigs parser_configs_;
  std::unique_ptr<SharedCSSFragment> fragment_;
  CSSKeyframesTokenMap keyframes_;
};

}  // namespace

LynxMLStyleParser::LynxMLStyleParser(std::string style,
                                     const CompileOptions& compile_options)
    : style_(std::move(style)), compile_options_(compile_options) {}

LynxMLStyleParseResult LynxMLStyleParser::Parse() {
  return StyleParserImpl(compile_options_).Parse(style_);
}

LynxMLStyleParseResult ParseLynxMLStyle(const std::string& style,
                                        const CompileOptions& compile_options) {
  return LynxMLStyleParser(style, compile_options).Parse();
}

}  // namespace tasm
}  // namespace lynx
