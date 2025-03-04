// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/renderer/css/parser/color_handler.h"

#include <string>

#include "core/renderer/css/css_style_utils.h"
#include "core/renderer/css/parser/css_string_parser.h"
#include "core/renderer/css/unit_handler.h"

namespace lynx {
namespace tasm {
namespace ColorHandler {

namespace {
// Previously Process method will not change input css_value if parsing
// failed. Extract method to keep this behavior and optimize handler at
// the same time.
// This method will parse directly to input css_value.
bool _Process(const lepus::Value& input, CSSValue& css_value,
              const CSSParserConfigs& configs, bool is_text_color) {
  if (!input.IsString()) {
    return false;
  }

  CSSStringParser parser = CSSStringParser::FromLepusString(input, configs);
  if (is_text_color) {
    parser.ParseTextColorTo(css_value);
  } else {
    parser.ParseCSSColorTo(css_value);
  }
  return !css_value.IsEmpty();
}
}  // namespace

bool Process(const lepus::Value& input, CSSValue& css_value,
             const CSSParserConfigs& configs, bool is_text_color) {
  CSSValue color;
  bool result = _Process(input, color, configs, is_text_color);
  if (result) {
    css_value = color;
  }
  return result;
}

HANDLER_IMPL() {
  const bool is_text_color = key == kPropertyIDColor;
  auto existing = output.insert_default_if_absent(key);
  if (existing.second) {
    // Insertion happened, key is not found in output but a default CSSValue is
    // constructed in output map and pointed to by existing.first iterator.
    // We parse directly to the value instance in map.
    if (UNLIKELY(
            !_Process(input, existing.first->second, configs, is_text_color))) {
      // Erase the inserted one to restore output map to original state.
      output.erase(key);
      goto fail;
    }
  } else {
    // Found existing key, but the parsing may fail and we should not changed
    // the existing value. So use Process it will not change
    // `existing.first->second` if parsing failed.
    if (UNLIKELY(
            !Process(input, existing.first->second, configs, is_text_color))) {
      goto fail;
    }
  }
  return true;

fail:
  if (configs.enable_css_strict_mode) {
    UnitHandler::CSSWarningUnconditional(
        FORMAT_ERROR, CSSProperty::GetPropertyNameCStr(key), input.CString());
  }
  return false;
}

}  // namespace ColorHandler
}  // namespace tasm
}  // namespace lynx
