// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/element/element_helper.h"

#include <string>

#include "base/include/log/logging.h"
#include "base/include/string/string_utils.h"
#include "core/inspector/style_sheet.h"
#include "core/renderer/css/css_decoder.h"
#include "devtool/lynx_devtool/agent/inspector_util.h"
#include "devtool/lynx_devtool/element/helper_util.h"
#include "devtool/lynx_devtool/element/inspector_css_helper.h"

namespace lynx {
namespace devtool {

const char* kLynxLocalUrl = "file:///Lynx.html";
const char* kLynxSecurityOrigin = "file://core";
const char* kLynxMimeType = "text/html";
const char* kPaddingCurlyBrackets = " {";

bool IsAnimationNameLegal(lynx::tasm::Element* ptr, std::string name) {
  bool res = false;
  lynx::tasm::Element* style = ElementInspector::StyleRoot(ptr);
  CHECK_NULL_AND_LOG_RETURN_VALUE(style, "style is null", res);
  auto animation_map = ElementInspector::GetAnimationMap(style);
  if (animation_map.find(name) != animation_map.end()) {
    res = true;
  }
  return res;
}

bool IsAnimationValueLegal(lynx::tasm::Element* ptr,
                           std::string animation_value) {
  bool res = false;

  std::vector<std::string> animation;
  static std::vector<std::string> animation_key = {"animation-name",
                                                   "animation-duration",
                                                   "animation-timing-function",
                                                   "animation-delay",
                                                   "animation-iteration-count",
                                                   "animation-direction",
                                                   "animation-fill-mode",
                                                   "animation-play-state"};
  for (size_t i = 0; i < animation_value.length(); i++) {
    int pos = static_cast<int>(animation_value.find(' ', i));
    if (pos == -1) pos = static_cast<int>(animation_value.length());
    if (!animation_value.substr(i, pos - i).empty())
      animation.push_back(animation_value.substr(i, pos - i));
    i = pos;
  }
  bool flag = IsAnimationNameLegal(ptr, animation[0]);
  for (size_t i = 1; i < animation.size(); i++) {
    if (flag) {
      flag =
          InspectorCSSHelper::IsAnimationLegal(animation_key[i], animation[i]);
    } else {
      break;
    }
  }
  if (flag) res = true;
  return res;
}

Element* ElementHelper::GetPreviousNode(Element* ptr) {
  CHECK_NULL_AND_LOG_RETURN_VALUE(ptr, "ptr is null", nullptr);
  Element* parent = ptr->parent();
  CHECK_NULL_AND_LOG_RETURN_VALUE(parent, "parent is null", nullptr);

  auto children = parent->GetChildren();
  for (auto iter = children.begin(); iter != children.end(); ++iter) {
    if ((*iter) == ptr) {
      if (iter != children.begin()) {
        return (*(--iter));
      }
      break;
    }
  }
  return nullptr;
}

Json::Value ElementHelper::GetDocumentBodyFromNode(Element* ptr) {
  Json::Value res = Json::Value(Json::ValueType::objectValue);

  CHECK_NULL_AND_LOG_RETURN_VALUE(ptr, "ptr is null", res);

  auto set_node_func = [](Json::Value& res, Element* ptr) {
    SetJsonValueOfNode(ptr, res);
    res["childNodeCount"] = static_cast<int>(ptr->GetChildren().size());
    res["children"] = Json::Value(Json::ValueType::arrayValue);
    for (Element* child : ptr->GetChildren()) {
      res["children"].append(GetDocumentBodyFromNode(child));
    }
  };

  Element* comp_ptr =
      ElementInspector::GetParentComponentElementFromDataModel(ptr);

  if (comp_ptr && ElementInspector::IsNeedEraseId(comp_ptr)) {
    // when the element tree is nested component tree like as below
    // fake component
    //    --> fake component
    //          -->fake component
    //               -->  true element
    // Then after we have finished constructing the subtree with the child
    // element of the bottom-most component as the root node, we need to
    // continuously loop upwards until we find a node that is not a
    // fake component element
    set_node_func(res, ptr);

    Json::Value current_res = res;
    Element* comp_ptr =
        ElementInspector::GetParentComponentElementFromDataModel(ptr);
    while (true) {
      Json::Value comp = Json::Value(Json::ValueType::objectValue);
      set_node_func(comp, comp_ptr);
      comp["childNodeCount"] = 1;
      comp["children"].append(current_res);

      comp_ptr =
          ElementInspector::GetParentComponentElementFromDataModel(comp_ptr);
      if (!comp_ptr || !ElementInspector::IsNeedEraseId(comp_ptr)) {
        return comp;
      }
      current_res = std::move(comp);
    }
  } else if (ElementInspector::Type(ptr) == InspectorElementType::COMPONENT) {
    set_node_func(res, ptr);
    if (ElementInspector::SelectorTag(ptr) == "page") {
      Json::Value doc = Json::Value(Json::ValueType::objectValue);
      set_node_func(doc, ElementInspector::DocElement(ptr));
      doc["childNodeCount"] = 1;
      doc["children"].append(res);
      return doc;
    }
  } else {
    set_node_func(res, ptr);
  }
  return res;
}

void ElementHelper::SetJsonValueOfNode(Element* ptr, Json::Value& value) {
  CHECK_NULL_AND_LOG_RETURN(ptr, "ptr is null");

  value["backendNodeId"] = ElementInspector::NodeId(ptr);
  value["nodeId"] = ElementInspector::NodeId(ptr);
  value["nodeType"] = ElementInspector::NodeType(ptr);
  value["localName"] = ElementInspector::LocalName(ptr);
  value["nodeName"] = ElementInspector::NodeName(ptr);
  value["nodeValue"] = ElementInspector::NodeValue(ptr);

  Element* parent = ptr->parent();
  if (parent != nullptr) {
    value["parentId"] = ElementInspector::NodeId(parent);
  }

  value["attributes"] = Json::Value(Json::ValueType::arrayValue);
  for (const std::string& name : ElementInspector::AttrOrder(ptr)) {
    value["attributes"].append(Json::Value(name));
    value["attributes"].append(
        Json::Value(ElementInspector::AttrMap(ptr).at(name)));
  }
  for (const std::string& name : ElementInspector::DataOrder(ptr)) {
    value["attributes"].append(Json::Value(name));
    value["attributes"].append(
        Json::Value(ElementInspector::DataMap(ptr).at(name)));
  }
  for (const std::string& name : ElementInspector::EventOrder(ptr)) {
    value["attributes"].append(Json::Value(name));
    value["attributes"].append(
        Json::Value(ElementInspector::EventMap(ptr).at(name)));
  }
  if (!ElementInspector::ClassOrder(ptr).empty()) {
    value["attributes"].append(Json::Value("class"));
    std::string temp;
    for (const std::string& str : ElementInspector::ClassOrder(ptr)) {
      temp += str.substr(1);
      if (str != ElementInspector::ClassOrder(ptr).back()) {
        temp += " ";
      }
    }
    value["attributes"].append(Json::Value(temp));
  }
  if (!ElementInspector::GetInlineStyleSheet(ptr).css_properties_.empty()) {
    value["attributes"].append(Json::Value("style"));
    value["attributes"].append(
        Json::Value(ElementInspector::GetInlineStyleSheet(ptr).css_text_));
  }
  if (ElementInspector::IsNeedEraseId(ptr)) {
    value["attributes"].append(Json::Value("fake-element"));
    value["attributes"].append(Json::Value("true"));
  }

  auto* inspector_attribute = ptr->inspector_attribute();
  CHECK_NULL_AND_LOG_RETURN(inspector_attribute, "inspector_attribute is null");

  if (inspector_attribute->wrapper_component_) {
    value["attributes"].append(Json::Value("wrapper-component"));
    value["attributes"].append(Json::Value("true"));
  }

  // If element is plug, then append the corresponding slot name to the
  // attributes.
  if (!inspector_attribute->slot_name_.empty()) {
    value["attributes"].append(Json::Value("slot"));
    value["attributes"].append(inspector_attribute->slot_name_.c_str());
  }

  if (!inspector_attribute->parent_component_name_.empty()) {
    value["attributes"].append(Json::Value("parent-component"));
    value["attributes"].append(
        inspector_attribute->parent_component_name_.c_str());
  }
}

Json::Value ElementHelper::GetMatchedStylesForNode(Element* ptr) {
  Json::Value content;
  if (ptr != nullptr && ElementInspector::HasDataModel(ptr)) {
    content["cssKeyframesRules"] = GetKeyframesRulesForNode(ptr);
    content["pseudoElements"] = Json::Value(Json::ValueType::arrayValue);
    content["inlineStyle"] = GetInlineStyleOfNode(ptr);
    content["matchedCSSRules"] = GetMatchedCSSRulesOfNode(ptr);
    if (ElementInspector::IsEnableCSSInheritance(ptr)) {
      content["inherited"] = GetInheritedCSSRulesOfNode(ptr);
    }

  } else {
    Json::Value error = Json::Value(Json::ValueType::objectValue);
    error["code"] = Json::Value(-32000);
    error["message"] = Json::Value("Node is not an Element");
    content["error"] = error;
  }
  return content;
}

void ElementHelper::FillKeyFramesRule(
    Element* ptr,
    const std::unordered_multimap<std::string, CSSPropertyDetail>& css_property,
    Json::Value& content, std::set<std::string>& animation_name_set,
    const std::string& key) {
  auto range = css_property.equal_range(key);
  for (auto it = range.first; it != range.second; ++it) {
    auto field = range.first->second;
    if (field.parsed_ok_ && !field.disabled_) {
      std::vector<std::string> anim_names =
          GetAnimationNames(field.value_, key == "animation");
      for (std::string& anim_name : anim_names) {
        std::pair<bool, Json::Value> pair = GetKeyframesRule(anim_name, ptr);
        if (!pair.first) continue;
        Json::Value keyframes_rule = pair.second;
        std::string name = keyframes_rule["animationName"]["text"].asString();
        if (animation_name_set.find(name) == animation_name_set.end()) {
          content.append(keyframes_rule);
          animation_name_set.insert(name);
        }
      }
    }
  }
}

void ElementHelper::FillKeyFramesRuleByStyleSheet(
    Element* ptr, const InspectorStyleSheet& style_sheet, Json::Value& content,
    std::set<std::string>& animation_name_set) {
  const auto& css_property = style_sheet.css_properties_;
  if (css_property.find("animation-name") != css_property.end()) {
    FillKeyFramesRule(ptr, css_property, content, animation_name_set,
                      "animation-name");
  } else if (css_property.find("animation") != css_property.end()) {
    FillKeyFramesRule(ptr, css_property, content, animation_name_set,
                      "animation");
  }
}

Json::Value ElementHelper::GetKeyframesRulesForNode(Element* ptr) {
  Json::Value content(Json::ValueType::arrayValue);
  CHECK_NULL_AND_LOG_RETURN_VALUE(ptr, "ptr is null", content);
  std::vector<std::string> class_vec = ElementInspector::ClassOrder(ptr);
  std::set<std::string> animation_name_set;

  if (ElementInspector::IsEnableCSSSelector(ptr)) {
    const std::vector<lynx::devtool::InspectorStyleSheet>& match_rules =
        ElementInspector::GetMatchedStyleSheet(ptr);
    for (const InspectorStyleSheet& match : match_rules) {
      FillKeyFramesRuleByStyleSheet(ptr, match, content, animation_name_set);
    }
  } else {
    for (std::string cls : class_vec) {
      const InspectorStyleSheet& style_sheet =
          ElementInspector::GetStyleSheetByName(ptr, cls);
      FillKeyFramesRuleByStyleSheet(ptr, style_sheet, content,
                                    animation_name_set);
    }
  }
  const InspectorStyleSheet& inline_stylesheet =
      ElementInspector::GetInlineStyleSheet(ptr);
  FillKeyFramesRuleByStyleSheet(ptr, inline_stylesheet, content,
                                animation_name_set);

  return content;
}

std::pair<bool, Json::Value> ElementHelper::GetKeyframesRule(
    const std::string& name, Element* ptr) {
  Json::Value keyframes_rule(Json::ValueType::objectValue);

  keyframes_rule["animationName"] = Json::Value(Json::ValueType::objectValue);
  keyframes_rule["keyframes"] = Json::Value(Json::ValueType::arrayValue);
  keyframes_rule["animationName"]["text"] = name;

  Element* style = ElementInspector::StyleRoot(ptr);
  CHECK_NULL_AND_LOG_RETURN_VALUE(style, "style is null",
                                  std::make_pair(false, keyframes_rule));

  auto animation = ElementInspector::GetAnimationKeyframeByName(ptr, name);
  if (animation.empty()) {
    return std::make_pair(false, keyframes_rule);
  }

  for (InspectorKeyframe part : animation) {
    Json::Value keyframe = Json::Value(Json::ValueType::objectValue);
    keyframe["keyText"] = Json::Value(Json::ValueType::objectValue);
    keyframe["keyText"]["text"] = part.key_text_;
    keyframe["origin"] = part.style_.origin_;
    keyframe["style"] = Json::Value(Json::ValueType::objectValue);
    keyframe["style"]["styleSheetId"] = part.style_.style_sheet_id_;
    keyframe["style"]["cssProperties"] =
        Json::Value(Json::ValueType::arrayValue);
    for (std::string prop : part.style_.property_order_) {
      Json::Value property = Json::Value(Json::ValueType::objectValue);
      property["name"] = prop;
      auto it = part.style_.css_properties_.find(prop);
      if (it != part.style_.css_properties_.end()) {
        property["value"] = it->second.value_;
      }
      keyframe["style"]["cssProperties"].append(property);
    }
    keyframe["style"]["shorthandEntries"] =
        Json::Value(Json::ValueType::arrayValue);
    for (auto entry : part.style_.shorthand_entries_) {
      Json::Value shorthand_entry = Json::Value(Json::ValueType::objectValue);
      shorthand_entry["name"] = entry.first;
      shorthand_entry["value"] = entry.second.value_;
      keyframe["style"]["shorthandEntries"].append(shorthand_entry);
    }
    keyframes_rule["keyframes"].append(keyframe);
  }
  return std::make_pair(true, keyframes_rule);
}

Json::Value ElementHelper::GetInlineStyleOfNode(Element* ptr) {
  Json::Value content(Json::ValueType::objectValue);
  Json::Value temp(Json::ValueType::objectValue);
  Json::Value range(Json::ValueType::objectValue);
  if (ptr != nullptr) {
    InspectorStyleSheet inline_style_sheet =
        ElementInspector::GetInlineStyleSheet(ptr);
    if (!inline_style_sheet.empty) {
      content["shorthandEntries"] = Json::Value(Json::ValueType::arrayValue);
      content["cssProperties"] = Json::Value(Json::ValueType::arrayValue);
      auto& css_properties = inline_style_sheet.css_properties_;
      for (auto& item : css_properties) item.second.looped_ = false;
      for (const std::string& name : inline_style_sheet.property_order_) {
        auto iter_range = css_properties.equal_range(name);
        for (auto it = iter_range.first; it != iter_range.second; ++it) {
          if (it->second.looped_) continue;
          CSSPropertyDetail& css_property_detail = it->second;
          css_property_detail.looped_ = true;
          temp["name"] = name;
          if (name == "animation") {
            temp["value"] =
                NormalizeAnimationString(css_property_detail.value_);
          } else {
            temp["value"] = css_property_detail.value_;
          }
          temp["implicit"] = css_property_detail.implicit_;
          temp["disabled"] = css_property_detail.disabled_;
          temp["parsedOk"] = css_property_detail.parsed_ok_;
          temp["text"] = css_property_detail.text_;
          range["startLine"] = css_property_detail.property_range_.start_line_;
          range["startColumn"] =
              css_property_detail.property_range_.start_column_;
          range["endLine"] = css_property_detail.property_range_.end_line_;
          range["endColumn"] = css_property_detail.property_range_.end_column_;
          temp["range"] = range;
          content["cssProperties"].append(temp);
          break;
        }
      }
      range["startLine"] = inline_style_sheet.style_value_range_.start_line_;
      range["startColumn"] =
          inline_style_sheet.style_value_range_.start_column_;
      range["endLine"] = inline_style_sheet.style_value_range_.end_line_;
      range["endColumn"] = inline_style_sheet.style_value_range_.end_column_;
      content["range"] = range;
      content["cssText"] = inline_style_sheet.css_text_;
      content["styleSheetId"] = inline_style_sheet.style_sheet_id_;
    }
  } else {
    Json::Value error = Json::Value(Json::ValueType::objectValue);
    error["code"] = Json::Value(-32000);
    error["message"] = Json::Value("Node is not an Element");
    content["error"] = error;
  }
  return content;
}

Json::Value ElementHelper::GetBackGroundColorsOfNode(Element* ptr) {
  Json::Value content = Json::Value(Json::ValueType::objectValue);
  if (ptr != nullptr && ElementInspector::HasDataModel(ptr)) {
    auto dict = ElementInspector::GetDefaultCss();
    if (ElementInspector::IsEnableCSSSelector(ptr)) {
      const std::vector<InspectorStyleSheet>& match_rules =
          ElementInspector::GetMatchedStyleSheet(ptr);
      for (const InspectorStyleSheet& match : match_rules) {
        ReplaceDefaultComputedStyle(dict, match.css_properties_);
      }
    } else {
      ReplaceDefaultComputedStyle(
          dict,
          ElementInspector::GetStyleSheetByName(ptr, "*").css_properties_);
      ReplaceDefaultComputedStyle(
          dict,
          ElementInspector::GetStyleSheetByName(ptr, "body *").css_properties_);
      for (size_t i = 0; i < ElementInspector::ClassOrder(ptr).size(); ++i) {
        ReplaceDefaultComputedStyle(
            dict, ElementInspector::GetStyleSheetByName(
                      ptr, ElementInspector::ClassOrder(ptr)[i])
                      .css_properties_);
      }
      ReplaceDefaultComputedStyle(dict,
                                  ElementInspector::GetStyleSheetByName(
                                      ptr, ElementInspector::SelectorTag(ptr))
                                      .css_properties_);
      ReplaceDefaultComputedStyle(dict,
                                  ElementInspector::GetStyleSheetByName(
                                      ptr, ElementInspector::SelectorId(ptr))
                                      .css_properties_);
    }
    ReplaceDefaultComputedStyle(
        dict, ElementInspector::GetInlineStyleSheet(ptr).css_properties_);
    Json::Value background_colors(Json::ValueType::arrayValue);
    background_colors.append(dict.at("background-color"));
    content["backgroundColors"] = background_colors;
    content["computedFontSize"] = dict.at("font-size");
    content["computedFontWeight"] = dict.at("font-weight");
  } else {
    Json::Value error = Json::Value(Json::ValueType::objectValue);
    error["code"] = Json::Value(-32000);
    error["message"] = Json::Value("Node is not an Element");
    content["error"] = error;
  }
  return content;
}

Json::Value ElementHelper::GetMatchedCSSRulesOfNode(Element* ptr) {
  Json::Value res(Json::ValueType::arrayValue);
  if (!ptr || !ElementInspector::HasDataModel(ptr)) {
    return res;
  }

  if (ElementInspector::IsEnableCSSSelector(ptr)) {
    const std::vector<lynx::devtool::InspectorStyleSheet>& match_styles =
        ElementInspector::GetMatchedStyleSheet(ptr);

    for (const InspectorStyleSheet& matched : match_styles) {
      MergeCSSStyle(res, matched, true);
    }
  } else {
    MergeCSSStyle(res, ElementInspector::GetStyleSheetByName(ptr, "*"), false);
    MergeCSSStyle(res,
                  ElementInspector::GetStyleSheetByName(
                      ptr, ElementInspector::SelectorTag(ptr)),
                  false);
    ApplyPseudoChildStyle(ptr, res, ElementInspector::SelectorTag(ptr));

    for (const std::string& name : ElementInspector::ClassOrder(ptr)) {
      MergeCSSStyle(res, ElementInspector::GetStyleSheetByName(ptr, name),
                    false);
      ApplyCascadeStyles(ptr, res, name);
      ApplyPseudoChildStyle(ptr, res, name);
      ApplyPseudoCascadeStyles(ptr, res, name);
    }
    if (!ElementInspector::SelectorId(ptr).empty()) {
      MergeCSSStyle(res,
                    ElementInspector::GetStyleSheetByName(
                        ptr, ElementInspector::SelectorId(ptr)),
                    false);
      ApplyCascadeStyles(ptr, res, ElementInspector::SelectorId(ptr));
      ApplyPseudoChildStyle(ptr, res, ElementInspector::SelectorId(ptr));
      ApplyPseudoCascadeStyles(ptr, res, ElementInspector::SelectorId(ptr));
    }
  }
  return res;
}

void ElementHelper::ApplyCascadeStyles(Element* ptr, Json::Value& result,
                                       const std::string& rule) {
  if (ElementInspector::IsStyleRootHasCascadeStyle(ptr)) {
    Element* parent = ptr->parent();
    while (parent) {
      for (const std::string& parent_rule :
           ElementInspector::ClassOrder(parent)) {
        InspectorStyleSheet style_sheet =
            ElementInspector::GetStyleSheetByName(ptr, rule + parent_rule);
        if (!style_sheet.empty) MergeCSSStyle(result, style_sheet, false);
      }
      parent = parent->parent();
    }

    parent = ptr->parent();
    while (parent) {
      if (!ElementInspector::SelectorId(parent).empty()) {
        InspectorStyleSheet style_sheet = ElementInspector::GetStyleSheetByName(
            ptr, rule + ElementInspector::SelectorId(parent));
        if (!style_sheet.empty) MergeCSSStyle(result, style_sheet, false);
      }
      parent = parent->parent();
    }
  }
}

void ElementHelper::ApplyPseudoCascadeStyles(Element* ptr, Json::Value& result,
                                             const std::string& rule) {
  if (ElementInspector::IsStyleRootHasCascadeStyle(ptr)) {
    Element* parent = ptr->parent();
    while (parent) {
      for (const std::string& parent_rule :
           ElementInspector::ClassOrder(parent)) {
        ApplyPseudoChildStyle(ptr, result, rule + parent_rule);
      }
      parent = parent->parent();
    }

    parent = ptr->parent();
    while (parent) {
      if (!ElementInspector::SelectorId(parent).empty()) {
        ApplyPseudoChildStyle(ptr, result,
                              rule + ElementInspector::SelectorId(parent));
      }
      parent = parent->parent();
    }
  }
}

std::string ElementHelper::GetPseudoChildNameForStyle(
    const std::string& rule, const std::string& pseudo_child) {
  std::string pseudo_style;
  size_t dot_index = rule.find_last_of(".");
  size_t id_index = rule.find_last_of("#");

  if (dot_index != std::string::npos && dot_index != 0) {
    std::string child_name = rule.substr(0, dot_index);
    std::string parent_name = rule.substr(dot_index);
    pseudo_style = child_name + pseudo_child + parent_name;
  } else if (id_index != std::string::npos && id_index != 0) {
    std::string child_name = rule.substr(0, id_index);
    std::string parent_name = rule.substr(id_index);
    pseudo_style = child_name + pseudo_child + parent_name;
  } else {
    pseudo_style = rule + pseudo_child;
  }
  return pseudo_style;
}

void ElementHelper::ApplyPseudoChildStyle(Element* ptr, Json::Value& result,
                                          const std::string& rule) {
  CHECK_NULL_AND_LOG_RETURN(ptr, "ptr is null");
  auto* parent = ptr->parent();
  CHECK_NULL_AND_LOG_RETURN(parent, "ptr->parent() is null");

  if (ptr == parent->GetChildAt(0)) {
    InspectorStyleSheet style_sheet = ElementInspector::GetStyleSheetByName(
        ptr, GetPseudoChildNameForStyle(rule, ":first-child"));
    if (!style_sheet.empty) MergeCSSStyle(result, style_sheet, false);
  }
  if (ptr == parent->GetChildAt(ptr->GetChildCount() - 1)) {
    InspectorStyleSheet style_sheet = ElementInspector::GetStyleSheetByName(
        ptr, GetPseudoChildNameForStyle(rule, ":last-child"));
    if (!style_sheet.empty) MergeCSSStyle(result, style_sheet, false);
  }
}

Json::Value ElementHelper::GetStyleSheetText(
    Element* ptr, const std::string& style_sheet_id) {
  Json::Value content;
  std::string text;
  if (ElementInspector::Type(ptr) != InspectorElementType::DOCUMENT) {
    Element* style_root = ElementInspector::StyleRoot(ptr);
    if (style_root != nullptr) {
      std::unordered_multimap<std::string, InspectorStyleSheet>& map =
          ElementInspector::GetStyleSheetMap(style_root);
      text += !map.empty() ? "\n" : "";
      for (const auto& item : map) {
        const InspectorStyleSheet& cur_style_sheet = item.second;
        text += cur_style_sheet.style_name_ + kPaddingCurlyBrackets +
                cur_style_sheet.css_text_ + "}\n";
      }
    }
  } else {
    // element type is document
    Element* document_ptr = ptr;
    const auto& css_rules = ElementInspector::GetCssRules(document_ptr);
    for (const InspectorCSSRule& css_rule : css_rules) {
      text += css_rule.style_.style_name_ + kPaddingCurlyBrackets +
              css_rule.style_.css_text_ + "}\n";
    }
  }
  content["text"] = text;
  return content;
}

Json::Value ElementHelper::GetInheritedCSSRulesOfNode(Element* ptr) {
  Json::Value res(Json::ValueType::arrayValue);
  CHECK_NULL_AND_LOG_RETURN_VALUE(ptr, "ptr is null", res);
  Json::Value content(Json::ValueType::objectValue);
  Element* parent_ptr = ptr->parent();
  while (parent_ptr != nullptr && ElementInspector::HasDataModel(parent_ptr)) {
    content["inlineStyle"] = GetInlineStyleOfNode(parent_ptr);
    content["matchedCSSRules"] = GetMatchedCSSRulesOfNode(parent_ptr);
    res.append(content);
    parent_ptr = parent_ptr->parent();
  }
  return res;
}

Json::Value ElementHelper::GetAttributesImpl(Element* ptr) {
  Json::Value attrs_array(Json::ValueType::arrayValue);
  for (const std::string& attr_name : ElementInspector::AttrOrder(ptr)) {
    attrs_array.append(Json::Value(attr_name.c_str()));
    attrs_array.append(
        Json::Value(ElementInspector::AttrMap(ptr).at(attr_name)));
  }
  if (!ElementInspector::ClassOrder(ptr).empty()) {
    attrs_array.append(Json::Value("class"));
    attrs_array.append(ElementHelper::GetAttributesAsTextOfNode(ptr, "class"));
  }
  if (!ElementInspector::GetInlineStyleSheet(ptr).css_text_.empty()) {
    attrs_array.append(Json::Value("style"));
    attrs_array.append(ElementHelper::GetAttributesAsTextOfNode(ptr, "style"));
  }
  return attrs_array;
}

Json::Value ElementHelper::GetAttributesAsTextOfNode(Element* ptr,
                                                     const std::string& name) {
  std::string res;
  if (name == "class") {
    for (const std::string& c : ElementInspector::ClassOrder(ptr)) {
      res += c.substr(1);
      if (c != ElementInspector::ClassOrder(ptr).back()) {
        res += " ";
      }
    }
  } else if (name == "style") {
    res = ElementInspector::GetInlineStyleSheet(ptr).css_text_;
  } else if (name == "id") {
    res = ElementInspector::SelectorId(ptr);
  } else if (ElementInspector::AttrMap(ptr).find(name) !=
             ElementInspector::AttrMap(ptr).end()) {
    res = ElementInspector::AttrMap(ptr).at(name);
  }
  return Json::Value(res);
}

Json::Value ElementHelper::GetStyleSheetAsText(
    const InspectorStyleSheet& style_sheet) {
  Json::Value content;
  Json::Value styles(Json::ValueType::objectValue);
  Json::Value range(Json::ValueType::objectValue);
  Json::Value property(Json::ValueType::objectValue);
  styles["styleSheetId"] = style_sheet.style_sheet_id_;
  styles["cssText"] = style_sheet.css_text_;
  range["startLine"] = style_sheet.style_value_range_.start_line_;
  range["startColumn"] = style_sheet.style_value_range_.start_column_;
  range["endLine"] = style_sheet.style_value_range_.end_line_;
  range["endColumn"] = style_sheet.style_value_range_.end_column_;
  styles["range"] = range;
  styles["cssProperties"] = Json::ValueType::arrayValue;
  styles["shorthandEntries"] = Json::ValueType::arrayValue;

  auto css_properties = style_sheet.css_properties_;
  for (auto& item : css_properties) item.second.looped_ = false;
  for (const std::string& name : style_sheet.property_order_) {
    auto iter_range = css_properties.equal_range(name);
    for (auto it = iter_range.first; it != iter_range.second; ++it) {
      if (it->second.looped_) continue;
      CSSPropertyDetail& css_property_detail = it->second;
      css_property_detail.looped_ = true;
      property["name"] = css_property_detail.name_;
      property["value"] = css_property_detail.value_;
      if (css_property_detail.disabled_) {
        property.removeMember("implicit");
        property["disabled"] = css_property_detail.disabled_;
      } else {
        property["implicit"] = css_property_detail.implicit_;
        property["disabled"] = css_property_detail.disabled_;
      }
      property["parsedOk"] = css_property_detail.parsed_ok_;
      property["text"] = css_property_detail.text_;
      range["startLine"] = css_property_detail.property_range_.start_line_;
      range["startColumn"] = css_property_detail.property_range_.start_column_;
      range["endLine"] = css_property_detail.property_range_.end_line_;
      range["endColumn"] = css_property_detail.property_range_.end_column_;
      property["range"] = range;
      styles["cssProperties"].append(property);
      break;
    }
  }
  content["styles"] = Json::ValueType::arrayValue;
  content["styles"].append(styles);
  Json::Value msg(Json::ValueType::objectValue);
  return content;
}

Json::Value ElementHelper::GetStyleSheetAsTextOfNode(
    Element* ptr, const std::string& style_sheet_id, const Range& range) {
  Json::Value content;
  if (ElementInspector::Type(ptr) == InspectorElementType::STYLEVALUE) {
    Element* style_root = ElementInspector::StyleRoot(ptr);
    if (style_root != nullptr) {
      std::unordered_multimap<std::string, InspectorStyleSheet>& map =
          ElementInspector::GetStyleSheetMap(style_root);
      for (const auto& item : map) {
        const InspectorStyleSheet& cur_style_sheet = item.second;
        if (cur_style_sheet.style_value_range_.start_line_ ==
            range.start_line_) {
          content = GetStyleSheetAsText(cur_style_sheet);
          break;
        }
      }
    }
  } else if (ElementInspector::Type(ptr) == InspectorElementType::DOCUMENT) {
    Element* document_root = ptr;
    auto& css_rules = ElementInspector::GetCssRules(document_root);
    for (const InspectorCSSRule& css_rule : css_rules) {
      const InspectorStyleSheet& cur_style_sheet = css_rule.style_;
      if (cur_style_sheet.style_value_range_.start_line_ == range.start_line_) {
        content = GetStyleSheetAsText(cur_style_sheet);
        break;
      }
    }
  } else {
    content = GetStyleSheetAsText(ElementInspector::GetInlineStyleSheet(ptr));
  }
  return content;
}

void ElementHelper::SetInlineStyleTexts(Element* ptr, const std::string& text,
                                        const Range& range) {
  InspectorStyleSheet pre_style_sheet =
      ElementInspector::GetInlineStyleSheet(ptr);
  InspectorStyleSheet modified_style_sheet =
      StyleTextParser(ptr, text, pre_style_sheet);
  ElementInspector::SetInlineStyleSheet(ptr, modified_style_sheet);
  ElementInspector::Flush(ptr);
  return;
}

void ElementHelper::SetInlineStyleSheet(
    Element* ptr, const InspectorStyleSheet& style_sheet) {
  ElementInspector::SetInlineStyleSheet(ptr, style_sheet);
  ElementInspector::Flush(ptr);
}

void ElementHelper::SetSelectorStyleTexts(Element* root, Element* ptr,
                                          const std::string& text,
                                          const Range& range) {
  CHECK_NULL_AND_LOG_RETURN(ptr, "ptr is null");
  Element* style_root = ElementInspector::StyleRoot(ptr);
  CHECK_NULL_AND_LOG_RETURN(style_root, "style_root is null");
  std::unordered_multimap<std::string, InspectorStyleSheet>& map =
      ElementInspector::GetStyleSheetMap(style_root);
  for (const auto& item : map) {
    const InspectorStyleSheet& cur_style_sheet = item.second;
    if (cur_style_sheet.style_value_range_.start_line_ == range.start_line_) {
      InspectorStyleSheet modified_style_sheet =
          StyleTextParser(ptr, text, cur_style_sheet);
      ElementInspector::SetStyleSheetByName(ptr, item.first,
                                            modified_style_sheet);
      std::vector<Element*> ptr_vec;
      GetElementPtrMatchingStyleSheet(ptr_vec, root, item.first);
      for (Element* temp_ptr : ptr_vec) ElementInspector::Flush(temp_ptr);
      break;
    }
  }
}

bool ElementHelper::GetElementPtrMatchingForCascadedStyleSheet(
    std::vector<lynx::tasm::Element*>& res, lynx::tasm::Element* root,
    const std::string& name, const std::string& style_sheet_name) {
  CHECK_NULL_AND_LOG_RETURN_VALUE(root, "root is null", false);
  Element* parent = root->parent();
  while (parent) {
    for (const std::string& parent_name :
         ElementInspector::ClassOrder(parent)) {
      if (style_sheet_name == name + parent_name) {
        res.push_back(root);
        return true;
      }
    }
    parent = parent->parent();
  }

  parent = root->parent();
  while (parent) {
    if (!ElementInspector::SelectorId(parent).empty()) {
      if (style_sheet_name == name + ElementInspector::SelectorId(parent)) {
        res.push_back(root);
        return true;
      }
    }
    parent = parent->parent();
  }
  return false;
}

void ElementHelper::GetElementPtrMatchingStyleSheet(
    std::vector<Element*>& res, Element* root,
    const std::string& style_sheet_name) {
  CHECK_NULL_AND_LOG_RETURN(root, "root is null");
  if (style_sheet_name.empty()) return;
  if (style_sheet_name == "*" ||
      style_sheet_name == ElementInspector::SelectorId(root) ||
      style_sheet_name == ElementInspector::SelectorTag(root)) {
    res.push_back(root);
  } else {
    for (const std::string& name : ElementInspector::ClassOrder(root)) {
      if (style_sheet_name == name) {
        res.push_back(root);
        break;
      } else {
        bool found = GetElementPtrMatchingForCascadedStyleSheet(
            res, root, name, style_sheet_name);
        if (found) break;
      }
    }
    if (!ElementInspector::SelectorId(root).empty())
      GetElementPtrMatchingForCascadedStyleSheet(
          res, root, ElementInspector::SelectorId(root), style_sheet_name);
  }
  if (!root->GetChildren().empty()) {
    for (Element* child : root->GetChildren()) {
      GetElementPtrMatchingStyleSheet(res, child, style_sheet_name);
    }
  }
}

void ElementHelper::SetStyleTexts(Element* root, Element* ptr,
                                  const std::string& text, const Range& range) {
  if (ElementInspector::Type(ptr) == InspectorElementType::STYLEVALUE) {
    SetSelectorStyleTexts(root, ptr, text, range);
  } else {
    SetInlineStyleTexts(ptr, text, range);
  }
}

void ElementHelper::SetAttributes(Element* ptr, const std::string& name,
                                  const std::string& text) {
  if (name == "style") {
    SetInlineStyleTexts(ptr, text, Range());
  } else if (name == "class") {
    std::vector<std::string> class_order;
    std::string class_name;
    std::string temp = ".";
    for (const char& c : text) {
      if (c != ' ') {
        class_name += c;
      } else {
        class_name = temp + class_name;
        class_order.push_back(class_name);
        class_name.clear();
      }
    }
    if (!class_name.empty()) {
      class_name = temp + class_name;
      class_order.push_back(class_name);
      class_name.clear();
    }
    ElementInspector::SetClassOrder(ptr, class_order);
  } else if (name == "id") {
    ElementInspector::SetSelectorId(ptr, "#" + text);
  } else {
    auto attr_map = ElementInspector::AttrMap(ptr);
    auto attr_order = ElementInspector::AttrOrder(ptr);
    if (attr_map.find(name) == attr_map.end()) {
      attr_order.push_back(name);
    }
    attr_map[name] = text;
    ElementInspector::SetAttrOrder(ptr, attr_order);
    ElementInspector::SetAttrMap(ptr, attr_map);
  }
  ElementInspector::Flush(ptr);
}

void ElementHelper::RemoveAttributes(Element* ptr, const std::string& name) {
  if (name == "style") {
    InspectorStyleSheet sheet = ElementInspector::GetInlineStyleSheet(ptr);
    sheet.css_text_.clear();
    sheet.css_properties_.clear();
    sheet.shorthand_entries_.clear();
    sheet.property_order_.clear();
    sheet.style_value_range_ = sheet.style_name_range_;
    ElementInspector::SetInlineStyleSheet(ptr, sheet);
  } else if (name == "class") {
    ElementInspector::SetClassOrder(ptr, std::vector<std::string>());
  } else if (name == "id") {
    ElementInspector::SetSelectorId(ptr, "");
  } else {
    auto attr_map = ElementInspector::AttrMap(ptr);
    auto attr_order = ElementInspector::AttrOrder(ptr);
    if (attr_map.find(name) != attr_map.end()) {
      for (auto iter = attr_order.begin(); iter != attr_order.end(); ++iter) {
        if (*iter == name) {
          attr_order.erase(iter);
          break;
        }
      }
      attr_map.erase(name);
    }
    ElementInspector::SetAttrOrder(ptr, attr_order);
    ElementInspector::SetAttrMap(ptr, attr_map);
  }
  ElementInspector::Flush(ptr);
}

void ElementHelper::SetOuterHTML(Element* manager, int indexId,
                                 std::string html) {}

std::vector<Json::Value> ElementHelper::SetAttributesAsText(Element* ptr,
                                                            std::string name,
                                                            std::string text) {
  std::vector<Json::Value> msg_v;
  Json::Value msg;
  std::string temp;
  size_t i;
  for (i = 0; i < text.size(); ++i) {
    if (text[i] != '=') {
      temp += text[i];
    } else {
      break;
    }
  }
  if (i + 1 < text.size() && text[i + 1] == '"') {
    text = text.substr(i + 2, text.size() - i - 3);
  } else if (i + 1 < text.size()) {
    text = text.substr(i + 1, text.size() - 1);
  }

  if (temp != name) {
    RemoveAttributes(ptr, name);
    msg["method"] = "DOM.attributeRemoved";
    msg["params"] = Json::ValueType::objectValue;
    msg["params"]["nodeId"] = ElementInspector::NodeId(ptr);
    msg["params"]["name"] = name;
    msg_v.push_back(msg);
  }
  SetAttributes(ptr, temp, text);
  msg["method"] = "DOM.attributeModified";
  msg["params"] = Json::Value(Json::ValueType::objectValue);
  msg["params"]["name"] = temp;
  msg["params"]["nodeId"] = ElementInspector::NodeId(ptr);
  if (ptr != nullptr) {
    msg["params"]["value"] =
        ElementHelper::GetAttributesAsTextOfNode(ptr, name);
  }
  msg_v.push_back(msg);
  if (name == "style") {
    msg["method"] = "CSS.styleSheetChanged";
    msg["params"] = Json::Value(Json::ValueType::objectValue);
    msg["params"]["styleSheetId"] =
        ElementInspector::GetInlineStyleSheet(ptr).style_sheet_id_;
    msg_v.push_back(msg);
  }
  return msg_v;
}

std::string ElementHelper::GetElementContent(Element* ptr, int num) {
  std::string res = "";
  CHECK_NULL_AND_LOG_RETURN_VALUE(ptr, "ptr is null", res);
  std::string temp;
  for (int i = 0; i < num; i++) {
    temp += "\t";
  }

  res += temp;
  res += "<" + ElementInspector::LocalName(ptr);
  if (!ElementInspector::ClassOrder(ptr).empty()) {
    res += " class=\"";
    int tmp = 0;
    for (std::string cls : ElementInspector::ClassOrder(ptr)) {
      if (tmp != 0) {
        res += " ";
      }
      res += cls.substr(cls.find(".") + 1);
      tmp++;
    }
    res += "\"";
  }
  if (!ElementInspector::SelectorId(ptr).empty()) {
    res += " id=\"" + ElementInspector::SelectorId(ptr) + "\"";
  }
  if (!ElementInspector::GetInlineStyleSheet(ptr).property_order_.empty()) {
    res += " style=\"" + ElementInspector::GetInlineStyleSheet(ptr).css_text_ +
           "\"";
  }
  std::unordered_map<std::string, std::string> attr_map =
      ElementInspector::AttrMap(ptr);
  if (!ElementInspector::AttrOrder(ptr).empty()) {
    for (std::string attr : ElementInspector::AttrOrder(ptr)) {
      auto it = attr_map.find(attr);
      if (it != attr_map.end()) {
        std::string value = it->second;
        res += " " + attr + "=\"" + value + "\"";
      }
    }
  }
  res += ">\n";
  if (!ptr->GetChildren().empty()) {
    for (Element* child : ptr->GetChildren()) {
      res += GetElementContent(child, num + 1);
    }
  }
  res += temp;
  res += "</" + ElementInspector::LocalName(ptr) + ">\n";

  return res;
}

std::string ElementHelper::GetStyleNodeText(Element* ptr) {
  return ElementInspector::NodeValue(ptr);
}

Json::Value ElementHelper::GetStyleSheetHeader(Element* ptr) {
  Json::Value header(Json::ValueType::objectValue);
  CHECK_NULL_AND_LOG_RETURN_VALUE(ptr, "ptr is null", header);
  Element* style_root = ElementInspector::StyleRoot(ptr);
  CHECK_NULL_AND_LOG_RETURN_VALUE(style_root, "style root is null", header);
  std::unordered_multimap<std::string, InspectorStyleSheet>& map =
      ElementInspector::GetStyleSheetMap(style_root);
  std::string style_sheet_id =
      std::to_string(ElementInspector::NodeId(style_root));
  header["styleSheetId"] = style_sheet_id;
  header["sourceURL"] = kLynxLocalUrl;
  header["origin"] = "regular";
  header["title"] = "";
  header["ownerNode"] = ElementInspector::NodeId(style_root);
  header["disabled"] = false;
  header["isInline"] = true;
  header["isMutable"] = true;
  header["startLine"] = 0;
  header["startColumn"] = 0;
  header["endLine"] = static_cast<int>(map.size() + 2);
  header["endColumn"] = 0;
  int length = 0;
  for (const auto& item : map) {
    length +=
        item.second.style_name_.length() + item.second.css_text_.length() + 4;
  }
  header["length"] = length;
  return header;
}

InspectorStyleSheet ElementHelper::GetInlineStyleTexts(Element* ptr) {
  return ElementInspector::GetInlineStyleSheet(ptr);
}

Json::Value ElementHelper::CreateStyleSheet(Element* ptr) {
  Json::Value header(Json::ValueType::objectValue);
  header["styleSheetId"] = std::to_string(ElementInspector::NodeId(ptr));
  header["origin"] = "inspector";
  header["sourceURL"] = "";
  header["title"] = "";
  header["ownerNode"] = ElementInspector::NodeId(ptr);
  header["disabled"] = false;
  header["isInline"] = false;
  header["startLine"] = 0;
  header["startColumn"] = 0;
  header["endLine"] = 0;
  header["endColumn"] = 0;
  header["length"] = 0;
  return header;
}

// FIXME(zhengyuwei):this function is used for cdp CSS.addRule
// not clear yet
Json::Value ElementHelper::AddRule(Element* ptr,
                                   const std::string& style_sheet_id,
                                   const std::string& rule_text,
                                   const Range& range) {
  // parse rule text
  bool parse_ok = true;
  std::string temp_text = rule_text;
  std::vector<std::string> parse_result;
  std::vector<std::string> strip_result;
  if (temp_text.length() > 3 &&
      temp_text.substr(temp_text.length() - 3, 3) == " {}") {
    temp_text = temp_text.substr(0, temp_text.length() - 3);
    std::string::size_type next_pos = 0;
    while (std::string::npos != (next_pos = temp_text.find_first_of(","))) {
      std::string temp_str = temp_text.substr(0, next_pos);
      parse_result.push_back(temp_str);
      if (next_pos + 1 >= temp_text.length()) break;
      temp_text = temp_text.substr(next_pos + 1);
    }
    if (next_pos + 1 >= temp_text.length()) {
      parse_result.emplace_back();
    } else {
      parse_result.push_back(temp_text.substr(next_pos + 1));
    }
  }
  if (parse_result.empty()) {
    parse_ok = false;
  } else {
    for (std::string& str : parse_result) {
      std::string temp_str = StripSpace(str);
      if (temp_str.empty()) {
        parse_ok = false;
        break;
      }
      strip_result.push_back(temp_str);
    }
  }

  Json::Value content(Json::ValueType::objectValue);
  if (parse_ok &&
      ElementInspector::Type(ptr) == InspectorElementType::DOCUMENT) {
    Element* document_ptr = ptr;
    auto& css_rules = ElementInspector::GetCssRules(document_ptr);
    int cur_line = static_cast<int>(css_rules.size());
    InspectorCSSRule new_css_rule;
    new_css_rule.origin_ = "inspector";
    new_css_rule.style_sheet_id_ = style_sheet_id;
    InspectorSelectorList& new_select_list = new_css_rule.selector_list_;
    std::string all_text;
    int prev_col = 0;
    Range new_range;
    for (size_t i = 0; i < strip_result.size() - 1; ++i) {
      all_text += strip_result[i] + ", ";
      new_range.start_line_ = cur_line;
      new_range.end_line_ = cur_line;
      new_range.start_column_ = prev_col;
      new_range.end_column_ =
          prev_col + static_cast<int>(strip_result[i].length());
      new_select_list.selectors_order_.emplace_back(strip_result[i]);
      new_select_list.selectors_[strip_result[i]] = new_range;
      prev_col += static_cast<int>(strip_result[i].length()) + 2;
    }
    all_text += strip_result[strip_result.size() - 1];
    new_range.start_line_ = cur_line;
    new_range.end_line_ = cur_line;
    new_range.start_column_ = prev_col;
    new_range.end_column_ =
        prev_col +
        static_cast<int>(strip_result[strip_result.size() - 1].length());
    new_select_list.selectors_order_.emplace_back(
        strip_result[strip_result.size() - 1]);
    new_select_list.selectors_[strip_result[strip_result.size() - 1]] =
        new_range;
    new_select_list.text_ = all_text;

    InspectorStyleSheet& new_style_sheet = new_css_rule.style_;
    new_style_sheet.style_sheet_id_ = style_sheet_id;
    new_range.start_line_ = cur_line;
    new_range.end_line_ = cur_line;
    new_range.start_column_ = 0;
    new_range.end_column_ = prev_col;
    new_style_sheet.style_name_ = all_text;
    new_style_sheet.style_name_range_ = new_range;
    prev_col +=
        static_cast<int>(strip_result[strip_result.size() - 1].length()) + 2;
    new_range.start_line_ = cur_line;
    new_range.end_line_ = cur_line;
    new_range.start_column_ = prev_col;
    new_range.end_column_ = prev_col;
    new_style_sheet.css_text_ = "";
    new_style_sheet.style_value_range_ = new_range;

    Json::Value rule(Json::ValueType::objectValue);
    rule["media"] = Json::ValueType::arrayValue;
    rule["origin"] = new_css_rule.origin_;
    rule["styleSheetId"] = new_css_rule.style_sheet_id_;
    rule["selectorList"] = Json::Value(Json::ValueType::objectValue);
    const InspectorSelectorList& selector_list = new_css_rule.selector_list_;
    rule["selectorList"]["text"] = selector_list.text_;
    rule["selectorList"]["selectors"] =
        Json::Value(Json::ValueType::arrayValue);
    for (const std::string& name : selector_list.selectors_order_) {
      Json::Value text(Json::ValueType::objectValue);
      text["text"] = name;
      text["range"]["startLine"] =
          selector_list.selectors_.at(name).start_line_;
      text["range"]["endLine"] = selector_list.selectors_.at(name).end_line_;
      text["range"]["startColumn"] =
          selector_list.selectors_.at(name).start_column_;
      text["range"]["endColumn"] =
          selector_list.selectors_.at(name).end_column_;
      rule["selectorList"]["selectors"].append(text);
    }
    Json::Value style(Json::ValueType::objectValue);
    const InspectorStyleSheet& style_sheet = new_css_rule.style_;
    style["styleSheetId"] = style_sheet.style_sheet_id_;
    style["cssProperties"] = Json::Value(Json::ValueType::arrayValue);
    style["shorthandEntries"] = Json::Value(Json::ValueType::arrayValue);
    style["cssText"] = "";
    style["range"] = Json::ValueType::objectValue;
    style["range"]["startLine"] = style_sheet.style_value_range_.start_line_;
    style["range"]["endLine"] = style_sheet.style_value_range_.end_line_;
    style["range"]["startColumn"] =
        style_sheet.style_value_range_.start_column_;
    style["range"]["endColumn"] = style_sheet.style_value_range_.end_column_;
    rule["style"] = style;
    content["rule"] = rule;
    // add style sheet to node
    css_rules.emplace_back(new_css_rule);
  } else {
    Json::Value error = Json::Value(Json::ValueType::objectValue);
    error["code"] = Json::Value(-32000);
    error["message"] = Json::Value("SyntaxError Rule text is not valid.");
    content["error"] = error;
  }
  return content;
}

int ElementHelper::QuerySelector(Element* ptr, const std::string& selector) {
  std::vector<Element*> element_arr =
      ElementInspector::SelectElementAll(ptr, selector);
  return !element_arr.empty() ? ElementInspector::NodeId((*element_arr.begin()))
                              : -1;
}

Json::Value ElementHelper::QuerySelectorAll(Element* ptr,
                                            const std::string& selector) {
  Json::Value res(Json::ValueType::arrayValue);
  std::vector<Element*> element_arr =
      ElementInspector::SelectElementAll(ptr, selector);
  for (Element* element : element_arr) {
    res.append(ElementInspector::NodeId(element));
  }
  return res;
}

std::string ElementHelper::GetProperties(Element* ptr) {
  return ptr ? ElementInspector::GetComponentProperties(ptr) : "";
}

std::string ElementHelper::GetData(Element* ptr) {
  return ptr ? ElementInspector::GetComponentData(ptr) : "";
}

std::string ElementHelper::GetComponentId(Element* ptr) {
  return ptr ? ElementInspector::GetComponentId(ptr) : "-1";
}

void ElementHelper::PerformSearchFromNode(Element* ptr, std::string& query,
                                          std::vector<int>& results) {
  CHECK_NULL_AND_LOG_RETURN(ptr, "ptr is null");
  bool match = false;
  if (ElementInspector::LocalName(ptr).find(query) != std::string::npos) {
    results.push_back(ElementInspector::NodeId(ptr));
    match = true;
  }
  if (!match && !ElementInspector::ClassOrder(ptr).empty()) {
    for (const std::string& className : ElementInspector::ClassOrder(ptr)) {
      if (className.find(query) != std::string::npos) {
        results.push_back(ElementInspector::NodeId(ptr));
        match = true;
        break;
      }
    }
  }
  if (!match && !ElementInspector::AttrMap(ptr).empty()) {
    for (auto iter = ElementInspector::AttrMap(ptr).begin();
         iter != ElementInspector::AttrMap(ptr).end(); ++iter) {
      if (iter->first.find(query) != std::string::npos ||
          iter->second.find(query) != std::string::npos) {
        results.push_back(ElementInspector::NodeId(ptr));
        break;
      }
    }
  }
  if (!ptr->GetChildren().empty()) {
    for (Element* child_ptr : ptr->GetChildren()) {
      PerformSearchFromNode(child_ptr, query, results);
    }
  }
}

}  // namespace devtool
}  // namespace lynx
