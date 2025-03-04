// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/css/parser/font_length_handler.h"

#include <utility>

#include "core/renderer/css/parser/css_string_parser.h"

namespace lynx {
namespace tasm {
namespace FontLengthHandler {
static constexpr float UNDEFINED = 10E20;

HANDLER_IMPL() {
  if (input.IsNumber()) {
    output.emplace_or_assign(key, input, CSSValuePattern::NUMBER);
    return true;
  }
  if (input.IsString()) {
    if (input.StdString() == "normal") {
      output.emplace_or_assign(key, lepus::Value(UNDEFINED),
                               CSSValuePattern::NUMBER);
      return true;
    }
    CSSStringParser parser = CSSStringParser::FromLepusString(input, configs);
    auto res = parser.ParseFontLength();
    if (!res.IsEmpty()) {
      output.insert_or_assign(key, std::move(res));
      return true;
    }
    return false;
  }
  return false;
}

HANDLER_REGISTER_IMPL() {
  // AUTO INSERT, DON'T CHANGE IT!
  array[kPropertyIDLineHeight] = &Handle;
  array[kPropertyIDLetterSpacing] = &Handle;
  array[kPropertyIDLineSpacing] = &Handle;
  // AUTO INSERT END, DON'T CHANGE IT!
}
}  // namespace FontLengthHandler
}  // namespace tasm
}  // namespace lynx
