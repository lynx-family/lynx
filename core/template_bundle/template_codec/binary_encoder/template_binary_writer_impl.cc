// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <algorithm>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <vector>

#include "base/include/sorted_for_each.h"
#include "base/include/value/array.h"
#include "core/renderer/css/ng/media_query/media_query_set.h"
#include "core/renderer/css/ng/parser/media_query_parser.h"
#include "core/renderer/simple_styling/style_object.h"
#include "core/renderer/utils/base/tasm_constants.h"
#include "core/runtime/lepusng/quick_context.h"
#include "core/template_bundle/template_codec/binary_encoder/style_object_encoder/style_object_parser.h"
#include "core/template_bundle/template_codec/binary_encoder/template_binary_writer.h"
#include "core/template_bundle/template_codec/template_binary.h"

namespace lynx {
namespace tasm {
namespace {

class TemplateSectionRecorder {
 public:
  TemplateSectionRecorder(BinarySection binary_section,
                          BinaryOffsetType offset_type,
                          TemplateBinaryWriter* writer,
                          lepus::OutputStream* stream,
                          TemplateBinary& binary_info,
                          std::map<uint8_t, Range>& offset_map,
                          std::map<BinarySection, uint32_t>& section_size_info)
      : binary_section_(binary_section),
        offset_type_(offset_type),
        writer_(writer),
        stream_(stream),
        binary_info_(binary_info),
        offset_map_(offset_map),
        section_size_info_(section_size_info) {
    // for binary info
    binary_info_start_ = stream_->size();
    writer_->WriteU8(binary_section_);

    // for offset map
    section_start_ = stream_->size();
  }

  virtual ~TemplateSectionRecorder() {
    // for binary info
    binary_info_end_ = stream_->size();
    binary_info_.AddSection(binary_section_, binary_info_start_,
                            binary_info_end_);

    // for offset map
    section_end_ = stream_->size();
    offset_map_[offset_type_] = Range(section_start_, section_end_);

    // for section size info
    section_size_info_[binary_section_] = section_end_ - section_start_ + 1;
  }

