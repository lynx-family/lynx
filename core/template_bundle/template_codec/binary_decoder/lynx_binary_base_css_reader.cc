// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/template_bundle/template_codec/binary_decoder/lynx_binary_base_css_reader.h"

#include <algorithm>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "base/trace/native/trace_event.h"
#include "core/renderer/css/ng/media_query/media_query_set.h"
#include "core/renderer/css/parser/css_parser_configs.h"
#include "core/runtime/lepus/base_binary_reader.h"
#include "core/template_bundle/template_codec/binary_decoder/binary_decoder_trace_event_def.h"

namespace lynx {
namespace tasm {
// static
bool LynxBinaryBaseCSSReader::EnableCssVariable(const CompileOptions& options) {
  return Config::IsHigherOrEqual(options.target_sdk_version_,
                                 FEATURE_CSS_STYLE_VARIABLES) &&
         options.enable_css_variable_;
}

// static
bool LynxBinaryBaseCSSReader::EnableCssParser(const CompileOptions& options) {
  return Config::IsHigherOrEqual(options.target_sdk_version_,
                                 FEATURE_CSS_VALUE_VERSION) &&
         options.enable_css_parser_;
}

// static
bool LynxBinaryBaseCSSReader::EnableCssVariableMultiDefaultValue(
    const CompileOptions& options) {
  return EnableCssVariable(options) &&
         Config::IsHigherOrEqual(options.target_sdk_version_,
                                 LYNX_VERSION_2_14);
}

bool LynxBinaryBaseCSSReader::DecodeCSSSelector(
    css::LynxCSSSelector* selector) {
  DECODE_VALUE(data);
  css::LynxCSSSelector::FromLepus(*selector, data);
  return true;
}

bool LynxBinaryBaseCSSReader::DecodeCSSRoute(CSSRoute& css_route) {
  // css async-decoding requires cutting the css section, so the precise
  // starting point and end point of the css setion need to be recorded here.
  uint32_t css_route_length = 0;
  DECODE_COMPACT_U32(size);
  css_route.fragment_ranges.reserve(size);
  for (size_t i = 0; i < size; ++i) {
    DECODE_COMPACT_S32(id);
    // CSSRange
    DECODE_COMPACT_U32(start);
    DECODE_COMPACT_U32(end);
    css_route_length = std::max(css_route_length, end);
    css_route.fragment_ranges.emplace(std::piecewise_construct,
                                      std::forward_as_tuple(id),
                                      std::forward_as_tuple(start, end));
  }
  css_section_range_.start = static_cast<uint32_t>(stream_->offset());
  css_section_range_.end = css_section_range_.start + css_route_length;
  return true;
}

bool LynxBinaryBaseCSSReader::DecodeCSSFragment(SharedCSSFragment* fragment,
                                                size_t descriptor_end) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, BINARY_BASE_CSS_READER_DECODE_CSS_FRAGMENT);
  // Id
  DECODE_COMPACT_U32(id);
  fragment->id_ = id;
  // Dependents css id
  DECODE_COMPACT_U32(dependent_ids_size);
  fragment->dependent_ids_.reserve(dependent_ids_size);
  for (size_t i = 0; i < dependent_ids_size; ++i) {
    DECODE_COMPACT_S32(id);
    fragment->dependent_ids_.emplace_back(id);
  }

  if (compile_options_.enable_css_rule_) {
    ERROR_UNLESS(DecodeCSSRules(fragment));
    return true;
  }

  // GetCSSParserConfig
  auto parser_config =
      CSSParserConfigs::GetCSSParserConfigsByComplierOptions(compile_options_);

