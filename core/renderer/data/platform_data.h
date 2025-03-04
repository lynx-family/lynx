// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DATA_PLATFORM_DATA_H_
#define CORE_RENDERER_DATA_PLATFORM_DATA_H_

#include "core/runtime/vm/lepus/lepus_value.h"

namespace lynx {
namespace tasm {

// TODO(songshourui.null): Currently, only iOS & Android support passing
// PlatformData to shell interfaces, and support is required on other platforms.
class PlatformData {
 public:
  PlatformData() = default;
  virtual ~PlatformData() = default;

  const lepus::Value& GetValue() {
    EnsureConvertData();
    return value_converted_from_platform_data_;
  }

 protected:
  virtual void EnsureConvertData() = 0;

  lepus::Value value_converted_from_platform_data_;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DATA_PLATFORM_DATA_H_
