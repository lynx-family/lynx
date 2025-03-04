// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/css/css_font_face_token.h"

#include "core/renderer/utils/value_utils.h"
#include "core/runtime/vm/lepus/table.h"

constexpr const static char* FONT_FAMILY = "font-family";

namespace lynx {
namespace tasm {

inline static std::string _innerTrTrim(const std::string& str) {
  static const std::string chs = "' \t\v\r\n\"";
  size_t first = str.find_first_not_of(chs);
  size_t last = str.find_last_not_of(chs);
  if (first == std::string::npos || last == std::string::npos) {
    return "";
  }
  return str.substr(first, (last - first + 1));
}

CSSFontFaceRule* MakeCSSFontFaceToken(lepus::Value value) {
  CSSFontFaceRule* rule = new CSSFontFaceRule();
  ForEachLepusValue(
      value, [rule](const lepus::Value& k, const lepus::Value& v) {
        if (k.IsString() && v.IsString()) {
          CSSFontTokenAddAttribute(rule, k.StdString(), v.StdString());
        }
      });
  return rule;
}

void CSSFontTokenAddAttribute(CSSFontFaceRule* rule, const std::string& name,
                              const std::string& val) {
  std::string newName = _innerTrTrim(name);
  std::string newVal = _innerTrTrim(val);
  if (name == FONT_FAMILY) {
    rule->first = newVal;
  }
  rule->second[std::move(newName)] = std::move(newVal);
}

}  // namespace tasm
}  // namespace lynx