  // Decode the selector and parse token when enable the css selector
  if (compile_options_.enable_css_selector_) {
    // If enable the CSS invalidation
    if (compile_options_.enable_css_invalidation_) {
      fragment->SetEnableCSSInvalidation();
    }
    fragment->SetEnableCSSSelector();
    DECODE_COMPACT_U32(selector_size);
    for (size_t i = 0; i < selector_size; i++) {
      DECODE_COMPACT_U32(flattened_size);
      if (flattened_size == 0) {
        // We do not support this CSS selector
        // See TemplateBinaryWriter::EncodeLynxCSSSelectorTuple
        continue;
      }
      auto selector_array =
          std::make_unique<css::LynxCSSSelector[]>(flattened_size);
      for (size_t i = 0; i < flattened_size; i++) {
        DecodeCSSSelector(&selector_array[i]);
        if (selector_array[i].GetPseudoType() ==
            css::LynxCSSSelector::kPseudoActive) {
          fragment->MarkHasTouchPseudoToken();
        }
      }
      auto parser_token = fml::MakeRefCounted<CSSParseToken>(parser_config);
      ERROR_UNLESS(DecodeCSSParseToken(parser_token.get()));
      fragment->AddStyleRule(std::move(selector_array),
                             std::move(parser_token));
    }
  }

  // When enable the css selector, the `css_size` will be zero
  TRACE_EVENT_BEGIN(LYNX_TRACE_CATEGORY,
                    BINARY_BASE_CSS_READER_DECODE_CSS_PARSE_TOKEN);
  DECODE_COMPACT_U32(size);
  uint32_t css_size = size << 16 >> 16;
  uint32_t keyframes_size = size >> 16;
  // CSS parse token
  fragment->css_.reserve(css_size);
  for (size_t i = 0; i < css_size; ++i) {
    DECODE_STDSTR(key);
    auto parser_token = fml::MakeRefCounted<CSSParseToken>(parser_config);
    ERROR_UNLESS(DecodeCSSParseToken(parser_token.get()));
    if (parser_token->IsTouchPseudoToken()) {
      fragment->MarkHasTouchPseudoToken();
    }
    fragment->FindSpecificMapAndAdd(key, parser_token);
    fragment->css_.emplace(std::move(key), std::move(parser_token));
  }
  TRACE_EVENT_END(LYNX_TRACE_CATEGORY);

  TRACE_EVENT_BEGIN(LYNX_TRACE_CATEGORY,
                    BINARY_BASE_CSS_READER_DECODE_CSS_KEYFRAMES_TOKEN);
  fragment->keyframes_.reserve(keyframes_size);
  for (size_t i = 0; i < keyframes_size; ++i) {
    DECODE_STDSTR(name);
    auto token = fml::MakeRefCounted<CSSKeyframesToken>(parser_config);
    ERROR_UNLESS(DecodeCSSKeyframesToken(token.get()));
    fragment->keyframes_.emplace(std::move(name), std::move(token));
  }
  TRACE_EVENT_END(LYNX_TRACE_CATEGORY);

  // for other types
  TRACE_EVENT_BEGIN(LYNX_TRACE_CATEGORY,
                    BINARY_BASE_CSS_READER_DECODE_CSS_FONT_FACE_TOKEN);
  while (CheckSize(5, static_cast<uint32_t>(descriptor_end))) {
    DECODE_U8(type);
    DECODE_COMPACT_U32(typed_size);
    switch (type) {
      case CSS_BINARY_FONT_FACE_TYPE:
        fragment->fontfaces_.reserve(typed_size);
        for (size_t i = 0; i < typed_size; ++i) {
          std::vector<std::shared_ptr<CSSFontFaceRule>> token_list;
          if (enable_css_font_face_extension_) {
            DECODE_COMPACT_U32(token_size);
            for (size_t i = 0; i < token_size; ++i) {
              auto token = std::make_shared<CSSFontFaceRule>();
              ERROR_UNLESS(DecodeCSSFontFaceToken(token.get()));
              token_list.emplace_back(std::move(token));
            }
          } else {
            auto token = std::make_shared<CSSFontFaceRule>();
            ERROR_UNLESS(DecodeCSSFontFaceToken(token.get()));
            token_list.emplace_back(std::move(token));
          }
          std::string token_key =
              token_list.size() > 0 ? token_list[0]->first : "";
          fragment->fontfaces_.emplace(std::move(token_key),
                                       std::move(token_list));
        }
        break;
      default:
        break;
    }
  }
  TRACE_EVENT_END(LYNX_TRACE_CATEGORY);

  return true;
}

