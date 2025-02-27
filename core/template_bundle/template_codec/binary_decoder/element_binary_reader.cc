// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/template_bundle/template_codec/binary_decoder/element_binary_reader.h"

#include <sys/types.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

#include "base/include/log/logging.h"
#include "base/include/value/base_string.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/fiber/image_element.h"
#include "core/renderer/dom/fiber/list_element.h"
#include "core/renderer/dom/fiber/none_element.h"
#include "core/renderer/dom/fiber/raw_text_element.h"
#include "core/renderer/dom/fiber/scroll_element.h"
#include "core/renderer/dom/fiber/text_element.h"
#include "core/renderer/dom/fiber/view_element.h"
#include "core/renderer/dom/fiber/wrapper_element.h"
#include "core/renderer/template_assembler.h"
#include "core/renderer/utils/base/element_template_info.h"
#include "core/template_bundle/template_codec/template_binary.h"

namespace lynx {
namespace tasm {

namespace {
const base::static_string::GenericCache& GetEventStringType(
    EventTypeEnum type) {
  using EventTypeEnumNameArray =
      std::array<base::static_string::GenericCache,
                 static_cast<size_t>(EventTypeEnum::kMax) + 1>;
  // The reason using std::call_once is that GenericCache is not copy
  // constructible and `NoDestructor<EventTypeEnumNameArray> xx{{}}`
  // won't compile
  static EventTypeEnumNameArray* kEnumNameArray;
  static std::once_flag init_flag;
  std::call_once(init_flag, []() {
    kEnumNameArray = new EventTypeEnumNameArray{
        kEventBindEvent,    kEventCatchEvent, kEventCaptureBind,
        kEventCaptureCatch, kEventGlobalBind, ""};
  });

  if (type < EventTypeEnum::kMax) {
    return (*kEnumNameArray)[static_cast<int>(type)];
  } else {
    return (*kEnumNameArray)[static_cast<int>(
        EventTypeEnum::kMax)];  // Empty string.
  }
};
}  // namespace

// These are the APIs used for decoding data and return fiber elements:

fml::RefPtr<FiberElement> ElementBinaryReader::DecodeSingleTemplate(
    ElementManager* manager, TemplateAssembler* tasm) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "ElementBinaryReader::DecodeSingleTemplate");
  // 1. Decode Element
  fml::RefPtr<FiberElement> element;
  // Because DecodeSingleTemplate always starts decoding from the page node, and
  // the parent component id of the page node is always its own impl id, we pass
  // an unavailable default value of -1 here.
  if (DecodeElementRecursively(element, manager, tasm, -1)) {
    return element;
  }
  return nullptr;
}

