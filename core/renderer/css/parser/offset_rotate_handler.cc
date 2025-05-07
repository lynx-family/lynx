// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/css/parser/offset_rotate_handler.h"

#include <string>

#include "core/renderer/css/parser/css_string_parser.h"
#include "core/renderer/css/unit_handler.h"

namespace lynx {
namespace tasm {
namespace OffsetRotateHandler {
HANDLER_IMPL() {
  CSS_HANDLER_FAIL_IF_NOT(input.IsString(), configs.enable_css_strict_mode,
                          TYPE_MUST_BE, CSSProperty::GetPropertyNameCStr(key),
                          STRING_TYPE)

  CSSStringParser parser = CSSStringParser::FromLepusString(input, configs);
  lepus::Value result = parser.ParseOffsetRotate();
  CSS_HANDLER_FAIL_IF_NOT(result.IsNumber(), configs.enable_css_strict_mode,
                          "offset-rotate format error.")
  output.emplace_or_assign(key, result.Number(), CSSValue::kCreateNumberTag);
  return true;
}

HANDLER_REGISTER_IMPL() { array[kPropertyIDOffsetRotate] = &Handle; }
}  // namespace OffsetRotateHandler
}  // namespace tasm
}  // namespace lynx
