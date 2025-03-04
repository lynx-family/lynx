// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/renderer/css/parser/gap_handler.h"

#include <string>
#include <utility>
#include <vector>

#include "base/include/debug/lynx_assert.h"
#include "base/include/string/string_utils.h"
#include "core/renderer/css/parser/css_string_parser.h"
#include "core/renderer/css/unit_handler.h"

namespace lynx {
namespace tasm {

namespace GapHandler {

HANDLER_IMPL() {
  const CSSPropertyID properties[2] = {kPropertyIDRowGap, kPropertyIDColumnGap};
  if (input.IsString()) {
    CSSStringParser parser = CSSStringParser::FromLepusString(input, configs);
    auto ret = parser.ParseGap();
    bool parse_success = false;
    if (!ret.first.IsEmpty()) {
      output.insert_or_assign(kPropertyIDRowGap, std::move(ret.first));
      parse_success = true;
    }
    if (!ret.second.IsEmpty()) {
      output.insert_or_assign(kPropertyIDColumnGap, std::move(ret.second));
    }
    return parse_success;
  } else if (input.IsNumber()) {
    UnitHandler::Process(properties[0], input, output, configs);
    if (auto it = output.find(properties[0]); it != output.end()) {
      output.insert_or_assign(properties[1], it->second);
    } else {
      return false;
    }
  } else {
    CSS_HANDLER_FAIL_IF_NOT(false, configs.enable_css_strict_mode, TYPE_MUST_BE,
                            CSSProperty::GetPropertyNameCStr(key),
                            STRING_OR_NUMBER_TYPE)
  }
  return true;
}

HANDLER_REGISTER_IMPL() { array[kPropertyIDGap] = &Handle; }

}  // namespace GapHandler
}  // namespace tasm
}  // namespace lynx
