// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/renderer/css/parser/grid_position_handler.h"

#include <string>

#include "base/include/string/string_utils.h"
#include "core/renderer/css/unit_handler.h"

namespace lynx {
namespace tasm {

namespace GridPositionHandler {
constexpr int32_t kAutoValue = 0;
constexpr const char* kAuto = "auto";
// "span"
constexpr int32_t kSpanStrSize = 4;
constexpr const char* kSpan = "span";

HANDLER_IMPL() {
  if (!input.IsString()) {
    return false;
  }

  const auto& str = input.StdString();
  if (str.find(kAuto) != std::string::npos) {
    output.emplace_or_assign(key, lepus::Value(kAutoValue),
                             CSSValuePattern::NUMBER);
    return true;
  }

  std::string::size_type span_pos = str.find(kSpan);
  if (span_pos != std::string::npos) {
    lepus::Value value =
        lepus::Value(atoi(str.substr(span_pos + kSpanStrSize).c_str()));
    if (key == kPropertyIDGridColumnStart || key == kPropertyIDGridColumnEnd) {
      UnitHandler::Process(kPropertyIDGridColumnSpan, value, output, configs);
    } else {
      UnitHandler::Process(kPropertyIDGridRowSpan, value, output, configs);
    }
  } else {
    output.emplace_or_assign(key, lepus::Value(atoi(str.c_str())),
                             CSSValuePattern::NUMBER);
  }
  return true;
}

HANDLER_REGISTER_IMPL() {
  array[kPropertyIDGridColumnStart] = &Handle;
  array[kPropertyIDGridColumnEnd] = &Handle;
  array[kPropertyIDGridRowStart] = &Handle;
  array[kPropertyIDGridRowEnd] = &Handle;
}

}  // namespace GridPositionHandler
}  // namespace tasm
}  // namespace lynx
