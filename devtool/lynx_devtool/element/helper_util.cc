// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/element/helper_util.h"

namespace lynx {
namespace devtool {

constexpr const char* kAnimationTimingFunctions[] = {
    "linear", "ease-in", "ease-out", "ease", "square-bezier", "cubic-bezier"};

constexpr const char* kAnimationDirections[] = {
    "normal", "reverse", "alternate", "alternate-reverse"};

constexpr const char* kAnimationFillModes[] = {"none", "forwards", "backwards",
                                               "both"};

constexpr const char* kAnimationPlayStates[] = {"paused", "running"};

std::string ConvertLepusValueToJsonValue(
    const lynx::lepus::Value& lepus_value) {
  std::string result = "";
  switch (lepus_value.Type()) {
    case lynx::lepus::ValueType::Value_Nil: {
      result = "null";
      break;
    }
    case lynx::lepus::ValueType::Value_String: {
      std::string value_string(lepus_value.CString());
      result = '"' + value_string + '"';
      break;
    }
    case lynx::lepus::ValueType::Value_Bool: {
      bool value_bool = lepus_value.Bool();
      result = value_bool ? "1" : "0";
      break;
    }
    case lynx::lepus::ValueType::Value_Int32: {
      result = std::to_string(lepus_value.Int32());
      break;
    }
    case lynx::lepus::ValueType::Value_Int64: {
      result = std::to_string(lepus_value.Int64());
      break;
    }
    case lynx::lepus::ValueType::Value_UInt64: {
      result = std::to_string(lepus_value.UInt64());
      break;
    }
    case lynx::lepus::ValueType::Value_UInt32: {
      result = std::to_string(lepus_value.UInt32());
      break;
    }
    case lynx::lepus::ValueType::Value_Double: {
      result = std::to_string(lepus_value.Number());
      break;
    }
    case lynx::lepus::ValueType::Value_Table: {
      std::string table_string =
          convertLepusTableToDictionaryString(lepus_value);
      if (table_string[table_string.length() - 1] == ',')
        table_string.erase(table_string.length() - 1);
      result = "{" + table_string + "}";
      break;
    }
    default:
      break;
  }
  return result;
}

std::string convertLepusTableToDictionaryString(
    const lynx::lepus::Value& lepus_value) {
  std::string result = "";
  auto table = lepus_value.Table();
  for (auto iter = table->begin(); iter != table->end(); iter++) {
    auto c_str = iter->first.c_str();
    std::string key(c_str);
    std::string val = ConvertLepusValueToJsonValue(iter->second);
    result += '"' + key + '"' + ": ";

    result += val + ",";
  }
  return result;
}

std::string ToSeconds(int milliseconds) {
  if (milliseconds == 0) {
    return "0s";
  } else if (milliseconds < 1000) {
    return std::to_string(milliseconds) + "ms";
  }

  auto seconds = std::to_string(milliseconds / 1000.0f);
  seconds.erase(seconds.find_last_not_of('0') + 1, std::string::npos);
  if (seconds[seconds.size() - 1] == '.') {
    seconds.erase(seconds.size() - 1, std::string::npos);
  }
  return seconds + "s";
}

std::string NormalizeSingleAnimationString(std::string& normalized_str,
                                           const Json::Value& animation_data) {
  std::string id = std::to_string(lynx::tasm::kPropertyIDAnimationName);
  if (animation_data.isMember(id)) {
    normalized_str += animation_data[id].asString();
  }

  id = std::to_string(lynx::tasm::kPropertyIDAnimationDuration);
  if (animation_data.isMember(id)) {
    normalized_str += " ";
    normalized_str += ToSeconds(animation_data[id].asInt());
  }

  id = std::to_string(lynx::tasm::kPropertyIDAnimationTimingFunction);
  if (animation_data.isMember(id)) {
    for (const auto& func : animation_data[id]) {
      normalized_str += " ";
      if (func.isArray()) {
        // e.g. cubic-bezier(.35, .75, 0, 1) -> [5, 0.35, 0.75, 0, 1]
        normalized_str += kAnimationTimingFunctions[func[0].asInt()];
        normalized_str += "(";
        for (Json::Value::ArrayIndex i = 1; i < func.size(); i++) {
          auto n = std::to_string(func[i].asDouble());
          n.erase(n.find_last_not_of('0') + 1, std::string::npos);
          if (n[n.size() - 1] == '.') {
            n.erase(n.size() - 1, std::string::npos);
          }
          normalized_str += n + ",";
        }
        normalized_str[normalized_str.size() - 1] = ')';
      } else {
        normalized_str += kAnimationTimingFunctions[func.asInt()];
      }
    }
  }

  id = std::to_string(lynx::tasm::kPropertyIDAnimationDelay);
  if (animation_data.isMember(id)) {
    normalized_str += " ";
    normalized_str += ToSeconds(animation_data[id].asInt());
  }

  id = std::to_string(lynx::tasm::kPropertyIDAnimationIterationCount);
  if (animation_data.isMember(id)) {
    normalized_str += " ";
    auto count = animation_data[id].asInt();
    normalized_str += (count == 1E9 ? "infinite" : std::to_string(count));
  }

  id = std::to_string(lynx::tasm::kPropertyIDAnimationDirection);
  if (animation_data.isMember(id)) {
    normalized_str += " ";
    normalized_str += kAnimationDirections[animation_data[id].asInt()];
  }

  id = std::to_string(lynx::tasm::kPropertyIDAnimationFillMode);
  if (animation_data.isMember(id)) {
    normalized_str += " ";
    normalized_str += kAnimationFillModes[animation_data[id].asInt()];
  }

  id = std::to_string(lynx::tasm::kPropertyIDAnimationPlayState);
  if (animation_data.isMember(id)) {
    normalized_str += " ";
    normalized_str += kAnimationPlayStates[animation_data[id].asInt()];
  }

  return normalized_str;
}

std::string NormalizeAnimationString(const std::string& animation_str) {
  Json::Reader reader;
  Json::Value animation_data;
  reader.parse(animation_str, animation_data);
  std::string normalized_str = "";
  if (animation_data.isArray()) {
    for (auto& data : animation_data) {
      if (normalized_str != "") {
        normalized_str += ", ";
      }
      NormalizeSingleAnimationString(normalized_str, data);
    }
  } else {
    NormalizeSingleAnimationString(normalized_str, animation_data);
  }
  return normalized_str;
}

std::vector<std::string> GetAnimationNames(const std::string& value,
                                           bool is_shorthand) {
  std::vector<std::string> animation_names;
  if (!is_shorthand) {
    if (!value.size()) return animation_names;

    // check animation is single/multiple
    // eg:
    // for single name, value is "rotateZ-ani"
    // for multiple names, value is "["rotateZ-ani","translateY-ani"]"
    if (value[0] != '[') {
      animation_names.push_back(value);
    } else {
      std::string temp = value.substr(1, value.size() - 2);
      std::istringstream ss(temp);
      std::string token;
      while (std::getline(ss, token, ',')) {
        animation_names.push_back(token.substr(1, token.size() - 2));
      }
    }
    return animation_names;
  }

  Json::Reader reader;
  Json::Value animation_data;
  reader.parse(value, animation_data);
  std::string id = std::to_string(lynx::tasm::kPropertyIDAnimationName);
  if (animation_data.isArray()) {
    for (auto&& item : animation_data) {
      if (item.isMember(id)) {
        animation_names.push_back(item[id].asString());
      }
    }
  } else {
    if (animation_data.isMember(id)) {
      animation_names.push_back(animation_data[id].asString());
    }
  }
  return animation_names;
}

void MergeCSSStyle(Json::Value& res,
                   const lynx::devtool::InspectorStyleSheet& style_sheet,
                   bool enable_css_selector) {
  if (style_sheet.empty) {
    return;
  }

  // handling cascading attributes name
  std::string style_name = style_sheet.style_name_;
  if (!enable_css_selector) {
    std::size_t dot_index = style_name.find_last_of(".");
    std::size_t id_index = style_name.find_last_of("#");
    if (dot_index != std::string::npos && dot_index != 0) {
      std::string child_class = style_name.substr(0, dot_index);
      std::string parent_class = style_name.substr(dot_index);
      style_name = parent_class + " " + child_class;
    } else if (id_index != std::string::npos && id_index != 0) {
      std::string child_class = style_name.substr(0, id_index);
      std::string parent_class = style_name.substr(id_index);
      style_name = parent_class + " " + child_class;
    }
  }

  Json::Value matchedCSSRule(Json::ValueType::objectValue);
  matchedCSSRule["matchingSelectors"] =
      Json::Value(Json::ValueType::arrayValue);
  matchedCSSRule["matchingSelectors"].append(0);
  Json::Value rule(Json::ValueType::objectValue);
  rule["media"] = Json::ValueType::arrayValue;
  rule["origin"] = "regular";
  rule["selectorList"] = Json::Value(Json::ValueType::objectValue);
  rule["selectorList"]["text"] = style_name;
  rule["selectorList"]["selectors"] = Json::Value(Json::ValueType::arrayValue);
  Json::Value text(Json::ValueType::objectValue);
  text["text"] = style_name;
  text["range"] = Json::ValueType::objectValue;
  text["range"]["startLine"] = style_sheet.style_name_range_.start_line_;
  text["range"]["startColumn"] = style_sheet.style_name_range_.start_column_;
  text["range"]["endLine"] = style_sheet.style_name_range_.end_line_;
  text["range"]["endColumn"] = style_sheet.style_name_range_.end_column_;
  rule["selectorList"]["selectors"].append(text);
  Json::Value style(Json::ValueType::objectValue);
  Json::Value temp(Json::ValueType::objectValue);
  style["styleSheetId"] = style_sheet.style_sheet_id_;
  style["cssProperties"] = Json::Value(Json::ValueType::arrayValue);
  style["shorthandEntries"] = Json::Value(Json::ValueType::arrayValue);
  style["range"] = Json::ValueType::objectValue;
  style["range"]["startLine"] = style_sheet.style_value_range_.start_line_;
  style["range"]["endLine"] = style_sheet.style_value_range_.start_line_;
  style["range"]["startColumn"] = style_sheet.style_value_range_.start_column_;
  style["range"]["endColumn"] = style_sheet.style_value_range_.end_column_;
  auto& css_properties =
      const_cast<lynx::devtool::InspectorStyleSheet&>(style_sheet)
          .css_properties_;
  for (auto& item : css_properties) item.second.looped_ = false;
  for (const auto& name : style_sheet.property_order_) {
    auto iter_range = css_properties.equal_range(name);
    for (auto it = iter_range.first; it != iter_range.second; ++it) {
      if (it->second.looped_) continue;
      CSSPropertyDetail& css_property_detail = it->second;
      css_property_detail.looped_ = true;
      temp["name"] = name;
      if (name == "animation") {
        temp["value"] = NormalizeAnimationString(css_property_detail.value_);
      } else {
        temp["value"] = css_property_detail.value_;
      }
      temp["implicit"] = css_property_detail.implicit_;
      temp["disabled"] = css_property_detail.disabled_;
      temp["parsedOk"] = css_property_detail.parsed_ok_;
      temp["text"] = css_property_detail.text_;
      temp["range"] = Json::ValueType::objectValue;
      temp["range"]["startLine"] =
          css_property_detail.property_range_.start_line_;
      temp["range"]["startColumn"] =
          css_property_detail.property_range_.start_column_;
      temp["range"]["endLine"] = css_property_detail.property_range_.end_line_;
      temp["range"]["endColumn"] =
          css_property_detail.property_range_.end_column_;
      style["cssProperties"].append(temp);
      break;
    }
  }
  style["cssText"] = style_sheet.css_text_;
  rule["style"] = style;
  rule["styleSheetId"] = style_sheet.style_sheet_id_;
  matchedCSSRule["rule"] = rule;
  res.append(matchedCSSRule);
}

void ReplaceDefaultComputedStyle(
    std::unordered_map<std::string, std::string>& dict,
    const std::unordered_multimap<
        std::string, lynx::devtool::CSSPropertyDetail>& css_attrs_map) {
  for (const auto& p : css_attrs_map) {
    if (!p.second.disabled_ && p.second.parsed_ok_) {
      auto name = p.first;
      auto value = p.second.value_;
      dict[name] = value;
    }
  }
  return;
}

std::string StripSpace(const std::string& str) {
  std::string::size_type first = str.find_first_not_of(" \t\r\n");
  std::string::size_type last = str.find_last_not_of(" \t\r\n");
  if (first == std::string::npos || last == std::string::npos) {
    return std::string();
  } else {
    return str.substr(first, last - first + 1);
  }
}

bool IsSpace(char letter) {
  return letter == ' ' || letter == '\t' || letter == '\r' || letter == '\n';
}

}  // namespace devtool
}  // namespace lynx