bool LynxBinaryBaseCSSReader::DecodeCSSRules(SharedCSSFragment* fragment) {
  auto parser_config =
      CSSParserConfigs::GetCSSParserConfigsByComplierOptions(compile_options_);

  if (compile_options_.enable_css_invalidation_) {
    fragment->SetEnableCSSInvalidation();
  }
  fragment->SetEnableCSSSelector();
  DECODE_COMPACT_U32(rules_size);
  for (size_t i = 0; i < rules_size; ++i) {
    DECODE_U8(rule_type);
    // Read the payload length so we can skip unknown rule types.
    DECODE_U32(payload_size);
    size_t next_rule_offset = Offset() + payload_size;

    switch (static_cast<CSSRuleType>(rule_type)) {
      case CSSRuleType::kStyle:
        ERROR_UNLESS(DecodeCSSStyleRule(fragment, parser_config));
        break;
      case CSSRuleType::kMedia:
      case CSSRuleType::kSupports:
        ERROR_UNLESS(
            DecodeCSSConditionRule(fragment, parser_config, rule_type));
        break;
      case CSSRuleType::kKeyframes:
        ERROR_UNLESS(DecodeCSSKeyframesRule(fragment, parser_config));
        break;
      case CSSRuleType::kFontFace:
        ERROR_UNLESS(DecodeCSSFontFaceRule(fragment));
        break;
      case CSSRuleType::kLayerBlock:
      case CSSRuleType::kLayerStatement:
        ERROR_UNLESS(DecodeCSSLayerRule(fragment, parser_config, rule_type));
        break;
      default:
        break;
    }
    // Align to the next rule boundary regardless of how much data the
    // individual decoder consumed.
    ERROR_UNLESS(Offset() <= next_rule_offset);
    Seek(static_cast<uint32_t>(next_rule_offset));
  }
  return true;
}

bool LynxBinaryBaseCSSReader::DecodeStyleRuleData(
    SharedCSSFragment* fragment, const CSSParserConfigs& parser_config,
    fml::RefPtr<css::StyleRule>* out_rule) {
  DECODE_COMPACT_U32(position);
  DECODE_COMPACT_U32(flattened_size);
  // We know selector size > 0
  auto selector_array =
      std::make_unique<css::LynxCSSSelector[]>(flattened_size);
  for (size_t i = 0; i < flattened_size; i++) {
    DecodeCSSSelector(&selector_array[i]);
    if (selector_array[i].GetPseudoType() ==
        css::LynxCSSSelector::kPseudoActive) {
      fragment->MarkHasTouchPseudoToken();
    }
  }
  auto parse_token = fml::MakeRefCounted<CSSParseToken>(parser_config);
  ERROR_UNLESS(DecodeCSSParseToken(parse_token.get()));
  *out_rule = fml::MakeRefCounted<css::StyleRule>(
      std::move(selector_array), std::move(parse_token), position);
  return true;
}

bool LynxBinaryBaseCSSReader::DecodeCSSStyleRule(
    SharedCSSFragment* fragment, const CSSParserConfigs& parser_config) {
  fml::RefPtr<css::StyleRule> rule;
  ERROR_UNLESS(DecodeStyleRuleData(fragment, parser_config, &rule));
  if (rule) {
    fragment->AddStyleRule(std::move(rule));
  }
  return true;
}

bool LynxBinaryBaseCSSReader::DecodeCSSConditionRule(
    SharedCSSFragment* fragment, const CSSParserConfigs& parser_config,
    uint8_t rule_type) {
  fml::RefPtr<css::ConditionRule> condition_rule;
  ERROR_UNLESS(DecodeConditionRuleData(fragment, parser_config, rule_type,
                                       &condition_rule));
  if (rule_type == static_cast<uint8_t>(CSSRuleType::kMedia)) {
    fragment->AddConditionRule(std::move(condition_rule));
  }
  return true;
}

