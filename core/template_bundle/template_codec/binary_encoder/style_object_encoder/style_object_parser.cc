// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/template_bundle/template_codec/binary_encoder/style_object_encoder/style_object_parser.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "core/renderer/css/parser/css_parser_configs.h"
#include "core/renderer/css/unit_handler.h"
#include "core/runtime/vm/lepus/json_parser.h"
#include "core/template_bundle/template_codec/binary_encoder/css_encoder/shared_css_fragment.h"

namespace lynx::tasm {

bool StyleObjectParser::Parse(const rapidjson::Value &value) {
  if (!value.IsArray()) {
    return false;
  }
  for (const auto &styleObj : value.GetArray()) {
    if (!styleObj.IsObject()) {
      return false;
    }

    // Keyframes token
    if (encoder::CSSKeyframesToken::IsCSSKeyframesToken(styleObj)) {
      std::string key =
          encoder::CSSKeyframesToken::GetCSSKeyframesTokenName(styleObj);
      if (!key.empty()) {
        fml::RefPtr<encoder::CSSKeyframesToken> token(
            new encoder::CSSKeyframesToken(styleObj, "", compile_options_));
        auto it = style_objects_keyframes_.find(key);
        if (it != style_objects_keyframes_.end()) {
          style_objects_keyframes_.erase(it);
        }
        style_objects_keyframes_.insert({key, token});
      }
      continue;
    }

    // Font-face token
    if (tasm::CSSFontFaceToken::IsCSSFontFaceToken(styleObj)) {
      std::string family =
          tasm::CSSFontFaceToken::GetCSSFontFaceTokenKey(styleObj);
      if (family.empty()) {
        continue;
      }

      std::shared_ptr<CSSFontFaceToken> font_token(
          new tasm::CSSFontFaceToken(styleObj, ""));
      auto it_font_face = style_objects_fontfaces_.find(family);
      if (it_font_face == style_objects_fontfaces_.end()) {
        std::vector<std::shared_ptr<CSSFontFaceToken>> font_face_token_list{
            font_token};
        style_objects_fontfaces_[family] = std::move(font_face_token_list);
      } else {
        it_font_face->second.emplace_back(font_token);
      }
      continue;
    }

    // Style object
    {
      StyleMap style;
      auto configs =
          tasm::CSSParserConfigs::GetCSSParserConfigsByComplierOptions(
              compile_options_);
      for (auto itr = styleObj.MemberBegin(); itr != styleObj.MemberEnd();
           ++itr) {
        const auto &name = itr->name.GetString();
        CSSPropertyID id = CSSProperty::GetPropertyID(name);
        if (!CSSProperty::IsPropertyValid(id)) {
          return false;
        }
        lepus::Value css_value = lepus::jsonValueTolepusValue(itr->value);
        UnitHandler::Process(id, css_value, style, configs);
      }
      style_objects_.emplace_back(std::move(style));
    }
  }
  return true;
}

}  // namespace lynx::tasm
