// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/renderer/css/parser/flex_flow_handler.h"

#include <string>
#include <utility>
#include <vector>

#include "base/include/debug/lynx_assert.h"
#include "base/include/string/string_utils.h"
#include "base/include/vector.h"
#include "core/renderer/css/unit_handler.h"

namespace lynx {
namespace tasm {
namespace FlexFlowHandler {

HANDLER_IMPL() {
  CSS_HANDLER_FAIL_IF_NOT(input.IsString(), configs.enable_css_strict_mode,
                          TYPE_MUST_BE, CSSProperty::GetPropertyNameCStr(key),
                          STRING_TYPE)

  auto str = input.StringView();
  base::InlineVector<std::string, 2> styles;
  base::SplitString(str, ' ', false, [&](const char* s, size_t length, int) {
    styles.emplace_back(s, length);
    return true;
  });
  if (styles.size() > 2) {
    return false;
  }
  StyleMap tmp;
  static CSSPropertyID property_ids[2] = {kPropertyIDFlexDirection,
                                          kPropertyIDFlexWrap};
  bool longhands[2] = {false, false};
  for (const auto& style : styles) {
    bool found_longhand = false;
    for (size_t i = 0; !found_longhand && i < std::size(property_ids); ++i) {
      if (longhands[i]) {
        continue;
      }
      longhands[i] = UnitHandler::Process(property_ids[i], lepus::Value(style),
                                          tmp, configs);

      if (longhands[i]) {
        found_longhand = true;
      }
    }
    if (!found_longhand) {
      return false;
    }
  }
  for (size_t i = 0; i < std::size(property_ids); ++i) {
    if (longhands[i]) {
      output.insert_or_assign(property_ids[i], std::move(tmp[property_ids[i]]));
    }
  }

  return true;
}

HANDLER_REGISTER_IMPL() { array[kPropertyIDFlexFlow] = &Handle; }

}  // namespace FlexFlowHandler
}  // namespace tasm
}  // namespace lynx