bool LynxBinaryBaseCSSReader::DecodeConditionRuleData(
    SharedCSSFragment* fragment, const CSSParserConfigs& parser_config,
    uint8_t rule_type, fml::RefPtr<css::ConditionRule>* out_rule) {
  std::string condition;
  fml::RefPtr<const css::MediaQuerySet> media_queries;
  if (rule_type == static_cast<uint8_t>(CSSRuleType::kMedia)) {
    DECODE_VALUE(media_value);
    media_queries = css::MediaQuerySet::FromLepus(media_value);
  } else {
    ERROR_UNLESS(DecodeUtf8Str(&condition));
  }
  DECODE_COMPACT_U32(child_count);
  auto condition_rule =
      fml::MakeRefCounted<css::ConditionRule>(std::move(condition), fragment);
  condition_rule->SetMediaQueries(std::move(media_queries));
  for (size_t i = 0; i < child_count; ++i) {
    DECODE_U8(child_type);
    // Read the payload length so we can skip unknown child rule types.
    DECODE_U32(child_payload_size);
    size_t next_child_offset = Offset() + child_payload_size;

    switch (static_cast<CSSRuleType>(child_type)) {
      case CSSRuleType::kStyle: {
        fml::RefPtr<css::StyleRule> rule;
        ERROR_UNLESS(DecodeStyleRuleData(fragment, parser_config, &rule));
        if (rule) {
          condition_rule->AddStyleRule(std::move(rule));
        }
        break;
      }
      case CSSRuleType::kKeyframes: {
        base::String name;
        fml::RefPtr<CSSKeyframesToken> token;
        ERROR_UNLESS(DecodeKeyframesRuleData(parser_config, &name, &token));
        break;
      }
      case CSSRuleType::kFontFace: {
        std::string family;
        std::vector<std::shared_ptr<CSSFontFaceRule>> token_list;
        ERROR_UNLESS(DecodeFontFaceRuleData(&family, &token_list));
        break;
      }
      default:
        break;
    }
    ERROR_UNLESS(Offset() <= next_child_offset);
    Seek(static_cast<uint32_t>(next_child_offset));
  }
  *out_rule = std::move(condition_rule);
  return true;
}

bool LynxBinaryBaseCSSReader::DecodeCSSKeyframesRule(
    SharedCSSFragment* fragment, const CSSParserConfigs& parser_config) {
  base::String name;
  fml::RefPtr<CSSKeyframesToken> token;
  ERROR_UNLESS(DecodeKeyframesRuleData(parser_config, &name, &token));
  fragment->keyframes_.emplace(std::move(name), std::move(token));
  return true;
}

bool LynxBinaryBaseCSSReader::DecodeKeyframesRuleData(
    const CSSParserConfigs& parser_config, base::String* out_name,
    fml::RefPtr<CSSKeyframesToken>* out_token) {
  DECODE_STDSTR(name);
  auto token = fml::MakeRefCounted<CSSKeyframesToken>(parser_config);
  ERROR_UNLESS(DecodeCSSKeyframesToken(token.get()));
  *out_name = base::String(std::move(name));
  *out_token = std::move(token);
  return true;
}

bool LynxBinaryBaseCSSReader::DecodeCSSFontFaceRule(
    SharedCSSFragment* fragment) {
  std::string family;
  std::vector<std::shared_ptr<CSSFontFaceRule>> token_list;
  ERROR_UNLESS(DecodeFontFaceRuleData(&family, &token_list));
  // Use the trimmed font-family from the token (consistent with old path
  // which derives the key via CSSFontTokenAddAttribute → _innerTrTrim).
  std::string token_key = token_list.size() > 0 ? token_list[0]->first : "";
  fragment->fontfaces_.emplace(std::move(token_key), std::move(token_list));
  return true;
}

bool LynxBinaryBaseCSSReader::DecodeFontFaceRuleData(
    std::string* out_family,
    std::vector<std::shared_ptr<CSSFontFaceRule>>* out_tokens) {
  DECODE_STDSTR(family);
  std::vector<std::shared_ptr<CSSFontFaceRule>> token_list;
  if (enable_css_font_face_extension_) {
    DECODE_COMPACT_U32(token_size);
    for (size_t i = 0; i < token_size; ++i) {
      auto token = std::make_shared<CSSFontFaceRule>();
      ERROR_UNLESS(DecodeCSSFontFaceToken(token.get()));
      token_list.emplace_back(std::move(token));
    }
  } else {
    auto token = std::make_shared<CSSFontFaceRule>();
    ERROR_UNLESS(DecodeCSSFontFaceToken(token.get()));
    token_list.emplace_back(std::move(token));
  }
  *out_family = std::move(family);
  *out_tokens = std::move(token_list);
  return true;
}

