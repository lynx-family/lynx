// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/renderer/css/parser/border_width_handler.h"

#include <string>
#include <utility>

#include "core/renderer/css/parser/css_string_parser.h"
#include "core/renderer/css/unit_handler.h"

namespace lynx {
namespace tasm {
namespace BorderWidthHandler {

HANDLER_IMPL() {
  if (input.IsString()) {
    CSSValue result = CSSValue::Empty();
    CSSStringParser parser = CSSStringParser::FromLepusString(input, configs);
    auto ret = parser.ParseBorderLineWidth(result);
    if (ret && !result.IsEmpty()) {
      output.insert_or_assign(key, std::move(result));
    }
    return ret;
  } else if (input.IsNumber()) {
    return LengthHandler::Handle(key, input, output, configs);
  } else {
    CSS_HANDLER_FAIL_IF_NOT(false, configs.enable_css_strict_mode, TYPE_MUST_BE,
                            CSSProperty::GetPropertyNameCStr(key),
                            STRING_OR_NUMBER_TYPE)
  }
}

}  // namespace BorderWidthHandler
}  // namespace tasm
}  // namespace lynx