bool ElementBinaryReader::DecodeElementRecursively(
    fml::RefPtr<FiberElement>& element, ElementManager* manager,
    TemplateAssembler* tasm, int64_t parent_component_id) {
  // 1. Decode children section offset
  DECODE_U32(children_section_offset);
  children_section_offset += Offset();

  // 2. Decode construction info and tag info then construct element.
  {
    DECODE_U8(section_type_val);
    ElementSectionEnum section_type =
        static_cast<ElementSectionEnum>(section_type_val);

    // 2.1 Try to decode construction info section
    // Construction info section is optional.
    if (section_type == ElementSectionEnum::ELEMENT_CONSTRUCTION_INFO) {
      if (!DecodeConstructionInfoSection()) {
        return false;
      }
      // Decode the next section type, it should be element tag section.
      DECODE_U8(section_type_val);
      section_type = static_cast<ElementSectionEnum>(section_type_val);
    }

    // 2.2 Decode element tag section and construct element.
    if (section_type == ElementSectionEnum::ELEMENT_TAG_ENUM ||
        section_type == ElementSectionEnum::ELEMENT_TAG_STR) {
      if (!ConstructElement(section_type, element, manager, tasm,
                            parent_component_id)) {
        return false;
      }
    } else {
      return false;
    }

    if (element->is_component() || element->is_page()) {
      parent_component_id = element->impl_id();
    }
  }

  // 3. Decode the other section
  bool done = false;
  while (!done) {
    DECODE_U8(section_type);
    switch (static_cast<ElementSectionEnum>(section_type)) {
      case ElementSectionEnum::ELEMENT_BUILTIN_ATTRIBUTE:
        if (!DecodeBuiltinAttributesSection(element)) {
          return false;
        }
        break;

      case ElementSectionEnum::ELEMENT_ID_SELECTOR:
        if (!DecodeIDSelectorSection(element)) {
          return false;
        }
        break;

      case ElementSectionEnum::ELEMENT_STYLES:
        if (!DecodeInlineStylesSection(element)) {
          return false;
        }
        break;

      case ElementSectionEnum::ELEMENT_CLASS:
        if (!DecodeClassesSection(element)) {
          return false;
        }
        break;

      case ElementSectionEnum::ELEMENT_EVENTS:
        if (!DecodeEventsSection(element)) {
          return false;
        }
        break;

      case ElementSectionEnum::ELEMENT_PIPER_EVENTS:
        if (!DecodePiperEventsSection(element)) {
          return false;
        }
        break;

      case ElementSectionEnum::ELEMENT_ATTRIBUTES:
        if (!DecodeAttributesSection(element)) {
          return false;
        }
        break;

      case ElementSectionEnum::ELEMENT_DATA_SET:
        if (!DecodeDatasetSection(element)) {
          return false;
        }
        break;

      case ElementSectionEnum::ELEMENT_PARSED_STYLES_KEY: {
        // For the single template mode, the parsed styles of all elements will
        // not be collected and encoded separately. Instead, the parsed styles
        // of each element will be encoded in place, so the code will never
        // reach this point.
        DECODE_STDSTR(key);
        break;
      }

      case ElementSectionEnum::ELEMENT_PARSED_STYLES:
        if (!DecodeParsedStylesSection(element)) {
          return false;
        }
        break;

      case ElementSectionEnum::ELEMENT_CHILDREN:
        if (!DecodeElementChildrenSection(element, manager, tasm,
                                          parent_component_id)) {
          return false;
        }
        // Children section is the last section, we need to break out this while
        // loop.
        done = true;
        break;

      default:
        // This implies that we've encountered an unrecognizable section.
        // Therefore, we skip these unrecognizable sections and directly seek to
        // the last section: the children section.
        Seek(children_section_offset);
        break;
    }
  }
  EXEC_EXPR_FOR_INSPECTOR(manager->OnElementNodeSetForInspector(element.get()));
  return true;
}

bool ElementBinaryReader::DecodeBuiltinAttributesSection(
    fml::RefPtr<FiberElement>& element) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY,
              "ElementBinaryReader::DecodeBuiltinAttributesSection");
  DECODE_COMPACT_U32(size);
  for (uint32_t i = 0; i < size; ++i) {
    DECODE_COMPACT_U32(key);
    DECODE_VALUE(value);
    element->SetBuiltinAttribute(static_cast<ElementBuiltInAttributeEnum>(key),
                                 std::move(value));
  }
  return true;
}

bool ElementBinaryReader::DecodeIDSelectorSection(
    fml::RefPtr<FiberElement>& element) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY,
              "ElementBinaryReader::DecodeIDSelectorSection");
  DECODE_STR(id_selector);
  element->SetIdSelector(std::move(id_selector));
  return true;
}

bool ElementBinaryReader::DecodeInlineStylesSection(
    fml::RefPtr<FiberElement>& element) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY,
              "ElementBinaryReader::DecodeInlineStylesSection");
  DECODE_COMPACT_U32(size);
  for (uint32_t i = 0; i < size; ++i) {
    DECODE_COMPACT_U32(key);
    DECODE_VALUE(style);
    element->SetStyle(static_cast<CSSPropertyID>(key), std::move(style));
  }
  return true;
}

bool ElementBinaryReader::DecodeClassesSection(
    fml::RefPtr<FiberElement>& element) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "ElementBinaryReader::DecodeClassesSection");
  DECODE_COMPACT_U32(size);
  for (uint32_t i = 0; i < size; ++i) {
    DECODE_STR(cls);
    element->SetClass(std::move(cls));
  }
  return true;
}

bool ElementBinaryReader::DecodeEventsSection(
    fml::RefPtr<FiberElement>& element) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "ElementBinaryReader::DecodeEventsSection");
  DECODE_COMPACT_U32(size);
  for (uint32_t i = 0; i < size; ++i) {
    DECODE_U8(type);
    DECODE_STR(name);
    DECODE_STR(value);
    const auto& event_string =
        GetEventStringType(static_cast<EventTypeEnum>(type));
    if (event_string.str().empty()) {
      return false;
    }
    element->SetJSEventHandler(std::move(name), event_string, std::move(value));
  }
  return true;
}

