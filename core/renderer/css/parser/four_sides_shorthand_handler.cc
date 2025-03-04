// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/renderer/css/parser/four_sides_shorthand_handler.h"

#include <string>
#include <utility>
#include <vector>

#include "base/include/debug/lynx_assert.h"
#include "base/include/string/string_utils.h"
#include "core/renderer/css/unit_handler.h"

namespace lynx {
namespace tasm {

namespace {
constexpr CSSPropertyID kMarginIDs[] = {
    kPropertyIDMarginTop, kPropertyIDMarginRight, kPropertyIDMarginBottom,
    kPropertyIDMarginLeft};
constexpr CSSPropertyID kBorderWidthIDs[] = {
    kPropertyIDBorderTopWidth, kPropertyIDBorderRightWidth,
    kPropertyIDBorderBottomWidth, kPropertyIDBorderLeftWidth};
constexpr CSSPropertyID kPaddingsIDs[] = {
    kPropertyIDPaddingTop, kPropertyIDPaddingRight, kPropertyIDPaddingBottom,
    kPropertyIDPaddingLeft};
constexpr CSSPropertyID kBorderColorIDs[] = {
    kPropertyIDBorderTopColor, kPropertyIDBorderRightColor,
    kPropertyIDBorderBottomColor, kPropertyIDBorderLeftColor};
constexpr CSSPropertyID kBorderStyleIDs[] = {
    kPropertyIDBorderTopStyle, kPropertyIDBorderRightStyle,
    kPropertyIDBorderBottomStyle, kPropertyIDBorderLeftStyle};
}  // namespace

namespace FourSidesShorthandHandler {

const CSSPropertyID* GetLonghandProperties(CSSPropertyID property) {
  const CSSPropertyID* properties = nullptr;
  switch (property) {
    case kPropertyIDMargin:
      properties = kMarginIDs;
      break;
    case kPropertyIDBorderWidth:
      properties = kBorderWidthIDs;
      break;
    case kPropertyIDPadding:
      properties = kPaddingsIDs;
      break;
    case kPropertyIDBorderColor:
      properties = kBorderColorIDs;
      break;
    case kPropertyIDBorderStyle:
      properties = kBorderStyleIDs;
      break;
    default:
      break;
  }
  return properties;
}

HANDLER_IMPL() {
  const CSSPropertyID* properties = GetLonghandProperties(key);
  if (!properties) {
    return false;
  }

  if (input.IsString()) {
    auto str = input.StringView();
    base::InlineVector<std::string, 4> combines;
    base::SplitStringBySpaceOutOfBrackets(str, combines);
    switch (combines.size()) {
      case 1: {
        UnitHandler::Process(properties[0], lepus::Value(combines[0]), output,
                             configs);
        auto it = output.find(properties[0]);
        if (it != output.end()) {
          output.insert_or_assign(properties[1], it->second);
          output.insert_or_assign(properties[2], it->second);
          output.insert_or_assign(properties[3], it->second);
        } else {
          return false;
        }
      } break;
      case 2: {
        UnitHandler::Process(properties[0], lepus::Value(combines[0]), output,
                             configs);
        UnitHandler::Process(properties[1], lepus::Value(combines[1]), output,
                             configs);
        auto it0 = output.find(properties[0]);
        auto it1 = output.find(properties[1]);
        if (it0 != output.end() && it1 != output.end()) {
          output.insert_or_assign(properties[2], it0->second);
          output.insert_or_assign(properties[3], it1->second);
        } else {
          return false;
        }
      } break;
      case 3: {
        UnitHandler::Process(properties[0], lepus::Value(combines[0]), output,
                             configs);
        UnitHandler::Process(properties[1], lepus::Value(combines[1]), output,
                             configs);
        UnitHandler::Process(properties[2], lepus::Value(combines[2]), output,
                             configs);
        auto it = output.find(properties[1]);
        if (it != output.end()) {
          output.insert_or_assign(properties[3], it->second);
        } else {
          return false;
        }
      } break;
      case 4: {
        UnitHandler::Process(properties[0], lepus::Value(combines[0]), output,
                             configs);
        UnitHandler::Process(properties[1], lepus::Value(combines[1]), output,
                             configs);
        UnitHandler::Process(properties[2], lepus::Value(combines[2]), output,
                             configs);
        UnitHandler::Process(properties[3], lepus::Value(combines[3]), output,
                             configs);
      } break;
      default:
        return false;
    }
  } else if (input.IsNumber()) {
    CSS_HANDLER_FAIL_IF(
        key == kPropertyIDBorderColor || key == kPropertyIDBorderStyle,
        TYPE_MUST_BE, CSSProperty::GetPropertyNameCStr(key), STRING_TYPE)
    UnitHandler::Process(properties[0], input, output, configs);
    if (auto find0 = output.find(properties[0]); find0 != output.end()) {
      output.insert_or_assign(properties[1], find0->second);
      output.insert_or_assign(properties[2], find0->second);
      output.insert_or_assign(properties[3], find0->second);
    } else {
      return false;
    }
  } else {
    CSS_HANDLER_FAIL_IF_NOT(false, configs.enable_css_strict_mode, TYPE_MUST_BE,
                            CSSProperty::GetPropertyNameCStr(key),
                            STRING_OR_NUMBER_TYPE)
  }
  return true;
}

HANDLER_REGISTER_IMPL() {
  array[kPropertyIDMargin] = &Handle;
  array[kPropertyIDPadding] = &Handle;
  array[kPropertyIDBorderWidth] = &Handle;
  array[kPropertyIDBorderColor] = &Handle;
  array[kPropertyIDBorderStyle] = &Handle;
}

void AddProperty(CSSPropertyID property, CSSValue&& value, StyleMap& output) {
  auto properties = GetLonghandProperties(property);
  if (!properties || value.IsEmpty()) {
    return;
  }
  output.insert_or_assign(properties[0], value);
  output.insert_or_assign(properties[1], value);
  output.insert_or_assign(properties[2], value);
  output.insert_or_assign(properties[3], std::move(value));
}

}  // namespace FourSidesShorthandHandler
}  // namespace tasm
}  // namespace lynx
