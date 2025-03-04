// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/template_bundle/template_codec/binary_encoder/csr_element_binary_writer.h"

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <string>
#include <unordered_map>

#include "base/include/sorted_for_each.h"
#include "core/renderer/css/css_property.h"
#include "core/renderer/css/css_property_id.h"
#include "core/renderer/dom/element_property.h"
#include "core/renderer/events/events.h"
#include "core/renderer/utils/base/tasm_constants.h"
#include "core/runtime/vm/lepus/exception.h"
#include "core/runtime/vm/lepus/json_parser.h"
#include "core/template_bundle/template_codec/binary_encoder/css_encoder/css_parser_token.h"
#include "core/template_bundle/template_codec/template_binary.h"

namespace lynx {
namespace tasm {

namespace {

bool CheckJSONObjectValid(const rapidjson::Value* element, const char* key) {
  auto obj = element->GetObject();
  auto iter = obj.FindMember(key);
  if (iter == obj.MemberEnd() || !iter->value.IsObject()) {
    return false;
  }
  if (iter->value.GetObject().MemberCount() <= 0) {
    return false;
  }
  return true;
}

bool CheckJSONArrayValid(const rapidjson::Value* element, const char* key) {
  auto obj = element->GetObject();
  auto iter = obj.FindMember(key);
  if (iter == obj.MemberEnd() || !iter->value.IsArray()) {
    return false;
  }
  if (iter->value.GetArray().Size() <= 0) {
    return false;
  }
  return true;
}

bool CheckJSONStringValid(const rapidjson::Value* element, const char* key) {
  auto obj = element->GetObject();
  auto iter = obj.FindMember(key);
  if (iter == obj.MemberEnd() || !iter->value.IsString()) {
    return false;
  }
  if (iter->value.GetStringLength() <= 0) {
    return false;
  }
  return true;
}
}  // namespace

void CSRElementBinaryWriter::EncodeSingleTemplateToBinary(
    const rapidjson::Value* single_template) {
  // 1. Encode element recursively.
  EncodeElementRecursively(single_template);
}

void CSRElementBinaryWriter::EncodeTemplatesToBinary(
    const rapidjson::Document* templates) {
  // Encode element template body.
  EncodeTemplatesBody(templates);
}

void CSRElementBinaryWriter::EncodeParsedStylesToBinary(
    const rapidjson::Value* parsed_styles) {
  // 1. Get parsed styles router start postion
  size_t router_start_pos = Offset();

  // 2. Encode parsed styles.
  EncodeParsedStyles(parsed_styles, router_start_pos);
}

void CSRElementBinaryWriter::EncodeTemplatesBody(
    const rapidjson::Document* templates) {
  // 1. Init ElementTemplate router
  OrderedStringKeyRouter router;
  uint32_t descriptor_offset = stream()->size();
  uint32_t start_offset = 0;
  router.descriptor_offset_ = descriptor_offset;

  // 2. Encode Element Template
  for (auto& pair : templates->GetObject()) {
    // Record start offset to router
    router.start_offsets_.emplace_or_assign(pair.name.GetString(),
                                            start_offset);

    // Encode Template array
    WriteCompactU32(pair.value.GetArray().Size());
    for (const auto& e : pair.value.GetArray()) {
      EncodeElementRecursively(&e);
    }

    // update start offset
    start_offset = stream()->size() - descriptor_offset;
  }

  // 3. Encode Element Template Router
  EncodeOrderedStringKeyRouter(router);
}

void CSRElementBinaryWriter::EncodeParsedStyles(
    const rapidjson::Value* parsed_styles, uint32_t router_start_pos) {
  // 1. Record the start position of parsed styles section.
  tasm::StringKeyRouter parsed_styles_router;
  parsed_styles_router.descriptor_offset_ = router_start_pos;

  // 2. Encode all parsed styles.
  EncodeElementParsedStylesInternal(parsed_styles, parsed_styles_router);

  // 3. Encode parsed styles router and move the bytecode to the start of the
  // parsed styles section.
  EncodeStringKeyRouter(parsed_styles_router);
}

void CSRElementBinaryWriter::EncodeElementParsedStylesInternal(
    const rapidjson::Value* parsed_styles, tasm::StringKeyRouter& router) {
  if (!parsed_styles->IsObject()) {
    return;
  }
  const auto& value = lepus::jsonValueTolepusValue(*parsed_styles);

  // Sort the parsed styles in lexicographical order by key to ensure the
  // stability of the encoded results, making it easier for us to perform
  // testing.
  base::sorted_for_each(
      value.Table()->begin(), value.Table()->end(),
      [this, &router](const auto& iter) {
        const base::String& key = iter.first;
        const lepus::Value& array = iter.second;
        if (!array.IsArray()) {
          return;
        }

        // Save the start offset of the parsed styles to router.
        router.start_offsets_.emplace(
            key.str(),
            static_cast<uint32_t>(Offset()) - router.descriptor_offset_);

        EncodeParsedStyle(array);
      },
      [](const auto& left, const auto& right) {
        return left.first.str() < right.first.str();
      });
}

void CSRElementBinaryWriter::EncodeParsedStyle(const lepus::Value& style) {
  StyleMap style_map;
  CSSVariableMap css_var_map;
  tasm::ForEachLepusValue(style, [this, &style_map, &css_var_map](
                                     const lepus::Value& index,
                                     const lepus::Value& token_lepus_value) {
    std::string rule = "rule";
    std::string path = "path";
    CompileOptions compile_options = compile_options_;
    compile_options.enable_css_parser_ = true;
    encoder::CSSParseToken token(
        token_lepus_value.GetProperty("style"), rule, path,
        token_lepus_value.GetProperty("variables"), compile_options);
    for (const auto& [key, value] : token.GetAttributes()) {
      style_map.insert_or_assign(key, value);
    }
    for (const auto& [key, value] : token.GetStyleVariables()) {
      css_var_map.insert_or_assign(key, value);
    }
  });

  // encode style map
  WriteCompactU32(static_cast<uint32_t>(style_map.size()));
  // Sort the parsed styles in lexicographical order by key to ensure the
  // stability of the encoded results, making it easier for us to perform
  // testing.
  base::SortedForEach(
      style_map,
      [this](const auto& pair) {
        // key
        WriteCompactU32(static_cast<uint32_t>(pair.first));
        // value
        EncodeCSSValue(pair.second);
      },
      [](const auto& left, const auto& right) {
        return left.first < right.first;
      });

  // encode css variable map
  WriteCompactU32(static_cast<uint32_t>(css_var_map.size()));
  // Sort the parsed styles in lexicographical order by key to ensure the
  // stability of the encoded results, making it easier for us to perform
  // testing.
  base::SortedForEach(
      css_var_map,
      [this](const auto& pair) {
        // key
        EncodeUtf8Str(pair.first.c_str(), pair.first.length());
        // value
        EncodeUtf8Str(pair.second.c_str(), pair.second.length());
      },
      [](const auto& left, const auto& right) {
        return left.first < right.first;
      });
}

void CSRElementBinaryWriter::EncodeElementParsedStyleSection(
    const rapidjson::Value* element) {
  if (!CheckJSONArrayValid(element, kElementParsedStyle)) {
    return;
  }
  auto parsed_style_iter = element->GetObject().FindMember(kElementParsedStyle);

  WriteU8(
      static_cast<uint8_t>(tasm::ElementSectionEnum::ELEMENT_PARSED_STYLES));

  const auto parsed_style =
      lepus::jsonValueTolepusValue(parsed_style_iter->value);
  EncodeParsedStyle(parsed_style);
}

void CSRElementBinaryWriter::EncodeStringKeyRouter(
    tasm::StringKeyRouter& router) {
  size_t start = Offset();
  WriteCompactU32(router.start_offsets_.size());

  for (const auto& [key, offset] : router.start_offsets_) {
    // key
    EncodeUtf8Str(key.c_str(), key.length());
    // offset
    WriteCompactU32(offset);
  }
  size_t end = Offset();

  // Move router binary to the start position.
  Move(router.descriptor_offset_, start, end - start);
}

void CSRElementBinaryWriter::EncodeOrderedStringKeyRouter(
    tasm::OrderedStringKeyRouter& router) {
  size_t start = Offset();
  WriteCompactU32(router.start_offsets_.size());

  for (const auto& [key, offset] : router.start_offsets_) {
    // key
    EncodeUtf8Str(key.c_str(), key.length());
    // offset
    WriteCompactU32(offset);
  }
  size_t end = Offset();

  // Move router binary to the start position.
  Move(router.descriptor_offset_, start, end - start);
}

void CSRElementBinaryWriter::EncodeElementRecursively(
    const rapidjson::Value* element) {
  // 1. encode children section router offset placeholder
  size_t children_offset_begin_pos = Offset();
  WriteU32(0);
  size_t children_offset_end_pos = Offset();

  // 2. Encode construction infos section if need
  // Construction infos section must be the first section. There is no
  // construction infos now.

  // 3. Encode tag section - tag section must be the second section.
  EncodeElementTagSection(element);

  // 4. Encode builtin attributes section
  EncodeElementBuiltinAttrSection(element);

  // 5. Encode ID selector section
  EncodeElementIDSelectorSection(element);

  // 6. Encode inline styles section
  EncodeElementInlineStyleSection(element);

  // 7. Encode class section
  EncodeElementClassSection(element);

  // 8. Encode event section
  EncodeElementJSEventSection(element);

  // 9. Encode attribute section
  EncodeElementAttributeSection(element);

  // 10. Encode dataset section
  EncodeElementDatasetSection(element);

  // 11. Encode parsed styles key section
  EncodeElementParsedStyleKeySection(element);

  // 12. Encode parsed styles section
  EncodeElementParsedStyleSection(element);

  // Future added sections must be placed immediately before the children
  // section and adjacent to it. This is because older versions of the SDK will
  // skip unrecognized sections and continue parsing from the children section.
  // If newly added sections are not placed adjacent to the children section, it
  // will cause the older SDK versions to skip sections that they can normally
  // recognize.

  // 13. Encode children section
  // 13.1 Overwrite children section offset first.
  uint32_t children_section_offset =
      static_cast<uint32_t>(Offset() - children_offset_end_pos);
  OverwriteData(reinterpret_cast<const uint8_t*>(&children_section_offset),
                sizeof(children_section_offset), children_offset_begin_pos);

  // 13.2 Encode children recursively
  WriteU8(static_cast<uint8_t>(tasm::ElementSectionEnum::ELEMENT_CHILDREN));

  if (!CheckJSONArrayValid(element, kElementChildren)) {
    // No child, we should encode size 0 to binary.
    WriteCompactU32(0);
    return;
  }
  auto children = element->GetObject()[kElementChildren].GetArray();
  WriteCompactU32(children.Size());
  for (const auto& c : children) {
    EncodeElementRecursively(&c);
  }
}

void CSRElementBinaryWriter::EncodeElementTagSection(
    const rapidjson::Value* element) {
  static const base::NoDestructor<
      std::unordered_map<std::string, ElementBuiltInTagEnum>>
      kElementBuiltInTagMap(
          {{kElementViewTag, ElementBuiltInTagEnum::ELEMENT_VIEW},
           {kElementComponentTag, ElementBuiltInTagEnum::ELEMENT_COMPONENT},
           {kElementPageTag, ElementBuiltInTagEnum::ELEMENT_PAGE},
           {kElementImageTag, ElementBuiltInTagEnum::ELEMENT_IMAGE},
           {kElementTextTag, ElementBuiltInTagEnum::ELEMENT_TEXT},
           {kElementXTextTag, ElementBuiltInTagEnum::ELEMENT_X_TEXT},
           {kElementRawTextTag, ElementBuiltInTagEnum::ELEMENT_RAW_TEXT},
           {kElementScrollViewTag, ElementBuiltInTagEnum::ELEMENT_SCROLL_VIEW},
           {kElementXScrollViewTag,
            ElementBuiltInTagEnum::ELEMENT_X_SCROLL_VIEW},
           {kElementListTag, ElementBuiltInTagEnum::ELEMENT_LIST},
           {kElementNoneElementTag, ElementBuiltInTagEnum::ELEMENT_NONE},
           {kElementWrapperElementTag,
            ElementBuiltInTagEnum::ELEMENT_WRAPPER}});

  auto obj = element->GetObject();
  auto iter = obj.FindMember(kElementType);
  if (iter == obj.MemberEnd() || !iter->value.IsString()) {
    throw lepus::EncodeException(
        "element must have a type, or the value of type must be string!");
  }
  const auto& tag = iter->value.GetString();
  auto tag_len = iter->value.GetStringLength();
  auto builtin_tag_iter = kElementBuiltInTagMap->find(tag);
  if (builtin_tag_iter != kElementBuiltInTagMap->end()) {
    WriteU8(static_cast<uint8_t>(tasm::ElementSectionEnum::ELEMENT_TAG_ENUM));
    WriteU8(static_cast<uint8_t>(builtin_tag_iter->second));
  } else {
    // encode string tag
    WriteU8(static_cast<uint8_t>(tasm::ElementSectionEnum::ELEMENT_TAG_STR));
    EncodeUtf8Str(tag, tag_len);
  }
}

void CSRElementBinaryWriter::EncodeElementBuiltinAttrSection(
    const rapidjson::Value* element) {
  if (!CheckJSONObjectValid(element, kElementBuiltinAttributes)) {
    return;
  }
  auto builtin_attrs =
      element->GetObject()[kElementBuiltinAttributes].GetObject();

  WriteU8(static_cast<uint8_t>(
      tasm::ElementSectionEnum::ELEMENT_BUILTIN_ATTRIBUTE));

  static const base::NoDestructor<
      std::unordered_map<std::string, ElementBuiltInAttributeEnum>>
      kElementBuiltInAttributeMap(
          {{kComponentID, ElementBuiltInAttributeEnum::COMPONENT_ID},
           {kElementComponentName, ElementBuiltInAttributeEnum::COMPONENT_NAME},
           {kElementComponentPath, ElementBuiltInAttributeEnum::COMPONENT_PATH},
           {kElementCSSID, ElementBuiltInAttributeEnum::CSS_ID},
           {kNodeIndex, ElementBuiltInAttributeEnum::NODE_INDEX},
           {kDirtyID, ElementBuiltInAttributeEnum::DIRTY_ID}});

  // Record the insert position of count.
  size_t count_insert_pos = Offset();
  size_t count = 0;
  // Get the count of valid styles when traversal the inline styles.
  for (const auto& pair : builtin_attrs) {
    const auto& key = pair.name;
    const auto& value = pair.value;
    const auto& key_enum_iter =
        kElementBuiltInAttributeMap->find(key.GetString());
    // Can not recognize this key, skip this key.
    if (key_enum_iter == kElementBuiltInAttributeMap->end()) {
      continue;
    }
    ++count;
    WriteCompactU32(static_cast<uint32_t>(key_enum_iter->second));
    const auto& lepus_val = lepus::jsonValueTolepusValue(value);
    EncodeValue(&lepus_val);
  }
  EncodeCountAndInsertAhead(count, count_insert_pos);
}

void CSRElementBinaryWriter::EncodeElementIDSelectorSection(
    const rapidjson::Value* element) {
  if (!CheckJSONStringValid(element, kElementIdSelector)) {
    return;
  }
  auto id_iter = element->GetObject().FindMember(kElementIdSelector);

  WriteU8(static_cast<uint8_t>(tasm::ElementSectionEnum::ELEMENT_ID_SELECTOR));
  EncodeUtf8Str(id_iter->value.GetString(), id_iter->value.GetStringLength());
}

void CSRElementBinaryWriter::EncodeElementInlineStyleSection(
    const rapidjson::Value* element) {
  if (!CheckJSONObjectValid(element, kElementStyles)) {
    return;
  }
  auto styles = element->GetObject()[kElementStyles].GetObject();

  WriteU8(static_cast<uint8_t>(tasm::ElementSectionEnum::ELEMENT_STYLES));

  // Record the insert position of count.
  size_t count_insert_pos = Offset();
  size_t count = 0;
  // Get the count of valid styles when traversal the inline styles.
  for (const auto& [key, value] : styles) {
    if (!key.IsString() || !value.IsString()) {
      continue;
    }
    CSSPropertyID property_id =
        static_cast<CSSPropertyID>(std::atoi(key.GetString()));
    // Can not recognize this key, skip this key.
    if (!CSSProperty::IsPropertyValid(property_id)) {
      continue;
    }
    ++count;
    WriteCompactU32(static_cast<uint32_t>(property_id));
    const auto& lepus_val = lepus::jsonValueTolepusValue(value);
    EncodeValue(&lepus_val);
  }

  EncodeCountAndInsertAhead(count, count_insert_pos);
}

void CSRElementBinaryWriter::EncodeElementClassSection(
    const rapidjson::Value* element) {
  // TODO: Enable the optimization that do not encode the classes of static
  // element.
  if (!CheckJSONArrayValid(element, kElementClass)) {
    return;
  }
  auto classes = element->GetObject()[kElementClass].GetArray();

  WriteU8(static_cast<uint8_t>(tasm::ElementSectionEnum::ELEMENT_CLASS));

  // Record the insert position of count.
  size_t count_insert_pos = Offset();
  size_t count = 0;
  // Get the count of valid styles when traversal the inline styles.
  for (const auto& c : classes) {
    if (!c.IsString()) {
      continue;
    }
    ++count;
    EncodeUtf8Str(c.GetString(), c.GetStringLength());
  }

  EncodeCountAndInsertAhead(count, count_insert_pos);
}

void CSRElementBinaryWriter::EncodeElementJSEventSection(
    const rapidjson::Value* element) {
  if (!CheckJSONArrayValid(element, kElementEvents)) {
    return;
  }
  auto events = element->GetObject()[kElementEvents].GetArray();

  WriteU8(static_cast<uint8_t>(tasm::ElementSectionEnum::ELEMENT_EVENTS));
  const auto& encode_specific_key = [this](const auto& event, const auto& key) {
    constexpr const static char* kEmptyString = "";
    auto iter = event.GetObject().FindMember(key);
    if (iter != event.GetObject().MemberEnd() && iter->value.IsString()) {
      EncodeUtf8Str(iter->value.GetString(), iter->value.GetStringLength());
    } else {
      EncodeUtf8Str(kEmptyString, 0);
    }
  };

  constexpr const static char* kType = "type";
  constexpr const static char* kName = "name";
  constexpr const static char* kValue = "value";

  // Record the insert position of count.
  size_t count_insert_pos = Offset();
  size_t count = 0;

  // TODO(wujintian): Make this lookup table shared so that it can be used
  // together with SSRElementBinaryWriter.
  static const base::NoDestructor<
      std::unordered_map<std::string, EventTypeEnum>>
      sEventEnumTypeLookupTable(
          {{kEventBindEvent, EventTypeEnum::kBindEvent},
           {kEventCatchEvent, EventTypeEnum::kCatchEvent},
           {kEventCaptureBind, EventTypeEnum::kCaptureBind},
           {kEventCaptureCatch, EventTypeEnum::kCaptureCatch},
           {kEventGlobalBind, EventTypeEnum::kGlobalBind}});

  // Get the count of valid styles when traversal the inline styles.
  for (const auto& event : events) {
    if (!event.IsObject()) {
      continue;
    }
    // type
    auto iter = event.GetObject().FindMember(kType);
    if (iter != event.GetObject().MemberEnd() && iter->value.IsString()) {
      auto type_iter = sEventEnumTypeLookupTable->find(iter->value.GetString());
      if (type_iter == sEventEnumTypeLookupTable->end()) {
        continue;
      }
      WriteU8(static_cast<uint8_t>(type_iter->second));
    }
    ++count;
    encode_specific_key(event, kName);
    encode_specific_key(event, kValue);
  }
  EncodeCountAndInsertAhead(count, count_insert_pos);
}

void CSRElementBinaryWriter::EncodeElementAttributeSection(
    const rapidjson::Value* element) {
  if (!CheckJSONObjectValid(element, kElementAttributes)) {
    return;
  }
  auto attributes = element->GetObject()[kElementAttributes].GetObject();

  WriteU8(static_cast<uint8_t>(tasm::ElementSectionEnum::ELEMENT_ATTRIBUTES));
  WriteCompactU32(attributes.MemberCount());

  for (const auto& [key, value] : attributes) {
    // key
    EncodeUtf8Str(key.GetString(), key.GetStringLength());
    // value
    lepus::Value lepus_val = lepus::jsonValueTolepusValue(value);
    EncodeValue(&lepus_val);
  }
}

void CSRElementBinaryWriter::EncodeElementDatasetSection(
    const rapidjson::Value* element) {
  if (!CheckJSONObjectValid(element, kElementDataset)) {
    return;
  }
  auto dataset = element->GetObject()[kElementDataset].GetObject();

  WriteU8(static_cast<uint8_t>(tasm::ElementSectionEnum::ELEMENT_DATA_SET));
  WriteCompactU32(dataset.MemberCount());

  for (const auto& [key, value] : dataset) {
    // key
    EncodeUtf8Str(key.GetString(), key.GetStringLength());
    // value
    lepus::Value lepus_val = lepus::jsonValueTolepusValue(value);
    EncodeValue(&lepus_val);
  }
}

void CSRElementBinaryWriter::EncodeElementParsedStyleKeySection(
    const rapidjson::Value* element) {
  if (!CheckJSONStringValid(element, kElementParsedStyleKey)) {
    return;
  }

  auto key_iter = element->GetObject().FindMember(kElementParsedStyleKey);

  WriteU8(static_cast<uint8_t>(ElementSectionEnum::ELEMENT_PARSED_STYLES_KEY));
  EncodeUtf8Str(key_iter->value.GetString(), key_iter->value.GetStringLength());
}

// Encode the 'count' and move the encoded result to a specific position
// 'insert_pos'.
void CSRElementBinaryWriter::EncodeCountAndInsertAhead(size_t count,
                                                       size_t insert_pos) {
  size_t start = Offset();
  WriteCompactU32(count);
  size_t end = Offset();
  Move(insert_pos, start, end - start);
}

}  // namespace tasm
}  // namespace lynx
