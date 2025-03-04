// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/renderer/css/parser/aspect_ratio_handler.h"

#include <string>
#include <utility>

#include "core/renderer/css/parser/css_string_parser.h"

namespace lynx {
namespace tasm {
namespace AspectRatioHandler {

HANDLER_IMPL() {
  if (input.IsNumber()) {
    output.emplace_or_assign(key, input, CSSValuePattern::NUMBER);
    return true;
  }
  CSSStringParser parser = CSSStringParser::FromLepusString(input, configs);
  auto ret = parser.ParseAspectRatio();
  if (!ret.IsEmpty()) {
    output.insert_or_assign(key, std::move(ret));
    return true;
  }
  return false;
}

HANDLER_REGISTER_IMPL() { array[kPropertyIDAspectRatio] = &Handle; }

}  // namespace AspectRatioHandler

}  // namespace tasm
}  // namespace lynx
