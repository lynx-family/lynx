// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/css/parser/grid_shorthand_handler.h"

#include <string>
#include <utility>

#include "base/include/string/string_utils.h"
#include "core/renderer/css/unit_handler.h"

namespace lynx {
namespace tasm {
namespace GridShorthandHandler {

HANDLER_IMPL() {
  if (!configs.enable_grid_placement_shorthands) {
    return false;
  }

  CSS_HANDLER_FAIL_IF_NOT(input.IsString(), configs.enable_css_strict_mode,
                          TYPE_MUST_BE, CSSProperty::GetPropertyNameCStr(key),
                          STRING_TYPE)

  const auto& str = input.StdString();

  CSSPropertyID start_prop;
  CSSPropertyID end_prop;
  if (key == kPropertyIDGridColumn) {
    start_prop = kPropertyIDGridColumnStart;
    end_prop = kPropertyIDGridColumnEnd;
  } else if (key == kPropertyIDGridRow) {
    start_prop = kPropertyIDGridRowStart;
    end_prop = kPropertyIDGridRowEnd;
  } else {
    return false;
  }

  std::string::size_type slash_pos = str.find('/');
  std::string start_str;
  std::string end_str;

  if (slash_pos != std::string::npos) {
    if (str.find('/', slash_pos + 1) != std::string::npos) {
      return false;
    }
    start_str = base::TrimString(std::string_view(str).substr(0, slash_pos));
    end_str = base::TrimString(std::string_view(str).substr(slash_pos + 1));
    if (start_str.empty() || end_str.empty()) {
      return false;
    }
  } else {
    start_str = base::TrimString(str);
    if (start_str.empty()) {
      return false;
    }
    end_str = "auto";
  }

  StyleMap tmp;
  if (!UnitHandler::Process(start_prop, lepus::Value(start_str), tmp,
                            configs)) {
    return false;
  }
  if (!UnitHandler::Process(end_prop, lepus::Value(end_str), tmp, configs)) {
    return false;
  }

  for (auto& kv : tmp) {
    output.insert_or_assign(kv.first, std::move(kv.second));
  }
  return true;
}

HANDLER_REGISTER_IMPL() {
  array[kPropertyIDGridColumn] = &Handle;
  array[kPropertyIDGridRow] = &Handle;
}

}  // namespace GridShorthandHandler
}  // namespace tasm
}  // namespace lynx
