// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/renderer/css/parser/border_radius_handler.h"

#include <string>
#include <utility>
#include <vector>

#include "core/renderer/css/parser/css_string_parser.h"
#include "core/renderer/css/unit_handler.h"
#include "core/runtime/vm/lepus/array.h"

namespace lynx {
namespace tasm {
namespace BorderRadiusHandler {

HANDLER_IMPL() {
  CSS_HANDLER_FAIL_IF_NOT(input.IsString(), configs.enable_css_strict_mode,
                          TYPE_MUST_BE, CSSProperty::GetPropertyNameCStr(key),
                          STRING_TYPE)

  CSSStringParser parser = CSSStringParser::FromLepusString(input, configs);
  switch (key) {
    case kPropertyIDBorderRadius: {
      CSSValue x_radii[4] = {CSSValue::Empty(), CSSValue::Empty(),
                             CSSValue::Empty(), CSSValue::Empty()};
      CSSValue y_radii[4] = {CSSValue::Empty(), CSSValue::Empty(),
                             CSSValue::Empty(), CSSValue::Empty()};
      if (!parser.ParseBorderRadius(x_radii, y_radii)) {
        return false;
      }
      static const CSSPropertyID radius_key_array[] = {
          kPropertyIDBorderTopLeftRadius, kPropertyIDBorderTopRightRadius,
          kPropertyIDBorderBottomRightRadius,
          kPropertyIDBorderBottomLeftRadius};
      for (int i = 0; i < 4; i++) {
        auto container = lepus::CArray::Create();
        container->emplace_back(std::move(x_radii[i].GetValue()));
        container->emplace_back(static_cast<int>(x_radii[i].GetPattern()));
        container->emplace_back(std::move(y_radii[i].GetValue()));
        container->emplace_back(static_cast<int>(y_radii[i].GetPattern()));
        output.emplace_or_assign(radius_key_array[i], std::move(container));
      }
      output.erase(key);
    } break;
    case kPropertyIDBorderTopLeftRadius:
    case kPropertyIDBorderTopRightRadius:
    case kPropertyIDBorderBottomRightRadius:
    case kPropertyIDBorderBottomLeftRadius:
    case kPropertyIDBorderStartStartRadius:
    case kPropertyIDBorderStartEndRadius:
    case kPropertyIDBorderEndStartRadius:
    case kPropertyIDBorderEndEndRadius: {
      CSSValue value = parser.ParseSingleBorderRadius();
      if (value.IsArray()) {
        output.insert_or_assign(key, std::move(value));
      }
    } break;
    default:
      break;
  }
  return true;
}

HANDLER_REGISTER_IMPL() {
  array[kPropertyIDBorderRadius] = &Handle;
  array[kPropertyIDBorderTopLeftRadius] = &Handle;
  array[kPropertyIDBorderTopRightRadius] = &Handle;
  array[kPropertyIDBorderBottomLeftRadius] = &Handle;
  array[kPropertyIDBorderBottomRightRadius] = &Handle;
  array[kPropertyIDBorderStartStartRadius] = &Handle;
  array[kPropertyIDBorderStartEndRadius] = &Handle;
  array[kPropertyIDBorderEndStartRadius] = &Handle;
  array[kPropertyIDBorderEndEndRadius] = &Handle;
}

}  // namespace BorderRadiusHandler
}  // namespace tasm
}  // namespace lynx
