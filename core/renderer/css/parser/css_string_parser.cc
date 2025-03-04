// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifdef OS_WIN
#define _USE_MATH_DEFINES
#endif

#include "core/renderer/css/parser/css_string_parser.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <string>

#include "base/include/float_comparison.h"
#include "base/include/string/string_number_convert.h"
#include "core/renderer/css/css_color.h"
#include "core/renderer/css/css_value.h"
#include "core/renderer/css/parser/css_string_scanner.h"
#include "core/renderer/css/unit_handler.h"
#include "core/runtime/vm/lepus/array.h"
#include "core/runtime/vm/lepus/table.h"

namespace lynx {
namespace tasm {

namespace {
void ConvertPositionEnumToValue(unsigned int type, float &value,
                                uint32_t &pattern) {
  switch (type) {
    case POS_CENTER:
      value = 50.f;
      pattern = PATTERN_PERCENT;
      return;
    case POS_LEFT:
    case POS_TOP:
      value = 0;
      pattern = PATTERN_PERCENT;
      return;
    case POS_RIGHT:
    case POS_BOTTOM:
      value = 100.f;
      pattern = PATTERN_PERCENT;
      return;
    default:
      pattern = type;
      return;
  }
}

bool PositionAddValue(fml::RefPtr<lepus::CArray> &arr, const CSSValue &value) {
  if (value.IsEmpty()) {
    return false;
  }

  uint32_t pattern = 0;
  float f = 0;
  if (value.IsEnum()) {
    ConvertPositionEnumToValue(value.GetValue().UInt32(), f, pattern);
    arr->emplace_back(f);
    arr->emplace_back(pattern);
  } else {  // Length value
    arr->emplace_back(value.GetValue());
    arr->emplace_back(static_cast<uint32_t>(value.GetPattern()));
  }
  return true;
}

void PositionAddLegacyValue(fml::RefPtr<lepus::CArray> &arr,
                            const CSSValue &pos) {
  // [patten|enum, value]
  if (pos.IsEnum()) {
    arr->emplace_back(pos.GetValue());
    arr->emplace_back(-pos.GetValue().Number());
  } else {  // Length
    arr->emplace_back(static_cast<uint32_t>(pos.GetPattern()));
    arr->emplace_back(pos.GetValue());
  }
}

void SizeAddLegacyValue(fml::RefPtr<lepus::CArray> &arr, const CSSValue &size) {
  // [patten|enum, value]
  arr->emplace_back(static_cast<uint32_t>(size.GetPattern()));
  arr->emplace_back(size.GetValue());
}
}  // namespace

bool CSSStringParser::AtEnd() {
  // Before eof, we allow a semicolon for compatibility.
  Consume(TokenType::SEMICOLON);
  return Check(TokenType::TOKEN_EOF);
}

CSSValue CSSStringParser::ParseBackgroundOrMask(bool mask) {
  Advance();
  auto image_array = lepus::CArray::Create();
  auto position_array = lepus::CArray::Create();
  auto size_array = lepus::CArray::Create();
  auto origin_array = lepus::CArray::Create();
  auto repeat_array = lepus::CArray::Create();
  auto clip_array = lepus::CArray::Create();

  std::optional<uint32_t> color;
  do {
    CSSBackgroundLayer layer;
    bool valid = BackgroundLayer(layer, mask);
    // Must be a valid layer and a color is only allowed in the final layer
    if (!valid || (layer.color.has_value() && !AtEnd())) {
      return CSSValue::Empty();
    }
    color = layer.color;
    // FIXME: If the background layer does not have an image, we should update
    // the current layer as well
    // But for performance we skip the layer, which is different from the web
    if (!layer.image) {
      continue;
    }
    BackgroundLayerToArray(layer, image_array.get(), position_array.get(),
                           size_array.get(), origin_array.get(),
                           repeat_array.get(), clip_array.get());
  } while (!AtEnd() && Consume(TokenType::COMMA));

  auto bg_array = lepus::CArray::Create();
  bg_array->emplace_back(color.has_value() ? *color : 0);
  bg_array->emplace_back(std::move(image_array));
  // Old version parser not handle <position> <size> <repeat> <origin> in
  // shorthand parser
  if (!legacy_parser_) {
    bg_array->emplace_back(std::move(position_array));
    bg_array->emplace_back(std::move(size_array));
    bg_array->emplace_back(std::move(repeat_array));
    bg_array->emplace_back(std::move(origin_array));
    bg_array->emplace_back(std::move(clip_array));
  }

  return CSSValue(std::move(bg_array));
}

CSSValue CSSStringParser::ParseBackgroundImage() {
  Advance();
  auto result = lepus::CArray::Create();
  while (!Check(TokenType::TOKEN_EOF) && !Check(TokenType::SEMICOLON) &&
         !Check(TokenType::ERROR)) {
    if (BackgroundImage()) {
      auto value = PopValue();
      result->emplace_back(TokenTypeToENUM(value.value_type));
      if (value.value) {
        result->emplace_back(std::move(*value.value));
      }
    } else {
      // parse failed
      return CSSValue::Empty();
    }
    if (Consume(TokenType::COMMA)) {
      // ','
      continue;
    }
  }

  if (!AtEnd()) {
    return CSSValue::Empty();
  }

  return CSSValue(std::move(result));
}

std::string CSSStringParser::ParseUrl() {
  Advance();
  std::string result;
  if (!Check(TokenType::TOKEN_EOF) && !Check(TokenType::SEMICOLON)) {
    if (Check(TokenType::URL) && Url()) {
      auto value = PopValue();
      if (value.value) {
        result = value.value->ToString();
      }
    } else {
      // parse failed
      result.clear();
    }
  }
  return result;
}

CSSValue CSSStringParser::ParseLength() {
  CSSValue result;
  ParseLengthTo(result);
  return result;
}

void CSSStringParser::ParseLengthTo(CSSValue &target) {
  Advance();
  LengthTo(target);
  if (!AtEnd()) {
    target = CSSValue::Empty();
  }
}

CSSValue CSSStringParser::ParseSingleBorderRadius() {
  Advance();
  CSSValue radii[2] = {CSSValue::Empty(), CSSValue::Empty()};
  radii[0] = Length();
  if (radii[0].IsEmpty()) {
    return CSSValue::Empty();
  }
  // Single value should not have slash, for compatibility
  Consume(TokenType::SLASH);
  radii[1] = Length();
  if (radii[1].IsEmpty()) {
    radii[1] = radii[0];
  }
  auto array = lepus::CArray::Create();
  array->emplace_back(std::move(radii[0].GetValue()));
  array->emplace_back(static_cast<int>(radii[0].GetPattern()));
  array->emplace_back(std::move(radii[1].GetValue()));
  array->emplace_back(static_cast<int>(radii[1].GetPattern()));
  return CSSValue(std::move(array));
}

static void Complete4Sides(CSSValue side[4]) {
  if (!side[3].IsEmpty()) return;
  if (side[2].IsEmpty()) {
    if (side[1].IsEmpty()) side[1] = side[0];
    side[2] = side[0];
  }
  side[3] = side[1];
}

bool CSSStringParser::ParseBorderRadius(CSSValue horizontal_radii[4],
                                        CSSValue vertical_radii[4]) {
  Advance();
  if (Check(TokenType::ERROR)) {
    return false;
  }
  if (!BorderRadius(horizontal_radii, vertical_radii)) {
    return false;
  }
  if (!AtEnd()) {
    return false;
  }
  Complete4Sides(horizontal_radii);
  Complete4Sides(vertical_radii);
  return true;
}

bool CSSStringParser::BorderRadius(CSSValue horizontal_radii[4],
                                   CSSValue vertical_radii[4]) {
  unsigned horizontal_value_count = 0;
  for (; horizontal_value_count < 4 && !Check(TokenType::SLASH);
       ++horizontal_value_count) {
    CSSValue length_value = Length();
    if (length_value.IsEmpty()) {
      break;
    }
    horizontal_radii[horizontal_value_count] = length_value;
  }
  if (horizontal_radii[0].IsEmpty()) return false;
  if (!CheckAndAdvance(TokenType::SLASH)) {
    Complete4Sides(horizontal_radii);
    for (unsigned i = 0; i < 4; ++i) {
      vertical_radii[i] = horizontal_radii[i];
    }
    return true;
  } else {
    for (unsigned i = 0; i < 4; ++i) {
      CSSValue length_value = Length();
      if (length_value.IsEmpty()) {
        break;
      }
      vertical_radii[i] = length_value;
    }
    if (vertical_radii[0].IsEmpty()) {
      return false;
    }
  }
  return true;
}

CSSValue CSSStringParser::ParseBackgroundPosition() {
  Advance();
  auto result = lepus::CArray::Create();
  do {
    CSSValue pos_x = CSSValue::Empty();
    CSSValue pos_y = CSSValue::Empty();
    if (!BackgroundPosition(pos_x, pos_y)) {
      return CSSValue::Empty();
    }

    auto array = lepus::CArray::Create();
    PositionAddLegacyValue(array, pos_x);
    PositionAddLegacyValue(array, pos_y);
    result->emplace_back(std::move(array));
  } while (Consume(TokenType::COMMA));

  if (!AtEnd()) {
    return CSSValue::Empty();
  }
  return CSSValue(std::move(result));
}

CSSValue CSSStringParser::ParseBackgroundSize() {
  Advance();
  auto result = lepus::CArray::Create();
  do {
    CSSValue size_x = CSSValue::Empty();
    CSSValue size_y = CSSValue::Empty();
    if (!BackgroundSize(size_x, size_y)) {
      return CSSValue::Empty();
    }

    if (legacy_parser_ && size_x.GetValue().Number() == SIZE_AUTO &&
        size_y.GetValue().Number() == SIZE_AUTO) {
      // For compatibility, <auto> <contain> and <cover> is all 100%
      // tailed
      size_x = CSSValue(lepus::Value(100.0), CSSValuePattern::PERCENT);
      size_y = CSSValue(lepus::Value(100.0), CSSValuePattern::PERCENT);
    }

    auto array = lepus::CArray::Create();
    SizeAddLegacyValue(array, size_x);
    SizeAddLegacyValue(array, size_y);
    result->emplace_back(std::move(array));
  } while (Consume(TokenType::COMMA));

  if (!AtEnd()) {
    return CSSValue::Empty();
  }
  return CSSValue(std::move(result));
}

CSSValue CSSStringParser::ParseBackgroundBox() {
  Advance();
  return ConsumeCommaSeparatedList(&CSSStringParser::BackgroundBox);
}

CSSValue CSSStringParser::ParseBackgroundRepeat() {
  Advance();

  auto arr = lepus::CArray::Create();
  do {
    uint32_t repeat_x;
    uint32_t repeat_y;
    if (!BackgroundRepeatStyle(repeat_x, repeat_y)) {
      return CSSValue::Empty();
    }
    auto repeat = lepus::CArray::Create();
    repeat->emplace_back(repeat_x);
    repeat->emplace_back(repeat_y);
    arr->emplace_back(std::move(repeat));
  } while (Consume(TokenType::COMMA));

  if (!AtEnd()) {
    return CSSValue::Empty();
  }
  return CSSValue(std::move(arr));
}

CSSValue CSSStringParser::ParseTextColor() {
  CSSValue result;
  ParseTextColorTo(result);
  return result;
}

CSSValue CSSStringParser::ParseCSSColor() {
  CSSValue result;
  ParseCSSColorTo(result);
  return result;
}

void CSSStringParser::ParseTextColorTo(CSSValue &target) {
  Advance();

  if (Color() || LinearGradient() || RadialGradient()) {
    StackValue stack_value = PopValue();
    if (stack_value.value_type == TokenType::NUMBER) {
      target.SetValueAndPattern(*stack_value.value, CSSValuePattern::NUMBER);
      return;
    }
    auto arr = lepus::CArray::Create();
    arr->emplace_back(TokenTypeToENUM(stack_value.value_type));
    arr->emplace_back(std::move(*stack_value.value));

    // For compatibility, don't check if it's finished
    target.SetArray(std::move(arr));
  } else {
    target = CSSValue::Empty();
  }
}

void CSSStringParser::ParseCSSColorTo(CSSValue &target) {
  Advance();
  ConsumeColor(target);
}

CSSValue CSSStringParser::ParseTextDecoration() {
  Advance();

  auto result = lepus::CArray::Create();
  int flag = 0;
  while (!Check(TokenType::TOKEN_EOF) && !Check(TokenType::SEMICOLON) &&
         !Check(TokenType::ERROR)) {
    int temp_flag = 0;
    if (TextDecorationLine()) {  // text-decoration-line
      auto value = PopValue();
      if (value.value_type == TokenType::NONE) {
        result = lepus::CArray::Create();
        result->emplace_back(TokenTypeToTextENUM(TokenType::NONE));
        return CSSValue(std::move(result));
      }
      result->emplace_back(TokenTypeToTextENUM(value.value_type));
    } else if (TextDecorationStyle()) {  // text-decoration-style
      auto value = PopValue();
      result->emplace_back(TokenTypeToTextENUM(value.value_type));
      temp_flag |= 1 << 1;
    } else if (Color()) {  // text-decoration-color
      StackValue value = PopValue();
      result->emplace_back(
          static_cast<uint32_t>(starlight::TextDecorationType::kColor));
      if (value.value) {
        result->emplace_back(std::move(*value.value));
      }
      temp_flag |= 1 << 2;
    } else {
      return CSSValue(lepus::CArray::Create(), CSSValuePattern::EMPTY);
    }
    if ((temp_flag & flag) != 0) {
      return CSSValue(lepus::CArray::Create(), CSSValuePattern::EMPTY);
    }
    flag |= temp_flag;
  }
  return CSSValue(std::move(result));
}

CSSValue CSSStringParser::ParseFontSrc() {
  Advance();

  auto result = lepus::CArray::Create();

  while (!Check(TokenType::TOKEN_EOF) && !Check(TokenType::SEMICOLON) &&
         !Check(TokenType::ERROR)) {
    bool check_url = false;
    bool check_local = false;
    bool check_format = false;

    if (Url()) {
      auto value = PopValue();
      result->emplace_back(
          static_cast<uint32_t>(starlight::FontFaceSrcType::kUrl));
      result->emplace_back(std::move(*value.value));
      check_url = true;
    }

    if (!check_url && Local()) {
      auto value = PopValue();
      result->emplace_back(
          static_cast<uint32_t>(starlight::FontFaceSrcType::kLocal));
      result->emplace_back(std::move(*value.value));
      check_local = true;
    }

    if (Format()) {
      // Ignore format for now
      PopValue();
      check_format = true;
    }

    if (Consume(TokenType::COMMA)) {
      if (!check_local && !check_url && !check_format) {
        result = lepus::CArray::Create();
        break;
      }

      continue;
    } else if (Consume(TokenType::SEMICOLON)) {
      // we have done
      break;
    } else {
      // any other unexpected token mark failed
      result = lepus::CArray::Create();
      break;
    }
  }

  return CSSValue(std::move(result));
}

CSSValue CSSStringParser::ParseFontWeight() {
  Advance();
  Token token;
  auto result = lepus::CArray::Create();

  while (!Check(TokenType::TOKEN_EOF) && !Check(TokenType::SEMICOLON) &&
         !Check(TokenType::ERROR)) {
    if (Consume(TokenType::NORMAL)) {
      // Normal is just like font-weight: 400
      result->emplace_back(static_cast<int32_t>(400));
    } else if (Consume(TokenType::BOLD)) {
      // Bold is just like font-weight: 700
      result->emplace_back(static_cast<int32_t>(700));
    } else if (ConsumeAndSave(TokenType::NUMBER, token)) {
      auto number = TokenToInt(token);
      // align number with 100
      number += 99;
      number /= 100;
      number *= 100;
      result->emplace_back(static_cast<int32_t>(number));
    } else {
      // unexpected error reset result and return
      result = lepus::CArray::Create();
      break;
    }
  }

  return CSSValue(std::move(result));
}

CSSValue CSSStringParser::ParseFontLength() {
  Advance();
  if (Check(TokenType::TOKEN_EOF)) {
    return CSSValue::Empty();
  }
  auto res = CSSValue::Empty();
  LengthTo(res);
  if (!Check(TokenType::TOKEN_EOF)) {
    return CSSValue::Empty();
  }
  return res;
}

CSSValue CSSStringParser::ParseListGap() {
  Advance();
  if (Check(TokenType::TOKEN_EOF)) {
    return CSSValue::Empty();
  }
  auto res = CSSValue::Empty();
  LengthTo(res);
  auto css_value_pattern = res.GetPattern();
  if (!(css_value_pattern == CSSValuePattern::PX ||
        css_value_pattern == CSSValuePattern::RPX ||
        css_value_pattern == CSSValuePattern::PPX ||
        css_value_pattern == CSSValuePattern::REM ||
        css_value_pattern == CSSValuePattern::EM)) {
    return CSSValue::Empty();
  }
  if (!Check(TokenType::TOKEN_EOF)) {
    return CSSValue::Empty();
  }
  return res;
}

CSSValue CSSStringParser::ParseCursor() {
  Advance();

  Token t1, t2;
  auto result = lepus::CArray::Create();
  while (!Check(TokenType::TOKEN_EOF) && !Check(TokenType::SEMICOLON) &&
         !Check(TokenType::ERROR)) {
    if (Url()) {
      auto value = PopValue();
      result->emplace_back(static_cast<uint32_t>(starlight::CursorType::kUrl));
      auto url = lepus::CArray::Create();
      url->emplace_back(std::move(*value.value));

      if (ConsumeAndSave(TokenType::NUMBER, t1) &&
          ConsumeAndSave(TokenType::NUMBER, t2)) {
        auto x = TokenToDouble(t1);
        auto y = TokenToDouble(t2);
        url->emplace_back(x);
        url->emplace_back(y);
      } else {
        url->emplace_back(0.f);
        url->emplace_back(0.f);
      }
      result->emplace_back(std::move(url));
    } else if (ConsumeAndSave(TokenType::IDENTIFIER, t1)) {
      result->emplace_back(
          static_cast<uint32_t>(starlight::CursorType::kKeyword));
      result->emplace_back(std::string(t1.start, t1.length));
    } else {
      result = lepus::CArray::Create();
      break;
    }
    if (Consume(TokenType::COMMA)) {
      // ','
      continue;
    }
  }
  return CSSValue(std::move(result));
}

lepus::Value CSSStringParser::ParseClipPath() {
  Advance();
  if (BasicShape()) {
    auto shape = PopValue().value;
    return shape.value_or(lepus::Value());
  }
  return lepus::Value();
}

bool CSSStringParser::BasicShape() {
  switch (current_token_.type) {
    case TokenType::CIRCLE:
      return BasicShapeCircle();
    case TokenType::ELLIPSE:
      return BasicShapeEllipse();
    case TokenType::PATH:
      return BasicShapePath();
    case TokenType::SUPER_ELLIPSE:
      return SuperEllipse();
    case TokenType::INSET:
      return BasicShapeInset();
    default:
      return false;
  }
}

CSSValue CSSStringParser::Length() {
  CSSValue result;
  LengthTo(result);
  return result;
}

void CSSStringParser::LengthTo(CSSValue &target) {
  Token token;
  if (!LengthOrPercentageValue(token)) {
    target = CSSValue::Empty();
  } else {
    TokenToLengthTarget(token, target);
  }
}

CSSValue CSSStringParser::TokenToLength(const Token &token) {
  CSSValue result;
  TokenToLengthTarget(token, result);
  return result;
}

void CSSStringParser::TokenToLengthTarget(const Token &token,
                                          CSSValue &css_value) {
  auto pattern = TokenTypeToENUM(token.type);
  if (pattern == static_cast<uint32_t>(CSSValuePattern::CALC) ||
      pattern == static_cast<uint32_t>(CSSValuePattern::ENV) ||
      pattern == static_cast<uint32_t>(CSSValuePattern::INTRINSIC)) {
    css_value.GetValue().SetString(base::String(token.start, token.length));
    css_value.SetPattern(static_cast<CSSValuePattern>(pattern));
  } else if (pattern == static_cast<uint32_t>(CSSValuePattern::ENUM)) {
    // We know the enum pattern is auto
    css_value.GetValue().SetNumber(
        static_cast<int>(starlight::LengthValueType::kAuto));
    css_value.SetPattern(CSSValuePattern::ENUM);
  } else if (pattern < static_cast<uint32_t>(CSSValuePattern::COUNT)) {
    auto dest = TokenToDouble(token);
    css_value.GetValue().SetNumber(dest);
    css_value.SetPattern(static_cast<CSSValuePattern>(pattern));

    // As the FE developer's wish, red screen won't show if no value exists
    // before unit. Only show a red screen when the value is Inf or NaN.
    bool is_normal_number = !(std::isnan(dest) || std::isinf(dest));
    UnitHandler::CSSWarning(is_normal_number,
                            parser_configs_.enable_css_strict_mode,
                            "invalid length: %s", scanner_.content());
  } else {
    css_value = CSSValue::Empty();
  }
}

lepus::Value CSSStringParser::NumberOrPercentage() {
  Token token;
  if (NumberOrPercentValue(token)) {
    float value = TokenToDouble(token);
    if (token.type == TokenType::PERCENTAGE) {
      value /= 100.f;
    }
    return lepus::Value(value);
  }
  return lepus::Value();
}

lepus::Value CSSStringParser::NumberOnly(bool nonnegative) {
  Token token;
  if (NumberValue(token)) {
    double res_value = TokenToDouble(token);
    if (nonnegative && res_value < 0) {
      return lepus::Value();
    }
    return lepus::Value(res_value);
  }
  return lepus::Value();
}

bool CSSStringParser::BackgroundLayer(CSSBackgroundLayer &layer, bool mask) {
  uint8_t full_byte = BG_ORIGIN | BG_CLIP_BOX | BG_IMAGE |
                      BG_POSITION_AND_SIZE | BG_REPEAT | BG_COLOR;

  uint8_t byte = full_byte;

  while (!Check(TokenType::TOKEN_EOF) && !Check(TokenType::SEMICOLON) &&
         !Check(TokenType::COMMA) && !Check(TokenType::ERROR)) {
    uint8_t curr_byte = byte;

    // check origin box first
    if (curr_byte & BG_ORIGIN) {
      auto origin = BackgroundBox();
      if (!origin.IsEmpty()) {
        curr_byte &= ~BG_ORIGIN;
        byte = curr_byte;
        layer.origin = origin.UInt32();
        layer.clip = layer.origin;
        continue;
      }
    } else {
      auto clip = BackgroundBox();
      if (!clip.IsEmpty()) {
        if ((curr_byte & BG_CLIP_BOX) == 0) {
          return false;
        }
        curr_byte &= ~BG_CLIP_BOX;
        byte = curr_byte;
        layer.clip = clip.UInt32();
        continue;
      }
    }

    if (BackgroundImage()) {
      if ((curr_byte & BG_IMAGE) == 0) {
        return false;
      }
      curr_byte &= ~BG_IMAGE;
      byte = curr_byte;
      layer.image = PopValue();
      continue;
    }

    if (!mask && Color()) {
      if ((curr_byte & BG_COLOR) == 0) {
        return false;
      }
      curr_byte &= ~BG_COLOR;
      byte = curr_byte;
      layer.color = PopValue().value->UInt32();
      continue;
    }

    // Add position and size to current background layer
    if (BackgroundPositionAndSize(layer)) {
      if ((curr_byte & BG_POSITION_AND_SIZE) == 0) {
        return false;
      }
      curr_byte &= ~BG_POSITION_AND_SIZE;
      byte = curr_byte;
      continue;
    }

    if (BackgroundRepeatStyle(layer.repeat_x, layer.repeat_y)) {
      if ((curr_byte & BG_REPEAT) == 0) {
        return false;
      }
      curr_byte &= ~BG_REPEAT;
      byte = curr_byte;
      continue;
    }

    if (curr_byte == byte) {
      return false;
    }
  }

  // Found property will return true
  return byte != full_byte;
}

bool CSSStringParser::BasicShapeInset() {
  // Begin with 'inset('
  if (!Consume(TokenType::INSET) || !Consume(TokenType::LEFT_PAREN)) {
    return false;
  }
  auto arr = lepus::CArray::Create();
  arr->emplace_back(static_cast<uint32_t>(starlight::BasicShapeType::kInset));
  CSSValue insets[4] = {CSSValue::Empty(), CSSValue::Empty(), CSSValue::Empty(),
                        CSSValue::Empty()};
  unsigned int length_value_num = 0;
  while (length_value_num < 4 && !Check(TokenType::TOKEN_EOF) &&
         !Check(TokenType::RIGHT_PAREN) && !Check(TokenType::ROUND) &&
         !Check(TokenType::SUPER_ELLIPSE)) {
    insets[length_value_num] = Length();
    if (insets[length_value_num].IsEmpty()) {
      return false;
    }
    length_value_num++;
  }
  // insets should be followed by 'round', 'super-ellipse' (lynx support) or
  // ')'.
  if (!Check(TokenType::RIGHT_PAREN) && !Check(TokenType::ROUND) &&
      !Check(TokenType::SUPER_ELLIPSE)) {
    return false;
  }
  Complete4Sides(insets);
  for (auto &inset : insets) {
    arr->emplace_back(std::move(inset.GetValue()));
    arr->emplace_back(static_cast<uint32_t>(inset.GetPattern()));
  }

  switch (current_token_.type) {
    case TokenType::RIGHT_PAREN: {
      break;
    }
    case TokenType::SUPER_ELLIPSE: {
      Consume(TokenType::SUPER_ELLIPSE);
      Token token;
      if (!ConsumeAndSave(TokenType::NUMBER, token) ||
          !Consume(TokenType::NUMBER)) {
        return false;
      }
      arr->emplace_back(TokenToDouble(token));
      arr->emplace_back(TokenToDouble(previous_token_));
    }
    case TokenType::ROUND: {
      Consume(TokenType::ROUND);
      CSSValue x_radii[4] = {CSSValue::Empty(), CSSValue::Empty(),
                             CSSValue::Empty(), CSSValue::Empty()};
      CSSValue y_radii[4] = {CSSValue::Empty(), CSSValue::Empty(),
                             CSSValue::Empty(), CSSValue::Empty()};
      if (!BorderRadius(x_radii, y_radii)) {
        return false;
      }
      Complete4Sides(x_radii);
      Complete4Sides(y_radii);
      for (int i = 0; i < 4; i++) {
        arr->push_back(x_radii[i].GetValue());
        arr->emplace_back(static_cast<int>(x_radii[i].GetPattern()));
        arr->push_back(y_radii[i].GetValue());
        arr->emplace_back(static_cast<int>(y_radii[i].GetPattern()));
      }
    } break;
    default:
      // error
      return false;
  }
  // not closed with right parenthesis or has other token after ')'.
  if (!Consume(TokenType::RIGHT_PAREN) || !Consume(TokenType::TOKEN_EOF)) {
    return false;
  }
  PushValue(StackValue(TokenType::INSET, std::move(arr)));
  return true;
}

bool CSSStringParser::BackgroundImage() {
  Token token;
  if (ConsumeAndSave(TokenType::NONE, token)) {
    PushValue(StackValue(token.type));
    return true;
  } else if (Check(TokenType::URL)) {
    return Url();
  } else {
    return Gradient();
  }
}

lepus::Value CSSStringParser::BackgroundBox() {
  Token token;
  if (Box(token)) {
    return lepus::Value(TokenTypeToENUM(token.type));
  }
  return lepus::Value();
}

bool CSSStringParser::Box(Token &token) {
  return ConsumeAndSave(TokenType::PADDING_BOX, token) ||
         ConsumeAndSave(TokenType::BORDER_BOX, token) ||
         ConsumeAndSave(TokenType::CONTENT_BOX, token);
}

bool CSSStringParser::BackgroundPositionAndSize(CSSBackgroundLayer &layer) {
  if (BackgroundPosition(layer.position_x, layer.position_y) &&
      !Check(TokenType::COMMA) && !Check(TokenType::SEMICOLON)) {
    // if pass <bg-position> parse and not reach ',' or end of string, try parse
    // <bg-size>
    if (Check(TokenType::SLASH)) {
      return Consume(TokenType::SLASH) &&
             BackgroundSize(layer.size_x, layer.size_y);
    }
  } else {
    return false;
  }
  return true;
}

bool CSSStringParser::ConsumePosition(bool &horizontal_edge,
                                      bool &vertical_edge, CSSValue &ret) {
  Token token;
  if (ConsumeAndSave(TokenType::LEFT, token) ||
      ConsumeAndSave(TokenType::RIGHT, token)) {
    if (horizontal_edge) {
      return false;
    }
    horizontal_edge = true;
    ret = CSSValue(lepus::Value(TokenTypeToENUM(token.type)),
                   CSSValuePattern::ENUM);
    return true;
  } else if (ConsumeAndSave(TokenType::TOP, token) ||
             ConsumeAndSave(TokenType::BOTTOM, token)) {
    if (vertical_edge) {
      return false;
    }
    vertical_edge = true;
    ret = CSSValue(lepus::Value(TokenTypeToENUM(token.type)),
                   CSSValuePattern::ENUM);
    return true;
  } else if (ConsumeAndSave(TokenType::CENTER, token)) {
    ret = CSSValue(lepus::Value(TokenTypeToENUM(token.type)),
                   CSSValuePattern::ENUM);
    return true;
  }
  // Maybe length value, should check if at the end
  ret = Length();
  return true;
}

static bool IsHorizontalPositionKeywordOnly(const CSSValue &value) {
  if (!value.IsEnum()) {
    return false;
  }
  return value.AsNumber() == POS_LEFT || value.AsNumber() == POS_RIGHT;
}

static bool IsVerticalPositionKeywordOnly(const CSSValue &value) {
  if (!value.IsEnum()) {
    return false;
  }
  return value.AsNumber() == POS_TOP || value.AsNumber() == POS_BOTTOM;
}

static void PositionFromOneValue(const CSSValue &value, CSSValue &result_x,
                                 CSSValue &result_y) {
  bool swap_x_y = IsVerticalPositionKeywordOnly(value);
  result_x = value;
  result_y = CSSValue(lepus::Value(POS_CENTER), CSSValuePattern::ENUM);
  if (swap_x_y) {
    std::swap(result_x, result_y);
  }
}

static void PositionFromTwoValues(const CSSValue &value1,
                                  const CSSValue &value2, CSSValue &result_x,
                                  CSSValue &result_y) {
  bool must_order_as_yx = IsVerticalPositionKeywordOnly(value1) ||
                          IsHorizontalPositionKeywordOnly(value2);
  result_x = value1;
  result_y = value2;
  if (must_order_as_yx) {
    std::swap(result_x, result_y);
  }
}

bool CSSStringParser::BackgroundPosition(CSSValue &x, CSSValue &y) {
  bool horizontal_edge = false;
  bool vertical_edge = false;
  CSSValue value1;
  if (!ConsumePosition(horizontal_edge, vertical_edge, value1) ||
      value1.IsEmpty()) {
    return false;
  }
  // Length value
  if (!value1.IsEnum()) {
    horizontal_edge = true;
  }

  if (vertical_edge && !Length().IsEmpty()) {
    // <length-percentage> is not permitted after top | bottom.
    return false;
  }

  // For compatibility, we support comma in transform-origin
  if (enable_transform_legacy_) {
    Consume(TokenType::COMMA);
  }

  CSSValue value2;
  if (!ConsumePosition(horizontal_edge, vertical_edge, value2)) {
    return false;
  }
  if (value2.IsEmpty()) {
    PositionFromOneValue(value1, x, y);
  } else {
    PositionFromTwoValues(value1, value2, x, y);
  }
  return true;
}

bool CSSStringParser::BackgroundSize(CSSValue &x, CSSValue &y) {
  Token token;
  if (ConsumeAndSave(TokenType::COVER, token) ||
      ConsumeAndSave(TokenType::CONTAIN, token)) {
    x = CSSValue(lepus::Value(-1.f * TokenTypeToENUM(token.type)),
                 CSSValuePattern::NUMBER);
    y = CSSValue(lepus::Value(-1.f * TokenTypeToENUM(token.type)),
                 CSSValuePattern::NUMBER);
    return true;
  }

  // check first value
  if (ConsumeAndSave(TokenType::AUTO, token)) {
    x = CSSValue(lepus::Value(SIZE_AUTO), CSSValuePattern::NUMBER);
  } else {
    x = Length();
  }

  if (x.IsEmpty()) {
    return false;
  }

  if (ConsumeAndSave(TokenType::AUTO, token)) {
    y = CSSValue(lepus::Value(SIZE_AUTO), CSSValuePattern::NUMBER);
  } else {
    y = Length();
  }
  if (y.IsEmpty()) {
    y = CSSValue(lepus::Value(SIZE_AUTO), CSSValuePattern::NUMBER);
  }
  return true;
}

bool CSSStringParser::BackgroundRepeatStyle(uint32_t &x, uint32_t &y) {
  Token token;
  if (ConsumeAndSave(TokenType::REPEAT_X, token)     // repeat-x
      || ConsumeAndSave(TokenType::REPEAT_Y, token)  // repeat-y
  ) {
    // make sure no other repeat style follow this two token
    if (Check(TokenType::REPEAT) || Check(TokenType::REPEAT_X) ||
        Check(TokenType::REPEAT_Y) || Check(TokenType::NO_REPEAT) ||
        Check(TokenType::SPACE) || Check(TokenType::ROUND)) {
      return false;
    }
    // repeat-x | repeat-y can only appear once
    x = TokenTypeToENUM(TokenType::REPEAT);
    y = TokenTypeToENUM(TokenType::NO_REPEAT);
    // repeat-y should swap
    if (token.type == TokenType::REPEAT_Y) {
      std::swap(x, y);
    }
    return true;
  }

  if (!ConsumeAndSave(TokenType::REPEAT, token)        // repeat
      && !ConsumeAndSave(TokenType::NO_REPEAT, token)  // no-repeat
      && !ConsumeAndSave(TokenType::SPACE, token)      // space
      && !ConsumeAndSave(TokenType::ROUND, token)      // round
  ) {
    return false;
  }
  x = TokenTypeToENUM(token.type);
  Token second_token;
  // try to check if there is second value
  if (ConsumeAndSave(TokenType::REPEAT, second_token) ||
      ConsumeAndSave(TokenType::NO_REPEAT, second_token) ||
      ConsumeAndSave(TokenType::SPACE, second_token) ||
      ConsumeAndSave(TokenType::ROUND, second_token)) {
    y = TokenTypeToENUM(second_token.type);
  } else {
    y = x;
  }
  return true;
}

bool CSSStringParser::TextDecorationLine() {
  Token token;
  if (ConsumeAndSave(TokenType::NONE, token) ||
      ConsumeAndSave(TokenType::UNDERLINE, token) ||
      ConsumeAndSave(TokenType::LINE_THROUGH, token)) {
    PushValue(StackValue{token.type});
    return true;
  }
  return false;
}

bool CSSStringParser::TextDecorationStyle() {
  Token token;
  if (ConsumeAndSave(TokenType::SOLID, token) ||
      ConsumeAndSave(TokenType::DOUBLE, token) ||
      ConsumeAndSave(TokenType::DOTTED, token) ||
      ConsumeAndSave(TokenType::DASHED, token) ||
      ConsumeAndSave(TokenType::WAVY, token)) {
    PushValue(StackValue(token.type));
    return true;
  }
  return false;
}

bool CSSStringParser::Format() {
  Token format, string;
  if (!ConsumeAndSave(TokenType::FORMAT, format)) {
    return false;
  }

  if (!Consume(TokenType::LEFT_PAREN)) {
    return false;
  }

  if (!ConsumeAndSave(TokenType::STRING, string)) {
    return false;
  }

  if (!Consume(TokenType::RIGHT_PAREN)) {
    return false;
  }
  PushValue(
      StackValue(TokenType::FORMAT, base::String(string.start, string.length)));

  return true;
}

bool CSSStringParser::Local() {
  Token local;
  ConsumeAndSave(TokenType::LOCAL, local);
  if (!Consume(TokenType::LEFT_PAREN)) {  // (
    return false;
  }
  Token string;
  if (ConsumeAndSave(TokenType::STRING, string) &&
      Consume(TokenType::RIGHT_PAREN)) {
    PushValue(StackValue{TokenType::LOCAL,
                         base::String(string.start, string.length)});
    return true;
  }

  if (!Check(TokenType::RIGHT_PAREN)) {
    // may be this <local>(...) with no quotes
    Token virtual_token;
    virtual_token.start = previous_token_.start + previous_token_.length;
    while (!Check(TokenType::RIGHT_PAREN)) {
      if (Check(TokenType::TOKEN_EOF) || Check(TokenType::ERROR)) {
        return false;
      }
      Advance();
    }

    virtual_token.length =
        static_cast<uint32_t>(current_token_.start - virtual_token.start);
    if (!Consume(TokenType::RIGHT_PAREN)) {
      return false;
    }
    PushValue(StackValue{TokenType::LOCAL, base::String(virtual_token.start,
                                                        virtual_token.length)});
    return true;
  }
  return false;
}

bool CSSStringParser::Url() {
  Token url;
  ConsumeAndSave(TokenType::URL, url);
  if (!Consume(TokenType::LEFT_PAREN)) {  // (
    return false;
  }
  Token data;
  if (ConsumeAndSave(TokenType::STRING, data) &&  // <string>
      Consume(TokenType::RIGHT_PAREN)) {          // )
    PushValue(
        StackValue{TokenType::URL, base::String(data.start, data.length)});
    return true;
  }

  if (ConsumeAndSave(TokenType::DATA, data)) {
    while (!Check(TokenType::RIGHT_PAREN)) {
      if (Check(TokenType::TOKEN_EOF) || Check(TokenType::ERROR)) {
        return false;
      }
      Advance();
    }
    data.length = static_cast<uint32_t>(current_token_.start - data.start);
    if (!Consume(TokenType::RIGHT_PAREN)) {
      return false;
    }
    PushValue(
        StackValue{TokenType::URL, base::String(data.start, data.length)});
    return true;
  }

  if (!Check(TokenType::RIGHT_PAREN)) {
    // may be this is <url>(...) with no quotes
    Token virtual_token;
    virtual_token.start = previous_token_.start + previous_token_.length;
    while (!Check(TokenType::RIGHT_PAREN)) {
      if (Check(TokenType::TOKEN_EOF) || Check(TokenType::ERROR)) {
        return false;
      }
      Advance();
    }

    virtual_token.length =
        static_cast<uint32_t>(current_token_.start - virtual_token.start);
    if (!Consume(TokenType::RIGHT_PAREN)) {
      return false;
    }
    PushValue(StackValue{TokenType::URL, base::String(virtual_token.start,
                                                      virtual_token.length)});
    return true;
  }

  return false;
}

bool CSSStringParser::Gradient() {
  if (Check(TokenType::LINEAR_GRADIENT)) {
    return LinearGradient();
  } else if (Check(TokenType::RADIAL_GRADIENT)) {
    return RadialGradient();
  } else {
    return false;
  }
}

bool CSSStringParser::LinearGradient() {
  if (!Consume(TokenType::LINEAR_GRADIENT)) {
    return false;
  }

  if (!Consume(TokenType::LEFT_PAREN)) {  // (
    return false;
  }

  auto side_or_corner = starlight::LinearGradientDirection::kBottom;

  float angle = 180.f;
  if (Check(TokenType::NUMBER) || Check(TokenType::DIMENSION)) {
    Token angle_token;
    if (!AngleValue(angle_token)) {
      return false;
    }
    side_or_corner = starlight::LinearGradientDirection::kAngle;
    angle = TokenToAngleValue(angle_token);
    // ','
    if (!Consume(TokenType::COMMA)) {
      return false;
    }
  } else if (Check(TokenType::TO)) {
    Consume(TokenType::TO);
    if (Consume(TokenType::LEFT)) {
      if (Consume(TokenType::TOP)) {
        angle = 315.f;
        side_or_corner = starlight::LinearGradientDirection::kTopLeft;
      } else if (Consume(TokenType::BOTTOM)) {
        angle = 225.f;
        side_or_corner = starlight::LinearGradientDirection::kBottomLeft;
      } else {
        angle = 270.f;
        side_or_corner = starlight::LinearGradientDirection::kLeft;
      }
    } else if (Consume(TokenType::BOTTOM)) {
      if (Consume(TokenType::LEFT)) {
        angle = 225.f;
        side_or_corner = starlight::LinearGradientDirection::kBottomLeft;
      } else if (Consume(TokenType::RIGHT)) {
        angle = 135.f;
        side_or_corner = starlight::LinearGradientDirection::kBottomRight;
      } else {
        angle = 180.f;
        side_or_corner = starlight::LinearGradientDirection::kBottom;
      }
    } else if (Consume(TokenType::TOP)) {
      if (Consume(TokenType::LEFT)) {
        angle = 315.f;
        side_or_corner = starlight::LinearGradientDirection::kTopLeft;
      } else if (Consume(TokenType::RIGHT)) {
        angle = 45.f;
        side_or_corner = starlight::LinearGradientDirection::kTopRight;
      } else {
        angle = 0.f;
        side_or_corner = starlight::LinearGradientDirection::kTop;
      }
    } else if (Consume(TokenType::RIGHT)) {
      if (Consume(TokenType::TOP)) {
        angle = 45.f;
        side_or_corner = starlight::LinearGradientDirection::kTopRight;
      } else if (Consume(TokenType::BOTTOM)) {
        angle = 135.f;
        side_or_corner = starlight::LinearGradientDirection::kBottomRight;
      } else {
        angle = 90.f;
        side_or_corner = starlight::LinearGradientDirection::kRight;
      }
    } else {
      return false;
    }
    // ','
    if (!Consume(TokenType::COMMA)) {
      return false;
    }
  } else if (Consume(TokenType::TOLEFT)) {
    angle = 270.f;
    side_or_corner = starlight::LinearGradientDirection::kLeft;
    // ','
    if (!Consume(TokenType::COMMA)) {
      return false;
    }
  } else if (Consume(TokenType::TOBOTTOM)) {
    angle = 180.f;
    side_or_corner = starlight::LinearGradientDirection::kBottom;
    // ','
    if (!Consume(TokenType::COMMA)) {
      return false;
    }
  } else if (Consume(TokenType::TOTOP)) {
    side_or_corner = starlight::LinearGradientDirection::kTop;
    angle = 0.f;
    // ','
    if (!Consume(TokenType::COMMA)) {
      return false;
    }
  } else if (Consume(TokenType::TORIGHT)) {
    side_or_corner = starlight::LinearGradientDirection::kRight;
    angle = 90.f;
    // ','
    if (!Consume(TokenType::COMMA)) {
      return false;
    }
  }

  auto color_array = lepus::CArray::Create();
  auto position_array = lepus::CArray::Create();

  if (!ColorStopList(color_array, position_array)) {
    return false;
  }

  if (color_array->size() == 0) {
    return false;
  }

  auto linear_gradient_obj = lepus::CArray::Create();
  linear_gradient_obj->emplace_back(angle);
  linear_gradient_obj->emplace_back(std::move(color_array));
  linear_gradient_obj->emplace_back(std::move(position_array));
  linear_gradient_obj->emplace_back(static_cast<int32_t>(side_or_corner));

  PushValue(
      StackValue(TokenType::LINEAR_GRADIENT, std::move(linear_gradient_obj)));
  return true;
}

bool CSSStringParser::RadialGradient() {
  Consume(TokenType::RADIAL_GRADIENT);
  if (!Consume(TokenType::LEFT_PAREN)) {  // '('
    return false;
  }

  auto color_array = lepus::CArray::Create();
  auto position_array = lepus::CArray::Create();

  uint32_t shape =
      static_cast<uint32_t>(starlight::RadialGradientShapeType::kEllipse);
  uint32_t shape_size =
      static_cast<uint32_t>(starlight::RadialGradientSizeType::kFarthestCorner);

  CSSValue pos_x = CSSValue(lepus::Value(50.f), CSSValuePattern::PERCENT);
  CSSValue pos_y = CSSValue(lepus::Value(50.f), CSSValuePattern::PERCENT);
  CSSValue size_x = CSSValue::Empty();
  CSSValue size_y = CSSValue::Empty();

  bool shape_valid = false;
  bool has_shape = false;
  if (EndingShape()) {
    StackValue value = PopValue();
    shape = TokenTypeToENUM(value.value_type);
    shape_valid = true;
    has_shape = true;
  }
  bool size_keyword = EndingShapeSizeIdent();
  if (size_keyword) {
    StackValue value = PopValue();
    shape_size = TokenTypeToENUM(value.value_type);
    shape_valid = true;
  }

  // Optional size
  size_x = Length();
  if (!size_x.IsEmpty()) {
    shape_valid = true;
    size_y = Length();
  }

  // Invalid value
  if (size_keyword && !size_x.IsEmpty()) {
    return false;
  }

  if (!size_x.IsEmpty()) {
    shape_size =
        static_cast<uint32_t>(starlight::RadialGradientSizeType::kLength);
  }

  // Circles must have 0 or 1 lengths.
  if (has_shape &&
      shape ==
          static_cast<uint32_t>(starlight::RadialGradientShapeType::kCircle) &&
      !size_y.IsEmpty()) {
    return false;
  }

  // Ellipses must have 0 or 2 length/percentages.
  if (has_shape &&
      shape ==
          static_cast<uint32_t>(starlight::RadialGradientShapeType::kEllipse) &&
      !size_x.IsEmpty() && size_y.IsEmpty()) {
    return false;
  }

  if (!size_x.IsEmpty() && size_y.IsEmpty()) {
    shape = static_cast<uint32_t>(starlight::RadialGradientShapeType::kCircle);
    size_y = size_x;
  }

  if (Consume(TokenType::AT)) {
    if (!BackgroundPosition(pos_x, pos_y)) {
      return false;
    }
    shape_valid = true;
  }

  if (shape_valid && !Consume(TokenType::COMMA)) {
    return false;
  }

  if (!ColorStopList(color_array, position_array)) {
    return false;
  }

  auto radial_gradient_obj = lepus::CArray::Create();
  // ending-shape size position
  {
    auto shape_arr = lepus::CArray::Create();
    shape_arr->emplace_back(shape);
    shape_arr->emplace_back(shape_size);
    PositionAddLegacyValue(shape_arr, pos_x);
    PositionAddLegacyValue(shape_arr, pos_y);
    // Has length value: [x_pattern, x_value, y_pattern, y_value]
    if (shape_size ==
        static_cast<uint32_t>(starlight::RadialGradientSizeType::kLength)) {
      shape_arr->emplace_back(static_cast<uint32_t>(size_x.GetPattern()));
      shape_arr->emplace_back(std::move(size_x.GetValue()));
      shape_arr->emplace_back(static_cast<uint32_t>(size_y.GetPattern()));
      shape_arr->emplace_back(std::move(size_y.GetValue()));
    }
    radial_gradient_obj->emplace_back(std::move(shape_arr));
  }
  radial_gradient_obj->emplace_back(std::move(color_array));
  radial_gradient_obj->emplace_back(std::move(position_array));

  PushValue(
      StackValue(TokenType::RADIAL_GRADIENT, std::move(radial_gradient_obj)));

  return true;
}

bool CSSStringParser::EndingShape() {
  if (Consume(TokenType::ELLIPSE)) {
    PushValue(StackValue(TokenType::ELLIPSE));
    return true;
  } else if (Consume(TokenType::CIRCLE)) {
    PushValue(StackValue(TokenType::CIRCLE));
    return true;
  } else {
    return false;
  }
}

bool CSSStringParser::EndingShapeSizeIdent() {
  if (Consume(TokenType::FARTHEST_CORNER)) {
    PushValue(StackValue(TokenType::FARTHEST_CORNER));
    return true;
  } else if (Consume(TokenType::FARTHEST_SIDE)) {
    PushValue(StackValue(TokenType::FARTHEST_SIDE));
    return true;
  } else if (Consume(TokenType::CLOSEST_CORNER)) {
    PushValue(StackValue(TokenType::CLOSEST_CORNER));
    return true;
  } else if (Consume(TokenType::CLOSEST_SIDE)) {
    PushValue(StackValue(TokenType::CLOSEST_SIDE));
    return true;
  } else {
    return false;
  }
}

bool CSSStringParser::ColorStopList(
    const fml::RefPtr<lepus::CArray> &color_array,
    const fml::RefPtr<lepus::CArray> &stop_array) {
  size_t position_begin_index = -1;
  float position_begin_value = 0.f;
  static constexpr size_t kColorStopInlineSize = 16;
  base::InlineVector<uint32_t, kColorStopInlineSize> temp_color_list;
  base::InlineVector<float, kColorStopInlineSize> temp_stop_list;
  while (Color() && !(Check(TokenType::TOKEN_EOF))) {
    auto color_value = PopValue();
    temp_color_list.emplace_back(color_value.value->UInt32());
    if (Check(TokenType::COMMA)) {
      // ',' after color, no position
      if (position_begin_index == static_cast<size_t>(-1)) {
        position_begin_index = temp_color_list.size() - 1;
      }
      Consume(TokenType::COMMA);
      continue;
    }
    if (Check(TokenType::RIGHT_PAREN)) {
      break;
    }
    Token position;
    if (!NumberOrPercentValue(position)) {
      return false;
    }
    float current_stop_position = TokenToDouble(position);
    if (position.type == TokenType::NUMBER) {
      current_stop_position *= 100.f;
    }

    if (position_begin_index != static_cast<size_t>(-1)) {
      // fill empty position with previous stop position and current stop
      // position

      size_t current_index = temp_color_list.size() - 1;
      if (position_begin_index > 0) {
        position_begin_value = temp_stop_list[position_begin_index - 1];
      } else if (position_begin_index == 0) {
        position_begin_index++;
        temp_stop_list.emplace_back(0.f);
      }

      float step = (current_stop_position - position_begin_value) /
                   (current_index - position_begin_index + 1);

      for (size_t j = position_begin_index; j < current_index; j++) {
        temp_stop_list.emplace_back(position_begin_value +
                                    (j - position_begin_index + 1) * step);
      }
    }
    temp_stop_list.emplace_back(current_stop_position);
    // clear position begin index
    position_begin_index = -1;

    if (Check(TokenType::COMMA)) {
      Consume(TokenType::COMMA);
    }
  }

  if (!Consume(TokenType::RIGHT_PAREN)) {
    return false;
  }
  // fill empty position to the end
  int32_t fill_step =
      static_cast<int32_t>(temp_color_list.size() - temp_stop_list.size());
  if (!temp_stop_list.empty() && fill_step > 0) {
    float step_value = (100.f - temp_stop_list.back()) / fill_step;
    float begin_value = temp_stop_list.back();
    for (int32_t i = 1; i < fill_step; i++) {
      temp_stop_list.emplace_back(begin_value + step_value * i);
    }
    temp_stop_list.emplace_back(100.f);
  }
  // clamp color and stop
  ClampColorAndStopList(temp_color_list, temp_stop_list);

  if (temp_color_list.size() < 2 ||
      (!temp_stop_list.empty() &&
       temp_stop_list.size() != temp_color_list.size())) {
    // gradient need at least two colors
    return false;
  }

  for (auto color_value : temp_color_list) {
    color_array->emplace_back(color_value);
  }

  for (auto stop_value : temp_stop_list) {
    stop_array->emplace_back(stop_value);
  }

  return true;
}

bool CSSStringParser::AngleValue(Token &token) {
  if (NumberValue(token)) {
    // For compatibility, we support number without unit in angle value
    return token.IsZero() || enable_transform_legacy_;
  }
  if (DimensionValue(token) &&
      (token.unit == TokenType::DEG || token.unit == TokenType::TURN ||
       token.unit == TokenType::RAD || token.unit == TokenType::GRAD)) {
    token.type = token.unit;
    return true;
  }

  return false;
}

bool CSSStringParser::TimeValue(Token &token) {
  // Time need unit include 0,
  // for compatibility, we support number without unit in time value
  if (enable_time_legacy_ && NumberValue(token)) {
    return true;
  }
  if (DimensionValue(token) && (token.unit == TokenType::SECOND ||
                                token.unit == TokenType::MILLISECOND)) {
    token.type = token.unit;
    return true;
  }

  return false;
}

bool CSSStringParser::TransitionProperty(Token &token) {
  SkipWhitespaceToken();
  // keyword and ident
  if (current_token_.IsIdent()) {
    token = current_token_;
    Advance();
    return true;
  }
  return false;
}

bool CSSStringParser::TimingFunctionValue(Token &token) {
  SkipWhitespaceToken();
  if ((current_token_.type >= TokenType::LINEAR &&
       current_token_.type <= TokenType::STEP_END) ||
      current_token_.type == TokenType::SQUARE_BEZIER ||
      current_token_.type == TokenType::CUBIC_BEZIER ||
      current_token_.type == TokenType::STEPS) {
    token = current_token_;
    Advance();
    return true;
  }

  return false;
}

CSSValue CSSStringParser::ConsumeTimingFunction(
    const Token &token, const CSSParserConfigs &configs) {
  CSSValue css_value = CSSValue::Empty();
  auto type = TokenToTimingFunctionType(token);
  css_value.GetValue().SetNumber(static_cast<int>(type));
  if (token.type >= TokenType::LINEAR & token.type <= TokenType::EASE_IN_OUT) {
    css_value.SetPattern(CSSValuePattern::ENUM);
  } else if (token.type == TokenType::STEP_START ||
             token.type == TokenType::STEP_END) {
    auto arr = lepus::CArray::Create();
    auto step_type = token.type == TokenType::STEP_START
                         ? starlight::StepsType::kStart
                         : starlight::StepsType::kEnd;
    arr->emplace_back(static_cast<int>(type));
    arr->emplace_back(1);
    arr->emplace_back(static_cast<int>(step_type));
    css_value.SetArray(std::move(arr));
  } else if (token.type == TokenType::SQUARE_BEZIER ||
             token.type == TokenType::CUBIC_BEZIER ||
             token.type == TokenType::STEPS) {
    auto arr = lepus::CArray::Create();
    CSSStringParser params_parser{token.start, token.length, configs};
    if (!params_parser.ParseTimingFunctionParams(token, arr)) {
      return CSSValue::Empty();
    }
    css_value.SetArray(std::move(arr));
  }
  return css_value;
}

bool CSSStringParser::ParseTimingFunctionParams(
    const Token &function_token, fml::RefPtr<lepus::CArray> &arr) {
  Advance();
  if (function_token.type == TokenType::SQUARE_BEZIER) {
    arr->emplace_back(
        static_cast<int>(starlight::TimingFunctionType::kSquareBezier));
    Token x, y;
    if (NumberValue(x) && Consume(TokenType::COMMA) && NumberValue(y) &&
        Check(TokenType::TOKEN_EOF)) {
      arr->emplace_back(TokenToDouble(x));
      arr->emplace_back(TokenToDouble(y));
      return true;
    } else {
      return false;
    }
  } else if (function_token.type == TokenType::CUBIC_BEZIER) {
    arr->emplace_back(
        static_cast<int>(starlight::TimingFunctionType::kCubicBezier));
    Token x1, y1, x2, y2;
    if (NumberValue(x1) && Consume(TokenType::COMMA) && NumberValue(y1) &&
        Consume(TokenType::COMMA) && NumberValue(x2) &&
        Consume(TokenType::COMMA) && NumberValue(y2) &&
        Check(TokenType::TOKEN_EOF)) {
      // x1 >= 0 && x1 <= 1
      // x2 >= 0 && x2 <= 1
      arr->emplace_back(TokenToDouble(x1));
      arr->emplace_back(TokenToDouble(y1));
      arr->emplace_back(TokenToDouble(x2));
      arr->emplace_back(TokenToDouble(y2));
      return true;
    } else {
      return false;
    }
  } else if (function_token.type == TokenType::STEPS) {
    arr->emplace_back(static_cast<int>(starlight::TimingFunctionType::kSteps));
    Token t;
    if (!NumberValue(t)) {
      return false;
    }
    arr->emplace_back(static_cast<int>(TokenToInt(t)));

    if (!Consume(TokenType::COMMA)) {
      return false;
    }
    SkipWhitespaceToken();
    std::string s_type_str =
        std::string(current_token_.start, current_token_.length);
    auto s_type = starlight::StepsType::kInvalid;
    if (s_type_str == "start" || s_type_str == "jump-start") {
      s_type = starlight::StepsType::kStart;
    } else if (s_type_str == "end" || s_type_str == "jump-end") {
      s_type = starlight::StepsType::kEnd;
    } else if (s_type_str == "jump-both") {
      s_type = starlight::StepsType::kJumpBoth;
    } else if (s_type_str == "jump-none") {
      s_type = starlight::StepsType::kJumpNone;
    } else {
      return false;
    }
    arr->emplace_back(static_cast<int>(s_type));
    Advance();
    return Check(TokenType::TOKEN_EOF);
  }
  return false;
}

void CSSStringParser::ConsumeBorderLineWidth(Token &token, CSSValue &result) {
  if (BorderWidthIdent(token)) {
    result.GetValue().SetNumber(TokenTypeToBorderWidth(token.type));
    result.SetPattern(CSSValuePattern::PX);
  } else {
    // The next token may be length
    LengthTo(result);
  }
}

bool CSSStringParser::BorderWidthIdent(Token &token) {
  if (ConsumeAndSave(TokenType::THIN, token) ||
      ConsumeAndSave(TokenType::MEDIUM, token) ||
      ConsumeAndSave(TokenType::THICK, token)) {
    return true;
  }
  return false;
}

bool CSSStringParser::BorderStyleIdent(Token &token) {
  SkipWhitespaceToken();
  // hidden, dotted, dashed, solid, double, groove, ridge, inset, outset, none
  if ((current_token_.type >= TokenType::HIDDEN &&
       current_token_.type <= TokenType::OUTSET) ||
      current_token_.type == TokenType::NONE) {
    token = current_token_;
    Advance();
    return true;
  }
  return false;
}

bool CSSStringParser::TransformFunctionIdent(Token &token) {
  SkipWhitespaceToken();
  if ((current_token_.type >= TokenType::ROTATE &&
       current_token_.type <= TokenType::MATRIX_3D)) {
    token = current_token_;
    Advance();
    return true;
  }
  return false;
}

void CSSStringParser::ConsumeColor(CSSValue &result) {
  if (Color()) {
    StackValue stack_value = PopValue();
    if (stack_value.value_type == TokenType::NUMBER) {
      result.SetValueAndPattern(*stack_value.value, CSSValuePattern::NUMBER);
    }
  } else {
    result = CSSValue::Empty();
  }
}

bool CSSStringParser::ShadowOptionIdent(Token &token) {
  if (ConsumeAndSave(TokenType::INSET, token)) {
    return true;
  }
  return false;
}

template <typename Func, typename... Args>
CSSValue CSSStringParser::ConsumeCommaSeparatedList(Func callback,
                                                    Args &&...args) {
  auto list = lepus::CArray::Create();
  do {
    lepus::Value value = (this->*callback)(std::forward<Args>(args)...);
    if (value.IsEmpty()) {
      return CSSValue::Empty();
    }
    list->emplace_back(std::move(value));
  } while (Consume(TokenType::COMMA));
  if (!AtEnd()) {
    return CSSValue::Empty();
  }
  return CSSValue(std::move(list));
}

bool CSSStringParser::Color() {
  if (CheckAndAdvance(TokenType::RGBA)) {
    return RGBAColor();
  } else if (CheckAndAdvance(TokenType::RGB)) {
    return RGBColor();
  } else if (CheckAndAdvance(TokenType::HSLA)) {
    return HSLAColor();
  } else if (CheckAndAdvance(TokenType::HSL)) {
    return HSLColor();
  } else if (Check(TokenType::HEX)) {
    return HexColor();
  } else if (CSSColor::IsColorIdentifier(current_token_.type)) {
    auto color = CSSColor::CreateFromKeyword(current_token_.type);
    PushValue(StackValue(TokenType::NUMBER, color.Cast()));
    Advance();
    return true;
  }
  return false;
}

bool CSSStringParser::RGBAColor() {
  // save RGBA prefix
  Token rgba[5] = {[0] = previous_token_};
  if (Consume(TokenType::LEFT_PAREN)      // (
      && NumberOrPercentValue(rgba[1])    // Number 1
      && Consume(TokenType::COMMA)        // ,
      && NumberOrPercentValue(rgba[2])    // Number 2
      && Consume(TokenType::COMMA)        // ,
      && NumberOrPercentValue(rgba[3])    // Number 3
      && Consume(TokenType::COMMA)        // ,
      && NumberOrPercentValue(rgba[4])    // Alpha
      && Consume(TokenType::RIGHT_PAREN)  // )
  ) {
    PushValue(MakeColorValue(rgba));
    return true;
  } else {
    return false;
  }
}

bool CSSStringParser::RGBColor() {
  Token rgb[4] = {[0] = previous_token_};
  if (Consume(TokenType::LEFT_PAREN)      // (
      && NumberOrPercentValue(rgb[1])     // Number 1
      && Consume(TokenType::COMMA)        // ,
      && NumberOrPercentValue(rgb[2])     // Number 2
      && Consume(TokenType::COMMA)        // ,
      && NumberOrPercentValue(rgb[3])     // Number 3
      && Consume(TokenType::RIGHT_PAREN)  // )
  ) {
    PushValue(MakeColorValue(rgb));
    return true;
  } else {
    return false;
  }
}

bool CSSStringParser::HSLAColor() {
  // save hsla prefix
  Token hsla[5] = {[0] = previous_token_};
  if (Consume(TokenType::LEFT_PAREN)      // (
      && NumberValue(hsla[1])             // hue
      && Consume(TokenType::COMMA)        // ,
      && PercentageValue(hsla[2])         // percentage
      && Consume(TokenType::COMMA)        // ,
      && PercentageValue(hsla[3])         // percentage
      && Consume(TokenType::COMMA)        // ,
      && NumberOrPercentValue(hsla[4])    // alpha
      && Consume(TokenType::RIGHT_PAREN)  // )
  ) {
    PushValue(MakeColorValue(hsla));
    return true;
  } else {
    return false;
  }
}

bool CSSStringParser::HSLColor() {
  Token hsl[4] = {[0] = previous_token_};

  if (Consume(TokenType::LEFT_PAREN)      // (
      && NumberValue(hsl[1])              // hue
      && Consume(TokenType::COMMA)        // ,
      && PercentageValue(hsl[2])          // percentage
      && Consume(TokenType::COMMA)        // ,
      && PercentageValue(hsl[3])          // percentage
      && Consume(TokenType::RIGHT_PAREN)  // )
  ) {
    PushValue(MakeColorValue(hsl));
    return true;
  } else {
    return false;
  }
}

bool CSSStringParser::HexColor() {
  Token hex_token[1];
  // number
  if (HexValue(hex_token[0])) {
    auto color = MakeColorValue(hex_token);
    if (color.value_type == TokenType::ERROR) {
      return false;
    }
    PushValue(color);
    return true;
  }
  return false;
}

bool CSSStringParser::NumberOrPercentValue(Token &token) {
  if (NumberValue(token)) {
    return true;
  }
  if (DimensionValue(token) && token.unit == TokenType::PERCENTAGE) {
    token.type = TokenType::PERCENTAGE;
    return true;
  }

  return false;
}

bool CSSStringParser::HexValue(Token &token) {
  return ConsumeAndSave(TokenType::HEX, token);
}

bool CSSStringParser::LengthOrPercentageValue(Token &token) {
  if (ConsumeAndSave(TokenType::CALC, token) ||
      ConsumeAndSave(TokenType::ENV, token) ||
      ConsumeAndSave(TokenType::FIT_CONTENT, token) ||
      ConsumeAndSave(TokenType::MAX_CONTENT, token) ||
      ConsumeAndSave(TokenType::AUTO, token)) {
    return true;
  }
  if (ConsumeAndSave(TokenType::NUMBER, token)) {
    if (token.IsZero()) {
      return true;
    }
    // For compatibility, we use numbers as valid length
    if (Check(TokenType::TOKEN_EOF)) {
      return true;
    }
    // engine version >= 2.6
    if (parser_configs_.enable_length_unit_check) {
      return false;
    }
    // If the next char is white space, comma or slash, can be
    // resolved to a valid length value, for compatibility
    const char *next = token.start + token.length;
    if (next[0] == ' ' || next[0] == '/' || next[0] == ',') {
      token.type = TokenType::NUMBER;
      return true;
    }
    return false;
  }

  if (ConsumeAndSave(TokenType::DIMENSION, token) &&
      ((token.unit >= TokenType::PX && token.unit <= TokenType::SP) ||
       token.unit == TokenType::PERCENTAGE)) {
    token.type = token.unit;
    return true;
  }
  return false;
}

bool CSSStringParser::PercentageValue(Token &token) {
  if (DimensionValue(token) && token.unit == TokenType::PERCENTAGE) {
    token.type = TokenType::PERCENTAGE;
    return true;
  }
  return false;
}

bool CSSStringParser::DimensionValue(Token &token) {
  return ConsumeAndSave(TokenType::DIMENSION, token);
}

bool CSSStringParser::NumberValue(Token &token) {
  return ConsumeAndSave(TokenType::NUMBER, token);
}

void CSSStringParser::PushValue(const StackValue &value) {
  if (stack_value_.has_value) {
    UnitHandler::CSSUnreachable(true, "PushValue has value");
  }
  stack_value_ = value;
  stack_value_.has_value = true;
}

void CSSStringParser::PushValue(StackValue &&value) {
  if (stack_value_.has_value) {
    UnitHandler::CSSUnreachable(true, "PushValue has value");
  }
  stack_value_ = std::move(value);
  stack_value_.has_value = true;
}

const CSSStringParser::StackValue &CSSStringParser::PopValue() {
  if (!stack_value_.has_value) {
    UnitHandler::CSSUnreachable(true, "PopValue has no value");
  }
  stack_value_.has_value = false;
  return stack_value_;
}

bool CSSStringParser::CheckAndAdvance(TokenType type) {
  if (!Check(type)) {
    return false;
  }
  Advance();
  return current_token_.type != TokenType::ERROR;
}

void CSSStringParser::SkipWhitespaceToken() {
  if (current_token_.type == TokenType::WHITESPACE) {
    Advance();
  }
}

bool CSSStringParser::Consume(TokenType type) {
  SkipWhitespaceToken();
  if (current_token_.type == type) {
    Advance();
    return current_token_.type != TokenType::ERROR;
  }
  return false;
}

bool CSSStringParser::ConsumeAndSave(TokenType type, Token &token) {
  if (Consume(type)) {
    token = previous_token_;
    return true;
  }
  return false;
}

bool CSSStringParser::Check(TokenType type) {
  SkipWhitespaceToken();
  return current_token_.type == type;
}

void CSSStringParser::Advance() {
  previous_token_ = current_token_;
  current_token_ = scanner_.ScanToken();
}

uint32_t CSSStringParser::TokenTypeToTextENUM(TokenType token_type) {
  switch (token_type) {
    case TokenType::NONE:
      return static_cast<uint32_t>(starlight::TextDecorationType::kNone);
    case TokenType::UNDERLINE:
      return static_cast<uint32_t>(starlight::TextDecorationType::kUnderLine);
    case TokenType::LINE_THROUGH:
      return static_cast<uint32_t>(starlight::TextDecorationType::kLineThrough);
    case TokenType::SOLID:
      return static_cast<uint32_t>(starlight::TextDecorationType::kSolid);
    case TokenType::DOUBLE:
      return static_cast<uint32_t>(starlight::TextDecorationType::kDouble);
    case TokenType::DOTTED:
      return static_cast<uint32_t>(starlight::TextDecorationType::kDotted);
    case TokenType::DASHED:
      return static_cast<uint32_t>(starlight::TextDecorationType::kDashed);
    case TokenType::WAVY:
      return static_cast<uint32_t>(starlight::TextDecorationType::kWavy);
    default:
      return -1;
  }
}

uint32_t CSSStringParser::TokenTypeToENUM(TokenType token_type) {
  switch (token_type) {
    case TokenType::NUMBER:
      return static_cast<uint32_t>(CSSValuePattern::NUMBER);
    case TokenType::URL:
      return static_cast<uint32_t>(starlight::BackgroundImageType::kUrl);
    case TokenType::LINEAR_GRADIENT:
      return static_cast<uint32_t>(
          starlight::BackgroundImageType::kLinearGradient);
    case TokenType::RADIAL_GRADIENT:
      return static_cast<uint32_t>(
          starlight::BackgroundImageType::kRadialGradient);
    case TokenType::ELLIPSE:
      return static_cast<uint32_t>(
          starlight::RadialGradientShapeType::kEllipse);
    case TokenType::CIRCLE:
      return static_cast<uint32_t>(starlight::RadialGradientShapeType::kCircle);
    case TokenType::FARTHEST_CORNER:
      return static_cast<uint32_t>(
          starlight::RadialGradientSizeType::kFarthestCorner);
    case TokenType::FARTHEST_SIDE:
      return static_cast<uint32_t>(
          starlight::RadialGradientSizeType::kFarthestSide);
    case TokenType::CLOSEST_CORNER:
      return static_cast<uint32_t>(
          starlight::RadialGradientSizeType::kClosestCorner);
    case TokenType::CLOSEST_SIDE:
      return static_cast<uint32_t>(
          starlight::RadialGradientSizeType::kClosestSide);
    case TokenType::BORDER_BOX:
      return static_cast<uint32_t>(starlight::BackgroundOriginType::kBorderBox);
    case TokenType::PADDING_BOX:
      return static_cast<uint32_t>(
          starlight::BackgroundOriginType::kPaddingBox);
    case TokenType::CONTENT_BOX:
      return static_cast<uint32_t>(
          starlight::BackgroundOriginType::kContentBox);
    case TokenType::LEFT:
      return POS_LEFT;
    case TokenType::RIGHT:
      return POS_RIGHT;
    case TokenType::TOP:
      return POS_TOP;
    case TokenType::BOTTOM:
      return POS_BOTTOM;
    case TokenType::CENTER:
      return POS_CENTER;
    case TokenType::PERCENTAGE:
      return static_cast<uint32_t>(CSSValuePattern::PERCENT);
    case TokenType::RPX:
      return static_cast<uint32_t>(CSSValuePattern::RPX);
    case TokenType::PX:
      return static_cast<uint32_t>(CSSValuePattern::PX);
    case TokenType::REM:
      return static_cast<uint32_t>(CSSValuePattern::REM);
    case TokenType::EM:
      return static_cast<uint32_t>(CSSValuePattern::EM);
    case TokenType::VW:
      return static_cast<uint32_t>(CSSValuePattern::VW);
    case TokenType::VH:
      return static_cast<uint32_t>(CSSValuePattern::VH);
    case TokenType::PPX:
      return static_cast<uint32_t>(CSSValuePattern::PPX);
    case TokenType::FR:
      return static_cast<uint32_t>(CSSValuePattern::FR);
    case TokenType::SP:
      return static_cast<uint32_t>(CSSValuePattern::SP);
    case TokenType::CALC:
      return static_cast<uint32_t>(CSSValuePattern::CALC);
    case TokenType::ENV:
      return static_cast<uint32_t>(CSSValuePattern::ENV);
    case TokenType::MAX_CONTENT:
    case TokenType::FIT_CONTENT:
      return static_cast<uint32_t>(CSSValuePattern::INTRINSIC);
    case TokenType::AUTO:
      return static_cast<uint32_t>(CSSValuePattern::ENUM);
    case TokenType::REPEAT:
    case TokenType::REPEAT_X:
    case TokenType::REPEAT_Y:
      return static_cast<uint32_t>(starlight::BackgroundRepeatType::kRepeat);
    case TokenType::NO_REPEAT:
      return static_cast<uint32_t>(starlight::BackgroundRepeatType::kNoRepeat);
    case TokenType::SPACE:
      return static_cast<uint32_t>(starlight::BackgroundRepeatType::kSpace);
    case TokenType::ROUND:
      return static_cast<uint32_t>(starlight::BackgroundRepeatType::kRound);
    case TokenType::COVER:
      return static_cast<uint32_t>(starlight::BackgroundSizeType::kCover);
    case TokenType::CONTAIN:
      return static_cast<uint32_t>(starlight::BackgroundSizeType::kContain);
    case TokenType::NONE:
      return static_cast<uint32_t>(starlight::BackgroundImageType::kNone);
    default:
      return -1;
  }
}

// For compatibility with old type
int32_t CSSStringParser::TokenTypeToBorderStyle(TokenType token_type) {
  switch (token_type) {
    case TokenType::HIDDEN:
      return static_cast<int32_t>(starlight::BorderStyleType::kHide);
    case TokenType::DOTTED:
      return static_cast<int32_t>(starlight::BorderStyleType::kDotted);
    case TokenType::DASHED:
      return static_cast<int32_t>(starlight::BorderStyleType::kDashed);
    case TokenType::SOLID:
      return static_cast<int32_t>(starlight::BorderStyleType::kSolid);
    case TokenType::DOUBLE:
      return static_cast<int32_t>(starlight::BorderStyleType::kDouble);
    case TokenType::GROOVE:
      return static_cast<int32_t>(starlight::BorderStyleType::kGroove);
    case TokenType::RIDGE:
      return static_cast<int32_t>(starlight::BorderStyleType::kRidge);
    case TokenType::INSET:
      return static_cast<int32_t>(starlight::BorderStyleType::kInset);
    case TokenType::OUTSET:
      return static_cast<int32_t>(starlight::BorderStyleType::kOutset);
    case TokenType::NONE:
    default:
      return static_cast<int32_t>(starlight::BorderStyleType::kNone);
  }
}

uint32_t CSSStringParser::TokenTypeToBorderWidth(TokenType token_type) {
  switch (token_type) {
    case TokenType::THIN:
      return 1;
    case TokenType::MEDIUM:
      return 3;
    case TokenType::THICK:
      return 5;
    default:
      return 0;
  }
}

int CSSStringParser::TokenTypeToShadowOption(TokenType token_type) {
  if (token_type == TokenType::INSET) {
    return static_cast<int>(starlight::ShadowOption::kInset);
  }
  return static_cast<int>(starlight::ShadowOption::kNone);
}

double CSSStringParser::GetColorValue(const Token &token, double max_value) {
  if (token.type == TokenType::PERCENTAGE) {
    return TokenToDouble(token) / 100.0 * max_value;
  }
  return CSSStringParser::TokenToDouble(token);
}

CSSStringParser::StackValue CSSStringParser::MakeColorValue(
    const Token token_list[]) {
  CSSColor color;
  switch (token_list[0].type) {
    case TokenType::RGBA:
      color = CSSColor::CreateFromRGBA(
          GetColorValue(token_list[1]), GetColorValue(token_list[2]),
          GetColorValue(token_list[3]), GetColorValue(token_list[4], 1));
      break;
    case TokenType::RGB:
      color = CSSColor::CreateFromRGBA(GetColorValue(token_list[1]),
                                       GetColorValue(token_list[2]),
                                       GetColorValue(token_list[3]), 1.f);
      break;
    case TokenType::HSLA:
      color = CSSColor::CreateFromHSLA(
          TokenToInt(token_list[1]), TokenToInt(token_list[2]),
          TokenToInt(token_list[3]), TokenToDouble(token_list[4]));
      break;
    case TokenType::HSL:
      color = CSSColor::CreateFromHSLA(TokenToInt(token_list[1]),
                                       TokenToInt(token_list[2]),
                                       TokenToInt(token_list[3]), 1.f);
      break;
    case TokenType::HEX: {
      std::string str = "#";
      str.append(token_list[0].start, token_list[0].length);
      if (!CSSColor::Parse(str, color)) {
        return StackValue(TokenType::ERROR);
      }
    } break;
    default:
      break;
  }

  return StackValue(TokenType::NUMBER, color.Cast());
}

int64_t CSSStringParser::TokenToInt(const Token &token) {
  int64_t ret;
  base::StringToInt(std::string(token.start, token.length), ret, 10);
  return ret;
}

double CSSStringParser::TokenToDouble(const Token &token) {
  double ret = 0;
  base::StringToDouble(std::string(token.start, token.length), ret);
  return ret;
}

float CSSStringParser::TokenToAngleValue(const Token &token) {
  switch (token.type) {
    case TokenType::DEG:
    case TokenType::NUMBER:
      return TokenToDouble(token);
    case TokenType::RAD:
      return TokenToDouble(token) * 180.f / M_PI;
    case TokenType::TURN:
      return TokenToDouble(token) * 360.f;
    case TokenType::GRAD:
      return TokenToDouble(token) * 360.f / 400.f;
    default:
      return 0.f;
  }
}

double CSSStringParser::TimeToNumber(const Token &token) {
  return token.type == TokenType::SECOND ? TokenToDouble(token) * 1000
                                         : TokenToDouble(token);
}

starlight::TransformType CSSStringParser::TokenToTransformFunction(
    const Token &token) {
  switch (token.type) {
    case TokenType::ROTATE:
      return starlight::TransformType::kRotate;
    case TokenType::ROTATE_X:
      return starlight::TransformType::kRotateX;
    case TokenType::ROTATE_Y:
      return starlight::TransformType::kRotateY;
    case TokenType::ROTATE_Z:
      return starlight::TransformType::kRotateZ;
    case TokenType::TRANSLATE:
      return starlight::TransformType::kTranslate;
    case TokenType::TRANSLATE_3D:
      return starlight::TransformType::kTranslate3d;
    case TokenType::TRANSLATE_X:
      return starlight::TransformType::kTranslateX;
    case TokenType::TRANSLATE_Y:
      return starlight::TransformType::kTranslateY;
    case TokenType::TRANSLATE_Z:
      return starlight::TransformType::kTranslateZ;
    case TokenType::SCALE:
      return starlight::TransformType::kScale;
    case TokenType::SCALE_X:
      return starlight::TransformType::kScaleX;
    case TokenType::SCALE_Y:
      return starlight::TransformType::kScaleY;
    case TokenType::SKEW:
      return starlight::TransformType::kSkew;
    case TokenType::SKEW_X:
      return starlight::TransformType::kSkewX;
    case TokenType::SKEW_Y:
      return starlight::TransformType::kSkewY;
    case TokenType::MATRIX:
      return starlight::TransformType::kMatrix;
    case TokenType::MATRIX_3D:
      return starlight::TransformType::kMatrix3d;
    default:
      return starlight::TransformType::kNone;
  }
}

starlight::AnimationPropertyType CSSStringParser::TokenToTransitionType(
    const Token &token, const CSSParserConfigs &configs) {
  switch (token.type) {
    case TokenType::NONE:
      return starlight::AnimationPropertyType::kNone;
    case TokenType::OPACITY:
      return starlight::AnimationPropertyType::kOpacity;
    case TokenType::SCALE_X:
      return starlight::AnimationPropertyType::kScaleX;
    case TokenType::SCALE_Y:
      return starlight::AnimationPropertyType::kScaleY;
    case TokenType::SCALE_XY:
      return starlight::AnimationPropertyType::kScaleXY;
    case TokenType::WIDTH:
      return starlight::AnimationPropertyType::kWidth;
    case TokenType::HEIGHT:
      return starlight::AnimationPropertyType::kHeight;
    case TokenType::BACKGROUND_COLOR:
      return starlight::AnimationPropertyType::kBackgroundColor;
    case TokenType::COLOR:
      return starlight::AnimationPropertyType::kColor;
    case TokenType::VISIBILITY:
      return starlight::AnimationPropertyType::kVisibility;
    case TokenType::LEFT:
      return starlight::AnimationPropertyType::kLeft;
    case TokenType::TOP:
      return starlight::AnimationPropertyType::kTop;
    case TokenType::RIGHT:
      return starlight::AnimationPropertyType::kRight;
    case TokenType::BOTTOM:
      return starlight::AnimationPropertyType::kBottom;
    case TokenType::TRANSFORM:
      return starlight::AnimationPropertyType::kTransform;
    case TokenType::ALL:
      return starlight::AnimationPropertyType::kAll;
    case TokenType::MAX_WIDTH:
      return starlight::AnimationPropertyType::kMaxWidth;
    case TokenType::MAX_HEIGHT:
      return starlight::AnimationPropertyType::kMaxHeight;
    case TokenType::MIN_WIDTH:
      return starlight::AnimationPropertyType::kMinWidth;
    case TokenType::MIN_HEIGHT:
      return starlight::AnimationPropertyType::kMinHeight;
    case TokenType::PADDING_LEFT:
      return starlight::AnimationPropertyType::kPaddingLeft;
    case TokenType::PADDING_RIGHT:
      return starlight::AnimationPropertyType::kPaddingRight;
    case TokenType::PADDING_TOP:
      return starlight::AnimationPropertyType::kPaddingTop;
    case TokenType::PADDING_BOTTOM:
      return starlight::AnimationPropertyType::kPaddingBottom;
    case TokenType::MARGIN_LEFT:
      return starlight::AnimationPropertyType::kMarginLeft;
    case TokenType::MARGIN_RIGHT:
      return starlight::AnimationPropertyType::kMarginRight;
    case TokenType::MARGIN_TOP:
      return starlight::AnimationPropertyType::kMarginTop;
    case TokenType::MARGIN_BOTTOM:
      return starlight::AnimationPropertyType::kMarginBottom;
    case TokenType::BORDER_LEFT_COLOR:
      return starlight::AnimationPropertyType::kBorderLeftColor;
    case TokenType::BORDER_RIGHT_COLOR:
      return starlight::AnimationPropertyType::kBorderRightColor;
    case TokenType::BORDER_TOP_COLOR:
      return starlight::AnimationPropertyType::kBorderTopColor;
    case TokenType::BORDER_BOTTOM_COLOR:
      return starlight::AnimationPropertyType::kBorderBottomColor;
    case TokenType::BORDER_LEFT_WIDTH:
      return starlight::AnimationPropertyType::kBorderLeftWidth;
    case TokenType::BORDER_RIGHT_WIDTH:
      return starlight::AnimationPropertyType::kBorderRightWidth;
    case TokenType::BORDER_TOP_WIDTH:
      return starlight::AnimationPropertyType::kBorderTopWidth;
    case TokenType::BORDER_BOTTOM_WIDTH:
      return starlight::AnimationPropertyType::kBorderBottomWidth;
    case TokenType::FLEX_BASIS:
      return starlight::AnimationPropertyType::kFlexBasis;
    case TokenType::FLEX_GROW:
      return starlight::AnimationPropertyType::kFlexGrow;
    case TokenType::BORDER_WIDTH:
      return starlight::AnimationPropertyType::kBorderWidth;
    case TokenType::BORDER_COLOR:
      return starlight::AnimationPropertyType::kBorderColor;
    case TokenType::MARGIN:
      return starlight::AnimationPropertyType::kMargin;
    case TokenType::PADDING:
      return starlight::AnimationPropertyType::kPadding;
    case TokenType::FILTER:
      return starlight::AnimationPropertyType::kFilter;
    default:
      UnitHandler::CSSWarning(false, configs.enable_css_strict_mode,
                              "Unsupported value: %s in transition-property "
                              "will be set to none!",
                              std::string(token.start, token.length).c_str());
      return starlight::AnimationPropertyType::kNone;
  }
}

starlight::TimingFunctionType CSSStringParser::TokenToTimingFunctionType(
    const Token &token) {
  switch (token.type) {
    case TokenType::LINEAR:
      return starlight::TimingFunctionType::kLinear;
    case TokenType::EASE_IN:
      return starlight::TimingFunctionType::kEaseIn;
    case TokenType::EASE_OUT:
      return starlight::TimingFunctionType::kEaseOut;
    case TokenType::EASE:
    case TokenType::EASE_IN_EASE_OUT:
    case TokenType::EASE_IN_OUT:
      return starlight::TimingFunctionType::kEaseInEaseOut;
    case TokenType::SQUARE_BEZIER:
      return starlight::TimingFunctionType::kSquareBezier;
    case TokenType::CUBIC_BEZIER:
      return starlight::TimingFunctionType::kCubicBezier;
    case TokenType::STEP_START:
    case TokenType::STEP_END:
    case TokenType::STEPS:
      return starlight::TimingFunctionType::kSteps;
    default:
      return starlight::TimingFunctionType::kLinear;
  }
}

void CSSStringParser::BackgroundLayerToArray(const CSSBackgroundLayer &layer,
                                             lepus::CArray *image_array,
                                             lepus::CArray *position_array,
                                             lepus::CArray *size_array,
                                             lepus::CArray *origin_array,
                                             lepus::CArray *repeat_array,
                                             lepus::CArray *clip_array) {
  image_array->emplace_back(TokenTypeToENUM(layer.image->value_type));
  if (layer.image->value) {
    image_array->emplace_back(*layer.image->value);
  }

  // position
  {
    auto array = lepus::CArray::Create();
    PositionAddLegacyValue(array, layer.position_x);
    PositionAddLegacyValue(array, layer.position_y);
    position_array->emplace_back(std::move(array));
  }
  // size
  {
    auto array = lepus::CArray::Create();
    SizeAddLegacyValue(array, layer.size_x);
    SizeAddLegacyValue(array, layer.size_y);
    size_array->emplace_back(std::move(array));
  }

  // repeat
  {
    auto array = lepus::CArray::Create();
    const auto vx = layer.repeat_x;
    const auto vy = layer.repeat_y;

    array->emplace_back(vx);
    array->emplace_back(vy);

    repeat_array->emplace_back(std::move(array));
  }

  // origin
  origin_array->emplace_back(layer.origin);

  // clip
  clip_array->emplace_back(layer.clip);
}

void CSSStringParser::ClampColorAndStopList(base::Vector<uint32_t> &colors,
                                            base::Vector<float> &stops) {
  if (stops.size() < 2) {
    return;
  }
  bool clamp_front = stops.front() < 0.f;
  bool clamp_back = stops.back() > 100.f;

  if (!clamp_front && !clamp_back) {
    return;
  }

  if (clamp_front) {
    // find first positive position
    uint32_t first_positive_index = 0;
    auto result = std::find_if(stops.begin(), stops.end(),
                               [](float v) { return v >= 0.f; });
    if (result != stops.end()) {
      first_positive_index = static_cast<uint32_t>(result - stops.begin());
    }

    if (first_positive_index != 0) {
      ClampColorAndStopListAtFront(colors, stops, first_positive_index);
    }
  }

  if (clamp_back) {
    // find fist greater than 100.f position
    uint32_t tail_position = 0;
    auto result = std::find_if(stops.begin(), stops.end(),
                               [](float v) { return v >= 100.f; });
    if (result != stops.end()) {
      tail_position = static_cast<uint32_t>(result - stops.begin());
    }
    if (tail_position != 0) {
      ClampColorAndStopListAtBack(colors, stops, tail_position);
    }
  }
}

void CSSStringParser::ClampColorAndStopListAtFront(
    base::Vector<uint32_t> &colors, base::Vector<float> &stops,
    uint32_t first_positive_index) {
  float prev_stop = stops[first_positive_index - 1];
  uint32_t prev_color = colors[first_positive_index - 1];

  float current_stop = stops[first_positive_index];
  uint32_t current_color = colors[first_positive_index];

  uint32_t result_color =
      LerpColor(prev_color, current_color, prev_stop, current_stop, 0.f);
  // update prev content
  stops[first_positive_index - 1] = 0.f;
  colors[first_positive_index - 1] = result_color;

  // remove all other negative stops and colors
  if (first_positive_index - 1 > 0) {
    stops.erase(stops.begin(), stops.begin() + first_positive_index - 1);
    colors.erase(colors.begin(), colors.begin() + first_positive_index - 1);
  }
}

void CSSStringParser::ClampColorAndStopListAtBack(
    base::Vector<uint32_t> &colors, base::Vector<float> &stops,
    uint32_t tail_position) {
  float prev_stop = stops[tail_position - 1];
  uint32_t prev_color = colors[tail_position - 1];

  float current_stop = stops[tail_position];
  uint32_t current_color = colors[tail_position];

  uint32_t result_color =
      LerpColor(prev_color, current_color, prev_stop, current_stop, 100.f);
  // update tail content
  stops[tail_position] = 100.f;
  colors[tail_position] = result_color;

  // remote all other greater than 100% stops and colors
  if (tail_position + 1 < stops.size()) {
    stops.erase(stops.begin() + tail_position + 1, stops.end());
    colors.erase(colors.begin() + tail_position + 1, colors.end());
  }
}

static uint8_t ClampColorValue(float x) {
  return static_cast<uint8_t>(round(x < 0 ? 0 : (x > 255 ? 255 : x)));
}

uint32_t CSSStringParser::LerpColor(uint32_t start_color, uint32_t end_color,
                                    float start_pos, float end_pos,
                                    float current_pos) {
  float weight = (current_pos - start_pos) / (end_pos - start_pos);

  int a1 = (start_color >> 24) & 0xFF;
  int b1 = (start_color >> 16) & 0xFF;
  int c1 = (start_color >> 8) & 0xFF;
  int d1 = (start_color) & 0xFF;

  int a2 = (end_color >> 24) & 0xFF;
  int b2 = (end_color >> 16) & 0xFF;
  int c2 = (end_color >> 8) & 0xFF;
  int d2 = (end_color) & 0xFF;

  uint8_t a = ClampColorValue(a1 + (a2 - a1) * weight);
  uint8_t b = ClampColorValue(b1 + (b2 - b1) * weight);
  uint8_t c = ClampColorValue(c1 + (c2 - c1) * weight);
  uint8_t d = ClampColorValue(d1 + (d2 - d1) * weight);

  return ((a << 24) & 0xffffffff) | ((b << 16) & 0xffffffff) |
         ((c << 8) & 0xffffffff) | (d & 0xffffffff);
}

bool CSSStringParser::BasicShapeCircle() {
  if (!Consume(TokenType::CIRCLE) || !Consume(TokenType::LEFT_PAREN)) {
    return false;
  }
  auto arr = lepus::CArray::Create();

  constexpr uint32_t BASIC_SHAPE_CIRCLE_TYPE =
      static_cast<uint32_t>(starlight::BasicShapeType::kCircle);
  arr->emplace_back(BASIC_SHAPE_CIRCLE_TYPE);

  // Radius is required
  if (!ConsumeLengthAndSetValue(arr)) {
    return false;
  }

  // position is optional
  if (Check(TokenType::RIGHT_PAREN)) {
    // default center x
    arr->emplace_back(50);
    arr->emplace_back(PATTERN_PERCENT);

    // default center y
    arr->emplace_back(50);
    arr->emplace_back(PATTERN_PERCENT);
  } else if (!AtPositionAndSetValue(arr)) {
    // parse [<position>]? failed
    return false;
  }

  PushValue(StackValue(TokenType::CIRCLE, std::move(arr)));
  return true;
}

bool CSSStringParser::AtPositionAndSetValue(fml::RefPtr<lepus::CArray> &arr) {
  if (!Consume(TokenType::AT)) {
    return false;
  }
  return ConsumePositionAndSetValue(arr);
}

bool CSSStringParser::ConsumePositionAndSetValue(
    fml::RefPtr<lepus::CArray> &arr) {
  CSSValue pos_x = CSSValue::Empty();
  CSSValue pos_y = CSSValue::Empty();
  if (!BackgroundPosition(pos_x, pos_y)) {
    return false;
  }
  return PositionAddValue(arr, pos_x) && PositionAddValue(arr, pos_y);
}

bool CSSStringParser::BasicShapeEllipse() {
  if (!Consume(TokenType::ELLIPSE) || !Consume(TokenType::LEFT_PAREN)) {
    return false;
  }
  auto arr = lepus::CArray::Create();

  constexpr uint32_t BASIC_SHAPE_ELLIPSE_TYPE =
      static_cast<uint32_t>(starlight::BasicShapeType::kEllipse);
  arr->emplace_back(BASIC_SHAPE_ELLIPSE_TYPE);

  // radius is required.
  if (!ConsumeLengthAndSetValue(arr)) {
    return false;
  }

  if (!ConsumeLengthAndSetValue(arr)) {
    return false;
  }

  if (Check(TokenType::RIGHT_PAREN)) {
    // [at <position>] is optional, use default value.
    // default center x
    arr->emplace_back(50);
    arr->emplace_back(PATTERN_PERCENT);

    // default center y
    arr->emplace_back(50);
    arr->emplace_back(PATTERN_PERCENT);
  } else if (!AtPositionAndSetValue(arr)) {
    // function not end, but parse position failed, return false.
    return false;
  }

  PushValue(StackValue{TokenType::ELLIPSE, std::move(arr)});
  return true;
}

bool CSSStringParser::ConsumeLengthAndSetValue(
    fml::RefPtr<lepus::CArray> &arr) {
  CSSValue value = Length();
  if (value.IsEmpty()) {
    return false;
  }
  arr->emplace_back(std::move(value.GetValue()));
  arr->emplace_back(static_cast<int>(value.GetPattern()));
  return true;
}

bool CSSStringParser::BasicShapePath() {
  // path()
  if (!Consume(TokenType::PATH) || !Consume(TokenType::LEFT_PAREN)) {
    return false;
  }
  // svg path data string
  if (!Consume(TokenType::STRING)) {
    return false;
  }
  std::string path_data{previous_token_.start, previous_token_.length};
  auto arr = lepus::CArray::Create();

  constexpr uint32_t BASIC_SHAPE_PATH_TYPE =
      static_cast<uint32_t>(starlight::BasicShapeType::kPath);
  arr->emplace_back(BASIC_SHAPE_PATH_TYPE);

  arr->emplace_back(path_data);
  PushValue(StackValue(TokenType::PATH, std::move(arr)));
  return true;
}

bool CSSStringParser::SuperEllipse() {
  // Begin with 'super-ellipse('
  if (!Consume(TokenType::SUPER_ELLIPSE) || !Consume(TokenType::LEFT_PAREN)) {
    return false;
  }
  auto arr = lepus::CArray::Create();

  // append type enum
  constexpr uint32_t SUPER_ELLIPSE_TYPE =
      static_cast<uint32_t>(starlight::BasicShapeType::kSuperEllipse);
  arr->emplace_back(SUPER_ELLIPSE_TYPE);

  // [<shape-radius>{2}] are required
  // parse radius x
  if (!ConsumeLengthAndSetValue(arr)) {
    return false;
  }

  // parse radius y
  if (!ConsumeLengthAndSetValue(arr)) {
    return false;
  }

  if (Check(TokenType::AT) || Check(TokenType::RIGHT_PAREN)) {
    // [<number>{2}]?  is optional, [at] means use default exponent
    // default exponent is 2
    arr->emplace_back(2);
    arr->emplace_back(2);

    // [at <position>]? is optional, append default position
    if (Check(TokenType::RIGHT_PAREN)) {
      // default center x
      arr->emplace_back(50);
      arr->emplace_back(static_cast<uint32_t>(CSSValuePattern::PERCENT));

      // default center y
      arr->emplace_back(50);
      arr->emplace_back(static_cast<uint32_t>(CSSValuePattern::PERCENT));
      // parse finished
    } else if (!AtPositionAndSetValue(arr)) {
      // params not end but parse [at <position>] failed
      return false;
    }
  } else if (Check(TokenType::NUMBER)) {
    Token token;
    if (!ConsumeAndSave(TokenType::NUMBER, token) ||
        !Consume(TokenType::NUMBER)) {
      // [<number>{2}] parse failed
      return false;
    }

    // append exponent x and y
    arr->emplace_back(TokenToDouble(token));
    arr->emplace_back(TokenToDouble(previous_token_));

    if (Check(TokenType::RIGHT_PAREN)) {
      // default center x, center
      arr->emplace_back(50);
      arr->emplace_back(PATTERN_PERCENT);

      // default center y, center
      arr->emplace_back(50);
      arr->emplace_back(PATTERN_PERCENT);
    } else if (!AtPositionAndSetValue(arr)) {
      return false;
    }
  }

  // Parse finished
  PushValue(StackValue(TokenType::ELLIPSE, std::move(arr)));
  return true;
}

CSSValue CSSStringParser::ParseGrayscale() {
  Token grayscale;
  Advance();
  if (!ConsumeGrayscale(grayscale) || !Check(TokenType::TOKEN_EOF)) {
    return CSSValue::Empty();
  }
  double value = TokenToDouble(grayscale);

  // Amount is specified as a <number> or a <percentage>. Convert <number> to
  // <percentage> here.
  if (grayscale.type == TokenType::NUMBER) {
    value *= 100;
  }

  return CSSValue{lepus::Value(value), CSSValuePattern::PERCENT};
}

bool CSSStringParser::ConsumeGrayscale(Token &token) {
  return NumberOrPercentValue(token);
}

CSSValue CSSStringParser::ParseBlur() {
  Token blur;
  Advance();
  if (!ConsumeBlur(blur) || !Check(TokenType::TOKEN_EOF)) {
    return CSSValue::Empty();
  }
  return CSSValue{lepus::Value(TokenToDouble(blur)),
                  static_cast<CSSValuePattern>(TokenTypeToENUM(blur.type))};
}

bool CSSStringParser::ConsumeBlur(Token &token) {
  return LengthOrPercentageValue(token) && token.type != TokenType::PERCENTAGE;
}

CSSValue CSSStringParser::ParseFilter() {
  Advance();
  Token function_token;
  if (Consume(TokenType::NONE) && Consume(TokenType::TOKEN_EOF)) {
    // None
    auto result = lepus::CArray::Create();
    result->emplace_back(static_cast<uint32_t>(starlight::FilterType::kNone));
    return CSSValue{std::move(result)};
  } else if (ConsumeAndSave(TokenType::GRAYSCALE, function_token)) {
    return FilterGrayscaleValue(function_token);
  } else if (ConsumeAndSave(TokenType::BLUR, function_token)) {
    return FilterBlurValue(function_token);
  }
  return CSSValue::Empty();
}

/**
 * Create CSSValue for the css property `filter: blur()`
 * @param function_token The function type token, whose type should be blur.
 * @return An array type CSSValue contains a array with [type, value, pattern]
 * or empty value when failed.
 */
CSSValue CSSStringParser::FilterBlurValue(const Token &function_token) {
  auto result = lepus::CArray::Create();
  // push back type blur
  result->emplace_back(static_cast<uint32_t>(starlight::FilterType::kBlur));

  // extract function args
  CSSStringParser blur_parser{function_token.start, function_token.length,
                              this->parser_configs_};

  // parse blur args
  CSSValue blur = blur_parser.ParseBlur();

  // append to result array
  if (!blur.IsEmpty() && Check(TokenType::TOKEN_EOF)) {
    result->emplace_back(std::move(blur.GetValue()));
    result->emplace_back(static_cast<uint32_t>(blur.GetPattern()));
    return CSSValue(std::move(result));
  }
  return CSSValue::Empty();
}

/**
 * Create CSSValue for the css property `filter: grayscale()`
 * @param function_token The function type token, whose type should be
 * grayscale.
 * @return An array type CSSValue contains a array with [type, value, pattern]
 * or empty value when failed.
 */
CSSValue CSSStringParser::FilterGrayscaleValue(const Token &function_token) {
  auto result = lepus::CArray::Create();
  // Push back type grayscale
  result->emplace_back(
      static_cast<uint32_t>(starlight::FilterType::kGrayscale));

  // Begin parse the args.
  CSSStringParser grayscale_parser = CSSStringParser{
      function_token.start, function_token.length, this->parser_configs_};

  CSSValue grayscale = grayscale_parser.ParseGrayscale();
  if (!grayscale.IsEmpty() && Check(TokenType::TOKEN_EOF)) {
    result->emplace_back(std::move(grayscale.GetValue()));
    result->emplace_back(static_cast<uint32_t>(CSSValuePattern::PERCENT));
    return CSSValue(std::move(result));
  }
  return CSSValue::Empty();
}

bool CSSStringParser::ParseBorderLineWidth(CSSValue &result_width) {
  Advance();
  Token token;
  ConsumeBorderLineWidth(token, result_width);
  return Check(TokenType::TOKEN_EOF);
}

bool CSSStringParser::ParseBorderStyle(CSSValue &result_style) {
  Advance();
  Token token;
  if (BorderStyleIdent(token)) {
    result_style.GetValue().SetNumber(TokenTypeToBorderStyle(token.type));
    result_style.SetPattern(CSSValuePattern::ENUM);
    return Check(TokenType::TOKEN_EOF);
  }
  return false;
}

bool CSSStringParser::ParseBorder(CSSValue &result_width,
                                  CSSValue &result_style,
                                  CSSValue &result_color) {
  Advance();
  Token token;
  while (result_width.IsEmpty() || result_style.IsEmpty() ||
         result_color.IsEmpty()) {
    if (result_width.IsEmpty()) {
      ConsumeBorderLineWidth(token, result_width);
      if (!result_width.IsEmpty()) {
        continue;
      }
    }
    if (result_style.IsEmpty() && BorderStyleIdent(token)) {
      result_style.GetValue().SetNumber(TokenTypeToBorderStyle(token.type));
      result_style.SetPattern(CSSValuePattern::ENUM);
      if (!result_style.IsEmpty()) {
        continue;
      }
    }
    if (result_color.IsEmpty() && Color()) {
      const auto &stack_value = PopValue();
      if (stack_value.value_type == TokenType::NUMBER) {
        result_color = CSSValue(*stack_value.value, CSSValuePattern::NUMBER);
      }
      if (!result_color.IsEmpty()) {
        continue;
      }
    }
    break;
  }

  if (!AtEnd()) {
    return false;
  }

  if (result_width.IsEmpty() && result_style.IsEmpty() &&
      result_color.IsEmpty()) {
    return false;
  }

  // Fill default values
  if (parser_configs_.enable_new_border_handler) {
    if (result_width.IsEmpty()) {
      result_width = CSSValue(lepus::Value(0), CSSValuePattern::NUMBER);
    }
    if (result_style.IsEmpty()) {
      result_style =
          CSSValue(lepus::Value(TokenTypeToBorderStyle(TokenType::SOLID)),
                   CSSValuePattern::ENUM);
    }
    if (result_color.IsEmpty()) {
      result_color =
          CSSValue(lepus::Value(CSSColor::Black), CSSValuePattern::NUMBER);
    }
  }
  return true;
}

CSSValue CSSStringParser::ParseShadow(bool inset_and_spread) {
  Advance();
  if (Consume(TokenType::NONE) && AtEnd()) {
    return CSSValue(lepus::CArray::Create());
  }
  return ConsumeCommaSeparatedList(&CSSStringParser::ParseSingleShadow,
                                   inset_and_spread);
}

lepus::Value CSSStringParser::ParseSingleShadow(bool inset_and_spread) {
  // [1px 2px 3px red, ] is invalid
  if (Check(TokenType::TOKEN_EOF) || Check(TokenType::SEMICOLON) ||
      Check(TokenType::ERROR)) {
    return lepus::Value();
  }

  // Shadow item
  auto dict = lepus::Dictionary::Create();

  CSSValue color = CSSValue::Empty();
  std::optional<int> option;
  CSSValue lengths[4] = {
      CSSValue::Empty(),  // horizontal_offset
      CSSValue::Empty(),  // vertical_offset
      CSSValue::Empty(),  // blur_radius
      CSSValue::Empty(),  // spread_distance
  };
  ConsumeColor(color);

  Token token;
  if (Check(TokenType::INSET)) {
    // text-shadow doesn't support inset and spread
    if (!inset_and_spread) {
      return lepus::Value();
    }
    if (ShadowOptionIdent(token)) {
      option = TokenTypeToShadowOption(token.type);
    }
    if (color.IsEmpty()) {
      ConsumeColor(color);
    }
  }

  // horizontal_offset
  lengths[0] = Length();
  if (lengths[0].IsEmpty()) {
    return lepus::Value();
  }

  // vertical_offset
  lengths[1] = Length();
  if (lengths[1].IsEmpty()) {
    return lepus::Value();
  }

  // blur_radius
  lengths[2] = Length();
  if (!lengths[2].IsEmpty()) {
    if (inset_and_spread) {
      // spread_distance
      lengths[3] = Length();
    }
  }

  // Still has token for current shadow
  if (!Check(TokenType::COMMA) || !Check(TokenType::TOKEN_EOF)) {
    if (color.IsEmpty()) {
      ConsumeColor(color);
    }

    if (Check(TokenType::INSET)) {
      if (!inset_and_spread || option.has_value()) {
        return lepus::Value();
      }
      if (ShadowOptionIdent(token)) {
        option = TokenTypeToShadowOption(token.type);
      }
      if (color.IsEmpty()) {
        ConsumeColor(color);
      }
    }
  }

  BASE_STATIC_STRING_DECL(kEnable, "enable");
  dict->SetValue(kEnable, true);
  if (option.has_value()) {
    BASE_STATIC_STRING_DECL(kOption, "option");
    dict->SetValue(kOption, *option);
  }

  BASE_STATIC_STRING_DECL(kColor, "color");
  dict->SetValue(kColor, color.GetValue());

  static constexpr const char kHOffset[] = "h_offset";
  static constexpr const char kVOffset[] = "v_offset";
  static constexpr const char kBlur[] = "blur";
  static constexpr const char kSpread[] = "spread";

  const std::array<const base::String, 4> props{
      BASE_STATIC_STRING(kHOffset), BASE_STATIC_STRING(kVOffset),
      BASE_STATIC_STRING(kBlur), BASE_STATIC_STRING(kSpread)};
  for (int i = 0; i < 4; ++i) {
    // horizontal_offset and vertical_offset can not be empty, early return
    if (lengths[i].IsEmpty()) {
      continue;
    }
    auto arr = lepus::CArray::Create();
    arr->emplace_back(std::move(lengths[i].GetValue()));
    arr->emplace_back(static_cast<int>(lengths[i].GetPattern()));
    dict->SetValue(props[i], std::move(arr));
  }

  return lepus::Value(std::move(dict));
}

CSSValue CSSStringParser::ParseTransformOrigin() {
  // For compatibility, we support comma in transform-origin
  enable_transform_legacy_ = !parser_configs_.enable_new_transform_handler;
  Advance();
  auto result = lepus::CArray::Create();
  if (ConsumePositionAndSetValue(result) && Check(TokenType::TOKEN_EOF)) {
    return CSSValue(std::move(result));
  }
  return CSSValue::Empty();
}

CSSValue CSSStringParser::ParseAspectRatio() {
  Advance();
  if (Check(TokenType::TOKEN_EOF)) {
    return CSSValue::Empty();
  }
  if (Consume(TokenType::NONE) && Check(TokenType::TOKEN_EOF)) {
    return CSSValue::Empty();
  }
  auto param1 = NumberOnly(false);
  if (param1.IsEmpty()) {
    return CSSValue::Empty();
  }
  if (Consume(TokenType::SLASH)) {
    auto param2 = NumberOnly(false);
    if (param2.IsEmpty() || base::IsZero(param2.Number())) {
      return CSSValue::Empty();
    }
    auto result = lepus::Value(param1.Number() / param2.Number());
    return CSSValue(std::move(result), CSSValuePattern::NUMBER);
  } else if (Check(TokenType::TOKEN_EOF)) {
    auto result = CSSValue(std::move(param1), CSSValuePattern::NUMBER);
    return result;
  }
  return CSSValue::Empty();
}

std::pair<CSSValue, CSSValue> CSSStringParser::ParseGap() {
  CSSValue default_gap1 = CSSValue(lepus::Value(0.0), CSSValuePattern::PX);
  CSSValue default_gap2 = CSSValue(lepus::Value(0.0), CSSValuePattern::PX);
  Advance();
  if (Check(TokenType::TOKEN_EOF)) {
    return std::make_pair(std::move(default_gap1), std::move(default_gap2));
  }
  if (Consume(TokenType::NONE) && Check(TokenType::TOKEN_EOF)) {
    return std::make_pair(std::move(default_gap1), std::move(default_gap2));
  }
  auto param1 = Length();
  if (param1.IsEmpty()) {
    param1 = std::move(default_gap1);
  }
  Advance();
  if (!Check(TokenType::TOKEN_EOF)) {
    auto param2 = Length();
    if (param2.IsEmpty()) {
      param2 = std::move(default_gap2);
    }
    return std::make_pair(param1, param2);
  }
  return std::make_pair(param1, param1);
}

bool CSSStringParser::ParseTextStroke(CSSValue &result_width,
                                      CSSValue &result_color) {
  Advance();
  if (Check(TokenType::TOKEN_EOF)) {
    return false;
  }
  if (Consume(TokenType::NONE)) {
    return true;
  }
  result_width = Length();
  if (result_width.IsEmpty()) {
    ConsumeColor(result_color);
    if (result_color.IsEmpty()) {
      return false;
    }
    Advance();
    if (!Check(TokenType::TOKEN_EOF)) {
      result_width = Length();
      if (result_width.IsEmpty()) {
        return false;
      }
      return true;
    }
    return false;
  } else {
    Advance();
    if (!Check(TokenType::TOKEN_EOF)) {
      ConsumeColor(result_color);
      if (result_color.IsEmpty()) {
        return false;
      }
      return true;
    }
    return false;
  }
}

CSSValue CSSStringParser::ParseBool() {
  Advance();
  if (Consume(TokenType::TOKEN_TRUE) || Consume(TokenType::TOKEN_FALSE)) {
    return CSSValue(lepus::Value(previous_token_.type == TokenType::TOKEN_TRUE),
                    CSSValuePattern::BOOLEAN);
  }
  return CSSValue::Empty();
}

bool CSSStringParser::ParseAutoFontSize(
    CSSValue &is_auto_font_size, CSSValue &auto_font_size_min_size,
    CSSValue &auto_font_size_max_size,
    CSSValue &auto_font_size_step_granularity) {
  Advance();
  if (Check(TokenType::TOKEN_EOF)) {
    return true;
  }

  if (!Consume(TokenType::TOKEN_TRUE) && !Consume(TokenType::TOKEN_FALSE)) {
    return false;
  }

  is_auto_font_size =
      CSSValue(lepus::Value(previous_token_.type == TokenType::TOKEN_TRUE),
               CSSValuePattern::BOOLEAN);
  if (Check(TokenType::TOKEN_EOF)) {
    return true;
  }

  auto temp_auto_font_size_min_size = Length();
  if (temp_auto_font_size_min_size.IsEmpty()) {
    return false;
  }
  auto_font_size_min_size = temp_auto_font_size_min_size;
  if (Check(TokenType::TOKEN_EOF)) {
    return true;
  }

  auto temp_auto_font_size_max_size = Length();
  if (temp_auto_font_size_max_size.IsEmpty()) {
    return false;
  }
  auto_font_size_max_size = temp_auto_font_size_max_size;
  if (Check(TokenType::TOKEN_EOF)) {
    return true;
  }

  auto temp_auto_font_size_step_granularity = Length();
  if (temp_auto_font_size_step_granularity.IsEmpty()) {
    return false;
  }
  auto_font_size_step_granularity = temp_auto_font_size_step_granularity;

  if (!Check(TokenType::TOKEN_EOF)) {
    is_auto_font_size = CSSValue(lepus::Value(false), CSSValuePattern::BOOLEAN);
    auto_font_size_min_size = CSSValue(lepus::Value(0), CSSValuePattern::PX);
    auto_font_size_max_size = CSSValue(lepus::Value(0), CSSValuePattern::PX);
    auto_font_size_step_granularity =
        CSSValue(lepus::Value(1), CSSValuePattern::PX);
    return false;
  }
  return true;
}

bool CSSStringParser::ParseAutoFontSizePresetSize(
    fml::RefPtr<lepus::CArray> &arr) {
  Advance();
  if (Check(TokenType::TOKEN_EOF)) {
    return true;
  }
  while (!Check(TokenType::TOKEN_EOF)) {
    if (!ConsumeLengthAndSetValue(arr)) {
      return false;
    }
  }
  return true;
}

CSSValue CSSStringParser::ParseTransform() {
  enable_transform_legacy_ = !parser_configs_.enable_new_transform_handler;
  Advance();
  if (Check(TokenType::TOKEN_EOF)) {
    return CSSValue::Empty();
  }
  if (Consume(TokenType::NONE) && Check(TokenType::TOKEN_EOF)) {
    return CSSValue(lepus::CArray::Create());
  }
  auto result = lepus::CArray::Create();
  while (!Check(TokenType::TOKEN_EOF) && !Check(TokenType::SEMICOLON) &&
         !Check(TokenType::ERROR)) {
    auto arr = lepus::CArray::Create();
    Token function_token;
    if (!TransformFunctionIdent(function_token)) {
      return CSSValue::Empty();
    }
    arr->emplace_back(
        static_cast<int>(TokenToTransformFunction(function_token)));

    CSSStringParser params_parser{function_token.start, function_token.length,
                                  parser_configs_};
    if (!params_parser.ParseTransformParams(function_token, arr)) {
      return CSSValue::Empty();
    }

    result->emplace_back(std::move(arr));
  }

  if (!AtEnd()) {
    return CSSValue::Empty();
  }
  return CSSValue(std::move(result));
}

bool CSSStringParser::ParseTransformParams(const Token &function_token,
                                           fml::RefPtr<lepus::CArray> &arr) {
  // For compatibility, we support number in angle value
  enable_transform_legacy_ = !parser_configs_.enable_new_transform_handler;
  Advance();
  switch (function_token.type) {
    case TokenType::ROTATE:
    case TokenType::ROTATE_X:
    case TokenType::ROTATE_Y:
    case TokenType::ROTATE_Z:
    case TokenType::SKEW_X:
    case TokenType::SKEW_Y:
    case TokenType::SKEW: {
      Token angle_token;
      if (!AngleValue(angle_token)) {
        return false;
      }
      arr->emplace_back(TokenToAngleValue(angle_token));
      // skew(angle, angle)
      if (function_token.type == TokenType::SKEW && Consume(TokenType::COMMA)) {
        if (!AngleValue(angle_token)) {
          return false;
        }
        arr->emplace_back(TokenToAngleValue(angle_token));
      }
    } break;
    case TokenType::SCALE_X:
    case TokenType::SCALE_Y:
    case TokenType::SCALE: {
      auto param = NumberOrPercentage();
      if (param.IsEmpty()) {
        return false;
      }
      arr->emplace_back(std::move(param));
      if (function_token.type == TokenType::SCALE &&
          Consume(TokenType::COMMA)) {
        auto param = NumberOrPercentage();
        if (param.IsEmpty()) {
          return false;
        }
        arr->emplace_back(std::move(param));
      }
    } break;
    case TokenType::TRANSLATE_X:
    case TokenType::TRANSLATE_Y:
    case TokenType::TRANSLATE_Z:
    case TokenType::TRANSLATE: {
      if (!ConsumeLengthAndSetValue(arr)) {
        return false;
      }
      // transform: translate(12px, 50%);
      if (function_token.type == TokenType::TRANSLATE) {
        if (Consume(TokenType::COMMA) && !ConsumeLengthAndSetValue(arr)) {
          return false;
        }
        // For compatibility, we support translate(12px, 50%, 3), it's equal
        // to translate(12px, 50%)
        if (enable_transform_legacy_ && Consume(TokenType::COMMA) &&
            !Length().IsEmpty()) {
          break;
        }
      }
    } break;
    case TokenType::TRANSLATE_3D: {
      // transform: translate3d(12px, 50%, 5px);
      if (!ConsumeLengthAndSetValue(arr)) {
        return false;
      }
      if (!Consume(TokenType::COMMA)) {
        return false;
      }
      if (!ConsumeLengthAndSetValue(arr)) {
        return false;
      }
      if (!Consume(TokenType::COMMA)) {
        return false;
      }
      if (!ConsumeLengthAndSetValue(arr)) {
        return false;
      }
    } break;
    case TokenType::MATRIX:
    case TokenType::MATRIX_3D:
      if (!ConsumeMatrixNumbers(
              arr, function_token.type == TokenType::MATRIX_3D ? 16 : 6)) {
        return false;
      }
      break;
    default:
      return false;
  }
  // Semicolon not allowed
  return Check(TokenType::TOKEN_EOF);
}

bool CSSStringParser::ConsumeMatrixNumbers(fml::RefPtr<lepus::CArray> &arr,
                                           int count) {
  do {
    auto param = NumberOrPercentage();
    if (param.IsEmpty()) {
      return false;
    }
    arr->emplace_back(std::move(param));
    if (--count && !Consume(TokenType::COMMA)) {
      return false;
    }
  } while (count);
  return true;
}

bool CSSStringParser::ParseFlex(double &flex_grow, double &flex_shrink,
                                CSSValue &flex_basis) {
  Advance();
  static const double unset_value = -1;

  if (Consume(TokenType::NONE) && Check(TokenType::TOKEN_EOF)) {
    flex_grow = 0;
    // For compatibility, none is equivalent to setting '0 1 auto'.
    // In fact, this should be '0 0 auto'
    flex_shrink = parser_configs_.enable_new_flex_handler ? 0 : 1;
    flex_basis.SetEnum(static_cast<int>(starlight::LengthValueType::kAuto));
    return true;
  }

  Token t;
  unsigned index = 0;
  while (!Check(TokenType::TOKEN_EOF) && index++ < 3) {
    double num = 0;
    if (LengthOrPercentageValue(t) ||
        // If enable length unit check number is not a valid length
        (parser_configs_.enable_length_unit_check &&
         t.type == TokenType::NUMBER)) {
      // number only
      if (t.type == TokenType::NUMBER) {
        num = TokenToDouble(t);
        if (num < 0) {
          return false;
        }
        if (flex_grow == unset_value) {
          flex_grow = num;
        } else if (flex_shrink == unset_value) {
          flex_shrink = num;
        } else if (!num || !parser_configs_.enable_length_unit_check) {
          // flex only allows a basis of 0
          // If disable unit check the last number can be basis
          flex_basis.SetNumber(num);
        } else {
          return false;
        }
      } else if (flex_basis.IsEmpty()) {  // length value
        TokenToLengthTarget(t, flex_basis);
        if (index == 2 && !Check(TokenType::TOKEN_EOF)) {
          return false;
        }
      }
    } else {
      return false;
    }
  }
  if (index == 0) {
    return false;
  }

  if (flex_grow == unset_value) {
    // FIXME: The legacy code. If flex only has a flex basis value, let's make
    // flex shrink 0
    if (flex_shrink == unset_value && !flex_basis.IsEmpty() &&
        !parser_configs_.enable_new_flex_handler) {
      flex_grow = 0;
    } else {
      flex_grow = 1;
    }
  }
  if (flex_shrink == unset_value) {
    flex_shrink = 1;
  }
  if (flex_basis.IsEmpty()) {
    flex_basis.SetNumber(0.f);
  }
  return AtEnd();
}

template <typename TokenFunc, typename ConsumeTokenFunc>
bool CSSStringParser::ParseNumberOrArray(bool single, TokenFunc is_token,
                                         ConsumeTokenFunc consume,
                                         CSSValue &ret) {
  Advance();
  Token t;
  if (single) {
    if ((this->*is_token)(t)) {
      CSSValue value = consume(t);
      if (value.IsEmpty()) {
        return false;
      }
      ret = std::move(value);
      return AtEnd();
    } else {
      return false;
    }
  } else {
    auto arr = lepus::CArray::Create();
    do {
      if ((this->*is_token)(t)) {
        CSSValue value = consume(t);
        if (value.IsEmpty()) {
          return false;
        }
        arr->emplace_back(std::move(value.GetValue()));
      } else {
        return false;
      }
    } while (Consume(TokenType::COMMA));
    ret.SetArray(std::move(arr));
    return AtEnd();
  }
}

bool CSSStringParser::ParseTime(bool single, bool no_negative, CSSValue &ret) {
  enable_time_legacy_ = !parser_configs_.enable_new_time_handler;
  return ParseNumberOrArray(
      single, &CSSStringParser::TimeValue,
      [no_negative](const Token &t) {
        double number = TimeToNumber(t);
        if (no_negative && number < 0) {
          return CSSValue::Empty();
        }
        return CSSValue(lepus::Value(number), CSSValuePattern::NUMBER);
      },
      ret);
}

bool CSSStringParser::ParseTimingFunction(bool single, CSSValue &ret) {
  return ParseNumberOrArray(
      single, &CSSStringParser::TimingFunctionValue,
      [&configs = parser_configs_](const Token &t) {
        return ConsumeTimingFunction(t, configs);
      },
      ret);
}

bool CSSStringParser::AnimationDirectionValue(Token &token) {
  if (ConsumeAndSave(TokenType::NORMAL, token) ||
      ConsumeAndSave(TokenType::REVERSE, token) ||
      ConsumeAndSave(TokenType::ALTERNATE, token) ||
      ConsumeAndSave(TokenType::ALTERNATE_REVERSE, token)) {
    return true;
  }
  return false;
}

starlight::AnimationDirectionType
CSSStringParser::TokenToAnimationDirectionType(const Token &token) {
  switch (token.type) {
    case TokenType::ALTERNATE_REVERSE:
      return starlight::AnimationDirectionType::kAlternateReverse;
    case TokenType::ALTERNATE:
      return starlight::AnimationDirectionType::kAlternate;
    case TokenType::REVERSE:
      return starlight::AnimationDirectionType::kReverse;
    case TokenType::NORMAL:
    default:
      return starlight::AnimationDirectionType::kNormal;
  }
}

bool CSSStringParser::ParseAnimationDirection(bool single, CSSValue &ret) {
  return ParseNumberOrArray(
      single, &CSSStringParser::AnimationDirectionValue,
      [](const Token &t) {
        return CSSValue(
            lepus::Value(static_cast<int>(TokenToAnimationDirectionType(t))),
            CSSValuePattern::ENUM);
      },
      ret);
}

bool CSSStringParser::AnimationFillModeValue(Token &token) {
  if (ConsumeAndSave(TokenType::NONE, token) ||
      ConsumeAndSave(TokenType::FORWARDS, token) ||
      ConsumeAndSave(TokenType::BACKWARDS, token) ||
      ConsumeAndSave(TokenType::BOTH, token)) {
    return true;
  }
  return false;
}

starlight::AnimationFillModeType CSSStringParser::TokenToAnimationFillModeType(
    const Token &token) {
  switch (token.type) {
    case TokenType::FORWARDS:
      return starlight::AnimationFillModeType::kForwards;
    case TokenType::BACKWARDS:
      return starlight::AnimationFillModeType::kBackwards;
    case TokenType::BOTH:
      return starlight::AnimationFillModeType::kBoth;
    case TokenType::NONE:
    default:
      return starlight::AnimationFillModeType::kNone;
  }
}

bool CSSStringParser::ParseAnimationFillMode(bool single, CSSValue &ret) {
  return ParseNumberOrArray(
      single, &CSSStringParser::AnimationFillModeValue,
      [](const Token &t) {
        return CSSValue(
            lepus::Value(static_cast<int>(TokenToAnimationFillModeType(t))),
            CSSValuePattern::ENUM);
      },
      ret);
}

bool CSSStringParser::AnimationIterCountValue(Token &token) {
  if (ConsumeAndSave(TokenType::INFINITE, token)) {
    return true;
  }
  return NumberValue(token);
}

double CSSStringParser::TokenToAnimationIterCount(const Token &token) {
  static constexpr double infinite = 10E8;
  if (token.type == TokenType::INFINITE) {
    return infinite;
  } else {
    return TokenToDouble(token);
  }
}

bool CSSStringParser::ParseAnimationIterCount(bool single, CSSValue &ret) {
  return ParseNumberOrArray(
      single, &CSSStringParser::AnimationIterCountValue,
      [](const Token &t) {
        double num = TokenToAnimationIterCount(t);
        if (num < 0) {
          return CSSValue::Empty();
        }
        return CSSValue(lepus::Value(num), CSSValuePattern::NUMBER);
      },
      ret);
}

bool CSSStringParser::AnimationPlayStateValue(Token &token) {
  if (ConsumeAndSave(TokenType::PAUSED, token) ||
      ConsumeAndSave(TokenType::RUNNING, token)) {
    return true;
  }
  return false;
}

bool CSSStringParser::AnimationNameValue(Token &token) {
  SkipWhitespaceToken();
  // keyword and ident
  if (current_token_.IsIdent()) {
    token = current_token_;
    Advance();
    return true;
  }
  return false;
}

starlight::AnimationPlayStateType CSSStringParser::TokenToAnimationPlayState(
    const Token &token) {
  if (token.type == TokenType::PAUSED) {
    return starlight::AnimationPlayStateType::kPaused;
  }
  return starlight::AnimationPlayStateType::kRunning;
}

bool CSSStringParser::ParseAnimationPlayState(bool single, CSSValue &ret) {
  return ParseNumberOrArray(
      single, &CSSStringParser::AnimationPlayStateValue,
      [](const Token &t) {
        return CSSValue(
            lepus::Value(static_cast<int>(TokenToAnimationPlayState(t))),
            CSSValuePattern::ENUM);
      },
      ret);
}

bool CSSStringParser::ParseAnimationName(bool single, CSSValue &ret) {
  return ParseNumberOrArray(
      single, &CSSStringParser::AnimationNameValue,
      [](const Token &t) {
        return CSSValue(lepus::Value(std::string(t.start, t.length)),
                        CSSValuePattern::STRING);
      },
      ret);
}

bool CSSStringParser::ParseTransitionProperty(bool single, CSSValue &ret) {
  return ParseNumberOrArray(
      single, &CSSStringParser::TransitionProperty,
      [&config = parser_configs_](const Token &t) {
        return CSSValue(
            lepus::Value(static_cast<int>(TokenToTransitionType(t, config))),
            CSSValuePattern::ENUM);
      },
      ret);
}

bool CSSStringParser::Transition(CSSTransitionLayer &layer) {
  SkipWhitespaceToken();
  if (AtEnd()) {
    return false;
  }
  Token t;
  bool longhands[4] = {};
  while (!Check(TokenType::COMMA) && !Check(TokenType::TOKEN_EOF)) {
    if (TimeValue(t)) {
      double time = TimeToNumber(t);
      if (!longhands[1] && time >= 0) {
        longhands[1] = true;
        layer.duration = time;
      } else if (!longhands[2]) {
        longhands[2] = true;
        layer.delay = time;
      } else {
        return false;
      }
    } else if (TimingFunctionValue(t)) {
      if (longhands[3]) {
        return false;
      }
      longhands[3] = true;
      layer.timing_function = ConsumeTimingFunction(t, parser_configs_);
    } else if (TransitionProperty(t)) {
      if (!longhands[0]) {
        longhands[0] = true;
        layer.property = TokenToTransitionType(t, parser_configs_);
      } else {
        return false;
      }
    } else {
      return false;
    }
  }
  return true;
}

bool CSSStringParser::ParseTransition(bool single, lepus::Value arr[4]) {
  Advance();
  if (single) {
    CSSTransitionLayer layer;
    if (!Transition(layer)) {
      return false;
    }
    arr[0].SetNumber(static_cast<int>(layer.property));
    arr[1].SetNumber(layer.duration);
    arr[2].SetNumber(layer.delay);
    auto timing_array = lepus::CArray::Create();
    timing_array->emplace_back(std::move(layer.timing_function.GetValue()));
    arr[3].SetArray(std::move(timing_array));
    return AtEnd();
  } else {
    auto property_array = lepus::CArray::Create();
    auto duration_array = lepus::CArray::Create();
    auto delay_array = lepus::CArray::Create();
    auto timing_array = lepus::CArray::Create();
    bool has_property_none = false;

    do {
      CSSTransitionLayer layer;
      if (!Transition(layer)) {
        return false;
      }
      if (layer.property == starlight::AnimationPropertyType::kNone) {
        if (has_property_none) {  // Only once otherwise failed
          return false;
        }
        has_property_none = true;
      }
      property_array->emplace_back(static_cast<int>(layer.property));
      duration_array->emplace_back(layer.duration);
      delay_array->emplace_back(layer.delay);
      timing_array->emplace_back(std::move(layer.timing_function.GetValue()));
    } while (Consume(TokenType::COMMA));

    arr[0].SetArray(std::move(property_array));
    arr[1].SetArray(std::move(duration_array));
    arr[2].SetArray(std::move(delay_array));
    arr[3].SetArray(std::move(timing_array));
    return AtEnd();
  }
}

bool CSSStringParser::Animation(CSSAnimationLayer &layer) {
  SkipWhitespaceToken();
  if (AtEnd()) {
    return false;
  }
  Token t;
  // [duration, delay, timing, count, direction, fill_mode, play_state, name]
  bool longhands[8] = {};
  while (!Check(TokenType::COMMA) && !Check(TokenType::TOKEN_EOF)) {
    if (TimeValue(t)) {
      double time = TimeToNumber(t);
      if (!longhands[0] && time >= 0) {
        longhands[0] = true;
        layer.duration = time;
      } else if (!longhands[1]) {
        longhands[1] = true;
        layer.delay = time;
      } else {
        return false;
      }
    } else if (TimingFunctionValue(t)) {
      if (longhands[2]) {
        return false;
      }
      longhands[2] = true;
      layer.timing_function = ConsumeTimingFunction(t, parser_configs_);
    } else if (AnimationIterCountValue(t)) {
      if (!longhands[3]) {
        longhands[3] = true;
        layer.count = TokenToAnimationIterCount(t);
        if (layer.count < 0) {
          return false;
        }
      } else {
        return false;
      }
    } else if (AnimationDirectionValue(t)) {
      if (!longhands[4]) {
        longhands[4] = true;
        layer.direction = TokenToAnimationDirectionType(t);
      } else {
        return false;
      }
    } else if (AnimationFillModeValue(t)) {
      if (!longhands[5]) {
        longhands[5] = true;
        layer.fill_mode = TokenToAnimationFillModeType(t);
      } else {
        return false;
      }
    } else if (AnimationPlayStateValue(t)) {
      if (!longhands[6]) {
        longhands[6] = true;
        layer.play_state = TokenToAnimationPlayState(t);
      } else {
        return false;
      }
    } else if (AnimationNameValue(t)) {
      if (!longhands[7]) {
        longhands[7] = true;
        layer.name = std::string(t.start, t.length);
      } else {
        return false;
      }
    } else {
      return false;
    }
  }
  return true;
}

bool CSSStringParser::ParseAnimation(bool single, lepus::Value arr[8]) {
  Advance();
  if (single) {
    CSSAnimationLayer layer;
    if (!Animation(layer)) {
      return false;
    }
    // [name, duration, delay, timing, count, direction, fill_mode, play_state]
    arr[0].SetString(layer.name);
    arr[1].SetNumber(layer.duration);
    arr[2].SetNumber(layer.delay);
    auto timing_array = lepus::CArray::Create();
    timing_array->emplace_back(std::move(layer.timing_function.GetValue()));
    arr[3].SetArray(std::move(timing_array));
    arr[4].SetNumber(layer.count);
    arr[5].SetNumber(static_cast<int>(layer.direction));
    arr[6].SetNumber(static_cast<int>(layer.fill_mode));
    arr[7].SetNumber(static_cast<int>(layer.play_state));
    return AtEnd();
  } else {
    auto name_array = lepus::CArray::Create();
    auto duration_array = lepus::CArray::Create();
    auto delay_array = lepus::CArray::Create();
    auto timing_array = lepus::CArray::Create();
    auto count_array = lepus::CArray::Create();
    auto direction_array = lepus::CArray::Create();
    auto fill_mode_array = lepus::CArray::Create();
    auto play_state_array = lepus::CArray::Create();

    do {
      CSSAnimationLayer layer;
      if (!Animation(layer)) {
        return false;
      }
      name_array->emplace_back(layer.name);
      duration_array->emplace_back(layer.duration);
      delay_array->emplace_back(layer.delay);
      timing_array->emplace_back(std::move(layer.timing_function.GetValue()));
      count_array->emplace_back(layer.count);
      direction_array->emplace_back(static_cast<int>(layer.direction));
      fill_mode_array->emplace_back(static_cast<int>(layer.fill_mode));
      play_state_array->emplace_back(static_cast<int>(layer.play_state));
    } while (Consume(TokenType::COMMA));

    // [name, duration, delay, timing, count, direction, fill_mode, play_state]
    arr[0].SetArray(std::move(name_array));
    arr[1].SetArray(std::move(duration_array));
    arr[2].SetArray(std::move(delay_array));
    arr[3].SetArray(std::move(timing_array));
    arr[4].SetArray(std::move(count_array));
    arr[5].SetArray(std::move(direction_array));
    arr[6].SetArray(std::move(fill_mode_array));
    arr[7].SetArray(std::move(play_state_array));
    return AtEnd();
  }
}

}  // namespace tasm
}  // namespace lynx
