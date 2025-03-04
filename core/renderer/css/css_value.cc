// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/renderer/css/css_value.h"

#include "core/runtime/vm/lepus/json_parser.h"

namespace lynx {
namespace tasm {

std::string CSSValue::AsJsonString() const {
  return lepus::lepusValueToString(value_);
}

bool CSSValue::AsBool() const { return value_.Bool(); }

}  // namespace tasm
}  // namespace lynx
