// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/renderer/css/parser/length_handler.h"

#include <string>

#include "base/include/debug/lynx_assert.h"
#include "base/include/string/string_number_convert.h"
#include "base/include/string/string_utils.h"
#include "core/renderer/css/parser/css_string_parser.h"
#include "core/renderer/css/unit_handler.h"

namespace lynx {
namespace tasm {
namespace LengthHandler {

void CheckLengthUnitValid(CSSPropertyID key, const lepus::Value& input,
                          const CSSValue& css_value,
                          const CSSParserConfigs& configs) {
  // TODO, currently the testcases online still doesnt carry units, so that
  // this red box warning message would block CQ test, add the sdk version
  // to turn off the warning until the testcases are fixed.
  if (!configs.enable_length_unit_check) {
    return;
  }
  // line-height: 3 is a valid css value(means the 3 times of font size)
  if (key == CSSPropertyID::kPropertyIDLineHeight) {
    return;
  }
  // number 0 doesn't need to carry any units
  if (css_value.IsNumber() && css_value.GetValue().Number() != 0) {
    UnitHandler::ReportError("CSS length need units (except 0)",
                             "Add unit for length value", key,
                             input.StdString());
  }
}

bool Process(const lepus::Value& input, CSSValue& css_value,
             const CSSParserConfigs& configs) {
  if (input.IsString()) {
    CSSStringParser parser = CSSStringParser::FromLepusString(input, configs);
    parser.ParseLengthTo(css_value);
    return !css_value.IsEmpty();
  }

  if (input.IsNumber()) {
    css_value.SetValue(input);
    css_value.SetPattern(CSSValuePattern::NUMBER);
    return true;
  }

  return false;
}

HANDLER_IMPL() {
  auto existing = output.insert_default_if_absent(key);
  if (existing.second) {
    // Insertion happened, key is not found in output but a default CSSValue is
    // constructed in output map and pointed to by existing.first iterator.
    // We parse directly to the value instance in map.
    if (UNLIKELY(!Process(input, existing.first->second, configs))) {
      // Erase the inserted one to restore output map to original state.
      output.erase(key);
      goto fail;
    }
  } else {
    // Found existing key, but the parsing may fail and we should not changed
    // the existing value. So parse to temporary css_value and if all are
    // successful assign to existing value.
    CSSValue css_value;
    if (UNLIKELY(!Process(input, css_value, configs))) {
      goto fail;
    }
    existing.first->second = css_value;
  }
  CheckLengthUnitValid(key, input, existing.first->second, configs);
  return true;

fail:
  if (configs.enable_css_strict_mode) {
    UnitHandler::CSSWarningUnconditional(TYPE_UNSUPPORTED,
                                         CSSProperty::GetPropertyNameCStr(key),
                                         input.CString());
  }
  return false;
}

}  // namespace LengthHandler
}  // namespace tasm
}  // namespace lynx
