// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/css/parser/text_stroke_handler.h"

#include <string>
#include <utility>
#include <vector>

#include "base/include/string/string_utils.h"
#include "core/renderer/css/css_color.h"
#include "core/renderer/css/css_style_utils.h"
#include "core/renderer/css/parser/color_handler.h"
#include "core/renderer/css/parser/css_string_parser.h"
#include "core/renderer/css/unit_handler.h"

namespace lynx {
namespace tasm {
namespace TextStrokeHandler {
HANDLER_IMPL() {
  CSS_HANDLER_FAIL_IF_NOT(input.IsString(), configs.enable_css_strict_mode,
                          TYPE_MUST_BE, CSSProperty::GetPropertyNameCStr(key),
                          STRING_TYPE)

  CSSStringParser parser = CSSStringParser::FromLepusString(input, configs);
  CSSValue result_width = CSSValue::Empty();
  CSSValue result_color = CSSValue::Empty();
  bool ret = parser.ParseTextStroke(result_width, result_color);
  output.insert_or_assign(kPropertyIDTextStrokeWidth,
                          ret ? std::move(result_width) : CSSValue::Empty());
  output.insert_or_assign(kPropertyIDTextStrokeColor,
                          ret ? std::move(result_color) : CSSValue::Empty());
  return ret;
}

HANDLER_REGISTER_IMPL() { array[kPropertyIDTextStroke] = &Handle; }
}  // namespace TextStrokeHandler
}  // namespace tasm
}  // namespace lynx
