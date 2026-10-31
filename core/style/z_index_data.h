// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_STYLE_Z_INDEX_DATA_H_
#define CORE_STYLE_Z_INDEX_DATA_H_

#include <tuple>

#include "core/renderer/starlight/style/css_type.h"
#include "core/style/default_computed_style.h"
#include "core/style/timing_function_data.h"

namespace lynx {
namespace starlight {
struct ZIndexData {
  ZIndexData() {}

  ~ZIndexData() = default;

  void Reset() {
    z_index_value = DefaultComputedStyle::DEFAULT_LONG;
    has_z_index = false;
  }

  int32_t z_index_value = DefaultComputedStyle::DEFAULT_LONG;
  bool has_z_index = false;
};
}  // namespace starlight
}  // namespace lynx

#endif  // CORE_STYLE_Z_INDEX_DATA_H_
