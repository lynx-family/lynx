// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/renderer/css/parser/cursor_handler.h"

#include "base/include/debug/lynx_assert.h"
#include "core/renderer/css/parser/css_string_parser.h"
#include "core/renderer/css/unit_handler.h"

namespace lynx {
namespace tasm {
namespace CursorHandler {

HANDLER_IMPL() {
  CSS_HANDLER_FAIL_IF_NOT(input.IsString(), configs.enable_css_strict_mode,
                          TYPE_MUST_BE, CSSProperty::GetPropertyNameCStr(key),
                          STRING_TYPE)

  CSSStringParser parser = CSSStringParser::FromLepusString(input, configs);
  output.insert_or_assign(key, parser.ParseCursor());
  return true;
}

HANDLER_REGISTER_IMPL() { array[kPropertyIDCursor] = &Handle; }

}  // namespace CursorHandler
}  // namespace tasm
}  // namespace lynx
