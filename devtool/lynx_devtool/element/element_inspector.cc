// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/element/element_inspector.h"

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

#include "core/base/utils/any.h"
#include "core/inspector/style_sheet.h"
#include "core/renderer/css/css_decoder.h"
#include "core/renderer/css/css_property.h"
#include "core/renderer/dom/fiber/component_element.h"
#include "core/renderer/dom/fiber/fiber_element.h"
#include "core/renderer/dom/selector/fiber_element_selector.h"
#include "core/renderer/dom/vdom/radon/node_select_options.h"
#include "core/renderer/dom/vdom/radon/node_selector.h"
#include "core/renderer/dom/vdom/radon/radon_base.h"
#include "core/renderer/dom/vdom/radon/radon_component.h"
#include "core/renderer/dom/vdom/radon/radon_node.h"
#include "core/runtime/lepus/json_parser.h"
#include "core/services/replay/replay_controller.h"
#include "devtool/lynx_devtool/agent/inspector_util.h"
#include "devtool/lynx_devtool/element/inspector_css_helper.h"

using lynx::base::any;
using lynx::base::String;
using lynx::lepus::Value;
using lynx::tasm::AttributeHolder;
using lynx::tasm::BaseComponent;
using lynx::tasm::CSSProperty;
using lynx::tasm::CSSVariableMap;
using lynx::tasm::RadonBase;
using lynx::tasm::RadonComponent;
using lynx::tasm::RadonNode;
using lynx::tasm::RadonSlot;
using lynx::tasm::StyleMap;