bool ElementBinaryReader::DecodePiperEventsSection(
    fml::RefPtr<FiberElement>& element) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY,
              "ElementBinaryReader::DecodePiperEventsSection");
  DECODE_COMPACT_U32(size);
  for (uint32_t i = 0; i < size; ++i) {
    DECODE_U8(type);
    DECODE_STR(name);
    DECODE_VALUE(value);

    const auto& event_string =
        GetEventStringType(static_cast<EventTypeEnum>(type));
    if (event_string.str().empty()) {
      return false;
    }

    std::vector<std::pair<base::String, lepus::Value>> piper_event_content;
    tasm::ForEachLepusValue(
        value, [&piper_event_content](const lepus::Value& key,
                                      const lepus::Value& value) {
          auto data = value.Table();
          piper_event_content.push_back(
              {data->GetValue(BASE_STATIC_STRING(
                                  PiperEventContent::kPiperFunctionName))
                   .String(),
               data->GetValue(
                   BASE_STATIC_STRING(PiperEventContent::kPiperFuncArgs))});
        });

    element->data_model()->SetStaticEvent(event_string, std::move(name),
                                          std::move(piper_event_content));
  }

  return true;
}

bool ElementBinaryReader::DecodeAttributesSection(
    fml::RefPtr<FiberElement>& element) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY,
              "ElementBinaryReader::DecodeAttributesSection");
  DECODE_COMPACT_U32(size);
  for (uint32_t i = 0; i < size; ++i) {
    DECODE_STR(key);
    DECODE_VALUE(value);
    element->SetAttribute(std::move(key), std::move(value));
  }
  return true;
}

bool ElementBinaryReader::DecodeDatasetSection(
    fml::RefPtr<FiberElement>& element) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "ElementBinaryReader::DecodeDatasetSection");
  DECODE_COMPACT_U32(size);
  for (uint32_t i = 0; i < size; ++i) {
    DECODE_STR(key);
    DECODE_VALUE(value);
    element->AddDataset(std::move(key), std::move(value));
  }
  return true;
}

bool ElementBinaryReader::DecodeParsedStylesSection(
    fml::RefPtr<FiberElement>& element) {
  StyleMap parsed_styles;
  CSSVariableMap css_var;
  if (!DecodeParsedStylesSectionInternal(parsed_styles, css_var)) {
    return false;
  }
  element->SetParsedStyles(std::move(parsed_styles), std::move(css_var));
  return true;
}

bool ElementBinaryReader::DecodeElementChildrenSection(
    fml::RefPtr<FiberElement>& element, ElementManager* manager,
    TemplateAssembler* tasm, int64_t parent_component_id) {
  DECODE_COMPACT_U32(size);
  for (size_t i = 0; i < size; ++i) {
    fml::RefPtr<FiberElement> child;
    if (!DecodeElementRecursively(child, manager, tasm, parent_component_id)) {
      return false;
    }
    element->InsertNode(child);
  }
  return true;
}

bool ElementBinaryReader::ConstructElement(ElementSectionEnum section_type,
                                           fml::RefPtr<FiberElement>& element,
                                           ElementManager* manager,
                                           TemplateAssembler* tasm,
                                           int64_t parent_component_id) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "ElementBinaryReader::ConstructElement");
  if (section_type == ElementSectionEnum::ELEMENT_TAG_ENUM) {
    DECODE_U8(tag_enum);
    ElementBuiltInTagEnum tag = static_cast<ElementBuiltInTagEnum>(tag_enum);
    if (tag == ELEMENT_EMPTY) {
      NOTREACHED();
    }
    element = manager->CreateFiberElement(tag);
    if (element->is_component() || element->is_page()) {
      element->set_style_sheet_manager(
          tasm->style_sheet_manager(DEFAULT_ENTRY_NAME));
    }
    if (element->is_list()) {
      static_cast<ListElement*>(element.get())->set_tasm(tasm);
    }
    // Since the page element sets its parent component ID to its own impl ID at
    // the time of construction, there is no need to set it again here.
    if (tag != ELEMENT_PAGE) {
      element->SetParentComponentUniqueIdForFiber(parent_component_id);
    }
  } else if (static_cast<ElementSectionEnum>(section_type) ==
             ElementSectionEnum::ELEMENT_TAG_STR) {
    DECODE_STR(str_tag);
    element = manager->CreateFiberNode(str_tag);
    element->SetParentComponentUniqueIdForFiber(parent_component_id);
  }

  EXEC_EXPR_FOR_INSPECTOR(manager->PrepareNodeForInspector(element.get()));
  EXEC_EXPR_FOR_INSPECTOR(
      manager->CheckAndProcessSlotForInspector(element.get()));
  EXEC_EXPR_FOR_INSPECTOR(
      manager->OnElementNodeAddedForInspector(element.get()));
  return true;
}