bool LynxBinaryBaseCSSReader::DecodeCSSLayerRule(
    SharedCSSFragment* fragment, const CSSParserConfigs& parser_config,
    uint8_t rule_type) {
  // Wire format (the leading rule-type byte and payload-length u32 were
  // already consumed by the outer switch in DecodeCSSRules):
  //   - name_segment_count : compact u32
  //   - N x utf8 str       : name segments (e.g. "framework"."theme")
  //   - layer_position     : compact u32  (parser document-order index, NOT
  //                                        cascade priority)
  //   - if kLayerBlock:
  //       child_count      : compact u32
  //       N x { type byte + payload_size u32 + child payload }
  //
  DECODE_COMPACT_U32(name_segment_count);
  for (size_t i = 0; i < name_segment_count; ++i) {
    DECODE_STDSTR(segment);
    (void)segment;
  }
  DECODE_COMPACT_U32(layer_position);
  (void)layer_position;

  if (rule_type != static_cast<uint8_t>(CSSRuleType::kLayerBlock)) {
    // Statement form has no children.
    return true;
  }

  DECODE_COMPACT_U32(child_count);
  for (size_t i = 0; i < child_count; ++i) {
    DECODE_U8(child_type);
    // Read the payload length so we can skip unknown child rule types.
    DECODE_U32(child_payload_size);
    size_t next_child_offset = Offset() + child_payload_size;

    switch (static_cast<CSSRuleType>(child_type)) {
      case CSSRuleType::kStyle: {
        fml::RefPtr<css::StyleRule> rule;
        ERROR_UNLESS(DecodeStyleRuleData(fragment, parser_config, &rule));
        break;
      }
      case CSSRuleType::kMedia:
      case CSSRuleType::kSupports: {
        fml::RefPtr<css::ConditionRule> condition_rule;
        ERROR_UNLESS(DecodeConditionRuleData(fragment, parser_config,
                                             child_type, &condition_rule));
        break;
      }
      case CSSRuleType::kKeyframes: {
        base::String name;
        fml::RefPtr<CSSKeyframesToken> token;
        ERROR_UNLESS(DecodeKeyframesRuleData(parser_config, &name, &token));
        break;
      }
      case CSSRuleType::kFontFace: {
        std::string family;
        std::vector<std::shared_ptr<CSSFontFaceRule>> token_list;
        ERROR_UNLESS(DecodeFontFaceRuleData(&family, &token_list));
        break;
      }
      case CSSRuleType::kLayerBlock:
      case CSSRuleType::kLayerStatement:
        ERROR_UNLESS(DecodeCSSLayerRule(fragment, parser_config, child_type));
        break;
      default:
        break;
    }
    ERROR_UNLESS(Offset() <= next_child_offset);
    Seek(static_cast<uint32_t>(next_child_offset));
  }
  return true;
}

bool LynxBinaryBaseCSSReader::DecodeCSSParseToken(CSSParseToken* token) {
  ERROR_UNLESS(DecodeCSSAttributes(token));

  if (Config::IsHigherOrEqual(compile_options_.target_sdk_version_,
                              FEATURE_CSS_IMPORTANT)) {
    ERROR_UNLESS(DecodeCSSAttributes(token->important_attributes(),
                                     token->raw_important_attributes(),
                                     token->GetCSSParserConfigs()));
  }

  if (enable_css_variable_) {
    DCHECK(token->style_variables().empty());
    ERROR_UNLESS(DecodeCSSStyleVariables(token->style_variables()));
  }

  if (!compile_options_.enable_css_selector_) {
    DECODE_COMPACT_U32(size);
    auto& sheets = token->sheets();
    sheets.clear();
    sheets.reserve(size);

    for (size_t i = 0; i < size; i++) {
      CSSSheet* parent = (i == 0) ? nullptr : sheets[i - 1].get();
      auto& sheet = sheets.emplace_back(new CSSSheet());
      ERROR_UNLESS(DecodeCSSSheet(parent, sheet.get()));
      if (sheet->IsTouchPseudo()) {
        token->MarkAsTouchPseudoToken();
      }
    }
  }

  return true;
}