namespace lynx {
namespace devtool {

namespace {

// Compare keyframes name order
// For exmaple:
//@keyframes identifier {
//  0% {
//    top: 0;
//  }
//  30% {
//    top: 50px;
//  100% {
//    top: 100px;
//  }
//}
// from = 0%  to = 100%

bool CompareKeyframesNameOrder(std::string str_lhs, std::string str_rhs) {
  if (str_lhs == "from")
    return true;
  else if (str_rhs == "from")
    return false;
  str_lhs = str_lhs.substr(0, str_lhs.find("%"));
  str_rhs = str_rhs.substr(0, str_rhs.find("%"));
  double num_lhs = std::atof(str_lhs.c_str());
  double num_rhs = std::atof(str_rhs.c_str());
  return num_lhs < num_rhs;
}

std::unordered_map<InspectorElementType, InspectorNodeType>&
GetInspectorElementTypeNodeMap() {
  static lynx::base::NoDestructor<
      std::unordered_map<InspectorElementType, InspectorNodeType>>
      s_inspector_element_type_node_map{
          {{InspectorElementType::STYLEVALUE, InspectorNodeType::kTextNode},
           {InspectorElementType::ELEMENT, InspectorNodeType::ElementNode},
           {InspectorElementType::COMPONENT, InspectorNodeType::ElementNode},
           {InspectorElementType::DOCUMENT, InspectorNodeType::kDocumentNode}}};
  return *s_inspector_element_type_node_map;
}

std::unordered_map<std::string, InspectorElementType>&
GetInspectorTagElementTypeMap() {
  static lynx::base::NoDestructor<
      std::unordered_map<std::string, InspectorElementType>>
      s_inspector_tag_element_type_map{
          {{"doc", InspectorElementType::DOCUMENT},
           {"page", InspectorElementType::COMPONENT},
           {"component", InspectorElementType::COMPONENT},
           {"stylevalue", InspectorElementType::STYLEVALUE}}};
  return *s_inspector_tag_element_type_map;
}

std::string BuildStyleSheetSourceTokenKey(
    const InspectorStyleSheet& style_sheet) {
  return style_sheet.style_name_ + "|" + std::to_string(style_sheet.position_) +
         "|" + std::to_string(style_sheet.style_value_range_.start_line_) +
         "|" + std::to_string(style_sheet.style_value_range_.start_column_);
}

std::unordered_map<std::string, fml::RefPtr<lynx::tasm::CSSParseToken>>*
StyleSheetSourceTokenMap(Element* style_root) {
  if (style_root == nullptr || style_root->inspector_attribute() == nullptr) {
    return nullptr;
  }
  return &style_root->inspector_attribute()->style_sheet_source_token_map_;
}

bool ShouldRecordStyleSheetSourceToken(Element* element) {
  return element != nullptr && element->element_manager() != nullptr &&
         element->element_manager()->EnableNewStylingPipeline();
}

}  // namespace

void ElementInspector::SetDocElement(const any& data) {
  auto tuple = lynx::base::any_cast<std::tuple<Element*, Element*>>(data);
  Element* element = std::get<0>(tuple);
  Element* doc = std::get<1>(tuple);
  CHECK_NULL_AND_LOG_RETURN(element, "element is null");
  auto* inspector_attribute = element->inspector_attribute();
  CHECK_NULL_AND_LOG_RETURN(inspector_attribute, "inspector_attribute is null");
  inspector_attribute->doc_ = std::unique_ptr<Element>(doc);
}

void ElementInspector::SetStyleValueElement(const any& data) {
  auto tuple = lynx::base::any_cast<std::tuple<Element*, Element*>>(data);
  Element* element = std::get<0>(tuple);
  Element* style_value = std::get<1>(tuple);
  CHECK_NULL_AND_LOG_RETURN(element, "element is null");
  auto* inspector_attribute = element->inspector_attribute();
  CHECK_NULL_AND_LOG_RETURN(inspector_attribute, "inspector_attribute is null");
  inspector_attribute->style_value_ = std::unique_ptr<Element>(style_value);
}

bool ElementInspector::HasDataModel(Element* element) {
  CHECK_NULL_AND_LOG_RETURN_VALUE(element, "element is null", false);
  return element->data_model() != nullptr &&
         !(static_cast<lynx::tasm::FiberElement*>(element)->is_wrapper());
}

void ElementInspector::InitForInspector(const any& data) {
  auto tuple = lynx::base::any_cast<std::tuple<Element*>>(data);
  Element* element = std::get<0>(tuple);
  CHECK_NULL_AND_LOG_RETURN(element, "element is null");
  element->set_inspector_attribute(
      std::make_unique<lynx::tasm::InspectorAttribute>());
  InitTypeForInspector(element);
  switch (element->inspector_attribute()->type_) {
    case InspectorElementType::DOCUMENT: {
      InitDocumentElement(element);
      break;
    }
    case InspectorElementType::COMPONENT: {
      InitComponentElement(element);
      break;
    }
    case InspectorElementType::STYLEVALUE: {
      break;
    }
    default: {
      InitNormalElement(element);
      break;
    }
  }
  InitInlineStyleSheetForInspector(element);
  InitIdForInspector(element);
  InitClassForInspector(element);
  InitAttrForInspector(element);
  InitDataSetForInspector(element);
  InitEventMapForInspector(element);

  InitStyleRoot(data);
}

void ElementInspector::InitTypeForInspector(Element* element) {
  CHECK_NULL_AND_LOG_RETURN(element, "element is null");
  auto inspector_attribute = element->inspector_attribute();
  CHECK_NULL_AND_LOG_RETURN(inspector_attribute, "inspector_attribute is null");
  std::string tag = element->GetTag().str();
  if (GetInspectorTagElementTypeMap().find(tag) !=
      GetInspectorTagElementTypeMap().end()) {
    inspector_attribute->type_ = GetInspectorTagElementTypeMap()[tag];
  } else {
    inspector_attribute->type_ = InspectorElementType::ELEMENT;
  }
}

void ElementInspector::InitInlineStyleSheetForInspector(Element* element) {
  if (HasDataModel(element)) {
    std::string name = "inline" + std::to_string(element->impl_id());
    auto* inspector_attribute = element->inspector_attribute();
    CHECK_NULL_AND_LOG_RETURN(inspector_attribute,
                              "inspector_attribute is null");
    inspector_attribute->inline_style_sheet_ = InitStyleSheet(
        element, 0, name, GetInlineStylesFromAttributeHolder(element));
  }
}

void ElementInspector::InitIdForInspector(Element* element) {
  if (HasDataModel(element)) {
    auto* inspector_attribute = element->inspector_attribute();
    CHECK_NULL_AND_LOG_RETURN(inspector_attribute,
                              "inspector_attribute is null");
    inspector_attribute->selector_id_ =
        GetSelectorIDFromAttributeHolder(element);
  }
}

void ElementInspector::InitClassForInspector(Element* element) {
  if (HasDataModel(element)) {
    auto* inspector_attribute = element->inspector_attribute();
    CHECK_NULL_AND_LOG_RETURN(inspector_attribute,
                              "inspector_attribute is null");
    inspector_attribute->class_order_ =
        GetClassOrderFromAttributeHolder(element);
  }
}

void ElementInspector::InitAttrForInspector(Element* element) {
  if (HasDataModel(element)) {
    auto res = GetAttrFromAttributeHolder(element);
    auto* inspector_attribute = element->inspector_attribute();
    CHECK_NULL_AND_LOG_RETURN(inspector_attribute,
                              "inspector_attribute is null");
    inspector_attribute->attr_order_ = res.first;
    inspector_attribute->attr_map_ = res.second;
  }
}

void ElementInspector::InitDataSetForInspector(Element* element) {
  if (HasDataModel(element)) {
    auto res = GetDataSetFromAttributeHolder(element);
    auto* inspector_attribute = element->inspector_attribute();
    CHECK_NULL_AND_LOG_RETURN(inspector_attribute,
                              "inspector_attribute is null");
    inspector_attribute->data_order_ = res.first;
    inspector_attribute->data_map_ = res.second;
  }
}

void ElementInspector::InitEventMapForInspector(Element* element) {
  if (HasDataModel(element)) {
    auto res = GetEventMapFromAttributeHolder(element);
    auto* inspector_attribute = element->inspector_attribute();
    CHECK_NULL_AND_LOG_RETURN(inspector_attribute,
                              "inspector_attribute is null");
    inspector_attribute->event_order_ = res.first;
    inspector_attribute->event_map_ = res.second;
  }
}

void ElementInspector::InitPlugForInspector(const lynx::base::any& data) {
  auto tuple = lynx::base::any_cast<std::tuple<Element*>>(data);
  Element* element = std::get<0>(tuple);
  CHECK_NULL_AND_LOG_RETURN(element, "element is null");
  auto* inspector_attribute = element->inspector_attribute();
  CHECK_NULL_AND_LOG_RETURN(inspector_attribute, "inspector_attribute is null");
  inspector_attribute->slot_name_ = GetVirtualSlotName(element);

  Element* parent_component = element->GetParentComponentElement();
  CHECK_NULL_AND_LOG_RETURN(parent_component, "parent_component is null");
  if (parent_component->GetTag() == "page") {
    inspector_attribute->parent_component_name_ = "page";
  } else {
    inspector_attribute->parent_component_name_ =
        GetComponentName(parent_component);
  }
}

void ElementInspector::InitDocumentElement(Element* element) {
  CHECK_NULL_AND_LOG_RETURN(element, "element is null");
  auto* inspector_attribute = element->inspector_attribute();
  CHECK_NULL_AND_LOG_RETURN(inspector_attribute, "inspector_attribute is null");
  inspector_attribute->local_name_ = "";
  inspector_attribute->node_name_ = "#document";
  inspector_attribute->node_type_ = static_cast<int>(
      GetInspectorElementTypeNodeMap()[inspector_attribute->type_]);
  inspector_attribute->node_value_ = "";
}

void ElementInspector::InitComponentElement(Element* element) {
  CHECK_NULL_AND_LOG_RETURN(element, "element is null");
  auto* inspector_attribute = element->inspector_attribute();
  CHECK_NULL_AND_LOG_RETURN(inspector_attribute, "inspector_attribute is null");

  std::string local_name;
  if (element->GetTag() == "page") {
    local_name = "page";
  } else {
    local_name = GetComponentName(element);
  }

  inspector_attribute->local_name_ = local_name;
  std::transform(local_name.begin(), local_name.end(), local_name.begin(),
                 ::toupper);
  inspector_attribute->node_name_ = local_name;
  inspector_attribute->node_type_ = static_cast<int>(
      GetInspectorElementTypeNodeMap()[inspector_attribute->type_]);
  inspector_attribute->node_value_ = "";
}

void ElementInspector::InitStyleValueElement(const any& data) {
  auto tuple = lynx::base::any_cast<std::tuple<Element*, Element*>>(data);
  Element* element = std::get<0>(tuple);
  CHECK_NULL_AND_LOG_RETURN(element, "element is null");
  auto* inspector_attribute = element->inspector_attribute();
  CHECK_NULL_AND_LOG_RETURN(inspector_attribute, "inspector_attribute is null");

  inspector_attribute->local_name_ = "";
  inspector_attribute->node_name_ = "STYLEVALUE";
  inspector_attribute->node_type_ = static_cast<int>(
      GetInspectorElementTypeNodeMap()[inspector_attribute->type_]);
  inspector_attribute->node_value_ = "";
  inspector_attribute->start_line_ = 1;
  inspector_attribute->node_value_ += "\n";
}

void ElementInspector::InitNormalElement(Element* element) {
  CHECK_NULL_AND_LOG_RETURN(element, "element is null");
  auto* inspector_attribute = element->inspector_attribute();
  CHECK_NULL_AND_LOG_RETURN(inspector_attribute, "inspector_attribute is null");

  std::string local_name = element->GetTag().str();
  inspector_attribute->local_name_ = local_name;
  std::transform(local_name.begin(), local_name.end(), local_name.begin(),
                 ::toupper);
  inspector_attribute->node_name_ = local_name;
  inspector_attribute->node_type_ = static_cast<int>(
      GetInspectorElementTypeNodeMap()[inspector_attribute->type_]);
  inspector_attribute->node_value_ = "";
}

lynx::devtool::InspectorStyleSheet ElementInspector::InitStyleSheet(
    Element* element, int start_line, std::string name,
    std::unordered_map<std::string, std::string> styles, uint64_t position) {
  InspectorStyleSheet res;
  res.empty = false;
  res.style_name_ = name;
  res.origin_ = "regular";
  res.style_sheet_id_ = element ? std::to_string(element->impl_id()) : "";
  res.style_name_range_.start_line_ = start_line;
  res.style_name_range_.end_line_ = start_line;
  res.style_name_range_.start_column_ = 0;
  res.position_ = position;

  int property_start_column;
  if (lynx::base::BeginsWith(name, "inline")) {
    res.style_name_range_.end_column_ = 0;
    property_start_column = 0;
  } else {
    res.style_name_range_.end_column_ =
        static_cast<int>(res.style_name_.size());
    property_start_column = res.style_name_range_.end_column_ + 1;
  }
  res.style_value_range_.start_line_ = start_line;
  res.style_value_range_.end_line_ = start_line;
  res.style_value_range_.start_column_ = property_start_column;

  std::unordered_multimap<std::string, lynx::devtool::CSSPropertyDetail>
      temp_map;
  lynx::devtool::CSSPropertyDetail temp_css_property;
  std::string css_text;
  for (const auto& style : styles) {
    temp_css_property.name_ = style.first;
    temp_css_property.value_ = style.second;
    temp_css_property.text_ = style.first + ":" + style.second + ";";
    css_text += temp_css_property.text_;
    temp_css_property.disabled_ = false;
    temp_css_property.implicit_ = false;
    temp_css_property.parsed_ok_ = true;
    temp_css_property.property_range_.start_line_ = start_line;
    temp_css_property.property_range_.end_line_ = start_line;
    temp_css_property.property_range_.start_column_ = property_start_column;
    temp_css_property.property_range_.end_column_ =
        property_start_column +
        static_cast<int>(temp_css_property.text_.size());
    property_start_column = temp_css_property.property_range_.end_column_;
    temp_map.insert(std::make_pair(style.first, temp_css_property));
    res.property_order_.push_back(style.first);
  }

  res.css_text_ = css_text;
  res.style_value_range_.end_column_ = property_start_column;
  res.css_properties_ = temp_map;
  return res;
}

void ElementInspector::RecordStyleSheetSourceToken(
    Element* style_root, const InspectorStyleSheet& style_sheet,
    fml::RefPtr<lynx::tasm::CSSParseToken> token) {
  if (style_root == nullptr || token == nullptr || style_sheet.empty) {
    return;
  }
  auto* map = StyleSheetSourceTokenMap(style_root);
  if (map == nullptr) {
    return;
  }
  (*map)[BuildStyleSheetSourceTokenKey(style_sheet)] = std::move(token);
}

fml::RefPtr<lynx::tasm::CSSParseToken>
ElementInspector::GetStyleSheetSourceToken(
    Element* style_root, const InspectorStyleSheet& style_sheet) {
  if (style_root == nullptr || style_sheet.empty) {
    return nullptr;
  }
  auto* map = StyleSheetSourceTokenMap(style_root);
  if (map == nullptr) {
    return nullptr;
  }
  auto iter = map->find(BuildStyleSheetSourceTokenKey(style_sheet));
  if (iter == map->end()) {
    return nullptr;
  }
  return iter->second;
}

void ElementInspector::EraseStyleSheetSourceToken(
    Element* style_root, const InspectorStyleSheet& style_sheet) {
  if (style_root == nullptr || style_sheet.empty) {
    return;
  }
  auto* map = StyleSheetSourceTokenMap(style_root);
  if (map == nullptr) {
    return;
  }
  map->erase(BuildStyleSheetSourceTokenKey(style_sheet));
}

Element* ElementInspector::GetParentComponentElementFromDataModel(
    Element* element) {
  CHECK_NULL_AND_LOG_RETURN_VALUE(element, "element is null", nullptr);
  // Get Element's parent, if the parent is component/page, return the parent.
  // Otherwise, return nullptr.
  auto parent = element->parent();
  if (parent &&
      static_cast<lynx::tasm::FiberElement*>(parent)->is_component()) {
    return parent;
  }
  return nullptr;
}

Element* ElementInspector::GetChildElementForComponentRemoveView(
    Element* element) {
  CHECK_NULL_AND_LOG_RETURN_VALUE(element, "element is null", nullptr);
  return nullptr;
}

void ElementInspector::Flush(Element* element) {
  if (HasDataModel(element)) {
    auto* inspector_attribute = element->inspector_attribute();
    CHECK_NULL_AND_LOG_RETURN(inspector_attribute,
                              "inspector_attribute is null");

    for (const auto& name : inspector_attribute->attr_order_) {
      element->SetAttribute(name,
                            Value(inspector_attribute->attr_map_.at(name)));
    }

    auto reset_names = GetOrderedInspectorResetStyleNames();
    element->ResetStyle(reset_names);
    auto* element_manager = element->element_manager();
    if (element->GetTag() == "page" && element_manager) {
      element_manager->SetRootOnLayout(element->impl_id());
    }
    auto options = std::make_shared<tasm::PipelineOptions>();
    if (element_manager) {
      element_manager->OnFinishUpdateProps(element, options);
    }

    if (ElementInspector::IsEnableCSSSelector(element)) {
      const std::vector<InspectorStyleSheet>& match_rules =
          GetMatchedStyleSheet(element);
      for (const auto& style : match_rules) {
        SetPropsAccordingToStyleSheet(element, style);
      }
    } else {
      SetPropsAccordingToStyleSheet(element, GetStyleSheetByName(element, "*"));
      SetPropsAccordingToStyleSheet(
          element, GetStyleSheetByName(element, SelectorTag(element)));
      for (const auto& name : inspector_attribute->class_order_) {
        SetPropsAccordingToStyleSheet(element,
                                      GetStyleSheetByName(element, name));
        SetPropsForCascadedStyleSheet(element, name);
      }
      if (!SelectorId(element).empty()) {
        SetPropsAccordingToStyleSheet(
            element, GetStyleSheetByName(element, SelectorId(element)));
        SetPropsForCascadedStyleSheet(element, SelectorId(element));
      }
    }

    SetPropsAccordingToStyleSheet(element, GetInlineStyleSheet(element));

    // Need to call OnPatchFinish() since some css styles will be updated there
    // e.g. margin calculation may rely on font-size configuration
    if (element_manager) {
      element_manager->OnFinishUpdateProps(element, options);
      element_manager->OnPatchFinish(options);
    }
  }
}

void ElementInspector::InitStyleRoot(const any& data) {
  auto tuple = lynx::base::any_cast<std::tuple<Element*>>(data);
  Element* element = std::get<0>(tuple);
  InitStyleRootWithElement(element);
}

void ElementInspector::InitStyleRootWithElement(Element* element) {
  CHECK_NULL_AND_LOG_RETURN(element, "element is null");
  if (element->GetTag() == "page") {
    return;
  }
  auto* inspector_attribute = element->inspector_attribute();
  CHECK_NULL_AND_LOG_RETURN(inspector_attribute, "inspector_attribute is null");
  if (HasDataModel(element)) {
    auto comp = GetCSSStyleComponentElement(element);
    if (comp != nullptr && Type(comp) == InspectorElementType::COMPONENT) {
      inspector_attribute->style_root_ = StyleValueElement(comp);
    }
  }
}

void ElementInspector::SetStyleRoot(const any& data) {
  auto tuple = lynx::base::any_cast<std::tuple<Element*, Element*>>(data);
  Element* element = std::get<0>(tuple);
  Element* style_root = std::get<1>(tuple);
  CHECK_NULL_AND_LOG_RETURN(element, "element is null");
  auto* inspector_attribute = element->inspector_attribute();
  CHECK_NULL_AND_LOG_RETURN(inspector_attribute, "inspector_attribute is null");
  inspector_attribute->style_root_ = style_root;
}

std::unordered_map<std::string, std::string> ElementInspector::GetCssByStyleMap(
    Element* element, const StyleMap& style_map) {
  std::unordered_map<std::string, std::string> res;
  for (const auto& pair : style_map) {
    const auto& name = lynx::tasm::CSSProperty::GetPropertyName(pair.first);

    if (pair.second.GetValueType() == lynx::tasm::CSSValueType::VARIABLE) {
      String property;
      if (element != nullptr && element->element_manager() != nullptr &&
          element->element_manager()->EnableNewStylingPipeline()) {
        // New styling pipeline: resolved custom properties live in
        // ComputedCSSStyle, not AttributeHolder.
        const lynx::tasm::CustomPropertiesMap* custom_properties = nullptr;
        if (element->base_css_style() != nullptr) {
          custom_properties = element->base_css_style()->GetCustomProperties();
        } else if (element->computed_css_style() != nullptr) {
          custom_properties =
              element->computed_css_style()->GetCustomProperties();
        }
        if (custom_properties != nullptr) {
          property = lynx::tasm::CSSValue::SubstitutionResolved(
              pair.second, *custom_properties);
        } else {
          property = pair.second.GetDefaultValue();
        }
      } else {
        // Legacy pipeline: use AttributeHolder's css_variables_.
        Value value_expr = pair.second.GetValue();
        property = pair.second.GetDefaultValue();
        auto default_value_map = pair.second.GetDefaultValueMapOpt();
        if (element && value_expr.IsString()) {
          lynx::tasm::CSSVariableHandler handler;
          property = handler.GetCSSVariableByRule(value_expr.StdString(),
                                                  element->data_model(),
                                                  property, default_value_map);
        }
      }
      lynx::tasm::CSSValue modified_value(Value(std::move(property)),
                                          pair.second.GetPattern());
      res[name.str()] =
          lynx::tasm::CSSDecoder::CSSValueToString(pair.first, modified_value);
    } else {
      res[name.str()] =
          lynx::tasm::CSSDecoder::CSSValueToString(pair.first, pair.second);
    }
  }
  return res;
}

std::unordered_map<std::string, std::string>
ElementInspector::GetCssVariableByMap(const CSSVariableMap& style_variables) {
  std::unordered_map<std::string, std::string> res;
  for (const auto& pair : style_variables) {
    res[pair.first.str()] = pair.second.str();
  }
  return res;
}

std::unordered_map<std::string, std::string> ElementInspector::GetCSSByName(
    Element* element, std::string name) {
  std::unordered_map<std::string, std::string> res;
  CHECK_NULL_AND_LOG_RETURN_VALUE(element, "element is null", res);
  lynx::tasm::CSSFragment* style_sheet = element->GetRelatedCSSFragment();
  CHECK_NULL_AND_LOG_RETURN_VALUE(style_sheet, "style_sheet is null", res);
  auto* css = style_sheet->GetCSSStyle(name);
  return GetCSSByParseToken(element, css);
}

std::unordered_map<std::string, std::string>
ElementInspector::GetCSSByParseToken(Element* element,
                                     lynx::tasm::CSSParseToken* token) {
  std::unordered_map<std::string, std::string> res;
  CHECK_NULL_AND_LOG_RETURN_VALUE(token, "token is null", res);
  const StyleMap& style_map = token->GetAttributes();
  res = GetCssByStyleMap(element, style_map);
  const CSSVariableMap& css_variable_map = token->GetStyleVariables();
  std::unordered_map<std::string, std::string> css_variable =
      GetCssVariableByMap(css_variable_map);
  res.insert(css_variable.begin(), css_variable.end());
  return res;
}

std::vector<lynx::devtool::InspectorStyleSheet>
ElementInspector::GetMatchedStyleSheet(Element* element) {
  std::vector<lynx::devtool::InspectorStyleSheet> res;
  CHECK_NULL_AND_LOG_RETURN_VALUE(element, "element is null", res);
  auto* attribute_holder = element->data_model();
  CHECK_NULL_AND_LOG_RETURN_VALUE(attribute_holder, "attribute_holder is null",
                                  res);

  lynx::tasm::CSSFragment* style_sheet = element->GetRelatedCSSFragment();
  CHECK_NULL_AND_LOG_RETURN_VALUE(style_sheet, "style_sheet is null", res);

  auto* inspector_attribute = element->inspector_attribute();
  CHECK_NULL_AND_LOG_RETURN_VALUE(inspector_attribute,
                                  "inspector_attribute is null", res);

  auto* style_root = inspector_attribute->style_root_;
  auto matched_rules = lynx::tasm::StyleResolver::GetCSSMatchedRule(
      attribute_holder, style_sheet, nullptr);
  for (const auto& matched : matched_rules) {
    auto matched_token = matched.Data()->Rule()->Token();
    if (matched_token != nullptr) {
      std::string name = matched.Data()->Selector().ToString();
      if (style_root != nullptr) {
        auto map = GetStyleSheetMap(style_root);
        auto range = map.equal_range(name);
        auto iter = range.first;
        for (; iter != range.second; ++iter) {
          auto field = iter->second;
          if (matched.Position() == field.position_) {
            if (ShouldRecordStyleSheetSourceToken(element)) {
              auto recorded_token = GetStyleSheetSourceToken(style_root, field);
              if (recorded_token != nullptr &&
                  recorded_token.get() != matched_token.get()) {
                continue;
              }
              RecordStyleSheetSourceToken(style_root, field, matched_token);
            }
            res.push_back(field);
            break;
          }
        }
        if (iter == range.second) {
          std::unordered_map<std::string, std::string> css =
              GetCSSByParseToken(element, matched_token.get());
          auto* inspector_attribute = style_root->inspector_attribute();
          if (inspector_attribute && !css.empty()) {
            lynx::devtool::InspectorStyleSheet style_sheet =
                InitStyleSheet(style_root, inspector_attribute->start_line_++,
                               name, css, matched.Position());
            if (ShouldRecordStyleSheetSourceToken(element)) {
              RecordStyleSheetSourceToken(style_root, style_sheet,
                                          matched_token);
            }
            res.push_back(style_sheet);
            inspector_attribute->style_sheet_map_.insert({name, style_sheet});
            inspector_attribute->node_value_ =
                inspector_attribute->node_value_ + name + "{" +
                style_sheet.css_text_ + "}\n";
          }
        }
      }
    }
  }
  return res;
}

lynx::devtool::LynxDoubleMapString ElementInspector::GetAnimationByName(
    Element* element, std::string name) {
  LynxDoubleMapString res;
  CHECK_NULL_AND_LOG_RETURN_VALUE(element, "element is null", res);

  lynx::tasm::CSSFragment* style_sheet = element->GetRelatedCSSFragment();
  CHECK_NULL_AND_LOG_RETURN_VALUE(style_sheet, "style_sheet is null", res);

  const auto& animation_map = style_sheet->GetKeyframesRuleMap();
  auto animation_item = animation_map.find(name);
  if (animation_item == animation_map.end()) return res;
  auto animation = animation_item->second;
  for (const auto& style : animation->GetKeyframesContent()) {
    std::unordered_map<std::string, std::string> keyframe;
    for (const auto& pair : *(style.second)) {
      const auto& name = lynx::tasm::CSSProperty::GetPropertyName(pair.first);
      keyframe[name.str()] =
          lynx::tasm::CSSDecoder::CSSValueToString(pair.first, pair.second);
    }
    res[std::to_string(style.first)] = std::move(keyframe);
  }
  return res;
}

lynx::devtool::InspectorStyleSheet ElementInspector::GetStyleSheetByName(
    Element* element, const std::string& name) {
  InspectorStyleSheet res;
  CHECK_NULL_AND_LOG_RETURN_VALUE(element, "element is null", res);
  auto* inspector_attribute = element->inspector_attribute();
  CHECK_NULL_AND_LOG_RETURN_VALUE(inspector_attribute,
                                  "inspector_attribute is null", res);
  auto* style_root = inspector_attribute->style_root_;
  CHECK_NULL_AND_LOG_RETURN_VALUE(style_root, "style_root is null", res);
  CHECK_NULL_AND_LOG_RETURN_VALUE(style_root->inspector_attribute(),
                                  "style_root's inspector_attribute is null",
                                  res);
  auto map = GetStyleSheetMap(style_root);
  if (map.find(name) != map.end()) {
    auto* style_sheet = ShouldRecordStyleSheetSourceToken(element)
                            ? element->GetRelatedCSSFragment()
                            : nullptr;
    fml::RefPtr<lynx::tasm::CSSParseToken> token = nullptr;
    if (style_sheet != nullptr) {
      token = style_sheet->GetSharedCSSStyle(name);
    }
    auto range = map.equal_range(name);
    if (token != nullptr) {
      for (auto iter = range.first; iter != range.second; ++iter) {
        auto source_token = GetStyleSheetSourceToken(style_root, iter->second);
        if (source_token != nullptr && source_token.get() == token.get()) {
          return iter->second;
        }
      }
    }
    res = map.find(name)->second;
    if (token != nullptr && map.count(name) == 1) {
      RecordStyleSheetSourceToken(style_root, res, token);
    }
  } else {
    std::unordered_map<std::string, std::string> css =
        GetCSSByName(element, name);
    auto* inspector_attribute = style_root->inspector_attribute();
    if (inspector_attribute && !css.empty()) {
      res = InitStyleSheet(style_root, inspector_attribute->start_line_++, name,
                           css);
      if (ShouldRecordStyleSheetSourceToken(element)) {
        auto* style_sheet = element->GetRelatedCSSFragment();
        if (style_sheet != nullptr) {
          auto token = style_sheet->GetSharedCSSStyle(name);
          if (token != nullptr) {
            RecordStyleSheetSourceToken(style_root, res, token);
          }
        }
      }
      inspector_attribute->style_sheet_map_.insert({name, res});
      inspector_attribute->node_value_ =
          inspector_attribute->node_value_ + name + "{" + res.css_text_ + "}\n";
    }
  }
  return res;
}

std::vector<lynx::devtool::InspectorKeyframe>
ElementInspector::GetAnimationKeyframeByName(Element* element,
                                             const std::string& name) {
  std::vector<lynx::devtool::InspectorKeyframe> res;
  CHECK_NULL_AND_LOG_RETURN_VALUE(element, "element is null", res);
  auto* inspector_attribute = element->inspector_attribute();
  CHECK_NULL_AND_LOG_RETURN_VALUE(inspector_attribute,
                                  "inspector_attribute is null", res);
  auto* style_root = inspector_attribute->style_root_;
  CHECK_NULL_AND_LOG_RETURN_VALUE(style_root, "style_root is null", res);
  auto animation_map = GetAnimationMap(style_root);
  if (animation_map.find(name) != animation_map.end()) {
    res = animation_map.at(name);
  } else {
    auto* inspector_attribute = style_root->inspector_attribute();
    CHECK_NULL_AND_LOG_RETURN_VALUE(inspector_attribute,
                                    "inspector_attribute is null", res);
    LynxDoubleMapString animation = GetAnimationByName(element, name);
    if (!animation.empty()) {
      std::vector<std::string> keyframe_names;
      for (auto pair : animation) {
        keyframe_names.push_back(pair.first);
      }
      std::sort(keyframe_names.begin(), keyframe_names.end(),
                CompareKeyframesNameOrder);
      if (inspector_attribute) {
        inspector_attribute->node_value_ += "@keyframes " + name + "{\n";
        inspector_attribute->start_line_++;
      }
      for (const auto& keyframe_name : keyframe_names) {
        InspectorKeyframe frame;
        frame.key_text_ = keyframe_name;
        frame.style_ =
            InitStyleSheet(style_root, inspector_attribute->start_line_++,
                           keyframe_name, animation[keyframe_name]);
        if (inspector_attribute->animation_map_.find(name) ==
            inspector_attribute->animation_map_.end()) {
          std::vector<InspectorKeyframe> frame_vec;
          frame_vec.push_back(frame);
          inspector_attribute->animation_map_[name] = frame_vec;
        } else
          inspector_attribute->animation_map_[name].push_back(frame);
        inspector_attribute->node_value_ +=
            frame.key_text_ + "{" + frame.style_.css_text_ + "}\n";
      }
      inspector_attribute->node_value_ += "}\n";
    }
    res = inspector_attribute->animation_map_[name];
  }
  return res;
}

std::string ElementInspector::GetVirtualSlotName(Element* slot_plug) {
  CHECK_NULL_AND_LOG_RETURN_VALUE(slot_plug, "slot_plug is null", "");
  auto* attribute_holder = slot_plug->data_model();
  CHECK_NULL_AND_LOG_RETURN_VALUE(attribute_holder, "attribute_holder is null",
                                  "");

  constexpr const static char kSlot[] = "slot";
  constexpr const static char kDefaultName[] = "default";

  // In fiber mode, lepus runtime will set slot name as a attribute to the
  // elment, which's key is "slot". Then we can get slot name from the pulg
  // element's attributes. If the attributes does not contain "slot", then
  // return kDefaultName.
  const auto& attr_map = attribute_holder->attributes();
  auto iter = attr_map.find(BASE_STATIC_STRING(kSlot));
  if (iter != attr_map.end()) {
    return iter->second.StdString();
  }
  return kDefaultName;
}

std::string ElementInspector::GetComponentName(Element* element) {
  CHECK_NULL_AND_LOG_RETURN_VALUE(element, "element is null", "");
  return static_cast<lynx::tasm::ComponentElement*>(element)
      ->component_name()
      .str();
}

Element* ElementInspector::GetElementByID(Element* element, int id) {
  CHECK_NULL_AND_LOG_RETURN_VALUE(element, "element is null", nullptr);
  auto* element_manager = element->element_manager();
  CHECK_NULL_AND_LOG_RETURN_VALUE(element_manager, "element_manager is null",
                                  nullptr);
  auto* node_manager = element_manager->node_manager();
  CHECK_NULL_AND_LOG_RETURN_VALUE(node_manager, "node_manager is null",
                                  nullptr);
  return node_manager->Get(id);
}

Element* ElementInspector::GetCSSStyleComponentElement(Element* element) {
  CHECK_NULL_AND_LOG_RETURN_VALUE(element, "element is null", nullptr);
  return element->GetParentComponentElement();
}

std::string ElementInspector::GetComponentProperties(Element* element) {
  std::string res = "";
  CHECK_NULL_AND_LOG_RETURN_VALUE(element, "element is null", res);
  // TODO(wujintian): Get the properties of fiber component from lepus
  // context.
  return res;
}

std::string ElementInspector::GetComponentData(Element* element) {
  std::string res = "";
  CHECK_NULL_AND_LOG_RETURN_VALUE(element, "element is null", res);
  // TODO(wujintian): Get the properties of fiber component from lepus
  // context.
  return res;
}

std::string ElementInspector::GetComponentId(Element* element) {
  std::string res = "-1";
  CHECK_NULL_AND_LOG_RETURN_VALUE(element, "element is null", res);
  return static_cast<lynx::tasm::ComponentElement*>(element)
      ->component_id()
      .str();
}

std::unordered_map<std::string, std::string>
ElementInspector::GetInlineStylesFromAttributeHolder(Element* element) {
  std::unordered_map<std::string, std::string> res;
  CHECK_NULL_AND_LOG_RETURN_VALUE(element, "element is null", res);
  auto* node = element->data_model();
  CHECK_NULL_AND_LOG_RETURN_VALUE(node, "node is null", res);
  const StyleMap& inline_style = node->inline_styles();
  res = GetCssByStyleMap(element, inline_style);

  auto insert_css_variables = [&res](const CSSVariableMap& var_map) {
    for (const auto& pair : var_map) {
      res[pair.first.str()] = pair.second.str();
    }
  };

  insert_css_variables(node->css_variables_map());
  insert_css_variables(node->css_variable_from_js());
  insert_css_variables(node->css_variable_related());

  return res;
}

std::string ElementInspector::GetSelectorIDFromAttributeHolder(
    Element* element) {
  CHECK_NULL_AND_LOG_RETURN_VALUE(element, "element is null", "");
  auto* node = element->data_model();
  CHECK_NULL_AND_LOG_RETURN_VALUE(node, "node is null", "");
  if (node->idSelector().empty()) {
    return "";
  } else {
    return "#" + node->idSelector().str();
  }
}

std::vector<std::string> ElementInspector::GetClassOrderFromAttributeHolder(
    Element* element) {
  std::vector<std::string> res;
  CHECK_NULL_AND_LOG_RETURN_VALUE(element, "element is null", res);
  auto* node = element->data_model();
  CHECK_NULL_AND_LOG_RETURN_VALUE(node, "node is null", res);
  const auto& classes = node->classes();
  for (const auto& c : classes) {
    res.push_back("." + c.str());
  }
  return res;
}

lynx::devtool::LynxAttributePair ElementInspector::GetAttrFromAttributeHolder(
    Element* element) {
  CHECK_NULL_AND_LOG_RETURN_VALUE(element, "element is null", {});
  auto* node = element->data_model();
  CHECK_NULL_AND_LOG_RETURN_VALUE(node, "node is null", {});
  const auto& attr = node->attributes();
  std::vector<std::string> order;
  std::unordered_map<std::string, std::string> map;
  for (const auto& pair : attr) {
    const auto& name = pair.first.str();
    std::string value;
    if (pair.second.IsNumber()) {
      std::ostringstream stm;
      stm << pair.second.Number();
      value = stm.str();
    } else {
      value = pair.second.StdString();
    }
    order.push_back(name);
    map[name] = value;
  }
  return std::make_pair(order, map);
}

lynx::devtool::LynxAttributePair
ElementInspector::GetDataSetFromAttributeHolder(Element* element) {
  CHECK_NULL_AND_LOG_RETURN_VALUE(element, "element is null", {});
  auto* node = element->data_model();
  CHECK_NULL_AND_LOG_RETURN_VALUE(node, "node is null", {});
  const auto& data = node->dataset();
  std::vector<std::string> order;
  std::unordered_map<std::string, std::string> map;
  for (const auto& pair : data) {
    constexpr const static char* kPrefix = "data-";
    const auto name = kPrefix + pair.first.str();
    std::string value = std::string();
    if (pair.second.IsNumber()) {
      std::ostringstream stm;
      stm << pair.second.Number();
      value += stm.str();
    } else {
      value += pair.second.StdString();
    }
    order.push_back(name);
    map[name] = value;
  }
  return std::make_pair(order, map);
}

lynx::devtool::LynxAttributePair
ElementInspector::GetEventMapFromAttributeHolder(Element* element) {
  CHECK_NULL_AND_LOG_RETURN_VALUE(element, "element is null", {});
  auto* node = element->data_model();
  CHECK_NULL_AND_LOG_RETURN_VALUE(node, "node is null", {});
  std::vector<std::string> order;
  std::unordered_map<std::string, std::string> map;
  const auto& event = node->static_events();
  const auto& global_event = node->global_bind_events();
  for (const auto& pair : event) {
    auto name = pair.first.str();
    if (pair.second->type().str() == "bindEvent") {
      name = "bind" + name;
    } else if (pair.second->type().str() == "catchEvent") {
      name = "catch" + name;
    } else if (pair.second->type().str() == "capture-bindEvent") {
      name = "capture-bind" + name;
    } else if (pair.second->type().str() == "capture-catchEvent") {
      name = "capture-catch" + name;
    } else {
      name = pair.second->type().str() + name;
    }
    const auto value = pair.second->function().str() +
                       pair.second->lepus_function().ToString();
    order.push_back(name);
    map[name] = value;
  }
  for (const auto& pair : global_event) {
    auto name = "global-bind" + pair.first.str();
    const auto value = pair.second->function().str() +
                       pair.second->lepus_function().ToString();
    order.push_back(name);
    map[name] = value;
  }
  return std::make_pair(order, map);
}

void ElementInspector::SetPropsAccordingToStyleSheet(
    Element* element, const lynx::devtool::InspectorStyleSheet& style_sheet) {
  CHECK_NULL_AND_LOG_RETURN(element, "element is null");
  auto* element_manager = element->element_manager();
  CHECK_NULL_AND_LOG_RETURN(element_manager, "element_manager is null");
  StyleMap styles;
  auto configs = element_manager->GetCSSParserConfigs();
  styles.reserve(style_sheet.css_properties_.size());
  for (const auto& pair : style_sheet.css_properties_) {
    if (pair.second.parsed_ok_ && !pair.second.disabled_) {
      auto id = CSSProperty::GetPropertyID(pair.second.name_);
      lynx::tasm::UnitHandler::Process(id, Value(pair.second.value_), styles,
                                       configs);
    }
  }
  element->ConsumeStyle(styles);
}

void ElementInspector::SetPropsForCascadedStyleSheet(Element* element,
                                                     const std::string& rule) {
  if (IsStyleRootHasCascadeStyle(element)) {
    Element* parent = element->parent();
    while (parent) {
      for (const auto& parent_name : ClassOrder(parent)) {
        auto style_sheet = GetStyleSheetByName(element, rule + parent_name);
        if (!style_sheet.empty) {
          SetPropsAccordingToStyleSheet(element, style_sheet);
        }
      }
      parent = parent->parent();
    }

    parent = element->parent();
    while (parent) {
      if (!SelectorId(parent).empty()) {
        auto style_sheet =
            GetStyleSheetByName(element, rule + SelectorId(parent));
        if (!style_sheet.empty) {
          SetPropsAccordingToStyleSheet(element, style_sheet);
        }
      }
      parent = parent->parent();
    }
  }
}

void ElementInspector::AdjustStyleSheet(Element* element) {
  CHECK_NULL_AND_LOG_RETURN(element, "element is null");
  auto* inspector_attribute = element->inspector_attribute();
  CHECK_NULL_AND_LOG_RETURN(inspector_attribute, "inspector_attribute is null");
  int start_line =
      inspector_attribute->inline_style_sheet_.style_name_range_.start_line_;
  int property_start_column = 0;

  inspector_attribute->inline_style_sheet_.style_value_range_.start_line_ =
      start_line;
  inspector_attribute->inline_style_sheet_.style_value_range_.end_line_ =
      start_line;
  inspector_attribute->inline_style_sheet_.style_value_range_.start_column_ =
      property_start_column;

  auto& map = inspector_attribute->inline_style_sheet_.css_properties_;
  std::string css_text;
  for (auto& item : map) item.second.looped_ = false;
  for (const auto& style :
       inspector_attribute->inline_style_sheet_.property_order_) {
    auto iter_range = map.equal_range(style);
    for (auto it = iter_range.first; it != iter_range.second; ++it) {
      auto& cur_value = it->second;
      if (cur_value.looped_) continue;
      cur_value.looped_ = true;
      cur_value.text_ = cur_value.name_ + ":" + cur_value.value_ + ";";
      css_text += cur_value.text_;
      cur_value.disabled_ = false;
      cur_value.implicit_ = false;
      cur_value.parsed_ok_ = true;
      cur_value.property_range_.start_line_ = start_line;
      cur_value.property_range_.end_line_ = start_line;
      cur_value.property_range_.start_column_ = property_start_column;
      cur_value.property_range_.end_column_ =
          property_start_column + static_cast<int>(cur_value.text_.size());
      property_start_column = cur_value.property_range_.end_column_;
      break;
    }
  }

  inspector_attribute->inline_style_sheet_.css_text_ = css_text;
  inspector_attribute->inline_style_sheet_.style_value_range_.end_column_ =
      property_start_column;
}

void ElementInspector::DeleteStyleFromInlineStyleSheet(
    Element* element, const std::string& name) {
  CHECK_NULL_AND_LOG_RETURN(element, "element is null");
  auto* inspector_attribute = element->inspector_attribute();
  CHECK_NULL_AND_LOG_RETURN(inspector_attribute, "inspector_attribute is null");
  auto& order = inspector_attribute->inline_style_sheet_.property_order_;
  auto& map = inspector_attribute->inline_style_sheet_.css_properties_;
  for (auto iter = order.begin(); iter != order.end();) {
    if (*iter == name) {
      iter = order.erase(iter);
    } else {
      iter++;
    }
  }
  map.erase(name);
  AdjustStyleSheet(element);
}

void ElementInspector::UpdateStyleToInlineStyleSheet(Element* element,
                                                     const std::string& name,
                                                     const std::string& value) {
  CHECK_NULL_AND_LOG_RETURN(element, "element is null");
  auto* inspector_attribute = element->inspector_attribute();
  CHECK_NULL_AND_LOG_RETURN(inspector_attribute, "inspector_attribute is null");
  auto& order = inspector_attribute->inline_style_sheet_.property_order_;
  auto& map = inspector_attribute->inline_style_sheet_.css_properties_;
  auto iter_range = map.equal_range(name);
  if (iter_range.first == iter_range.second) {
    order.push_back(name);
    auto new_iter = map.insert({name, CSSPropertyDetail()});
    new_iter->second.name_ = name;
    new_iter->second.value_ = value;
  } else {
    for (auto it = iter_range.first; it != iter_range.second; ++it) {
      it->second.name_ = name;
      it->second.value_ = value;
    }
  }
  AdjustStyleSheet(element);
}

void ElementInspector::DeleteStyle(Element* element, const std::string& name) {
  DeleteStyleFromInlineStyleSheet(element, name);
}

void ElementInspector::UpdateStyle(Element* element, const std::string& name,
                                   const std::string& value) {
  UpdateStyleToInlineStyleSheet(element, name, value);
}

void ElementInspector::DeleteAttr(Element* element, const std::string& name) {
  CHECK_NULL_AND_LOG_RETURN(element, "element is null");
  auto* inspector_attribute = element->inspector_attribute();
  CHECK_NULL_AND_LOG_RETURN(inspector_attribute, "inspector_attribute is null");
  if (inspector_attribute->attr_map_.find(name) !=
      inspector_attribute->attr_map_.end()) {
    inspector_attribute->attr_map_.erase(name);
    for (auto iter = inspector_attribute->attr_order_.begin();
         iter != inspector_attribute->attr_order_.end(); ++iter) {
      if (*iter == name) {
        inspector_attribute->attr_order_.erase(iter);
        break;
      }
    }
  }
}

void ElementInspector::UpdateAttr(Element* element, const std::string& name,
                                  const std::string& value) {
  CHECK_NULL_AND_LOG_RETURN(element, "element is null");
  auto* inspector_attribute = element->inspector_attribute();
  CHECK_NULL_AND_LOG_RETURN(inspector_attribute, "inspector_attribute is null");
  if (inspector_attribute->attr_map_.find(name) ==
      inspector_attribute->attr_map_.end()) {
    inspector_attribute->attr_order_.push_back(name);
  }
  inspector_attribute->attr_map_[name] = value;
}

void ElementInspector::DeleteClasses(Element* element) {
  CHECK_NULL_AND_LOG_RETURN(element, "element is null");
  auto* inspector_attribute = element->inspector_attribute();
  CHECK_NULL_AND_LOG_RETURN(inspector_attribute, "inspector_attribute is null");
  inspector_attribute->class_order_.clear();
}
void ElementInspector::UpdateClasses(Element* element,
                                     const std::vector<std::string> classes) {
  CHECK_NULL_AND_LOG_RETURN(element, "element is null");
  auto* inspector_attribute = element->inspector_attribute();
  CHECK_NULL_AND_LOG_RETURN(inspector_attribute, "inspector_attribute is null");
  inspector_attribute->class_order_ = classes;
}

void ElementInspector::SetStyleSheetByName(
    Element* element, const std::string& name,
    const lynx::devtool::InspectorStyleSheet& style_sheet) {
  auto* style_root = StyleRoot(element);
  CHECK_NULL_AND_LOG_RETURN(style_root, "style_root is null");
  std::unordered_multimap<std::string, InspectorStyleSheet>& map =
      GetStyleSheetMap(style_root);
  if (map.count(name) == 1) {
    auto iter = map.find(name);
    iter->second = style_sheet;
  } else {
    auto range = map.equal_range(name);
    for (auto iter = range.first; iter != range.second; ++iter) {
      if (style_sheet.position_ == iter->second.position_) {
        iter->second = style_sheet;
        break;
      }
    }
  }
}

bool ElementInspector::IsStyleRootHasCascadeStyle(Element* element) {
  CHECK_NULL_AND_LOG_RETURN_VALUE(element, "element is null", false);
  auto style_sheet = element->GetRelatedCSSFragment();
  CHECK_NULL_AND_LOG_RETURN_VALUE(style_sheet, "style_sheet is null", false);
  return style_sheet->HasCascadeStyle();
}

bool ElementInspector::IsEnableCSSSelector(Element* element) {
  CHECK_NULL_AND_LOG_RETURN_VALUE(element, "element is null", false);
  auto style_sheet = element->GetRelatedCSSFragment();
  CHECK_NULL_AND_LOG_RETURN_VALUE(style_sheet, "style_sheet is null", false);
  return style_sheet->enable_css_selector();
}

bool ElementInspector::IsEnableCSSInheritance(Element* element) {
  CHECK_NULL_AND_LOG_RETURN_VALUE(element, "element is null", false);
  auto* element_manager = element->element_manager();
  CHECK_NULL_AND_LOG_RETURN_VALUE(element_manager, "element_manager is null",
                                  false);
  return element_manager->GetCSSInheritance();
}

std::unordered_map<std::string, std::string> ElementInspector::GetDefaultCss() {
  return CSSProperty::GetComputeStyleMap();
}

base::InlineVector<lynx::tasm::CSSPropertyID, 16>
ElementInspector::GetOrderedInspectorResetStyleNames() {
  base::InlineVector<lynx::tasm::CSSPropertyID, 16> reset_names;
  for (int id = lynx::tasm::CSSPropertyID::kPropertyStart + 1;
       id < lynx::tasm::CSSPropertyID::kPropertyEnd; ++id) {
    auto css_id = static_cast<lynx::tasm::CSSPropertyID>(id);
    if (!CSSProperty::IsInspectorFilteredProperty(css_id)) {
      reset_names.push_back(css_id);
    }
  }
  return reset_names;
}

std::vector<double> ElementInspector::GetOverlayNGBoxModel(Element* element) {
  std::vector<double> res;

  CHECK_NULL_AND_LOG_RETURN_VALUE(element, "element is null", res);
  auto* catalyzer = element->GetCaCatalyzer();
  CHECK_NULL_AND_LOG_RETURN_VALUE(catalyzer, "catalyzer is null", res);

  auto size = catalyzer->getWindowSize(element);
  res.push_back(size[0]);
  res.push_back(size[1]);

  // content/padding/border/margin box is the same
  // left_top  right_top  right_bottom  left_bottom  coordinate is :
  //  0   0 res[0](w) 0  res[0] res[1](h) 0 res[1]
  for (int i = 0; i < 4; i++) {
    res.push_back(0);
    res.push_back(0);
    res.push_back(res[0]);
    res.push_back(0);
    res.push_back(res[0]);
    res.push_back(res[1]);
    res.push_back(0);
    res.push_back(res[1]);
  }
  return res;
}

std::vector<float> ElementInspector::GetRectToWindow(Element* element) {
  CHECK_NULL_AND_LOG_RETURN_VALUE(element, "element is null", {});
  auto* catalyzer = element->GetCaCatalyzer();
  CHECK_NULL_AND_LOG_RETURN_VALUE(catalyzer, "catalyzer is null", {});
  return catalyzer->GetRectToWindow(element);
}

std::vector<Element*> ElementInspector::SelectElementAll(
    Element* element, const std::string& selector) {
  std::vector<Element*> res;
  CHECK_NULL_AND_LOG_RETURN_VALUE(element, "element is null", res);
  lynx::tasm::NodeSelectOptions options(
      lynx::tasm::NodeSelectOptions::IdentifierType::CSS_SELECTOR, selector);
  options.first_only = false;
  options.only_current_component = false;
  auto nodes = lynx::tasm::FiberElementSelector::Select(
                   static_cast<tasm::FiberElement*>(element), options)
                   .nodes;
  if (!nodes.empty()) {
    for (auto* node : nodes) {
      res.push_back(node);
    }
  }
  return res;
}

}  // namespace devtool
}  // namespace lynx
