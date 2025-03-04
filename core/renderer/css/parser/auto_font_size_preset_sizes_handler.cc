// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/renderer/css/parser/auto_font_size_preset_sizes_handler.h"

#include <string>
#include <utility>
#include <vector>

#include "base/include/debug/lynx_assert.h"
#include "core/renderer/css/parser/css_string_parser.h"
#include "core/renderer/css/unit_handler.h"
#include "core/runtime/vm/lepus/array.h"

namespace lynx {
namespace tasm {
namespace AutoFontSizePresetSizesHandler {

HANDLER_IMPL() {
  CSS_HANDLER_FAIL_IF_NOT(input.IsString(), configs.enable_css_strict_mode,
                          TYPE_MUST_BE, CSSProperty::GetPropertyNameCStr(key),
                          STRING_TYPE)
  CSSStringParser parser = CSSStringParser::FromLepusString(input, configs);
  auto arr = lepus::CArray::Create();
  if (!parser.ParseAutoFontSizePresetSize(arr)) {
    return false;
  }
  output.emplace_or_assign(key, std::move(arr));
  return true;
}

HANDLER_REGISTER_IMPL() {
  array[kPropertyIDXAutoFontSizePresetSizes] = &Handle;
}

}  // namespace AutoFontSizePresetSizesHandler
}  // namespace tasm
}  // namespace lynx
