// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/renderer/css/parser/text_decoration_handler.h"

#include <string>
#include <vector>

#include "base/include/debug/lynx_assert.h"
#include "base/include/string/string_utils.h"
#include "core/renderer/css/parser/css_string_parser.h"
#include "core/renderer/css/unit_handler.h"
#include "core/renderer/starlight/style/css_type.h"

namespace lynx {
namespace tasm {
namespace TextDecorationHandler {

using starlight::TextDecorationType;

HANDLER_IMPL() {
  CSS_HANDLER_FAIL_IF_NOT(input.IsString(), configs.enable_css_strict_mode,
                          TYPE_MUST_BE, CSSProperty::GetPropertyNameCStr(key),
                          STRING_TYPE)

  CSSStringParser parser = CSSStringParser::FromLepusString(input, configs);
  return !(output.insert_or_assign(key, parser.ParseTextDecoration())
               .first->second.IsEmpty());
}

HANDLER_REGISTER_IMPL() { array[kPropertyIDTextDecoration] = &Handle; }

}  // namespace TextDecorationHandler
}  // namespace tasm
}  // namespace lynx
