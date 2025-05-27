// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/data/harmony/platform_data_harmony.h"

#include "core/renderer/data/harmony/template_data_harmony.h"
#include "core/runtime/vm/lepus/json_parser.h"

namespace lynx {
namespace tasm {

void PlatformDataHarmony::EnsureConvertData() {
  if (is_json_string_) {
    value_converted_from_platform_data_ =
        lepus::jsonValueTolepusValue(json_string_.c_str());
  }
}

}  // namespace tasm
}  // namespace lynx
