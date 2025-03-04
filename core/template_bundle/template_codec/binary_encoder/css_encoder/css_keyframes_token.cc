// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/template_bundle/template_codec/binary_encoder/css_encoder/css_keyframes_token.h"

#include <utility>

#include "core/renderer/css/unit_handler.h"
#include "core/renderer/tasm/config.h"
#include "core/runtime/vm/lepus/exception.h"
#include "core/runtime/vm/lepus/json_parser.h"
#include "third_party/rapidjson/document.h"
#include "third_party/rapidjson/rapidjson.h"

constexpr const static char* kType = "type";
constexpr const static char* kKeyframesRule = "KeyframesRule";
constexpr const static char* kName = "name";
constexpr const static char* kValue = "value";
constexpr const static char* kStyles = "styles";
constexpr const static char* kKeyText = "keyText";
constexpr const static char* kStyle = "style";

/*
 *      {
 *           "type": "KeyframesRule",
 *           "name": "myAnimation",
 *           "styles": [
 *               {
 *                   "keyText": "from",
 *                   "style": [
 *                       {
 *                           "name": "top",
 *                           "value": "0px"
 *                       }
 *                   ]
 *               },
 *              {
 *                   "keyText": "to",
 *                   "style": [
 *                       {
 *                           "name": "top",
 *                           "value": "200px"
 *                       }
 *                   ]
 *               }
 *           ]
 *       }
 */
// parse for mini token

namespace lynx {
namespace encoder {

CSSKeyframesToken::CSSKeyframesToken(
    const rapidjson::Value& value, const std::string& file,
    const tasm::CompileOptions& compile_options)
    : tasm::CSSKeyframesToken(tasm::CSSParserConfigs()),
      file_(file),
      compile_options_(compile_options) {
  if (value.HasMember(kType) &&
      std::string(kKeyframesRule).compare(value[kType].GetString()) == 0) {
    if (value.HasMember(kStyles)) {
      ParseStyles(value[kStyles]);
    }
  }
}

std::string CSSKeyframesToken::GetCSSKeyframesTokenName(
    const rapidjson::Value& value) {
  if (value.HasMember(kType) &&
      std::string(kKeyframesRule).compare(value[kType].GetString()) == 0) {
    if (value.HasMember(kName)) {
      return value[kName][kValue].GetString();
    }
  }
  return "";
}

bool CSSKeyframesToken::IsCSSKeyframesToken(const rapidjson::Value& value) {
  return value.HasMember(kType) &&
         std::string(kKeyframesRule).compare(value[kType].GetString()) == 0;
}

void CSSKeyframesToken::ParseStyles(const rapidjson::Value& value) {
  for (rapid_value::ConstValueIterator itr = value.Begin(); itr != value.End();
       ++itr) {
    std::string key_text = (*itr)[kKeyText][kValue].GetString();
    std::shared_ptr<tasm::StyleMap> css_map(new tasm::StyleMap());
    ConvertToCSSAttrsMap((*itr)[kStyle], *css_map);
    styles_.insert(std::pair<std::string, std::shared_ptr<tasm::StyleMap>>(
        key_text, css_map));
  }
}

void CSSKeyframesToken::ConvertToCSSAttrsMap(const rapidjson::Value& value,
                                             tasm::StyleMap& css_map) {
  auto iterate = [this, &css_map](const rapidjson::Value& name,
                                  const rapidjson::Value& value) {
    tasm::CSSPropertyID id = tasm::CSSProperty::GetPropertyID(name.GetString());
    lepus::Value css_value = lepus::jsonValueTolepusValue(value[kValue]);
    tasm::CSSValueType type = tasm::CSSValueType::DEFAULT;
    if (value[kType] == "css_var") {
      type = tasm::CSSValueType::VARIABLE;
    }
    base::String default_value;
    if (value.HasMember("defaultValue")) {
      default_value = value["defaultValue"].GetString();
    }
    if (!tasm::CSSProperty::IsPropertyValid(id)) {
      std::stringstream error;
      error << "Error In CSSParse: \"" << name.GetString()
            << "\" is not supported now !";

      const rapidjson::Value& loc = value[rapidjson::Value("keyLoc")];
      throw lepus::ParseException(error.str().c_str(), file_.c_str(), loc);
    }
    if (tasm::Config::IsHigherOrEqual(compile_options_.target_sdk_version_,
                                      FEATURE_CSS_STYLE_VARIABLES) &&
        compile_options_.enable_css_variable_ &&
        type == tasm::CSSValueType::VARIABLE) {
      if (tasm::Config::IsHigherOrEqual(compile_options_.target_sdk_version_,
                                        LYNX_VERSION_2_14)) {
        lepus::Value default_value_map;
        if (value.HasMember("defaultValueMap")) {
          default_value_map =
              lepus::jsonValueTolepusValue(value["defaultValueMap"]);
        }
        css_map.insert_or_assign(
            id, tasm::CSSValue(css_value, tasm::CSSValuePattern::STRING,
                               tasm::CSSValueType::VARIABLE, default_value,
                               default_value_map));
      } else {
        css_map.insert_or_assign(
            id, tasm::CSSValue(css_value, tasm::CSSValuePattern::STRING,
                               tasm::CSSValueType::VARIABLE, default_value));
      }
      return;
    }
    if (tasm::Config::IsHigherOrEqual(compile_options_.target_sdk_version_,
                                      FEATURE_CSS_VALUE_VERSION) &&
        compile_options_.enable_css_parser_) {
      auto configs =
          tasm::CSSParserConfigs::GetCSSParserConfigsByComplierOptions(
              compile_options_);
      tasm::UnitHandler::Process(id, css_value, css_map, configs);
    } else {
      css_map.insert_or_assign(id, tasm::CSSValue(css_value));
    }
  };
  if (value.IsObject()) {
    for (auto itr = value.MemberBegin(); itr != value.MemberEnd(); ++itr) {
      iterate(itr->name, itr->value);
    }
  } else if (value.IsArray()) {
    for (const auto& attribute : value.GetArray()) {
      iterate(attribute[kName], attribute);
    }
  }
}

}  // namespace encoder
}  // namespace lynx
