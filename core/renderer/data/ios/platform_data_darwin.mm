// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/data/ios/platform_data_darwin.h"

namespace lynx {
namespace tasm {

void PlatformDataDarwin::EnsureConvertData() {
  if (value_converted_from_platform_data_.IsEmpty() && _data != nil) {
    value_converted_from_platform_data_ = [_data getDataForJSThread];
  }
}

}  // namespace tasm
}  // namespace lynx
