// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/renderer/css/parser/vertical_align_handler.h"

#include <utility>

#include "base/include/debug/lynx_assert.h"
#include "core/renderer/css/parser/length_handler.h"
#include "core/renderer/css/unit_handler.h"
#include "core/runtime/vm/lepus/array.h"

namespace lynx {
namespace tasm {
namespace VerticalAlignHandler {

using starlight::VerticalAlignType;

HANDLER_IMPL() {
  CSS_HANDLER_FAIL_IF_NOT(input.IsString(), configs.enable_css_strict_mode,
                          TYPE_MUST_BE, CSSProperty::GetPropertyNameCStr(key),
                          STRING_TYPE)

  const auto& str = input.StdString();
  auto array = lepus::CArray::Create();
  VerticalAlignType vertical_align_type = VerticalAlignType::kDefault;
  tasm::CSSValue css_value;
  if (str == "baseline") {
    vertical_align_type = VerticalAlignType::kBaseline;
  } else if (str == "sub") {
    vertical_align_type = VerticalAlignType::kSub;
  } else if (str == "super") {
    vertical_align_type = VerticalAlignType::kSuper;
  } else if (str == "top") {
    vertical_align_type = VerticalAlignType::kTop;
  } else if (str == "text-top") {
    vertical_align_type = VerticalAlignType::kTextTop;
  } else if (str == "middle") {
    vertical_align_type = VerticalAlignType::kMiddle;
  } else if (str == "bottom") {
    vertical_align_type = VerticalAlignType::kBottom;
  } else if (str == "text-bottom") {
    vertical_align_type = VerticalAlignType::kTextBottom;
  } else if (str == "center") {
    vertical_align_type = VerticalAlignType::kCenter;
  } else {
    lepus::Value lepus_value(str);
    if (!lynx::tasm::LengthHandler::Process(lepus_value, css_value, configs)) {
      return false;
    }
    if (str[str.length() - 1] == '%') {
      vertical_align_type = VerticalAlignType::kPercent;
    } else {
      vertical_align_type = VerticalAlignType::kLength;
    }
  }

  array->emplace_back(static_cast<int>(vertical_align_type));
  array->emplace_back(static_cast<int>(CSSValuePattern::ENUM));
  array->emplace_back(std::move(css_value.GetValue()));
  array->emplace_back(static_cast<int32_t>(css_value.GetPattern()));
  output.emplace_or_assign(key, std::move(array));
  return true;
}

HANDLER_REGISTER_IMPL() { array[kPropertyIDVerticalAlign] = &Handle; }

}  // namespace VerticalAlignHandler
}  // namespace tasm
}  // namespace lynx
