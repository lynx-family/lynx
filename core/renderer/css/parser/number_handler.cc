// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/renderer/css/parser/number_handler.h"

#include "base/include/debug/lynx_assert.h"
#include "base/include/string/string_number_convert.h"
#include "core/renderer/css/unit_handler.h"

namespace lynx {
namespace tasm {
namespace NumberHandler {

HANDLER_IMPL() {
  double num = 0;
  if (input.IsNumber()) {
    num = input.Number();
  } else if (input.IsString()) {
    const auto& str = input.StdString();
    if (str == "infinite") {
      num = 10E8;
    } else {
      CSS_HANDLER_FAIL_IF_NOT(
          base::StringToDouble(str, num, true), configs.enable_css_strict_mode,
          TYPE_UNSUPPORTED, CSSProperty::GetPropertyNameCStr(key), str.c_str())
    }
  } else {
    CSS_HANDLER_FAIL_IF_NOT(false, configs.enable_css_strict_mode, TYPE_MUST_BE,
                            FLOAT_TYPE, STRING_OR_NUMBER_TYPE)
  }
  output.emplace_or_assign(key, num, CSSValue::kCreateNumberTag);
  return true;
}

}  // namespace NumberHandler
}  // namespace tasm
}  // namespace lynx
