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
    CSSKeyframesToken* token = new CSSKeyframesToken(parser_config);
    ERROR_UNLESS(DecodeCSSKeyframesToken(token));
    fragment->keyframes_.emplace(std::move(name), fml::AdoptRef(token));
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
              CSSFontFaceRule* token = new CSSFontFaceRule();
              ERROR_UNLESS(DecodeCSSFontFaceToken(token));
              token_list.emplace_back(token);
            }
          } else {
            CSSFontFaceRule* token = new CSSFontFaceRule();
            ERROR_UNLESS(DecodeCSSFontFaceToken(token));
            token_list.emplace_back(token);
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

bool LynxBinaryBaseCSSReader::DecodeCSSParseToken(CSSParseToken* token) {
  ERROR_UNLESS(DecodeCSSAttributes(token));

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