bool LynxBinaryBaseCSSReader::DecodeCSSFontFaceToken(CSSFontFaceRule* token) {
  DECODE_COMPACT_U32(size);
  for (size_t i = 0; i < size; ++i) {
    DECODE_STDSTR(str_key);
    DECODE_STDSTR(str_val);
    CSSFontTokenAddAttribute(token, str_key, str_val);
  }
  return true;
}

bool LynxBinaryBaseCSSReader::DecodeCSSKeyframesToken(
    CSSKeyframesToken* token) {
  CSSKeyframesContent map;
  CSSRawKeyframesContent raw_map;
  ERROR_UNLESS(DecodeCSSKeyframesMap(&map, &raw_map, token->parser_configs_));
  token->SetKeyframesContent(std::move(map));
  token->SetRawKeyframesContent(std::move(raw_map));
  if (tasm::Config::IsHigherOrEqual(
          compile_options_.target_sdk_version_,
          FEATURE_CUSTOM_PROPERTY_DECLARATION_KEYFRAME) &&
      compile_options_.enable_keyframe_custom_property_declaration_) {
    CSSKeyframesCustomPropertyContent custom_property_map;
    ERROR_UNLESS(DecodeCSSKeyframesCustomPropertyContent(&custom_property_map));
    token->SetKeyframesCustomPropertyContent(std::move(custom_property_map));
  }
  return true;
}

bool LynxBinaryBaseCSSReader::DecodeCSSSheet(CSSSheet* parent,
                                             CSSSheet* sheet) {
  DECODE_COMPACT_U32(type);  // Not used
  DECODE_STR(name);
  DECODE_STR(selector);
  sheet->type_ = 0;  // The ConfirmType will update the value of type
  sheet->name_ = std::move(name);
  sheet->selector_ = std::move(selector);
  sheet->parent_ = parent;
  sheet->ConfirmType();
  return true;
}

bool LynxBinaryBaseCSSReader::DecodeCSSAttributes(CSSParseToken* token) {
  if (enable_css_parser_ || enable_pre_process_attributes_) {
    token->MarkParsed();
  }
  return DecodeCSSAttributes(token->attributes(), token->raw_attributes(),
                             token->GetCSSParserConfigs());
}

bool LynxBinaryBaseCSSReader::DecodeCSSAttributes(
    StyleMap& attr, RawStyleMap& raw_attr, const CSSParserConfigs& configs) {
  DECODE_COMPACT_U32(size);
  if (enable_css_parser_) {
    DCHECK(attr.empty());
    attr.reserve(size);
    for (size_t i = 0; i < size; ++i) {
      DECODE_COMPACT_U32(id);
      CSSPropertyID property_id = static_cast<CSSPropertyID>(id);
      DECODE_CSS_VALUE_INTO(attr[property_id]);
    }
  } else if (enable_pre_process_attributes_) {
    // Predecode all values and calculate map size for reserving memory.
    struct PredecodePair {
      CSSPropertyID id;
      CSSValue value;

      struct TraitID {
        static inline CSSPropertyID GetPropertyID(const PredecodePair& input) {
          return input.id;
        }
      };
    };
    PredecodePair decode_values[size];
    for (size_t i = 0; i < size; ++i) {
      DECODE_COMPACT_U32(id);
      decode_values[i].id = static_cast<CSSPropertyID>(id);
      DECODE_CSS_VALUE_INTO(decode_values[i].value);
    }

    // We can calculate accurate reserving size from decoded property ids.
    attr.reserve(
        CSSProperty::GetTotalParsedStyleCountFromArray(decode_values, size));
    for (size_t i = 0; i < size; ++i) {
      UnitHandler::ProcessCSSValue(decode_values[i].id, decode_values[i].value,
                                   attr, configs);
    }
  } else {
    DCHECK(raw_attr.empty());
    raw_attr.reserve(size);
    for (size_t i = 0; i < size; ++i) {
      DECODE_COMPACT_U32(id);
      CSSPropertyID property_id = static_cast<CSSPropertyID>(id);
      DECODE_CSS_VALUE_INTO(raw_attr[property_id]);
    }
  }
  return true;
}

