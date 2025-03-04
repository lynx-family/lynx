// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_STYLE_SHADOW_DATA_H_
#define CORE_STYLE_SHADOW_DATA_H_

#include <tuple>

#include "core/renderer/starlight/style/css_type.h"

namespace lynx {
namespace starlight {

struct ShadowData {
  float h_offset = 0;
  float v_offset = 0;
  float blur = 0;
  float spread = 0;
  unsigned int color = 0;
  ShadowOption option = ShadowOption::kNone;

  ShadowData() = default;
  ~ShadowData() = default;

  void Reset();

  bool operator==(const ShadowData& rhs) const {
    return std::tie(h_offset, v_offset, blur, spread, color, option) ==
           std::tie(rhs.h_offset, rhs.v_offset, rhs.blur, rhs.spread, rhs.color,
                    rhs.option);
  }

  bool operator!=(const ShadowData& rhs) const { return !(*this == rhs); }
};

}  // namespace starlight
}  // namespace lynx

#endif  // CORE_STYLE_SHADOW_DATA_H_
