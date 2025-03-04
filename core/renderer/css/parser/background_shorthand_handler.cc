// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/css/parser/background_shorthand_handler.h"

#include <string>

#include "base/include/debug/lynx_assert.h"
#include "base/include/string/string_utils.h"
#include "core/renderer/css/parser/css_string_parser.h"
#include "core/renderer/css/unit_handler.h"
#include "core/runtime/vm/lepus/array.h"

namespace lynx {
namespace tasm {
namespace BackgroundShorthandHandler {

HANDLER_IMPL() {
  CSS_HANDLER_FAIL_IF_NOT(input.IsString(), configs.enable_css_strict_mode,
                          TYPE_MUST_BE, CSSProperty::GetPropertyNameCStr(key),
                          STRING_TYPE)

  CSSStringParser parser = CSSStringParser::FromLepusString(input, configs);
  parser.SetIsLegacyParser(configs.enable_legacy_parser);
  auto ret = parser.ParseBackgroundOrMask(false);
  if (ret.IsEmpty()) {
    return false;
  }
  const auto& background = ret.GetValue().Array();
  output.emplace_or_assign(kPropertyIDBackgroundColor, background->get(0),
                           CSSValuePattern::NUMBER);
  output.emplace_or_assign(kPropertyIDBackgroundImage,
                           background->get(1).Array());
  // CSS parser and background compatible with old version
  if (background->size() == 7) {
    output.emplace_or_assign(kPropertyIDBackgroundPosition,
                             background->get(2).Array());
    output.emplace_or_assign(kPropertyIDBackgroundSize,
                             background->get(3).Array());
    output.emplace_or_assign(kPropertyIDBackgroundRepeat,
                             background->get(4).Array());
    output.emplace_or_assign(kPropertyIDBackgroundOrigin,
                             background->get(5).Array());
    output.emplace_or_assign(kPropertyIDBackgroundClip,
                             background->get(6).Array());
  }
  return true;
}
HANDLER_REGISTER_IMPL() { array[kPropertyIDBackground] = &Handle; }
}  // namespace BackgroundShorthandHandler
}  // namespace tasm
}  // namespace lynx
