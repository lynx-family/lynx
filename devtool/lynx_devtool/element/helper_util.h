// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_LYNX_DEVTOOL_ELEMENT_HELPER_UTIL_H_
#define DEVTOOL_LYNX_DEVTOOL_ELEMENT_HELPER_UTIL_H_

#include "core/inspector/style_sheet.h"
#include "core/shell/lynx_shell.h"
#include "devtool/lynx_devtool/element/inspector_css_helper.h"
#include "third_party/jsoncpp/include/json/json.h"

namespace lynx {
namespace tasm {
class Element;
}
}  // namespace lynx

namespace lynx {
namespace devtool {

class InspectorElement;

std::string ConvertLepusValueToJsonValue(const lynx::lepus::Value& lepus_value);
std::string convertLepusTableToDictionaryString(
    const lynx::lepus::Value& lepus_value);

void MergeCSSStyle(Json::Value& res,
                   const lynx::devtool::InspectorStyleSheet& style_sheet,
                   bool enable_css_selector);

void ReplaceDefaultComputedStyle(
    std::unordered_map<std::string, std::string>& dict,
    const std::unordered_multimap<
        std::string, lynx::devtool::CSSPropertyDetail>& css_attrs_map);

std::string NormalizeAnimationString(const std::string& animation_str);

enum class ParserState {
  BEGIN,
  NAME,
  VALUE,
  ANNOTATION_NAME,
  ANNOTATION_VALUE,
  END,
};

std::string StripSpace(const std::string& str);

bool IsSpace(char letter);

bool IsAnimationNameLegal(std::shared_ptr<InspectorElement> ptr,
                          std::string name);

bool IsAnimationNameLegal(lynx::tasm::Element* ptr, std::string name);

bool IsAnimationValueLegal(std::shared_ptr<InspectorElement> ptr,
                           std::string animation_value);

bool IsAnimationValueLegal(lynx::tasm::Element* ptr,
                           std::string animation_value);

std::vector<std::string> GetAnimationNames(const std::string& animation_str,
                                           bool is_shorthand);

// This function is used to process the text to generate InspectorStyleSheet
// for example, when we modify style in insepctor page, the text is
// "width:10px;height:10px" of node through this function, we will get
// corresponding InspectorStyleSheet for debugging T ptr T is Element* or
// std::shared_ptr<InspectorElement>
template <typename T>
InspectorStyleSheet StyleTextParser(T ptr, std::string text,
                                    const InspectorStyleSheet& pre_style) {
  text = StripSpace(text);

  ParserState state = ParserState::BEGIN;
  InspectorStyleSheet style_sheet;
  std::unordered_multimap<std::string, lynx::devtool::CSSPropertyDetail>
      temp_map;
  lynx::devtool::CSSPropertyDetail temp_css_property;

  std::string css_text;
  std::string name;
  std::string value;
  std::string property_text;
  bool disabled = false;
  bool implicit = false;

  // process the text, extract each style key:value and save to temp_map
  size_t i = 0;
  while (i < text.size()) {
    switch (state) {
      case ParserState::BEGIN:
        if (text[i] == '/') {
          state = ParserState::ANNOTATION_NAME;
          disabled = true;
          implicit = false;
          property_text += "/* ";
        } else {
          state = ParserState::NAME;
          disabled = false;
          implicit = false;
        }
        break;
      case ParserState::NAME:
        if (text[i] == ':') {
          state = ParserState::VALUE;
          property_text += ":";
        } else if (!IsSpace(text[i])) {
          name += text[i];
          property_text += text[i];
        }
        ++i;
        break;
      case ParserState::VALUE:
        if (text[i] != ';') {
          value += text[i];
          property_text += text[i];
        }
        if (text[i] == ';' || i == text.size() - 1) {
          state = ParserState::END;
          property_text += ";";
        }
        ++i;
        break;
      case ParserState::ANNOTATION_NAME:
        if (text[i] == ':') {
          state = ParserState::ANNOTATION_VALUE;
          property_text += ":";
        } else if (!IsSpace(text[i]) && text[i] != '/' && text[i] != '*') {
          name += text[i];
          property_text += text[i];
        }
        ++i;
        break;
      case ParserState::ANNOTATION_VALUE:
        if (text[i] == '/') {
          state = ParserState::END;
          property_text += "; */";
        } else if (text[i] != '/' && text[i] != '*' && text[i] != ';') {
          value += text[i];
          property_text += text[i];
        }
        ++i;
        break;
      case ParserState::END:
        int length = static_cast<int>(value.length());
        while (length > 1 && IsSpace(value[0])) {
          value = value.substr(1);
          length--;
        }
        if (length == 1 && IsSpace(value[0])) value.clear();
        while (length != 0 && IsSpace(value[length - 1])) {
          value = value.substr(0, length - 1);
          length--;
        }

        temp_css_property.name_ = name;
        temp_css_property.value_ = value;
        temp_css_property.text_ = property_text;
        temp_css_property.disabled_ = disabled;
        temp_css_property.implicit_ = implicit;
        temp_css_property.looped_ = false;
        style_sheet.property_order_.push_back(name);
        temp_map.insert(std::make_pair(name, temp_css_property));
        css_text += temp_css_property.text_;
        name.clear();
        value.clear();
        property_text.clear();
        state = ParserState::BEGIN;
    }
  }
  if (!name.empty() || !value.empty()) {
    size_t length = value.length();
    while (length > 1 && IsSpace(value[0])) {
      value = value.substr(1);
      length--;
    }
    if (length == 1 && IsSpace(value[0])) value.clear();
    while (length != 0 && value[length - 1] == ' ') {
      value = value.substr(0, length - 1);
      length--;
    }

    temp_css_property.name_ = name;
    temp_css_property.value_ = value;
    temp_css_property.text_ = property_text;
    temp_css_property.disabled_ = disabled;
    temp_css_property.implicit_ = implicit;
    temp_css_property.looped_ = false;
    style_sheet.property_order_.push_back(name);
    temp_map.insert(std::make_pair(name, temp_css_property));
    css_text += temp_css_property.text_;
  }

  // check IsAnimationLegal
  for (auto& pair : temp_map) {
    if (pair.first.find("animation") != std::string::npos) {
      std::string animation_name = "";
      if (pair.first == "animation-name") {
        animation_name = pair.second.value_;
      } else if (pair.first == "animation") {
        auto animation_value = pair.second.value_;
        animation_name = animation_value.substr(0, animation_value.find(" "));
      } else {
        pair.second.parsed_ok_ = InspectorCSSHelper::IsAnimationLegal(
            pair.second.name_, pair.second.value_);
        continue;
      }

      bool legal = IsAnimationNameLegal(ptr, animation_name);
      if (!legal)
        pair.second.parsed_ok_ = false;
      else if ((pair.first == "animation" &&
                IsAnimationValueLegal(ptr, pair.second.value_)) ||
               pair.first == "animation-name")
        pair.second.parsed_ok_ = true;
      else
        pair.second.parsed_ok_ = false;
    } else
      pair.second.parsed_ok_ =
          InspectorCSSHelper::IsLegal(pair.second.name_, pair.second.value_);
  }

  style_sheet.css_text_ = css_text;
  style_sheet.css_properties_ = temp_map;
  style_sheet.empty = false;
  style_sheet.origin_ = pre_style.origin_;
  style_sheet.style_name_ = pre_style.style_name_;
  style_sheet.style_sheet_id_ = pre_style.style_sheet_id_;
  style_sheet.style_name_range_ = pre_style.style_name_range_;
  style_sheet.style_value_range_.start_line_ =
      style_sheet.style_name_range_.start_line_;
  style_sheet.style_value_range_.end_line_ =
      style_sheet.style_name_range_.end_line_;
  style_sheet.style_value_range_.start_column_ =
      style_sheet.style_name_range_.end_column_ + 1;
  style_sheet.position_ = pre_style.position_;

  int property_start_column = style_sheet.style_value_range_.start_column_;
  for (auto& item : style_sheet.css_properties_) item.second.looped_ = false;
  for (const auto& name : style_sheet.property_order_) {
    auto iter_range = style_sheet.css_properties_.equal_range(name);
    for (auto it = iter_range.first; it != iter_range.second; ++it) {
      if (it->second.looped_) continue;
      it->second.looped_ = true;
      it->second.property_range_.start_line_ =
          style_sheet.style_value_range_.start_line_;
      it->second.property_range_.end_line_ =
          style_sheet.style_value_range_.end_line_;
      it->second.property_range_.start_column_ = property_start_column;
      it->second.property_range_.end_column_ =
          property_start_column + static_cast<int>(it->second.text_.size());
      property_start_column = it->second.property_range_.end_column_;
      break;
    }
  }
  style_sheet.style_value_range_.end_column_ = property_start_column;
  return style_sheet;
}
enum class TagParserState {
  TAG_BEGIN,
  TAG_NAME,
  ATTR_NAME,
  ATTR_VALUE,
  TAG_END,
};

}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_LYNX_DEVTOOL_ELEMENT_HELPER_UTIL_H_