// These are the APIs used for decoding data and return element infos:

// Lazy decode. Only decode templates router. The
// decoding of the template waits until it is actually needed.
bool ElementBinaryReader::DecodeElementTemplatesRouter() {
  // Decode templates router
  if (!DecodeOrderedStringKeyRouter(element_templates_router_)) {
    return false;
  }
  return true;
}

// Lazy decode. Only decode parsed styles router. The
// decoding of the styles waits until it is actually needed.
bool ElementBinaryReader::DecodeParsedStylesRouter() {
  // Decode parsed styles router
  if (!DecodeStringKeyRouter(string_key_parsed_styles_router_)) {
    return false;
  }
  return true;
}

std::shared_ptr<ElementTemplateInfo>
ElementBinaryReader::DecodeTemplatesInfoWithKey(const std::string& key) {
  std::shared_ptr<ElementTemplateInfo> info =
      std::make_shared<ElementTemplateInfo>();
  info->key_ = key;
  // 1. Get templates offset info and seek to the start position of template
  // info.
  const auto& iter = element_templates_router_.start_offsets_.find(key);
  if (iter == element_templates_router_.start_offsets_.end()) {
    return info;
  }
  uint32_t start = element_templates_router_.descriptor_offset_ + iter->second;
  stream_->Seek(start);

  // 2. Decode templates
  DecodeTemplates(*info);

  return info;
}

bool ElementBinaryReader::DecodeTemplates(ElementTemplateInfo& info) {
  // 1. Decode array size
  DECODE_COMPACT_U32(size);
  info.elements_.reserve(size);
  for (uint32_t index = 0; index < size; ++index) {
    if (!DecodeElementRecursively(info.elements_.emplace_back())) {
      return false;
    }
  }
  info.exist_ = true;
  return true;
}

bool ElementBinaryReader::DecodeElementRecursively(ElementInfo& info) {
  // 1. Decode children section offset
  DECODE_U32(children_section_offset);
  children_section_offset += Offset();

  // 2. Decode all sections
  bool done = false;
  while (!done) {
    DECODE_U8(section_type);
    switch (static_cast<ElementSectionEnum>(section_type)) {
      case ElementSectionEnum::ELEMENT_CONSTRUCTION_INFO:
        if (!DecodeConstructionInfoSection()) {
          return false;
        }
        break;

      case ElementSectionEnum::ELEMENT_TAG_ENUM:
        if (!DecodeEnumTagSection(info)) {
          return false;
        }
        break;

      case ElementSectionEnum::ELEMENT_TAG_STR:
        if (!DecodeStrTagSection(info)) {
          return false;
        }
        break;

      case ElementSectionEnum::ELEMENT_BUILTIN_ATTRIBUTE:
        if (!DecodeBuiltinAttributesSection(info)) {
          return false;
        }
        break;

      case ElementSectionEnum::ELEMENT_ID_SELECTOR:
        if (!DecodeIDSelectorSection(info)) {
          return false;
        }
        break;

      case ElementSectionEnum::ELEMENT_STYLES:
        if (!DecodeInlineStylesSection(info)) {
          return false;
        }
        break;

      case ElementSectionEnum::ELEMENT_CLASS:
        if (!DecodeClassesSection(info)) {
          return false;
        }
        break;

      case ElementSectionEnum::ELEMENT_EVENTS:
        if (!DecodeEventsSection(info)) {
          return false;
        }
        break;

      case ElementSectionEnum::ELEMENT_ATTRIBUTES:
        if (!DecodeAttributesSection(info)) {
          return false;
        }
        break;

      case ElementSectionEnum::ELEMENT_DATA_SET:
        if (!DecodeDatasetSection(info)) {
          return false;
        }
        break;

      case ElementSectionEnum::ELEMENT_PARSED_STYLES_KEY:
        if (!DecodeParsedStyleStringKeySection(info)) {
          return false;
        }
        break;

      case ElementSectionEnum::ELEMENT_PARSED_STYLES:
        if (!DecodeParsedStylesSection(info)) {
          return false;
        }
        break;

      case ElementSectionEnum::ELEMENT_CHILDREN:
        if (!DecodeElementChildrenSection(info)) {
          return false;
        }
        // Children section is the last section, we need to break out this while
        // loop.
        done = true;
        break;

      default:
        // This implies that we've encountered an unrecognizable section.
        // Therefore, we skip these unrecognizable sections and directly seek to
        // the last section: the children section.
        Seek(children_section_offset);
        break;
    }
  }
  return true;
}

