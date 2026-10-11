// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/css/parser/corner_shape_handler.h"

#include <utility>

#include "core/renderer/css/parser/css_string_parser.h"
#include "core/renderer/css/unit_handler.h"

namespace lynx {
namespace tasm {
namespace CornerShapeHandler {

HANDLER_IMPL() {
  CSS_HANDLER_FAIL_IF_NOT(input.IsString(), configs.enable_css_strict_mode,
                          TYPE_MUST_BE, CSSProperty::GetPropertyNameCStr(key),
                          STRING_TYPE)

  CSSStringParser parser = CSSStringParser::FromLepusString(input, configs);
  CSSValue shapes[4];
  const bool shorthand = key == kPropertyIDCornerShape;
  if (!parser.ParseCornerShape(shapes, shorthand)) {
    return false;
  }
  if (shorthand) {
    size_t count = 0;
    const auto* longhands = CSSProperty::GetExpandedLonghands(key, &count);
    for (size_t i = 0; i < count; ++i) {
      output.insert_or_assign(longhands[i], std::move(shapes[i]));
    }
    output.erase(key);
  } else {
    output.insert_or_assign(key, std::move(shapes[0]));
  }
  return true;
}

HANDLER_REGISTER_IMPL() {
  array[kPropertyIDCornerShape] = &Handle;
  array[kPropertyIDCornerTopLeftShape] = &Handle;
  array[kPropertyIDCornerTopRightShape] = &Handle;
  array[kPropertyIDCornerBottomRightShape] = &Handle;
  array[kPropertyIDCornerBottomLeftShape] = &Handle;
}

}  // namespace CornerShapeHandler
}  // namespace tasm
}  // namespace lynx
