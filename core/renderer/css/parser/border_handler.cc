// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/renderer/css/parser/border_handler.h"

#include <string>
#include <utility>
#include <vector>

#include "core/renderer/css/css_style_utils.h"
#include "core/renderer/css/parser/css_string_parser.h"
#include "core/renderer/css/parser/four_sides_shorthand_handler.h"
#include "core/renderer/css/unit_handler.h"

namespace lynx {
namespace tasm {
namespace BorderHandler {

static inline void AddProperty(CSSPropertyID property, CSSValue&& value,
                               StyleMap& output) {
  if (value.IsEmpty()) {
    return;
  }
  output.insert_or_assign(property, std::move(value));
}

HANDLER_IMPL() {
  CSS_HANDLER_FAIL_IF_NOT(input.IsString(), configs.enable_css_strict_mode,
                          TYPE_MUST_BE, CSSProperty::GetPropertyNameCStr(key),
                          STRING_TYPE)

  CSSStringParser parser = CSSStringParser::FromLepusString(input, configs);
  CSSValue result_width = CSSValue::Empty();
  CSSValue result_style = CSSValue::Empty();
  CSSValue result_color = CSSValue::Empty();
  bool ret = parser.ParseBorder(result_width, result_style, result_color);
  if (!ret) {
    return false;
  }
  if (key == kPropertyIDBorder) {
    FourSidesShorthandHandler::AddProperty(kPropertyIDBorderWidth,
                                           std::move(result_width), output);
    FourSidesShorthandHandler::AddProperty(kPropertyIDBorderColor,
                                           std::move(result_color), output);
    FourSidesShorthandHandler::AddProperty(kPropertyIDBorderStyle,
                                           std::move(result_style), output);
  } else {
    CSSPropertyID width_id;
    CSSPropertyID style_id;
    CSSPropertyID color_id;
    switch (key) {
      case kPropertyIDBorderTop:
        width_id = kPropertyIDBorderTopWidth;
        style_id = kPropertyIDBorderTopStyle;
        color_id = kPropertyIDBorderTopColor;
        break;
      case kPropertyIDBorderRight:
        width_id = kPropertyIDBorderRightWidth;
        style_id = kPropertyIDBorderRightStyle;
        color_id = kPropertyIDBorderRightColor;
        break;
      case kPropertyIDBorderBottom:
        width_id = kPropertyIDBorderBottomWidth;
        style_id = kPropertyIDBorderBottomStyle;
        color_id = kPropertyIDBorderBottomColor;
        break;
      case kPropertyIDBorderLeft:
        width_id = kPropertyIDBorderLeftWidth;
        style_id = kPropertyIDBorderLeftStyle;
        color_id = kPropertyIDBorderLeftColor;
        break;
      case kPropertyIDOutline:
        width_id = kPropertyIDOutlineWidth;
        style_id = kPropertyIDOutlineStyle;
        color_id = kPropertyIDOutlineColor;
        break;
      default:
        UnitHandler::CSSUnreachable(configs.enable_css_strict_mode,
                                    "BorderCombineInterceptor id unreachable!");
        return false;
    }

    AddProperty(width_id, std::move(result_width), output);
    AddProperty(color_id, std::move(result_color), output);
    AddProperty(style_id, std::move(result_style), output);
  }
  return ret;
}

HANDLER_REGISTER_IMPL() {
  array[kPropertyIDBorder] = &Handle;
  array[kPropertyIDBorderTop] = &Handle;
  array[kPropertyIDBorderRight] = &Handle;
  array[kPropertyIDBorderBottom] = &Handle;
  array[kPropertyIDBorderLeft] = &Handle;
  array[kPropertyIDOutline] = &Handle;
}

}  // namespace BorderHandler
}  // namespace tasm
}  // namespace lynx
