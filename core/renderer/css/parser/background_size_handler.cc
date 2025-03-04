// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/renderer/css/parser/background_size_handler.h"

#include <string>
#include <utility>

#include "base/include/debug/lynx_assert.h"
#include "core/renderer/css/parser/css_string_parser.h"
#include "core/renderer/css/unit_handler.h"
#include "core/renderer/tasm/config.h"

namespace lynx {
namespace tasm {
namespace BackgroundSizeHandler {

HANDLER_IMPL() {
  CSS_HANDLER_FAIL_IF_NOT(input.IsString(), configs.enable_css_strict_mode,
                          TYPE_MUST_BE, CSSProperty::GetPropertyNameCStr(key),
                          STRING_TYPE)

  CSSStringParser parser = CSSStringParser::FromLepusString(input, configs);
  parser.SetIsLegacyParser(configs.enable_legacy_parser);
  auto size = parser.ParseBackgroundSize();
  if (size.IsEmpty()) {
    return false;
  }
  output.insert_or_assign(key, std::move(size));
  return true;
}

HANDLER_REGISTER_IMPL() {
  array[kPropertyIDBackgroundSize] = &Handle;
  array[kPropertyIDMaskSize] = &Handle;
}

}  // namespace BackgroundSizeHandler
}  // namespace tasm
}  // namespace lynx
