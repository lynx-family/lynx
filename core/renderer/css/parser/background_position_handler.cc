// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/renderer/css/parser/background_position_handler.h"

#include <string>
#include <utility>

#include "base/include/debug/lynx_assert.h"
#include "core/renderer/css/parser/css_string_parser.h"
#include "core/renderer/css/unit_handler.h"

namespace lynx {
namespace tasm {
namespace BackgroundPositionHandler {

HANDLER_IMPL() {
  CSS_HANDLER_FAIL_IF_NOT(input.IsString(), configs.enable_css_strict_mode,
                          TYPE_MUST_BE, CSSProperty::GetPropertyNameCStr(key),
                          STRING_TYPE)

  CSSStringParser parser = CSSStringParser::FromLepusString(input, configs);
  auto ret = parser.ParseBackgroundPosition();
  if (ret.IsEmpty()) {
    return false;
  }
  output.insert_or_assign(key, std::move(ret));
  return true;
}

HANDLER_REGISTER_IMPL() {
  array[kPropertyIDBackgroundPosition] = &Handle;
  array[kPropertyIDMaskPosition] = &Handle;
}

}  // namespace BackgroundPositionHandler
}  // namespace tasm
}  // namespace lynx