bool ElementBinaryReader::DecodeBuiltinAttributesSection(ElementInfo& info) {
  DECODE_COMPACT_U32(size);
  info.builtin_attrs_.reserve(size);
  for (uint32_t i = 0; i < size; ++i) {
    DECODE_COMPACT_U32(key);
    DECODE_VALUE_INTO(
        info.builtin_attrs_[static_cast<ElementBuiltInAttributeEnum>(key)]);
  }
  return true;
}

bool ElementBinaryReader::DecodeIDSelectorSection(ElementInfo& info) {
  DECODE_STR_INTO(info.id_selector_);
  return true;
}

bool ElementBinaryReader::DecodeInlineStylesSection(ElementInfo& info) {
  DECODE_COMPACT_U32(size);
  info.inline_styles_.reserve(size);
  for (uint32_t i = 0; i < size; ++i) {
    DECODE_COMPACT_U32(key);
    DECODE_VALUE(style);
    if (style.IsString()) {
      info.inline_styles_.emplace(static_cast<CSSPropertyID>(key),
                                  style.String());
    }
  }
  return true;
}

bool ElementBinaryReader::DecodeClassesSection(ElementInfo& info) {
  DECODE_COMPACT_U32(size);
  info.class_selector_.reserve(size);
  for (uint32_t i = 0; i < size; ++i) {
    DECODE_STR_INTO(info.class_selector_.emplace_back());
  }
  return true;
}

bool ElementBinaryReader::DecodeEventsSection(ElementInfo& info) {
  DECODE_COMPACT_U32(size);
  info.events_.reserve(size);
  for (uint32_t i = 0; i < size; ++i) {
    DECODE_U8(type);
    ElementEventInfo event_info;
    DECODE_STR_INTO(event_info.name_);
    DECODE_STR_INTO(event_info.value_);
    const auto& event_string =
        GetEventStringType(static_cast<EventTypeEnum>(type));
    if (event_string.str().empty()) {
      return false;
    }
    event_info.type_ = event_string;
    info.events_.emplace_back(std::move(event_info));
  }
  return true;
}

bool ElementBinaryReader::DecodeAttributesSection(ElementInfo& info) {
  DECODE_COMPACT_U32(size);
  info.attrs_.reserve(size);
  for (uint32_t i = 0; i < size; ++i) {
    DECODE_STR(key);
    DECODE_VALUE_INTO(info.attrs_[std::move(key)]);
  }
  return true;
}

bool ElementBinaryReader::DecodeDatasetSection(ElementInfo& info) {
  DECODE_COMPACT_U32(size);
  auto table = lepus::Dictionary::Create();
  for (uint32_t i = 0; i < size; ++i) {
    DECODE_STR(key);
    DECODE_VALUE_INTO(*table->At(std::move(key)));
  }
  info.data_set_ = lepus::Value(table);
  return true;
}

bool ElementBinaryReader::DecodeParsedStylesSection(ElementInfo& info) {
  StyleMap parsed_styles;
  CSSVariableMap css_var;
  if (!DecodeParsedStylesSectionInternal(parsed_styles, css_var)) {
    return false;
  }
  info.parsed_styles_ = std::make_shared<ParsedStyles>(std::move(parsed_styles),
                                                       std::move(css_var));
  return true;
}

bool ElementBinaryReader::DecodeElementChildrenSection(ElementInfo& info) {
  DECODE_COMPACT_U32(size);
  for (size_t i = 0; i < size; ++i) {
    if (!DecodeElementRecursively(info.children_.emplace_back())) {
      return false;
    }
  }
  return true;
}

