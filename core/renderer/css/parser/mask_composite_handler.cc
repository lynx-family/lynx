// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/css/parser/mask_composite_handler.h"

#include <utility>

#include "core/renderer/css/parser/css_string_parser.h"
#include "core/renderer/css/unit_handler.h"

namespace lynx {
namespace tasm {
namespace MaskCompositeHandler {

HANDLER_IMPL() {
  CSS_HANDLER_FAIL_IF_NOT(input.IsString(), configs.enable_css_strict_mode,
                          TYPE_MUST_BE, CSSProperty::GetPropertyNameCStr(key),
                          STRING_TYPE)

  CSSStringParser parser = CSSStringParser::FromLepusString(input, configs);
  auto composite = parser.ParseMaskComposite();
  if (composite.IsEmpty()) {
    return false;
  }
  output.insert_or_assign(key, std::move(composite));
  return true;
}

HANDLER_REGISTER_IMPL() { array[kPropertyIDMaskComposite] = &Handle; }

}  // namespace MaskCompositeHandler
}  // namespace tasm
}  // namespace lynx
