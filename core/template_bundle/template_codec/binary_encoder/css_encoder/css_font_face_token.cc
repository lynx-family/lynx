// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/template_bundle/template_codec/binary_encoder/css_encoder/css_font_face_token.h"

#include "core/renderer/utils/value_utils.h"

namespace lynx {
namespace tasm {

constexpr const static char* TYPE = "type";
constexpr const static char* FONTFACE_RULE = "FontFaceRule";
constexpr const static char* STYLE = "style";
constexpr const static char* FONT_FAMILY = "font-family";

/*
 * {
 *  "type":"FontFaceRule",
 *  "style": [
 *    {
 *      "name": "font-family",
 *      "value": "\"Bitstream Vera Serif Bold\"",
 *      "keyLoc": {
 *        "column": 12,
 *        "line": 7
 *      },
 *      "valLoc": {
 *        "column": 19,
 *        "line": 7
 *      },
 *    }
 *  ]
 * }
 */
// parse for mini token

inline static std::string _innerTrTrim(const std::string& str) {
  static const std::string chs = "' \t\v\r\n\"";
  size_t first = str.find_first_not_of(chs);
  size_t last = str.find_last_not_of(chs);
  if (first == std::string::npos || last == std::string::npos) {
    return "";
  }
  return str.substr(first, (last - first + 1));
}

bool CSSFontFaceToken::IsCSSFontFaceToken(const rapidjson::Value& value) {
  return value.HasMember(TYPE) &&
         std::string(FONTFACE_RULE).compare(value[TYPE].GetString()) == 0;
}

std::string CSSFontFaceToken::GetCSSFontFaceTokenKey(
    const rapidjson::Value& value) {
  constexpr static const char* kName = "name";
  constexpr static const char* kValue = "value";
  if (value.HasMember(TYPE) &&
      std::string(FONTFACE_RULE).compare(value[TYPE].GetString()) == 0) {
    if (value.HasMember(STYLE)) {
      const rapidjson::Value& style_value = value[STYLE];
      if (style_value.IsObject() && style_value.HasMember(FONT_FAMILY)) {
        return style_value[FONT_FAMILY][kValue].GetString();
      } else if (style_value.IsArray()) {
        for (const auto& attribute : style_value.GetArray()) {
          if (attribute.IsObject() && attribute.HasMember(kName) &&
              attribute.HasMember(kValue)) {
            if (std::string(FONT_FAMILY)
                    .compare(attribute[kName].GetString()) == 0) {
              return attribute[kValue].GetString();
            }
          }
        }
      }
    }
  }

  return "";
}

CSSFontFaceToken::CSSFontFaceToken(const rapidjson::Value& value,
                                   const std::string& file)
    : file_(file) {
  font_family_ = GetCSSFontFaceTokenKey(value);
  if (font_family_.empty()) {
    return;
  }
  const rapidjson::Value& style_value = value[STYLE];
  auto iterate = [this](const rapidjson::Value& name,
                        const rapidjson::Value& value) {
    attrs_[name.GetString()] = value["value"].GetString();
  };

  if (style_value.IsObject()) {
    for (auto itr = style_value.MemberBegin(); itr != style_value.MemberEnd();
         ++itr) {
      iterate(itr->name, itr->value);
    }
  } else if (style_value.IsArray()) {
    for (const auto& value : style_value.GetArray()) {
      iterate(value["name"], value);
    }
  }
}

CSSFontFaceToken::CSSFontFaceToken(const lepus::Value& value) {
  tasm::ForEachLepusValue(value,
                          [this](const lepus::Value& k, const lepus::Value& v) {
                            if (k.IsString() && v.IsString()) {
                              AddAttribute(k.StdString(), v.StdString());
                            }
                          });
}

void CSSFontFaceToken::AddAttribute(const std::string& name,
                                    const std::string& val) {
  std::string newName = _innerTrTrim(name);
  std::string newVal = _innerTrTrim(val);
  if (name == FONT_FAMILY) {
    font_family_ = newVal;
  }
  attrs_[newName] = newVal;
}

}  // namespace tasm
}  // namespace lynx