bool LynxBinaryBaseCSSReader::DecodeCSSStyleVariables(
    CSSVariableMap& style_variables) {
  DECODE_COMPACT_U32(size);
  style_variables.reserve(size);
  for (size_t i = 0; i < size; ++i) {
    std::string key;
    ReadStringDirectly(&key);
    std::string value;
    ReadStringDirectly(&value);
    style_variables.insert_or_assign(std::move(key), std::move(value));
  }
  return true;
}

bool LynxBinaryBaseCSSReader::DecodeCSSKeyframesCustomProperty(
    CustomPropertiesMap& custom_properties) {
  DECODE_COMPACT_U32(size);
  custom_properties.reserve(size);
  for (size_t i = 0; i < size; ++i) {
    DECODE_STDSTR(key);
    DECODE_CSS_VALUE(value);
    custom_properties.insert_or_assign(std::move(key), std::move(value));
  }
  return true;
}

bool LynxBinaryBaseCSSReader::DecodeCSSKeyframesMap(
    CSSKeyframesContent* keyframes, CSSRawKeyframesContent* raw_keyframes,
    const CSSParserConfigs& parser_config) {
  DECODE_COMPACT_U32(size);
  keyframes->reserve(size);
  raw_keyframes->reserve(size);
  for (size_t i = 0; i < size; ++i) {
    float key;
    if (enable_css_parser_) {
      DECODE_DOUBLE(key_val);
      key = key_val;
    } else {
      DECODE_STDSTR(key_text);
      key = CSSKeyframesToken::ParseKeyStr(
          key_text, compile_options_.enable_css_strict_mode_);
    }

    auto attrs_ptr = std::make_shared<StyleMap>();
    auto raw_attrs_ptr = std::make_shared<RawStyleMap>();
    ERROR_UNLESS(
        DecodeCSSAttributes(*attrs_ptr, *raw_attrs_ptr, parser_config));
    keyframes->emplace(key, std::move(attrs_ptr));
    if (!raw_attrs_ptr->empty()) {
      raw_keyframes->emplace(key, std::move(raw_attrs_ptr));
    }
  }
  return true;
}

bool LynxBinaryBaseCSSReader::DecodeCSSKeyframesCustomPropertyContent(
    CSSKeyframesCustomPropertyContent* keyframes) {
  DECODE_COMPACT_U32(size);
  keyframes->reserve(size);
  for (size_t i = 0; i < size; ++i) {
    float key;
    if (enable_css_parser_) {
      DECODE_DOUBLE(key_val);
      key = key_val;
    } else {
      DECODE_STDSTR(key_text);
      key = CSSKeyframesToken::ParseKeyStr(
          key_text, compile_options_.enable_css_strict_mode_);
    }

    auto custom_properties = std::make_shared<CustomPropertiesMap>();
    ERROR_UNLESS(DecodeCSSKeyframesCustomProperty(*custom_properties));
    keyframes->emplace(key, std::move(custom_properties));
  }
  return true;
}

bool LynxBinaryBaseCSSReader::DecodeCSSValue(tasm::CSSValue* result) {
  ERROR_UNLESS(DecodeCSSValue(result, enable_css_parser_, enable_css_variable_,
                              enable_css_variable_multi_default_value_));
  return true;
}

