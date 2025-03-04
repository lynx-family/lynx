// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/renderer/css/parser/border_style_handler.h"

#include <utility>

#include "core/renderer/css/parser/css_string_parser.h"
#include "core/renderer/css/unit_handler.h"

namespace lynx {
namespace tasm {
namespace BorderStyleHandler {

HANDLER_IMPL() {
  CSS_HANDLER_FAIL_IF_NOT(input.IsString(), configs.enable_css_strict_mode,
                          TYPE_MUST_BE, CSSProperty::GetPropertyNameCStr(key),
                          STRING_TYPE)

  CSSStringParser parser = CSSStringParser::FromLepusString(input, configs);
  CSSValue ret = CSSValue::Empty();
  parser.ParseBorderStyle(ret);
  if (ret.IsEmpty()) {
    CSS_HANDLER_FAIL_IF_NOT(
        false, configs.enable_css_strict_mode, TYPE_UNSUPPORTED,
        CSSProperty::GetPropertyNameCStr(key), parser.content())
  }
  output.insert_or_assign(key, std::move(ret));
  return true;
}

}  // namespace BorderStyleHandler
}  // namespace tasm
}  // namespace lynx
