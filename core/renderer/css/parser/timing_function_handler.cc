// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/renderer/css/parser/timing_function_handler.h"

#include <utility>
#include <vector>

#include "base/include/debug/lynx_assert.h"
#include "base/include/string/string_utils.h"
#include "core/renderer/css/parser/css_string_parser.h"
#include "core/renderer/css/unit_handler.h"
#include "core/renderer/starlight/style/css_type.h"
#include "core/runtime/vm/lepus/array.h"

namespace lynx {
namespace tasm {
namespace TimingFunctionHandler {

HANDLER_IMPL() {
  CSSValue css_value;
  CSSStringParser parser = CSSStringParser::FromLepusString(input, configs);
  // For compatibility, the output must be an array
  if (parser.ParseTimingFunction(false, css_value)) {
    output.insert_or_assign(key, std::move(css_value));
    return true;
  }
  return false;
}

}  // namespace TimingFunctionHandler
}  // namespace tasm
}  // namespace lynx
