// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/css/parser/relative_align_handler.h"

#include <string>

#include "base/include/debug/lynx_assert.h"
#include "base/include/string/string_number_convert.h"
#include "core/renderer/css/unit_handler.h"
#include "core/renderer/starlight/style/css_type.h"

namespace lynx {
namespace tasm {

namespace {
static constexpr char kRelativeAlignErrorMessage[] =
    "Value of %s must be \"parent\" or a positive number";
}

namespace RelativeAlignHandler {

HANDLER_IMPL() {
  bool valid = false;
  int result = starlight::RelativeAlignType::kNone;

  if (input.IsString()) {
    const auto& str = input.StdString();
    if (str == "parent") {
      valid = true;
      result = starlight::RelativeAlignType::kParent;
    } else {
      // TODO, parse from std::string_view with std::from_chars. Current
      // toolchain does not support that.
      valid = base::StringToInt(str, &result, 10);
    }
  } else if (input.IsNumber()) {
    int number = input.Number();
    valid = number > 0;
    result = number;
  }
  CSS_HANDLER_FAIL_IF_NOT(valid, configs.enable_css_strict_mode,
                          kRelativeAlignErrorMessage,
                          CSSProperty::GetPropertyNameCStr(key))
  output.emplace_or_assign(key, lepus::Value(result), CSSValuePattern::NUMBER);
  return true;
}

HANDLER_REGISTER_IMPL() {
  array[kPropertyIDRelativeAlignTop] = &Handle;
  array[kPropertyIDRelativeAlignBottom] = &Handle;
  array[kPropertyIDRelativeAlignLeft] = &Handle;
  array[kPropertyIDRelativeAlignRight] = &Handle;
  array[kPropertyIDRelativeAlignInlineStart] = &Handle;
  array[kPropertyIDRelativeAlignInlineEnd] = &Handle;
}

}  // namespace RelativeAlignHandler
}  // namespace tasm
}  // namespace lynx
