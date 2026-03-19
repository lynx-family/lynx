// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/css/parser/auto_font_size_line_ranges_handler.h"

#include <utility>

#include "base/include/value/array.h"
#include "core/renderer/css/parser/css_string_parser.h"
#include "core/renderer/css/unit_handler.h"

namespace lynx {
namespace tasm {
namespace AutoFontSizeLineRangesHandler {

HANDLER_IMPL() {
  CSS_HANDLER_FAIL_IF_NOT(input.IsString(), configs.enable_css_strict_mode,
                          TYPE_MUST_BE, CSSProperty::GetPropertyNameCStr(key),
                          STRING_TYPE)

  CSSStringParser parser = CSSStringParser::FromLepusString(input, configs);
  auto arr = lepus::CArray::Create();
  if (!parser.ParseAutoFontSizeLineRanges(arr)) {
    return false;
  }

  output.emplace_or_assign(key, std::move(arr));
  return true;
}

HANDLER_REGISTER_IMPL() { array[kPropertyIDXAutoFontSizeLineRanges] = &Handle; }

}  // namespace AutoFontSizeLineRangesHandler
}  // namespace tasm
}  // namespace lynx