 private:
  BinarySection binary_section_{};
  BinaryOffsetType offset_type_{};
  TemplateBinaryWriter* writer_{nullptr};
  lepus::OutputStream* stream_{nullptr};
  TemplateBinary& binary_info_;
  std::map<uint8_t, Range>& offset_map_;
  std::map<BinarySection, uint32_t>& section_size_info_;
  uint32_t section_start_{};
  uint32_t section_end_{};
  uint32_t binary_info_start_{};
  uint32_t binary_info_end_{};
};

}  // namespace

size_t TemplateBinaryWriter::EncodeFlexibleTemplateBody(
    std::function<void()> encode_func) {
  // Record size after write section count, and record this size as header size
  header_size_ = stream_->size();

  encode_func();

  EncodeSectionRoute();

  MoveLastSectionToFirst(BinarySection::SECTION_ROUTE);

  binary_info_.total_size_ = stream_->size();
  return stream_->size();
}

void TemplateBinaryWriter::EncodeSectionRoute() {
  TemplateSectionRecorder recorder(
      BinarySection::SECTION_ROUTE, BinaryOffsetType::TYPE_SECTION_ROUTE, this,
      stream_.get(), binary_info_, offset_map_, section_size_info_);
  // Encode section count
  WriteCompactU32(binary_info_.section_ary_.size());
  uint32_t start_pos = binary_info_.section_ary_[0].start_offset_;
  for (const auto& info : binary_info_.section_ary_) {
    WriteU8(info.type_);
    WriteCompactU32(info.start_offset_ - start_pos);
    WriteCompactU32(info.end_offset_ - start_pos);
  }
}

void TemplateBinaryWriter::MoveLastSectionToFirst(
    const BinarySection& section) {
  DCHECK(binary_info_.section_ary_.size() > 0);
  TemplateBinary::SectionInfo info =
      binary_info_.section_ary_[binary_info_.section_ary_.size() - 1];
  DCHECK(info.type_ == section);

  uint32_t insert_pos = binary_info_.section_ary_[0].start_offset_;
  uint32_t cur_size = stream_->size();
  stream_->Move(insert_pos, info.start_offset_, cur_size - info.start_offset_);

  for (auto& kv : offset_map_) {
    kv.second.start += cur_size - info.start_offset_;
    kv.second.end += cur_size - info.start_offset_;
  }
  offset_map_[section] =
      Range(insert_pos + 1, insert_pos + cur_size - info.start_offset_);
}

bool TemplateBinaryWriter::EncodeHeaderInfo(
    const CompileOptions& compile_options) {
  // register fields
  std::list<HeaderExtInfo::HeaderExtInfoField> header_info_fields;

#define REGISTER_FIXED_LENGTH_FIELD(type, field, id)              \
  header_info_fields.push_back(HeaderExtInfo::HeaderExtInfoField{ \
      HeaderExtInfo::TYPE_##type, id, HeaderExtInfo::SIZE_##type, \
      (void*)(&compile_options.field)})

  FOREACH_FIXED_LENGTH_FIELD(REGISTER_FIXED_LENGTH_FIELD)
#undef REGISTER_FIXED_LENGTH_FIELD

#define REGISTER_STRING_FIELD(field, id)                                      \
  header_info_fields.push_back(HeaderExtInfo::HeaderExtInfoField{             \
      HeaderExtInfo::TYPE_STRING, id, (uint16_t)compile_options.field.size(), \
      (void*)(compile_options.field.c_str())})

  FOREACH_STRING_FIELD(REGISTER_STRING_FIELD)
#undef REGISTER_STRING_FIELD
  // encode section header
  uint32_t header_ext_info_total_size = 0;
  header_ext_info_total_size += sizeof(HeaderExtInfo);
  for (const auto& field : header_info_fields) {
    header_ext_info_total_size +=
        sizeof(field) - sizeof(void*) + field.payload_size_;
  }
  header_ext_info_ = {header_ext_info_total_size, HEADER_EXT_INFO_MAGIC,
                      (uint32_t)header_info_fields.size()};
  stream_->WriteData((uint8_t*)&header_ext_info_, sizeof(header_ext_info_));

  // encode fields
  for (const auto& field : header_info_fields) {
    EncodeHeaderInfoField(field);
  }

  return true;
}

bool TemplateBinaryWriter::EncodeHeaderInfoField(
    const HeaderExtInfo::HeaderExtInfoField& header_info_field) {
  stream_->WriteData((const uint8_t*)(&header_info_field),
                     sizeof(header_info_field) - sizeof(void*));
  stream_->WriteData(static_cast<const uint8_t*>(header_info_field.payload_),
                     header_info_field.payload_size_);
  return true;
}

bool TemplateBinaryWriter::WriteToFile(const char* file_name) {
  stream_->WriteToFile(file_name);
  return true;
}

const std::vector<uint8_t> TemplateBinaryWriter::WriteToVector() {
  auto buffer = stream_->byte_array();
  return buffer;
}

LepusDebugInfo TemplateBinaryWriter::GetDebugInfo() const {
  LepusDebugInfo info;

  if (IsLepusNGContext()) {
    info.debug_info_.source_code = quick_context()->GetDebugSourceCode();
    info.debug_info_.top_level_function =
        quick_context()->GetTopLevelFunction();
  } else {
    info.lepus_funcs_ = GetContextFunc();
  }

  return info;
}

const std::vector<lynx::fml::RefPtr<lynx::lepus::Function>>&
TemplateBinaryWriter::GetContextFunc() const {
  return func_vec;
}

void TemplateBinaryWriter::EncodeCSSDescriptor() {
  TemplateSectionRecorder recorder(
      BinarySection::CSS, BinaryOffsetType::TYPE_CSS, this, stream_.get(),
      binary_info_, offset_map_, section_size_info_);

  auto& fragments = css_parser_->fragments();
  CSSRoute route;
  uint32_t descriptor_offset = stream()->size();
  uint32_t start = 0;
  uint32_t end = 0;
  base::sorted_for_each(
      fragments.begin(), fragments.end(),
      [descriptor_offset, &route, &start, &end, this](const auto& it) {
        auto& fragment = it.second;
        EncodeCSSFragment(fragment);
        end = stream()->size() - descriptor_offset;
        route.fragment_ranges.insert({fragment->id(), CSSRange(start, end)});
        start = end;
      });

  start = stream()->size();
  EncodeCSSRoute(route);
  end = stream()->size();

  if (!fragments.empty()) {
    // Insert CSSRouter before CSSFragments
    stream_->Move(descriptor_offset, start, end - start);
  }
}

void TemplateBinaryWriter::EncodeCSSRoute(const CSSRoute& css_route) {
  WriteCompactU32(css_route.fragment_ranges.size());
  base::sorted_for_each(css_route.fragment_ranges.begin(),
                        css_route.fragment_ranges.end(),
                        [this](const auto& it) {
                          WriteCompactS32(it.first);
                          // CSSRange
                          WriteCompactU32(it.second.start);
                          WriteCompactU32(it.second.end);
                        });
}

void TemplateBinaryWriter::EncodeCSSFragment(
    encoder::SharedCSSFragment* fragment) {
  // Id
  WriteCompactU32(fragment->id());
  // Dependents css id
  WriteCompactU32(fragment->dependent_ids().size());
  for (auto id : fragment->dependent_ids()) {
    WriteCompactS32(id);
  }

  if (compile_options_.enable_css_rule_) {
    EncodeCSSRules(fragment);
    return;
  }

  // SelectorList
  if (compile_options_.enable_css_selector_) {
    size_t selector_size = fragment->selector_tuple().size();
    WriteCompactU32(selector_size);
    for (const auto& it : fragment->selector_tuple()) {
      EncodeLynxCSSSelectorTuple(it);
    }
  }
  // CSS parse token
  size_t size = fragment->css().size();
  size_t keyframes_size = fragment->GetKeyframesRuleMapForEncode().size() << 16;
  size += keyframes_size;
  WriteCompactU32(size);
  base::sorted_for_each(fragment->css().begin(), fragment->css().end(),
                        [this](const auto& it) {
                          EncodeUtf8Str(it.first.c_str(), it.first.length());
                          EncodeCSSParseToken(it.second.get());
                        });

  base::sorted_for_each(fragment->GetKeyframesRuleMapForEncode().begin(),
                        fragment->GetKeyframesRuleMapForEncode().end(),
                        [this](const auto& it) {
                          EncodeUtf8Str(it.first.c_str(), it.first.length());
                          EncodeCSSKeyframesToken(it.second.get());
                        });

  const size_t fontface_size = fragment->GetFontFaceTokenMapForEncode().size();
  if (fontface_size > 0) {
    WriteU8(CSS_BINARY_FONT_FACE_TYPE);
    WriteCompactU32(fontface_size);
    if (lynx::tasm::Config::IsHigherOrEqual(
            compile_options_.target_sdk_version_,
            FEATURE_CSS_FONT_FACE_EXTENSION)) {
      base::sorted_for_each(
          fragment->GetFontFaceTokenMapForEncode().begin(),
          fragment->GetFontFaceTokenMapForEncode().end(),
          [this](const auto& it) { EncodeCSSFontFaceTokenList(it.second); });
    } else {
      base::sorted_for_each(fragment->GetFontFaceTokenMapForEncode().begin(),
                            fragment->GetFontFaceTokenMapForEncode().end(),
                            [this](const auto& it) {
                              EncodeCSSFontFaceToken(it.second[0].get());
                            });
    }
  }
}

bool TemplateBinaryWriter::EncodeCSSParseToken(CSSParseToken* token) {
  DCHECK(token != nullptr);
  EncodeCSSAttributes(token->GetAttributes());
  if (lynx::tasm::Config::IsHigherOrEqual(compile_options_.target_sdk_version_,
                                          FEATURE_CSS_IMPORTANT)) {
    EncodeCSSAttributes(token->GetImportantAttributes());
  }
  if (lynx::tasm::Config::IsHigherOrEqual(compile_options_.target_sdk_version_,
                                          FEATURE_CSS_STYLE_VARIABLES) &&
      compile_options_.enable_css_variable_) {
    EncodeCSSStyleVariables(token->GetStyleVariables());
  }
  if (compile_options_.enable_css_selector_) {
    return true;
  }
  const auto& sheets = token->sheets();
  size_t size = sheets.size();
  WriteCompactU32(size);

  if (sheets.size() == 0) {
    return true;
  }
  for (size_t i = 0; i < sheets.size(); i++) {
    CSSSheet* sheet = sheets[i].get();
    DCHECK(sheet != nullptr);
    EncodeCSSSheet(sheet);
  }

  return true;
}

bool TemplateBinaryWriter::EncodeLynxCSSSelectorTuple(
    const encoder::LynxCSSSelectorTuple& selector_tuple) {
  size_t flattened_size = selector_tuple.flattened_size;
  // size
  WriteCompactU32(flattened_size);
  if (flattened_size == 0 || !selector_tuple.selector_arr) {
    return true;
  }
  EncodeCSSSelector(selector_tuple.selector_arr.get());
  EncodeCSSParseToken(selector_tuple.parse_token.get());
  return true;
}

bool TemplateBinaryWriter::EncodeCSSSelector(
    const css::LynxCSSSelector* selector) {
  DCHECK(selector != nullptr);
  auto current = selector;
  while (current) {
    auto value = current->ToLepus();
    EncodeValue(&value);
    if (current->IsLastInTagHistory() && current->IsLastInSelectorList()) {
      break;
    }
    current++;
  }
  return true;
}

void TemplateBinaryWriter::EncodeCSSRules(
    encoder::SharedCSSFragment* fragment) {
  const auto& rules = fragment->rules();
  WriteCompactU32(rules.size());
  for (const auto& rule : rules) {
    WriteU8(static_cast<uint8_t>(rule->type));
    switch (rule->type) {
      case encoder::LynxStyleRuleBase::kStyle:
        EncodeCSSStyleRule(
            *static_cast<const encoder::LynxStyleRule*>(rule.get()));
        break;
      case encoder::LynxStyleRuleBase::kMedia:
      case encoder::LynxStyleRuleBase::kSupports:
        EncodeCSSConditionRule(
            *static_cast<const encoder::LynxStyleRuleCondition*>(rule.get()));
        break;
      case encoder::LynxStyleRuleBase::kKeyframes:
        EncodeCSSKeyframesRule(
            *static_cast<const encoder::LynxStyleRuleKeyframes*>(rule.get()));
        break;
      case encoder::LynxStyleRuleBase::kFontFace:
        EncodeCSSFontFaceRule(
            *static_cast<const encoder::LynxStyleRuleFontFace*>(rule.get()));
        break;
      default:
        break;
    }
  }
}

void TemplateBinaryWriter::EncodeCSSStyleRule(
    const encoder::LynxStyleRule& rule) {
  WriteCompactU32(static_cast<uint32_t>(rule.position));
  WriteCompactU32(rule.flattened_size);
  EncodeCSSSelector(rule.selector_arr.get());
  EncodeCSSParseToken(rule.properties.get());
}

void TemplateBinaryWriter::EncodeCSSConditionRule(
    const encoder::LynxStyleRuleCondition& rule) {
  if (rule.type == encoder::LynxStyleRuleBase::kMedia) {
    auto media_queries =
        css::MediaQueryParser::ParseMediaQuerySet(rule.condition);
    // `MediaQueryParser::ParseMediaQuerySet` never returns null, but guard
    // anyway so future refactors cannot silently write a non-array payload.
    auto media_value = media_queries ? media_queries->ToLepus()
                                     : lepus_value(lepus::CArray::Create());
    EncodeValue(&media_value);
  } else {
    EncodeUtf8Str(rule.condition.c_str(), rule.condition.length());
  }
  WriteCompactU32(rule.child_rules.size());
  for (const auto& child : rule.child_rules) {
    WriteU8(static_cast<uint8_t>(child->type));
    switch (child->type) {
      case encoder::LynxStyleRuleBase::kStyle:
        EncodeCSSStyleRule(
            *static_cast<const encoder::LynxStyleRule*>(child.get()));
        break;
      default:
        break;
    }
  }
}

void TemplateBinaryWriter::EncodeCSSKeyframesRule(
    const encoder::LynxStyleRuleKeyframes& rule) {
  EncodeUtf8Str(rule.name.c_str(), rule.name.length());
  EncodeCSSKeyframesToken(rule.properties.get());
}

void TemplateBinaryWriter::EncodeCSSFontFaceRule(
    const encoder::LynxStyleRuleFontFace& rule) {
  EncodeUtf8Str(rule.family.c_str(), rule.family.length());
  if (lynx::tasm::Config::IsHigherOrEqual(compile_options_.target_sdk_version_,
                                          FEATURE_CSS_FONT_FACE_EXTENSION)) {
    EncodeCSSFontFaceTokenList(rule.properties);
  } else {
    EncodeCSSFontFaceToken(rule.properties[0].get());
  }
}

void TemplateBinaryWriter::EncodeSimpleStyleObjects() {
  TemplateSectionRecorder recorder(
      BinarySection::STYLE_OBJECT, BinaryOffsetType::TYPE_STYLE_OBJECT, this,
      stream_.get(), binary_info_, offset_map_, section_size_info_);
  if (!style_object_parser_) {
    return;
  }
  uint32_t style_object_section_count =
      static_cast<uint32_t>(StyleObjectSectionType::SECTION_COUNT);
  WriteCompactU32(style_object_section_count);
  // Encode style_object section
  static_assert(static_cast<uint32_t>(StyleObjectSectionType::STYLE_OBJECT) ==
                0);
  auto& style_objects = style_object_parser_->StyleObjects();
  StyleObjectRoute route;
  uint32_t descriptor_offset = stream()->size();
  uint32_t start = 0;
  uint32_t end = 0;
  if (!style_objects.empty()) {
    std::for_each(style_objects.begin(), style_objects.end(),
                  [descriptor_offset, &route, &start, &end,
                   this](const style::StyleObject& style_obj) {
                    EncodeCSSAttributes(style_obj.Properties());
                    end = stream()->size() - descriptor_offset;
                    route.style_object_ranges.emplace_back(start, end);
                    start = end;
                  });
  }
  start = stream()->size();
  EncodeSimpleStyleObjectsRoute(route);
  end = stream()->size();
  stream_->Move(descriptor_offset, start, end - start);
  // Encode keyframes section
  static_assert(static_cast<uint32_t>(
                    StyleObjectSectionType::STYLE_OBJECT_KEYFRAMES) == 1);
  auto& style_objects_keyframes = style_object_parser_->StyleObjectsKeyframes();
  descriptor_offset = stream()->size();
  start = 0;
  end = 0;
  StyleObjectRoute keyframes_route;
  if (!style_objects_keyframes.empty()) {
    base::sorted_for_each(
        style_objects_keyframes.begin(), style_objects_keyframes.end(),
        [descriptor_offset, &keyframes_route, &start, &end,
         this](const auto& it) {
          EncodeUtf8Str(it.first.c_str(), it.first.length());
          EncodeCSSKeyframesToken(it.second.get());
          end = stream()->size() - descriptor_offset;
          keyframes_route.style_object_ranges.emplace_back(start, end);
          start = end;
        });
  }
  start = stream()->size();
  EncodeSimpleStyleObjectsRoute(keyframes_route);
  end = stream()->size();
  stream_->Move(descriptor_offset, start, end - start);
  // Encode fontfaces section
  static_assert(static_cast<uint32_t>(
                    StyleObjectSectionType::STYLE_OBJECT_FONTFACES) == 2);
  auto& style_objects_fontfaces = style_object_parser_->StyleObjectsFontFaces();
  descriptor_offset = stream()->size();
  start = 0;
  end = 0;
  StyleObjectRoute fontfaces_route;

  if (!style_objects_fontfaces.empty()) {
    base::sorted_for_each(
        style_objects_fontfaces.begin(), style_objects_fontfaces.end(),
        [descriptor_offset, &fontfaces_route, &start, &end,
         this](const auto& it) {
          EncodeUtf8Str(it.first.c_str(), it.first.length());
          EncodeCSSFontFaceTokenList(it.second);
          end = stream()->size() - descriptor_offset;
          fontfaces_route.style_object_ranges.emplace_back(start, end);
          start = end;
        });
  }

  start = stream()->size();
  EncodeSimpleStyleObjectsRoute(fontfaces_route);
  end = stream()->size();
  stream_->Move(descriptor_offset, start, end - start);
}

void TemplateBinaryWriter::EncodeSimpleStyleObjectsRoute(
    const StyleObjectRoute& route) {
  WriteCompactU32(route.style_object_ranges.size());
  std::for_each(route.style_object_ranges.begin(),
                route.style_object_ranges.end(), [this](const CSSRange& it) {
                  // CSSRange
                  WriteCompactU32(it.start);
                  WriteCompactU32(it.end);
                });
}

bool TemplateBinaryWriter::EncodeCSSKeyframesToken(
    encoder::CSSKeyframesToken* token) {
  DCHECK(token != nullptr);

  EncodeCSSKeyframesMap(token->GetKeyframes());
  if (tasm::Config::IsHigherOrEqual(
          compile_options_.target_sdk_version_,
          FEATURE_CUSTOM_PROPERTY_DECLARATION_KEYFRAME) &&
      compile_options_.enable_keyframe_custom_property_declaration_) {
    EncodeCSSKeyframesCustomPropertyContent(
        token->GetKeyframesCustomPropertyMap());
  }
  return true;
}

bool TemplateBinaryWriter::EncodeCSSFontFaceTokenList(
    const std::vector<std::shared_ptr<CSSFontFaceToken>>& tokenList) {
  uint32_t size = tokenList.size();
  // size
  WriteCompactU32(size);
  if (size == 0) {
    return true;
  }
  for (size_t i = 0; i < size; i++) {
    CSSFontFaceToken* token = tokenList[i].get();
    EncodeCSSFontFaceToken(token);
  }
  return true;
}

bool TemplateBinaryWriter::EncodeCSSFontFaceToken(CSSFontFaceToken* token) {
  DCHECK(token != nullptr);
  auto attrMap = token->GetAttrMap();

  uint32_t size = attrMap.size();
  // size
  WriteCompactU32(size);

  base::sorted_for_each(attrMap.begin(), attrMap.end(),
                        [this](const auto& itr) {
                          // key text
                          EncodeUtf8Str(itr.first.c_str());
                          // value text
                          EncodeUtf8Str(itr.second.c_str());
                        });
  return true;
}

bool TemplateBinaryWriter::EncodeCSSSheet(CSSSheet* sheet) {
  DCHECK(sheet != nullptr);

  WriteCompactU32(sheet->GetType());
  EncodeUtf8Str(sheet->GetName().c_str());
  EncodeUtf8Str(sheet->GetSelector().c_str());

  return true;
}

bool TemplateBinaryWriter::EncodeCSSAttributes(const StyleMap& attrs) {
  uint32_t size = attrs.size();
  // size
  WriteCompactU32(size);

  for (auto it = attrs.begin(); it != attrs.end(); ++it) {
    // id
    WriteCompactU32(it->first);
    EncodeCSSValue(it->second);
  }
  return true;
}

bool TemplateBinaryWriter::EncodeCSSStyleVariables(
    const CSSVariableMap& style_variables) {
  uint32_t size = style_variables.size();
  WriteCompactU32(size);
  for (auto it = style_variables.begin(); it != style_variables.end(); ++it) {
    WriteStringDirectly(it->first.c_str());
    WriteStringDirectly(it->second.c_str());
  }
  return true;
}

bool TemplateBinaryWriter::EncodeCSSKeyframesMap(
    const CSSKeyframesMap& keyframes) {
  uint32_t size = keyframes.size();
  // size
  WriteCompactU32(size);

  base::sorted_for_each(
      keyframes.begin(), keyframes.end(), [this](const auto& itr) {
        if (Config::IsHigherOrEqual(compile_options_.target_sdk_version_,
                                    FEATURE_CSS_VALUE_VERSION) &&
            compile_options_.enable_css_parser_) {
          // key text
          WriteCompactD64(CSSKeyframesToken::ParseKeyStr(
              itr.first, compile_options_.enable_css_strict_mode_));
        } else {
          EncodeUtf8Str(itr.first.c_str());
        }
        // css attrs
        EncodeCSSAttributes(*itr.second);
      });
  return true;
}

bool TemplateBinaryWriter::EncodeCSSKeyframesCustomProperty(
    const CustomPropertiesMap& custom_properties) {
  WriteCompactU32(custom_properties.size());
  for (const auto& [key, value] : custom_properties) {
    EncodeUtf8Str(key.c_str());
    EncodeCSSValue(value);
  }
  return true;
}

bool TemplateBinaryWriter::EncodeCSSKeyframesCustomPropertyContent(
    const CSSKeyframesCustomPropertyMap& keyframes) {
  WriteCompactU32(keyframes.size());
  base::sorted_for_each(
      keyframes.begin(), keyframes.end(), [this](const auto& itr) {
        if (Config::IsHigherOrEqual(compile_options_.target_sdk_version_,
                                    FEATURE_CSS_VALUE_VERSION) &&
            compile_options_.enable_css_parser_) {
          WriteCompactD64(CSSKeyframesToken::ParseKeyStr(
              itr.first, compile_options_.enable_css_strict_mode_));
        } else {
          EncodeUtf8Str(itr.first.c_str());
        }
        if (itr.second == nullptr) {
          WriteCompactU32(0);
          return;
        }
        EncodeCSSKeyframesCustomProperty(*itr.second);
      });
  return true;
}

}  // namespace tasm
}  // namespace lynx
