// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/renderer/css/parser/auto_font_size_handler.h"

#include <string>
#include <utility>
#include <vector>

#include "base/include/debug/lynx_assert.h"
#include "base/include/string/string_utils.h"
#include "core/renderer/css/parser/css_string_parser.h"
#include "core/renderer/css/parser/length_handler.h"
#include "core/renderer/css/unit_handler.h"
#include "core/runtime/vm/lepus/array.h"

namespace lynx {
namespace tasm {
namespace AutoFontSizeHandler {

HANDLER_IMPL() {
  CSS_HANDLER_FAIL_IF_NOT(input.IsString(), configs.enable_css_strict_mode,
                          TYPE_MUST_BE, CSSProperty::GetPropertyNameCStr(key),
                          STRING_TYPE)

  auto array = lepus::CArray::Create();

  CSSValue is_auto_font_size =
      CSSValue(lepus::Value(false), CSSValuePattern::BOOLEAN);
  CSSValue auto_font_size_min_size =
      CSSValue(lepus::Value(0), CSSValuePattern::PX);
  CSSValue auto_font_size_max_size =
      CSSValue(lepus::Value(0), CSSValuePattern::PX);
  CSSValue auto_font_size_step_granularity =
      CSSValue(lepus::Value(1), CSSValuePattern::PX);
  CSSStringParser parser = CSSStringParser::FromLepusString(input, configs);
  auto res = parser.ParseAutoFontSize(
      is_auto_font_size, auto_font_size_min_size, auto_font_size_max_size,
      auto_font_size_step_granularity);
  if (!res) {
    return false;
  }

  array->emplace_back(is_auto_font_size.GetValue());
  array->emplace_back(auto_font_size_min_size.GetValue());
  array->emplace_back(
      static_cast<int32_t>(auto_font_size_min_size.GetPattern()));
  array->emplace_back(auto_font_size_max_size.GetValue());
  array->emplace_back(
      static_cast<int32_t>(auto_font_size_max_size.GetPattern()));
  array->emplace_back(auto_font_size_step_granularity.GetValue());
  array->emplace_back(
      static_cast<int32_t>(auto_font_size_step_granularity.GetPattern()));

  output.emplace_or_assign(key, std::move(array));
  return true;
}

HANDLER_REGISTER_IMPL() { array[kPropertyIDXAutoFontSize] = &Handle; }

}  // namespace AutoFontSizeHandler
}  // namespace tasm
}  // namespace lynx
