// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RENDERER_UTILS_BASE_ELEMENT_TEMPLATE_INFO_H_
#define CORE_RENDERER_UTILS_BASE_ELEMENT_TEMPLATE_INFO_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "core/renderer/css/css_property.h"
#include "core/renderer/dom/element_property.h"
#include "core/renderer/utils/base/tasm_constants.h"
#include "core/runtime/vm/lepus/lepus_value.h"
#include "core/template_bundle/template_codec/template_binary.h"

namespace lynx {
namespace tasm {

// Event Info
struct ElementEventInfo {
  // event type
  base::String type_;
  // event name
  base::String name_;
  // event value
  base::String value_;
};

// Element Info
struct ElementInfo {
  // Make ElementInfo move only
  ElementInfo() = default;
  ElementInfo(const ElementInfo&) = delete;
  ElementInfo& operator=(const ElementInfo&) = delete;
  ElementInfo(ElementInfo&&) = default;
  ElementInfo& operator=(ElementInfo&&) = default;

  bool is_component_{false};

  // If the element is built-in type, the tag_enum_ will not be
  // ElementBuiltInTagEnum::ELEMENT_OTHER.
  ElementBuiltInTagEnum tag_enum_{ElementBuiltInTagEnum::ELEMENT_OTHER};

  // Element's tag selector
  base::String tag_;
  // Element's id selector
  base::String id_selector_;
  // Element's class selector
  std::vector<base::String> class_selector_;
  // Element's inline style
  std::unordered_map<CSSPropertyID, base::String> inline_styles_;
  // Element's builtin attribute
  std::unordered_map<ElementBuiltInAttributeEnum, lepus::Value> builtin_attrs_;
  // Element's attribute
  std::unordered_map<base::String, lepus::Value> attrs_;
  // Element's dataset
  lepus::Value data_set_{};
  // Element's events
  std::vector<ElementEventInfo> events_;

  // Flag used to mark whether there is a parsed style
  bool has_parser_style_{false};
  base::String parser_style_key_;
  std::shared_ptr<ParsedStyles> parsed_styles_{};

  // Element's children info
  std::vector<ElementInfo> children_;

  // config
  lepus::Value config_{};

  // component name
  base::String component_name_;
  // component path
  base::String component_path_;
  // component id
  base::String component_id_;
  // css id
  int32_t css_id_{kInvalidCssId};
};

struct ElementTemplateInfo {
  // Make ElementTemplateInfo move only
  ElementTemplateInfo() = default;
  ElementTemplateInfo(const ElementTemplateInfo&) = delete;
  ElementTemplateInfo& operator=(const ElementTemplateInfo&) = delete;
  ElementTemplateInfo(ElementTemplateInfo&&) = default;
  ElementTemplateInfo& operator=(ElementTemplateInfo&&) = default;

  bool exist_{false};
  std::string key_;
  std::vector<ElementInfo> elements_;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_UTILS_BASE_ELEMENT_TEMPLATE_INFO_H_
