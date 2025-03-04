// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/renderer/css/parser/background_image_handler.h"

#include <string>
#include <utility>

#include "core/renderer/css/parser/css_string_parser.h"
#include "core/renderer/css/unit_handler.h"

namespace lynx {
namespace tasm {
namespace BackgroundImageHandler {

HANDLER_IMPL() {
  CSS_HANDLER_FAIL_IF_NOT(input.IsString(), configs.enable_css_strict_mode,
                          TYPE_MUST_BE, CSSProperty::GetPropertyNameCStr(key),
                          STRING_TYPE)

  CSSStringParser parser = CSSStringParser::FromLepusString(input, configs);
  auto image = parser.ParseBackgroundImage();
  if (image.IsEmpty()) {
    return false;
  }
  output.insert_or_assign(key, std::move(image));
  return true;
}

HANDLER_REGISTER_IMPL() {
  array[kPropertyIDBackgroundImage] = &Handle;
  array[kPropertyIDMaskImage] = &Handle;
}

}  // namespace BackgroundImageHandler
}  // namespace tasm
}  // namespace lynx
