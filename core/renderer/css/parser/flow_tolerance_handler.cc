// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/css/parser/flow_tolerance_handler.h"

#include <utility>

#include "core/renderer/css/parser/css_string_parser.h"
#include "core/renderer/css/parser/length_handler.h"
#include "core/renderer/css/unit_handler.h"

namespace lynx {
namespace tasm {
namespace FlowToleranceHandler {

HANDLER_IMPL() {
  CSS_HANDLER_FAIL_IF_NOT(
      configs.enable_grid_lanes, configs.enable_css_strict_mode,
      TYPE_UNSUPPORTED, CSSProperty::GetPropertyNameCStr(key), input.CString())

  CSSValue parsed;
  if (input.IsString()) {
    CSSStringParser parser = CSSStringParser::FromLepusString(input, configs);
    parsed = parser.ParseFlowTolerance();
  } else if (input.IsNumber()) {
    parsed.SetValueAndPattern(input, CSSValuePattern::NUMBER);
  }
  CSS_HANDLER_FAIL_IF_NOT(
      !parsed.IsEmpty() &&
          (parsed.IsCalc() || parsed.IsEnum() || parsed.GetNumber() >= 0),
      configs.enable_css_strict_mode, TYPE_UNSUPPORTED,
      CSSProperty::GetPropertyNameCStr(key), input.CString())
  LengthHandler::CheckLengthUnitValid(key, input, parsed, configs);
  output.insert_or_assign(key, std::move(parsed));
  return true;
}

HANDLER_REGISTER_IMPL() { array[kPropertyIDFlowTolerance] = &Handle; }

}  // namespace FlowToleranceHandler
}  // namespace tasm
}  // namespace lynx
