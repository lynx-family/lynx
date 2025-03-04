// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/renderer/css/parser/animation_property_handler.h"

#include <string>
#include <utility>
#include <vector>

#include "base/include/debug/lynx_assert.h"
#include "base/include/string/string_utils.h"
#include "core/renderer/css/parser/css_string_parser.h"
#include "core/renderer/css/unit_handler.h"
#include "core/renderer/starlight/style/css_type.h"
#include "core/runtime/vm/lepus/array.h"

namespace lynx {
namespace tasm {
namespace AnimationPropertyHandler {

HANDLER_IMPL() {
  CSSStringParser parser = CSSStringParser::FromLepusString(input, configs);
  CSSValue css_value;
  if (parser.ParseSingleOrMultipleValuePreview<
          &CSSStringParser::ParseTransitionProperty>(css_value)) {
    output.insert_or_assign(key, std::move(css_value));
    return true;
  }
  return false;
}

}  // namespace AnimationPropertyHandler
}  // namespace tasm
}  // namespace lynx
