// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/renderer/css/parser/flex_handler.h"

#include <string>
#include <utility>

#include "core/renderer/css/parser/css_string_parser.h"
#include "core/renderer/css/unit_handler.h"

namespace lynx {
namespace tasm {
namespace FlexHandler {

HANDLER_IMPL() {
  if (input.IsNumber()) {
    StyleMap num_map;
    if (UnitHandler::Process(kPropertyIDFlexGrow, lepus::Value(input.Double()),
                             num_map, configs) &&
        UnitHandler::Process(kPropertyIDFlexShrink, lepus::Value(1), num_map,
                             configs) &&
        UnitHandler::Process(kPropertyIDFlexBasis, lepus::Value(0), num_map,
                             configs)) {
      for (auto& v : num_map) {
        output.insert_or_assign(v.first, std::move(v.second));
      }
      return true;
    } else {
      CSS_HANDLER_FAIL_IF_NOT(
          false, configs.enable_css_strict_mode, FORMAT_ERROR,
          CSSProperty::GetPropertyNameCStr(key), input.Number())
    }
  }

  if (!input.IsString()) {
    CSS_HANDLER_FAIL_IF_NOT(false, configs.enable_css_strict_mode, TYPE_MUST_BE,
                            CSSProperty::GetPropertyNameCStr(key),
                            STRING_OR_NUMBER_TYPE)
  }

  double flex_grow = -1;
  double flex_shrink = -1;
  CSSValue flex_basis = CSSValue::Empty();
  CSSStringParser parser = CSSStringParser::FromLepusString(input, configs);
  auto ret = parser.ParseFlex(flex_grow, flex_shrink, flex_basis);
  if (ret) {
    output.emplace_or_assign(kPropertyIDFlexGrow, flex_grow,
                             CSSValue::kCreateNumberTag);
    output.emplace_or_assign(kPropertyIDFlexShrink, flex_shrink,
                             CSSValue::kCreateNumberTag);
    output.insert_or_assign(kPropertyIDFlexBasis, std::move(flex_basis));
  }
  return ret;
}

HANDLER_REGISTER_IMPL() {
  // AUTO INSERT, DON'T CHANGE IT!
  array[kPropertyIDFlex] = &Handle;
  // AUTO INSERT END, DON'T CHANGE IT!
}

}  // namespace FlexHandler
}  // namespace tasm
}  // namespace lynx
