// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/renderer/css/parser/string_handler.h"

#include "base/include/debug/lynx_assert.h"
#include "core/renderer/css/unit_handler.h"

namespace lynx {
namespace tasm {
namespace StringHandler {

HANDLER_IMPL() {
  CSS_HANDLER_FAIL_IF_NOT(input.IsString(), configs.enable_css_strict_mode,
                          "id:%d value must be string.",
                          CSSProperty::GetPropertyNameCStr(key))
  output.emplace_or_assign(key, input);
  return true;
}

}  // namespace StringHandler
}  // namespace tasm
}  // namespace lynx
