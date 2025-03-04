// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_BINARY_ENCODER_CSS_ENCODER_CSS_FONT_FACE_TOKEN_H_
#define CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_BINARY_ENCODER_CSS_ENCODER_CSS_FONT_FACE_TOKEN_H_

#include <string>
#include <unordered_map>
#include <utility>

#include "core/renderer/css/css_font_face_token.h"
#include "core/renderer/css/css_parser_token.h"

namespace lynx {
namespace tasm {

// TODO(songshourui.null): Subsequently, `tasm::CSSFontFaceToken` will be
// renamed to `encoder::CSSFontFaceRuleForEncode`.
class CSSFontFaceToken {
 public:
  static bool IsCSSFontFaceToken(const rapidjson::Value& value);
  static std::string GetCSSFontFaceTokenKey(const rapidjson::Value& value);

  CSSFontFaceToken(const rapidjson::Value& value, const std::string& file);
  CSSFontFaceToken(const lepus::Value& value);
  ~CSSFontFaceToken() = default;

  void AddAttribute(const std::string& name, const std::string& val);
  const std::string& GetKey() const { return font_family_; }
  const CSSFontFaceAttrsMap& GetAttrMap() const { return attrs_; }

 private:
  CSSFontFaceToken() = default;

  std::string file_;
  std::string font_family_;
  CSSFontFaceAttrsMap attrs_;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_BINARY_ENCODER_CSS_ENCODER_CSS_FONT_FACE_TOKEN_H_
