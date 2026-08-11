// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/renderer/css/parser/number_handler.h"

#include "base/include/debug/lynx_assert.h"
#include "base/include/string/string_number_convert.h"
#include "core/renderer/css/unit_handler.h"

namespace lynx {
namespace tasm {
namespace NumberHandler {

namespace {

bool HandleInternal(CSSPropertyID key, const lepus::Value& input,
                    StyleMap& output, const CSSParserConfigs& configs,
                    bool non_negative) {
  double num = 0;
  if (input.IsNumber()) {
    num = input.Number();
  } else if (input.IsString()) {
    const auto& str = input.StdString();
    if (str == "infinite") {
      num = 10E8;
    } else {
      CSS_HANDLER_FAIL_IF_NOT(
          base::StringToDouble(str, num, true), configs.enable_css_strict_mode,
          TYPE_UNSUPPORTED, CSSProperty::GetPropertyNameCStr(key), str.c_str())
    }
  } else {
    CSS_HANDLER_FAIL_IF_NOT(false, configs.enable_css_strict_mode, TYPE_MUST_BE,
                            FLOAT_TYPE, STRING_OR_NUMBER_TYPE)
  }
  CSS_HANDLER_FAIL_IF_NOT(
      !non_negative || num >= 0, configs.enable_css_strict_mode,
      NON_NEGATIVE_NUMBER_ERROR, CSSProperty::GetPropertyNameCStr(key))
  output.emplace_or_assign(key, num, CSSValuePattern::NUMBER);
  return true;
}

}  // namespace

HANDLER_IMPL() { return HandleInternal(key, input, output, configs, false); }

bool HandleNonNegative(CSSPropertyID key, const lepus::Value& input,
                       StyleMap& output, const CSSParserConfigs& configs) {
  return HandleInternal(key, input, output, configs, true);
}

}  // namespace NumberHandler
}  // namespace tasm
}  // namespace lynx
