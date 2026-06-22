// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/css/parser/offset_distance_handler.h"

#include "core/renderer/css/parser/css_string_parser.h"
#include "core/renderer/css/unit_handler.h"

namespace lynx {
namespace tasm {
namespace OffsetDistanceHandler {

HANDLER_IMPL() {
  double distance = 0;
  if (input.IsNumber()) {
    distance = input.Number();
  } else if (input.IsString()) {
    CSSStringParser parser = CSSStringParser::FromLepusString(input, configs);
    lepus::Value result = parser.ParseOffsetDistance();
    CSS_HANDLER_FAIL_IF_NOT(result.IsNumber(), configs.enable_css_strict_mode,
                            "offset-distance format error.")
    distance = result.Number();
  } else {
    CSS_HANDLER_FAIL_IF_NOT(false, configs.enable_css_strict_mode, TYPE_MUST_BE,
                            CSSProperty::GetPropertyNameCStr(key),
                            STRING_OR_NUMBER_TYPE)
  }

  output.emplace_or_assign(key, distance, CSSValuePattern::NUMBER);
  return true;
}

HANDLER_REGISTER_IMPL() { array[kPropertyIDOffsetDistance] = &Handle; }

}  // namespace OffsetDistanceHandler
}  // namespace tasm
}  // namespace lynx