bool ElementBinaryReader::DecodeParsedStyleStringKeySection(ElementInfo& info) {
  // 1. Decode parsed styles key.
  DECODE_STDSTR(key);
  info.parsed_styles_ = GetParsedStyles(key);
  return true;
}

bool ElementBinaryReader::DecodeEnumTagSection(ElementInfo& info) {
  DECODE_U8(tag_enum);
  info.tag_enum_ = static_cast<ElementBuiltInTagEnum>(tag_enum);
  return true;
}

bool ElementBinaryReader::DecodeStrTagSection(ElementInfo& info) {
  DECODE_STR_INTO(info.tag_);
  return true;
}

// Common methods
bool ElementBinaryReader::DecodeParsedStylesSectionInternal(
    StyleMap& style_map, CSSVariableMap& css_var_map) {
  // 1. Decode parsed styles
  DECODE_COMPACT_U32(parsed_style_size);
  style_map.reserve(parsed_style_size);
  for (uint32_t i = 0; i < parsed_style_size; ++i) {
    DECODE_COMPACT_U32(key);
    DECODE_CSS_VALUE_INTO(style_map[static_cast<CSSPropertyID>(key)]);
  }
  // 2. Decode css variable
  DECODE_COMPACT_U32(css_var_size);
  css_var_map.reserve(css_var_size);
  for (uint32_t i = 0; i < css_var_size; ++i) {
    DECODE_STR(key);
    DECODE_STR_INTO(css_var_map[std::move(key)]);
  }
  return true;
}

bool ElementBinaryReader::DecodeStringKeyRouter(StringKeyRouter& router) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY,
              "ElementBinaryReader::DecodeStringKeyRouter");
  DECODE_COMPACT_U32(size);
  router.start_offsets_.reserve(size);
  for (size_t i = 0; i < size; ++i) {
    DECODE_STDSTR(key);
    DECODE_COMPACT_U32(start);
    router.start_offsets_.emplace(std::move(key), start);
  }
  router.descriptor_offset_ = static_cast<uint32_t>(stream_->offset());

  return true;
}

bool ElementBinaryReader::DecodeOrderedStringKeyRouter(
    OrderedStringKeyRouter& router) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY,
              "ElementBinaryReader::DecodeOrderedStringKeyRouter");
  DECODE_COMPACT_U32(size);
  router.start_offsets_.reserve(size);
  for (size_t i = 0; i < size; ++i) {
    DECODE_STDSTR(key);
    DECODE_COMPACT_U32(start);
    router.start_offsets_.insert_or_assign(std::move(key), start);
  }
  router.descriptor_offset_ = static_cast<uint32_t>(stream_->offset());

  return true;
}

bool ElementBinaryReader::DecodeConstructionInfoSection() {
  DECODE_COMPACT_U32(size);
  for (uint32_t i = 0; i < size; ++i) {
    DECODE_COMPACT_U32(key);
    DECODE_VALUE(value);
    // The construction info section is optional, and the current encoding stage
    // does not encode the construction info section. To ensure that future
    // encoded outputs with a construction info section can be correctly decoded
    // by the current SDK, the current SDK needs to implement the corresponding
    // decoding logic, but it does not need to consume the decoded results.
  }
  return true;
}

const std::shared_ptr<ParsedStyles>& ElementBinaryReader::GetParsedStyles(
    const std::string& key) {
  const auto& cache_iter = parsed_styles_cache_.find(key);
  if (cache_iter != parsed_styles_cache_.end()) {
    return cache_iter->second;
  }

  // Cache miss, try to decode parsed styles.
  std::shared_ptr<ParsedStyles> parsed_styles =
      std::make_shared<ParsedStyles>();
  const auto& iter = string_key_parsed_styles_router_.start_offsets_.find(key);
  if (iter != string_key_parsed_styles_router_.start_offsets_.end()) {
    uint32_t temp_offset = static_cast<uint32_t>(Offset());
    Seek(string_key_parsed_styles_router_.descriptor_offset_ + iter->second);
    DecodeParsedStylesSectionInternal(parsed_styles->first,
                                      parsed_styles->second);
    Seek(temp_offset);
  } else {
    LOGE("Can not find the parsed styles with key: " << key);
  }
  auto res = parsed_styles_cache_.emplace(key, std::move(parsed_styles));
  return res.first->second;
}

}  // namespace tasm
}  // namespace lynx
