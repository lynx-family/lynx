// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/renderer/css/parser/time_handler.h"

#include <string>
#include <utility>
#include <vector>

#include "base/include/debug/lynx_assert.h"
#include "base/include/string/string_utils.h"
#include "core/renderer/css/parser/css_string_parser.h"
#include "core/renderer/css/unit_handler.h"
#include "core/runtime/vm/lepus/array.h"

namespace lynx {
namespace tasm {
namespace TimeHandler {

HANDLER_IMPL() {
  if (input.IsNumber()) {
    output.emplace_or_assign(key, input.Number(), CSSValue::kCreateNumberTag);
    return true;
  } else if (input.IsString()) {
    CSSValue css_value;
    bool duration = key == kPropertyIDAnimationDuration ||
                    key == kPropertyIDTransitionDuration;
    CSSStringParser parser = CSSStringParser::FromLepusString(input, configs);
    if (parser.ParseSingleOrMultipleValuePreview<&CSSStringParser::ParseTime>(
            duration, css_value)) {
      output.insert_or_assign(key, std::move(css_value));
      return true;
    }
  } else {
    CSS_HANDLER_FAIL_IF_NOT(false, configs.enable_css_strict_mode, TYPE_MUST_BE,
                            TIME_VALUE, STRING_OR_NUMBER_TYPE)
  }
  return false;
}

}  // namespace TimeHandler
}  // namespace tasm
}  // namespace lynx
