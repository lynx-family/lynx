// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/css/parser/bool_handler.h"

#include <utility>

#include "base/include/debug/lynx_assert.h"
#include "core/renderer/css/parser/css_string_parser.h"
#include "core/renderer/css/unit_handler.h"

namespace lynx {
namespace tasm {
namespace BoolHandler {

HANDLER_IMPL() {
  CSSValue css_value;
  if (input.IsBool()) {
    css_value.SetBoolean(input.Bool());
  } else if (input.IsString()) {
    CSSStringParser parser = CSSStringParser::FromLepusString(input, configs);
    css_value = parser.ParseBool();
  } else {
    return false;
  }
  if (!css_value.IsEmpty()) {
    output.insert_or_assign(key, std::move(css_value));
    return true;
  }
  return false;
}

}  // namespace BoolHandler
}  // namespace tasm
}  // namespace lynx
