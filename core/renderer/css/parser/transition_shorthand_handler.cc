// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/renderer/css/parser/transition_shorthand_handler.h"

#include <string>
#include <vector>

#include "core/renderer/css/parser/css_string_parser.h"
#include "core/renderer/css/unit_handler.h"
#include "core/runtime/vm/lepus/array.h"

namespace lynx {
namespace tasm {
namespace TransitionShorthandHandler {

HANDLER_IMPL() {
  CSS_HANDLER_FAIL_IF_NOT(input.IsString(), configs.enable_css_strict_mode,
                          TYPE_MUST_BE, CSSProperty::GetPropertyNameCStr(key),
                          STRING_TYPE)

  CSSStringParser parser = CSSStringParser::FromLepusString(input, configs);
  bool single = std::strchr(parser.content(), ',') == nullptr;
  lepus::Value arr[4];
  if (!parser.ParseTransition(single, arr)) {
    return false;
  }
  // [property, duration, delay, timing-function]
  output.emplace_or_assign(
      CSSPropertyID::kPropertyIDTransitionProperty, arr[0],
      single ? CSSValuePattern::ENUM : CSSValuePattern::ARRAY);
  output.emplace_or_assign(
      CSSPropertyID::kPropertyIDTransitionDuration, arr[1],
      single ? CSSValuePattern::NUMBER : CSSValuePattern::ARRAY);
  output.emplace_or_assign(
      CSSPropertyID::kPropertyIDTransitionDelay, arr[2],
      single ? CSSValuePattern::NUMBER : CSSValuePattern::ARRAY);
  output.emplace_or_assign(CSSPropertyID::kPropertyIDTransitionTimingFunction,
                           arr[3].Array());
  return true;
}

HANDLER_REGISTER_IMPL() { array[kPropertyIDTransition] = &Handle; }

}  // namespace TransitionShorthandHandler
}  // namespace tasm
}  // namespace lynx