bool LynxBinaryBaseCSSReader::DecodeCSSValue(
    tasm::CSSValue* result, bool enable_css_parser, bool enable_css_variable,
    bool enable_css_variable_multi_default_value) {
  CSSValuePattern pattern;
  if (enable_css_parser) {
    DECODE_COMPACT_U32(pattern_value);
    pattern = static_cast<tasm::CSSValuePattern>(pattern_value);
  } else {
    pattern = CSSValuePattern::STRING;
  }

  lynx_value buffer;
  DecodeRawLynxValue(buffer);
  result->val_uint64 = buffer.val_uint64;
  result->type_ = buffer.type;
  result->SetPattern(pattern);

  if (enable_css_variable) {
    DECODE_COMPACT_U32(value_type);
    DECODE_STR(default_value);
    result->SetType(static_cast<tasm::CSSValueType>(value_type));
    result->SetDefaultValue(std::move(default_value));
    if (enable_css_variable_multi_default_value) {
      DECODE_VALUE(default_value_map);
      if (!default_value_map.IsNil()) {
        // Empty values will be encoded in the current template.
        // Checking IsNil() in advance can slightly improve performance.
        result->SetDefaultValueMap(std::move(default_value_map));
      }
    }
    if (enable_css_inline_variables_) {
      //  Compatible to new VarReference with ex {{}}
      //  referenced string.
      //  When inline CSS variables are enabled, check if the value contains
      //  {{}} format and create VarReference structures for proper variable
      //  resolution
      result->ToVarReference();
    }
  }
  return true;
}

bool LynxBinaryBaseCSSReader::DecodeUtf8Str(base::String& result) {
  if (decode_string_directly_) {
    return ReadStringDirectly(result);
  }
  return lepus::BaseBinaryReader::DecodeUtf8Str(result);
}

bool LynxBinaryBaseCSSReader::DecodeUtf8Str(std::string* result) {
  if (decode_string_directly_) {
    return ReadStringDirectly(result);
  }
  return lepus::BaseBinaryReader::DecodeUtf8Str(result);
}

bool LynxBinaryBaseCSSReader::DecodeUtf8Str(lynx_value& result) {
  if (decode_string_directly_) {
    base::String tmp;
    ERROR_UNLESS(ReadStringDirectly(tmp));
    auto* ptr = base::String::Unsafe::GetUntaggedStringRawRef(tmp);
    result.val_ptr = reinterpret_cast<lynx_value_ptr>(ptr);
    result.type = lynx_value_string;
    base::String::Unsafe::SetStringToEmpty(tmp);
    return true;
  }
  return lepus::BaseBinaryReader::DecodeUtf8Str(result);
}

bool LynxBinaryBaseCSSReader::GetEnableNewImportRule() {
  return compile_options_.enable_css_selector_ ||
         Config::IsHigherOrEqual(compile_options_.target_sdk_version_,
                                 LYNX_VERSION_2_9);
}

#pragma region SimpleStyling Decoder

bool LynxBinaryBaseCSSReader::DecodeStyleObjectRoute(
    StyleObjectRoute& style_object_route) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "DecodeStyleObjectRoute");
  uint32_t css_route_length = 0;
  DECODE_COMPACT_U32(size);
  for (size_t i = 0; i < size; ++i) {
    DECODE_COMPACT_U32(start);
    DECODE_COMPACT_U32(end);
    css_route_length = std::max(css_route_length, end);
    style_object_route.style_object_ranges.emplace_back(start, end);
  }
  style_objects_section_range_.start = static_cast<uint32_t>(stream_->offset());
  style_objects_section_range_.end =
      style_objects_section_range_.start + css_route_length;
  return true;
}

bool LynxBinaryBaseCSSReader::DecodeCSSAttributes(StyleMap& attr,
                                                  uint32_t size) {
  DCHECK(attr.empty());
  attr.reserve(size);
  bool success = true;
  for (size_t i = 0; i < size && success; ++i) {
    DECODE_COMPACT_U32(id);
    CSSPropertyID property_id = static_cast<CSSPropertyID>(id);
    success = DecodeCSSValue(&attr[property_id]);
  }
  return success;
}

bool LynxBinaryBaseCSSReader::DecodeStyleObject(StyleMap& attr,
                                                const CSSRange& range) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "DecodeStyleObject");
  size_t descriptor_start = stream_->offset();
  stream_->Seek(descriptor_start + range.start);
  DECODE_COMPACT_U32(size);
  attr.reserve(size);
  bool success = true;
  for (size_t i = 0; i < size && success; ++i) {
    DECODE_COMPACT_U32(id);
    CSSPropertyID property_id = static_cast<CSSPropertyID>(id);
    success = DecodeCSSValue(&attr[property_id], true, false, false);
  }
  return success;
}

#pragma endregion

}  // namespace tasm
}  // namespace lynx
