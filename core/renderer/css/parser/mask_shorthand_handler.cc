// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/renderer/css/parser/mask_shorthand_handler.h"

#include <string>

#include "base/include/debug/lynx_assert.h"
#include "base/include/string/string_utils.h"
#include "core/renderer/css/parser/css_string_parser.h"
#include "core/renderer/css/unit_handler.h"
#include "core/runtime/vm/lepus/array.h"

namespace lynx {
namespace tasm {
namespace MaskShorthandHandler {

HANDLER_IMPL() {
  CSS_HANDLER_FAIL_IF_NOT(input.IsString(), configs.enable_css_strict_mode,
                          TYPE_MUST_BE, CSSProperty::GetPropertyNameCStr(key),
                          STRING_TYPE)

  CSSStringParser parser = CSSStringParser::FromLepusString(input, configs);
  parser.SetIsLegacyParser(false);
  auto ret = parser.ParseBackgroundOrMask(true);
  if (ret.IsEmpty()) {
    return false;
  }
  const auto& mask = ret.GetValue().Array();
  output.emplace_or_assign(kPropertyIDMaskImage, mask->get(1).Array());
  output.emplace_or_assign(kPropertyIDMaskPosition, mask->get(2).Array());
  output.emplace_or_assign(kPropertyIDMaskSize, mask->get(3).Array());
  output.emplace_or_assign(kPropertyIDMaskRepeat, mask->get(4).Array());
  output.emplace_or_assign(kPropertyIDMaskOrigin, mask->get(5).Array());
  output.emplace_or_assign(kPropertyIDMaskClip, mask->get(6).Array());
  return true;
}
HANDLER_REGISTER_IMPL() { array[kPropertyIDMask] = &Handle; }
}  // namespace MaskShorthandHandler
}  // namespace tasm
}  // namespace lynx
