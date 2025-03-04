// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/renderer/css/parser/list_gap_handler.h"

#include <utility>

#include "core/renderer/css/parser/css_string_parser.h"

namespace lynx {
namespace tasm {
namespace ListGapHandler {

HANDLER_IMPL() {
  if (input.IsString()) {
    CSSStringParser parser = CSSStringParser::FromLepusString(input, configs);
    auto res = parser.ParseListGap();
    if (!res.IsEmpty()) {
      output.insert_or_assign(key, std::move(res));
      return true;
    }
    return false;
  }
  return false;
}

HANDLER_REGISTER_IMPL() {
  array[kPropertyIDListCrossAxisGap] = &Handle;
  array[kPropertyIDListMainAxisGap] = &Handle;
}

}  // namespace ListGapHandler
}  // namespace tasm
}  // namespace lynx
