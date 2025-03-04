// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/renderer/css/parser/transform_handler.h"

#include <string>
#include <utility>
#include <vector>

#include "core/renderer/css/parser/css_string_parser.h"
#include "core/renderer/css/unit_handler.h"
#include "core/runtime/vm/lepus/array.h"

#ifdef OS_WIN
#define _USE_MATH_DEFINES
#include <math.h>
#endif

namespace lynx {
namespace tasm {
namespace TransformHandler {

HANDLER_IMPL() {
  CSS_HANDLER_FAIL_IF_NOT(input.IsString(), configs.enable_css_strict_mode,
                          TYPE_MUST_BE, CSSProperty::GetPropertyNameCStr(key),
                          STRING_TYPE)

  CSSStringParser parser = CSSStringParser::FromLepusString(input, configs);
  auto ret = parser.ParseTransform();
  if (!ret.IsEmpty()) {
    output.insert_or_assign(key, std::move(ret));
    return true;
  }
  return false;
}

HANDLER_REGISTER_IMPL() { array[kPropertyIDTransform] = &Handle; }
}  // namespace TransformHandler
}  // namespace tasm
}  // namespace lynx
