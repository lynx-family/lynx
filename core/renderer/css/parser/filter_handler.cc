// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/css/parser/filter_handler.h"

#include <utility>

#include "base/include/value/base_string.h"
#include "core/renderer/css/parser/css_string_parser.h"
#include "core/renderer/css/unit_handler.h"
#include "core/runtime/vm/lepus/array.h"

namespace lynx {
namespace tasm {
namespace FilterHandler {

HANDLER_IMPL() {
  CSS_HANDLER_FAIL_IF_NOT(input.IsString(), configs.enable_css_strict_mode,
                          TYPE_MUST_BE, CSSProperty::GetPropertyNameCStr(key),
                          STRING_TYPE)

  CSSStringParser parser = CSSStringParser::FromLepusString(input, configs);
  CSSValue filter_value = parser.ParseFilter();
  if (filter_value.IsEmpty()) {
    return false;
  }
  output.insert_or_assign(key, std::move(filter_value));
  return true;
}

HANDLER_REGISTER_IMPL() { array[kPropertyIDFilter] = &Handle; }

}  // namespace FilterHandler
}  // namespace tasm
}  // namespace lynx
